import { createServer } from 'node:http';
import { randomUUID } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { buildSessionReadModel, AuthorizationError, requireAuthorizedChild } from './domain/authorization.mjs';
import { createParentDashboardReadModel } from './domain/dashboard.mjs';
import { buildPrivacyRequest, validateLinkCode } from './domain/privacy.mjs';
import { demoFamily } from './data/demo-family.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const publicRoot = join(__dirname, '..', 'public');
const demoAuthEnabled = process.env.WORLD_MAKERS_ALLOW_DEMO_AUTH === 'true';
const port = Number.parseInt(process.env.PORT ?? '4173', 10);
const sessions = new Map();
const cookieName = 'wm_parent_session';

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

async function handleApi(req, res, url) {
  if (req.method === 'GET' && url.pathname === '/api/health') {
    return json(res, 200, { status: 'ok', authMode: demoAuthEnabled ? 'demo' : 'provider-required' });
  }

  if (req.method === 'POST' && url.pathname === '/api/demo/session') {
    assertSameOrigin(req);
    if (!demoAuthEnabled) return json(res, 503, { code: 'identity_provider_required' });
    const token = randomUUID();
    sessions.set(token, { parentId: 'parent.demo.guardian-01', createdAt: Date.now() });
    res.setHeader('Set-Cookie', cookieHeader(token));
    return json(res, 201, buildSessionReadModel(demoFamily, 'parent.demo.guardian-01'));
  }

  if (req.method === 'GET' && url.pathname === '/api/session') {
    const session = requireSession(req);
    return json(res, 200, buildSessionReadModel(demoFamily, session.parentId));
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
    const childProfileId = url.searchParams.get('childId') ?? '';
    return json(res, 200, createParentDashboardReadModel(demoFamily, session.parentId, childProfileId));
  }

  if (req.method === 'POST' && url.pathname === '/api/privacy-requests') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    const request = buildPrivacyRequest({
      family: demoFamily,
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
      if (error instanceof SyntaxError || error instanceof TypeError) return json(res, 400, { code: 'invalid_request' });
      console.error('Parent portal request failed', error);
      return json(res, 500, { code: 'internal_error' });
    }
  });
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  createParentPortalServer().listen(port, '127.0.0.1', () => {
    console.log(`World Makers Parent Portal listening on http://127.0.0.1:${port}`);
    if (!demoAuthEnabled) console.log('Identity provider is not configured; authenticated APIs fail closed.');
  });
}
