import { createServer } from 'node:http';
import { randomUUID } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { buildSessionReadModel, AuthorizationError, requireAuthorizedChild } from './domain/authorization.mjs';
import { createParentDashboardReadModel } from './domain/dashboard.mjs';
import { buildPrivacyRequest, validateLinkCode } from './domain/privacy.mjs';
import { demoFamily } from './data/demo-family.mjs';
import { createSupabaseClient, SupabaseAuthError, SupabaseQueryError } from './domain/supabase-client.mjs';
import { ensureFamilyForParent, fetchFamilyReadModel, createChildProfile } from './domain/family-store.mjs';
import { createIdentityAssertion } from './domain/identity-issuer.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const publicRoot = join(__dirname, '..', 'public');
const demoAuthEnabled = process.env.WORLD_MAKERS_ALLOW_DEMO_AUTH === 'true';
const port = Number.parseInt(process.env.PORT ?? '4173', 10);
const sessions = new Map();
const cookieName = 'wm_parent_session';

const supabaseUrl = process.env.SUPABASE_URL ?? '';
const supabaseAnonKey = process.env.SUPABASE_ANON_KEY ?? '';
const supabaseConfigured = Boolean(supabaseUrl && supabaseAnonKey);
const supabase = supabaseConfigured ? createSupabaseClient({ url: supabaseUrl, anonKey: supabaseAnonKey }) : null;

const identityAssertionSecret = process.env.WORLD_MAKERS_IDENTITY_ASSERTION_SECRET ?? '';
const playerDashboardUrl = (process.env.WORLD_MAKERS_PLAYER_DASHBOARD_URL ?? '').replace(/\/$/, '');

const staticFiles = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
  ['/app.css', ['app.css', 'text/css; charset=utf-8']],
]);

function setSecurityHeaders(res) {
  res.setHeader('Content-Security-Policy', "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'; form-action 'self'");
  res.setHeader('Referrer-Policy', 'no-referrer');
  res.setHeader('X-Content-Type-Options', 'nosniff');
  res.setHeader('X-Frame-Options', 'DENY');
  res.setHeader('Permissions-Policy', 'camera=(), microphone=(), geolocation=(), payment=()');
  res.setHeader('Cross-Origin-Opener-Policy', 'same-origin');
}

function json(res, status, payload) {
  setSecurityHeaders(res);
  res.statusCode = status;
  res.setHeader('Content-Type', 'application/json; charset=utf-8');
  res.setHeader('Cache-Control', 'no-store');
  res.end(JSON.stringify(payload));
}

function parseCookies(header = '') {
  return Object.fromEntries(header.split(';').map((part) => part.trim()).filter(Boolean).map((part) => {
    const index = part.indexOf('=');
    return index > 0 ? [part.slice(0, index), decodeURIComponent(part.slice(index + 1))] : [part, ''];
  }));
}

function getSession(req) {
  const token = parseCookies(req.headers.cookie)[cookieName];
  if (!token) return null;
  return sessions.get(token) ?? null;
}

function requireSession(req) {
  const session = getSession(req);
  if (!session) throw new AuthorizationError('Authentication required.');
  return session;
}

function cookieHeader(token, maxAge = 3600) {
  const secure = process.env.NODE_ENV === 'production' ? '; Secure' : '';
  return `${cookieName}=${encodeURIComponent(token)}; HttpOnly; SameSite=Strict; Path=/; Max-Age=${maxAge}${secure}`;
}

function assertSameOrigin(req) {
  const origin = req.headers.origin;
  if (!origin) return;
  const host = req.headers.host;
  const expected = `${process.env.NODE_ENV === 'production' ? 'https' : 'http'}://${host}`;
  if (origin !== expected) throw new AuthorizationError('Origin rejected.');
}

async function readJson(req) {
  let raw = '';
  for await (const chunk of req) {
    raw += chunk;
    if (Buffer.byteLength(raw) > 16_384) throw new TypeError('Request body too large.');
  }
  if (!raw) return {};
  return JSON.parse(raw);
}

async function serveStatic(res, pathname) {
  const target = staticFiles.get(pathname);
  if (!target) return false;
  const [filename, contentType] = target;
  const body = await readFile(join(publicRoot, filename));
  setSecurityHeaders(res);
  res.statusCode = 200;
  res.setHeader('Content-Type', contentType);
  res.setHeader('Cache-Control', pathname === '/' ? 'no-cache' : 'public, max-age=300');
  res.end(body);
  return true;
}

function requireSupabase() {
  if (!supabase) throw new SupabaseAuthError('Account sign-in is not configured on this deployment.', 503);
}

function validateEmail(value) {
  if (typeof value !== 'string' || !/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(value)) throw new TypeError('A valid email is required.');
  return value.trim().toLowerCase();
}

function validatePassword(value) {
  if (typeof value !== 'string' || value.length < 8 || value.length > 128) throw new TypeError('Password must be 8-128 characters.');
  return value;
}

function validateDisplayName(value) {
  const trimmed = String(value ?? '').trim().slice(0, 40);
  if (trimmed.length < 1) throw new TypeError('displayName is required.');
  return trimmed;
}

async function ensureFreshAccessToken(session) {
  if (Date.now() < session.accessTokenExpiresAt - 10_000) return session.accessToken;
  const refreshed = await supabase.refreshSession({ refreshToken: session.refreshToken });
  session.accessToken = refreshed.access_token;
  session.refreshToken = refreshed.refresh_token;
  session.accessTokenExpiresAt = Date.now() + refreshed.expires_in * 1000;
  return session.accessToken;
}

async function loadFamilyForSession(session) {
  const accessToken = await ensureFreshAccessToken(session);
  return fetchFamilyReadModel(supabase, accessToken, session.familyId);
}

function createLocalSession(res, { parentId, familyId, accessToken, refreshToken, expiresIn }) {
  const token = randomUUID();
  sessions.set(token, {
    parentId,
    familyId,
    accessToken,
    refreshToken,
    accessTokenExpiresAt: Date.now() + expiresIn * 1000,
    createdAt: Date.now(),
  });
  res.setHeader('Set-Cookie', cookieHeader(token, expiresIn));
  return sessions.get(token);
}

async function handleApi(req, res, url) {
  if (req.method === 'GET' && url.pathname === '/api/health') {
    return json(res, 200, {
      status: 'ok',
      authMode: supabaseConfigured ? 'supabase' : demoAuthEnabled ? 'demo' : 'provider-required',
      supabaseConfigured,
      identityAssertionConfigured: identityAssertionSecret.length >= 32,
      playerDashboardConfigured: playerDashboardUrl.length > 0,
    });
  }

  if (req.method === 'POST' && url.pathname === '/api/auth/register') {
    assertSameOrigin(req);
    requireSupabase();
    const body = await readJson(req);
    const email = validateEmail(body.email);
    const password = validatePassword(body.password);
    const displayName = validateDisplayName(body.displayName);
    const signup = await supabase.signUp({ email, password, displayName });
    if (!signup.session) {
      return json(res, 202, { status: 'confirmation_required', message: 'Check your email to confirm your account, then sign in.' });
    }
    const familyId = await ensureFamilyForParent(supabase, signup.session.access_token, signup.user.id);
    const session = createLocalSession(res, {
      parentId: signup.user.id,
      familyId,
      accessToken: signup.session.access_token,
      refreshToken: signup.session.refresh_token,
      expiresIn: signup.session.expires_in,
    });
    const family = await loadFamilyForSession(session);
    return json(res, 201, buildSessionReadModel(family, session.parentId));
  }

  if (req.method === 'POST' && url.pathname === '/api/auth/login') {
    assertSameOrigin(req);
    requireSupabase();
    const body = await readJson(req);
    const email = validateEmail(body.email);
    const password = validatePassword(body.password);
    const result = await supabase.signInWithPassword({ email, password });
    const familyId = await ensureFamilyForParent(supabase, result.access_token, result.user.id);
    const session = createLocalSession(res, {
      parentId: result.user.id,
      familyId,
      accessToken: result.access_token,
      refreshToken: result.refresh_token,
      expiresIn: result.expires_in,
    });
    const family = await loadFamilyForSession(session);
    return json(res, 201, buildSessionReadModel(family, session.parentId));
  }

  if (req.method === 'POST' && url.pathname === '/api/demo/session') {
    assertSameOrigin(req);
    if (!demoAuthEnabled) return json(res, 503, { code: 'identity_provider_required' });
    const token = randomUUID();
    sessions.set(token, { parentId: 'parent.demo.guardian-01', createdAt: Date.now(), demo: true });
    res.setHeader('Set-Cookie', cookieHeader(token));
    return json(res, 201, buildSessionReadModel(demoFamily, 'parent.demo.guardian-01'));
  }

  if (req.method === 'GET' && url.pathname === '/api/session') {
    const session = requireSession(req);
    const family = session.demo ? demoFamily : await loadFamilyForSession(session);
    return json(res, 200, buildSessionReadModel(family, session.parentId));
  }

  if (req.method === 'DELETE' && url.pathname === '/api/session') {
    assertSameOrigin(req);
    const cookies = parseCookies(req.headers.cookie);
    if (cookies[cookieName]) sessions.delete(cookies[cookieName]);
    res.setHeader('Set-Cookie', cookieHeader('', 0));
    return json(res, 200, { authenticated: false });
  }

  if (req.method === 'GET' && url.pathname === '/api/dashboard') {
    const session = requireSession(req);
    const family = session.demo ? demoFamily : await loadFamilyForSession(session);
    const childProfileId = url.searchParams.get('childId') ?? '';
    return json(res, 200, createParentDashboardReadModel(family, session.parentId, childProfileId));
  }

  if (req.method === 'POST' && url.pathname === '/api/children') {
    assertSameOrigin(req);
    const session = requireSession(req);
    if (session.demo) return json(res, 503, { code: 'not_available_in_demo' });
    const body = await readJson(req);
    const accessToken = await ensureFreshAccessToken(session);
    const child = await createChildProfile(supabase, accessToken, session.familyId, body.displayAlias);
    const family = await loadFamilyForSession(session);
    return json(res, 201, { child, session: buildSessionReadModel(family, session.parentId) });
  }

  if (req.method === 'POST' && url.pathname === '/api/children/launch') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    if (identityAssertionSecret.length < 32 || !playerDashboardUrl) {
      return json(res, 503, { code: 'launch_not_configured' });
    }
    const family = session.demo ? demoFamily : await loadFamilyForSession(session);
    const child = requireAuthorizedChild(family, session.parentId, body.childProfileId);
    const playerProfileId = `player.${child.childProfileId.replace(/^child\./, '')}`;
    const assertion = createIdentityAssertion(
      { subject: child.childProfileId, playerProfileId, displayName: child.displayAlias },
      identityAssertionSecret,
    );
    return json(res, 201, { assertion, playerDashboardUrl, handoffUrl: `${playerDashboardUrl}/handoff?assertion=${encodeURIComponent(assertion)}` });
  }

  if (req.method === 'POST' && url.pathname === '/api/privacy-requests') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    const family = session.demo ? demoFamily : await loadFamilyForSession(session);
    const request = buildPrivacyRequest({
      family,
      parentId: session.parentId,
      childProfileId: body.childProfileId,
      operation: body.operation,
      acknowledged: body.acknowledged,
    });
    return json(res, 202, { ...request, requestId: `privacy.${randomUUID()}` });
  }

  if (req.method === 'POST' && url.pathname === '/api/link-requests') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    const linkCode = validateLinkCode(body.linkCode);
    // MVP never trusts a link code enough to attach a child locally. A production backend must verify it.
    if (!session.parentId) throw new AuthorizationError();
    return json(res, 202, {
      requestId: `link.${randomUUID()}`,
      codeFingerprint: linkCode.slice(-2).padStart(8, '•'),
      status: 'pending-backend-verification',
    });
  }

  return false;
}

export function createParentPortalServer() {
  return createServer(async (req, res) => {
    try {
      const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);
      if (url.pathname.startsWith('/api/')) {
        const handled = await handleApi(req, res, url);
        if (handled !== false) return;
        return json(res, 404, { code: 'not_found' });
      }
      if (await serveStatic(res, url.pathname)) return;
      json(res, 404, { code: 'not_found' });
    } catch (error) {
      if (error instanceof AuthorizationError) return json(res, 404, { code: 'not_available' });
      if (error instanceof SupabaseAuthError) return json(res, error.status === 503 ? 503 : 401, { code: 'authentication_failed', message: error.message });
      if (error instanceof SupabaseQueryError) return json(res, 502, { code: 'account_service_error' });
      if (error instanceof SyntaxError || error instanceof TypeError) return json(res, 400, { code: 'invalid_request', message: error.message });
      console.error('Parent portal request failed', error);
      return json(res, 500, { code: 'internal_error' });
    }
  });
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const host = process.env.HOST ?? '0.0.0.0';
  createParentPortalServer().listen(port, host, () => {
    console.log(`World Makers Parent Portal listening on http://${host}:${port}`);
    if (!supabaseConfigured) console.log('Supabase is not configured; only demo identity (if enabled) will work.');
    if (!demoAuthEnabled) console.log('Demo identity is disabled.');
    if (identityAssertionSecret.length < 32 || !playerDashboardUrl) console.log('Child launch hand-off to the player dashboard is not configured.');
  });
}
