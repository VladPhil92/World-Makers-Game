import { createServer } from 'node:http';
import { randomUUID } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { dashboardCatalog } from './domain/catalog.mjs';
import { createLaunchContext } from './domain/launch.mjs';
import { createNativeLaunchUri } from './domain/native-handoff.mjs';
import { verifyIdentityAssertion, deriveIdentityLinkId } from './domain/identity.mjs';
import { JsonFileProfileStore, ProfileConflictError } from './domain/profile-store.mjs';
import { SupabaseProfileStore } from './domain/supabase-profile-store.mjs';
import { appendStoreRequest, createPlayerProfile, profilePlayerReadModel, updateProfileLoadout, updateProfilePreferences, updateProfileSelection } from './domain/profile.mjs';
import { createDemoPlayer } from './data/demo-player.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const publicRoot = join(__dirname, '..', 'public');
const port = Number.parseInt(process.env.PORT ?? '4273', 10);
const demoAuthEnabled = process.env.WORLD_MAKERS_ALLOW_DEMO_AUTH === 'true';
const launchSigningSecret = process.env.WORLD_MAKERS_LAUNCH_SIGNING_SECRET ?? '';
const identityAssertionSecret = process.env.WORLD_MAKERS_IDENTITY_ASSERTION_SECRET ?? '';
const identityLinkSecret = process.env.WORLD_MAKERS_IDENTITY_LINK_SECRET ?? identityAssertionSecret;
const configuredProfileStorePath = process.env.WORLD_MAKERS_PROFILE_STORE_PATH ?? '';
const developmentProfileStorePath = process.env.NODE_ENV === 'production' ? '' : join(process.cwd(), 'Build', 'PlayerDashboard', 'profiles.json');
const defaultProfileStorePath = configuredProfileStorePath || developmentProfileStorePath;
const supabaseUrl = process.env.SUPABASE_URL ?? '';
const supabaseAnonKey = process.env.SUPABASE_ANON_KEY ?? '';
const defaultProfileStore = supabaseUrl && supabaseAnonKey
  ? new SupabaseProfileStore({ url: supabaseUrl, anonKey: supabaseAnonKey })
  : defaultProfileStorePath ? new JsonFileProfileStore(defaultProfileStorePath) : null;
const cookieName = 'wm_player_session';
const sessions = new Map();

const staticFiles = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
  ['/app.css', ['app.css', 'text/css; charset=utf-8']],
  ['/handoff', ['handoff.html', 'text/html; charset=utf-8']],
  ['/handoff.js', ['handoff.js', 'text/javascript; charset=utf-8']],
  ['/logo-primary.png', ['logo-primary.png', 'image/png']],
]);

class AuthenticationError extends Error {}
class ProfileStoreUnavailableError extends Error {}

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

function cookieHeader(token, maxAge = 3600) {
  const secure = process.env.NODE_ENV === 'production' ? '; Secure' : '';
  return `${cookieName}=${encodeURIComponent(token)}; HttpOnly; SameSite=Strict; Path=/; Max-Age=${maxAge}${secure}`;
}

function assertSameOrigin(req) {
  const origin = req.headers.origin;
  if (!origin) return;
  const host = req.headers.host;
  const expected = `${process.env.NODE_ENV === 'production' ? 'https' : 'http'}://${host}`;
  if (origin !== expected) throw new AuthenticationError('Origin rejected.');
}

function requireSession(req) {
  const token = parseCookies(req.headers.cookie)[cookieName];
  const session = token ? sessions.get(token) : null;
  if (!session) throw new AuthenticationError('Authentication required.');
  return session;
}

function requireProfileStore(store) {
  if (!store) throw new ProfileStoreUnavailableError('Profile store is not configured.');
  return store;
}

async function readJson(req) {
  let raw = '';
  for await (const chunk of req) {
    raw += chunk;
    if (Buffer.byteLength(raw) > 32_768) throw new TypeError('Request body too large.');
  }
  return raw ? JSON.parse(raw) : {};
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

async function loadProfile(store, session) {
  const profile = await requireProfileStore(store).get(session.playerProfileId);
  if (!profile) throw new AuthenticationError('Player profile not available.');
  return profile;
}

function assertRevision(body, profile) {
  if (!Number.isInteger(body.profileRevision) || body.profileRevision !== profile.revision) throw new ProfileConflictError('Profile revision conflict.');
}

function dashboardReadModel(session, profile) {
  return {
    authenticated: true,
    identity: {
      provider: session.authProvider,
      profileId: profile.playerProfileId,
      persistence: 'server-side',
    },
    player: profilePlayerReadModel(profile),
    profile: {
      revision: profile.revision,
      updatedAt: profile.updatedAt,
      persistent: true,
    },
    catalog: dashboardCatalog(),
    storeRequests: structuredClone(profile.storeRequests),
    launch: {
      ready: launchSigningSecret.length >= 32,
      ttlSeconds: 120,
      protocol: 'worldmakers-launch-v1',
      nativeScheme: 'worldmakers',
    },
  };
}

async function ensureDemoProfile(store) {
  const player = createDemoPlayer();
  const existing = await store.get(player.playerId);
  if (existing) return existing;
  const profile = createPlayerProfile({
    playerProfileId: player.playerId,
    identityLinkId: 'identity-link.demo.explorer-01',
    displayName: player.displayName,
    seed: player,
  });
  return store.put(profile, { expectedRevision: 0 });
}

async function ensureIdentityProfile(store, claims) {
  const identityLinkId = deriveIdentityLinkId({ issuer: claims.issuer, subject: claims.subject }, identityLinkSecret);
  const existing = await store.get(claims.playerProfileId);
  if (existing) {
    if (existing.identityLinkId !== identityLinkId) throw new AuthenticationError('Identity link mismatch.');
    if (existing.displayName !== claims.displayName) {
      const changed = { ...existing, displayName: claims.displayName };
      return store.put(changed, { expectedRevision: existing.revision });
    }
    return existing;
  }
  const profile = createPlayerProfile({
    playerProfileId: claims.playerProfileId,
    identityLinkId,
    displayName: claims.displayName,
  });
  return store.put(profile, { expectedRevision: 0 });
}

function createSession(res, profile, authProvider) {
  const token = randomUUID();
  sessions.set(token, { playerProfileId: profile.playerProfileId, authProvider, createdAt: Date.now() });
  res.setHeader('Set-Cookie', cookieHeader(token));
  return sessions.get(token);
}

async function persistMutation(store, session, body, mutate) {
  const profile = await loadProfile(store, session);
  assertRevision(body, profile);
  const changed = mutate(profile);
  return store.put(changed, { expectedRevision: profile.revision });
}

function createStoreRequest(itemId) {
  const item = dashboardCatalog().storeCatalog.find((candidate) => candidate.id === itemId);
  if (!item) throw new TypeError('Store item does not exist.');
  return {
    requestId: `store-request.${randomUUID()}`,
    itemId,
    entitlementId: item.entitlementId,
    status: 'parent-approval-required',
    createdAt: new Date().toISOString(),
  };
}

async function handleApi(req, res, url, store) {
  if (req.method === 'GET' && url.pathname === '/api/health') {
    return json(res, 200, {
      status: 'ok',
      authMode: identityAssertionSecret.length >= 32 ? 'ctg-one-assertion' : demoAuthEnabled ? 'demo' : 'provider-required',
      identityAssertionConfigured: identityAssertionSecret.length >= 32,
      profileStoreConfigured: Boolean(store),
      launchSigningConfigured: launchSigningSecret.length >= 32,
    });
  }

  if (req.method === 'POST' && url.pathname === '/api/identity/session') {
    const profileStore = requireProfileStore(store);
    if (identityAssertionSecret.length < 32 || identityLinkSecret.length < 32) return json(res, 503, { code: 'identity_provider_required' });
    const body = await readJson(req);
    const claims = verifyIdentityAssertion(body.assertion, identityAssertionSecret);
    const profile = await ensureIdentityProfile(profileStore, claims);
    const session = createSession(res, profile, 'ctg-one');
    return json(res, 201, dashboardReadModel(session, profile));
  }

  if (req.method === 'POST' && url.pathname === '/api/demo/session') {
    assertSameOrigin(req);
    if (!demoAuthEnabled) return json(res, 503, { code: 'identity_provider_required' });
    const profileStore = requireProfileStore(store);
    const profile = await ensureDemoProfile(profileStore);
    const session = createSession(res, profile, 'demo');
    return json(res, 201, dashboardReadModel(session, profile));
  }

  if (req.method === 'GET' && (url.pathname === '/api/session' || url.pathname === '/api/profile')) {
    const session = requireSession(req);
    const profile = await loadProfile(store, session);
    return json(res, 200, dashboardReadModel(session, profile));
  }

  if (req.method === 'DELETE' && url.pathname === '/api/session') {
    assertSameOrigin(req);
    const cookies = parseCookies(req.headers.cookie);
    if (cookies[cookieName]) sessions.delete(cookies[cookieName]);
    res.setHeader('Set-Cookie', cookieHeader('', 0));
    return json(res, 200, { authenticated: false });
  }

  if (req.method === 'PATCH' && url.pathname === '/api/avatar/loadout') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    const saved = await persistMutation(store, session, body, (profile) => updateProfileLoadout(profile, body.loadout));
    return json(res, 200, { avatarId: saved.avatarId, loadout: saved.loadout, profileRevision: saved.revision, profileUpdatedAt: saved.updatedAt });
  }

  if (req.method === 'PATCH' && url.pathname === '/api/selection') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    const saved = await persistMutation(store, session, body, (profile) => updateProfileSelection(profile, body.selection));
    return json(res, 200, { selection: saved.selection, profileRevision: saved.revision, profileUpdatedAt: saved.updatedAt });
  }

  if (req.method === 'PATCH' && url.pathname === '/api/preferences') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    const saved = await persistMutation(store, session, body, (profile) => updateProfilePreferences(profile, body.preferences ?? {}));
    return json(res, 200, { preferences: saved.preferences, profileRevision: saved.revision, profileUpdatedAt: saved.updatedAt });
  }

  if (req.method === 'GET' && url.pathname === '/api/store') {
    const profile = await loadProfile(store, requireSession(req));
    return json(res, 200, {
      items: dashboardCatalog().storeCatalog,
      entitlements: profile.entitlements,
      requests: profile.storeRequests,
      directPurchaseEnabled: false,
      profileRevision: profile.revision,
    });
  }

  if (req.method === 'POST' && url.pathname === '/api/store/requests') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    const profile = await loadProfile(store, session);
    assertRevision(body, profile);
    const candidate = createStoreRequest(body.itemId);
    const result = appendStoreRequest(profile, candidate);
    if (!result.changed) return json(res, 200, { request: result.request, profileRevision: profile.revision });
    const saved = await requireProfileStore(store).put(result.profile, { expectedRevision: profile.revision });
    return json(res, 202, { request: result.request, profileRevision: saved.revision, profileUpdatedAt: saved.updatedAt });
  }

  if (req.method === 'POST' && url.pathname === '/api/launch-context') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const profile = await loadProfile(store, session);
    if (launchSigningSecret.length < 32) return json(res, 503, { code: 'launch_signing_not_configured' });
    const player = profilePlayerReadModel(profile);
    const context = { ...createLaunchContext({ player, selection: profile.selection, secret: launchSigningSecret }), profileRevision: profile.revision };
    return json(res, 201, {
      protocol: 'worldmakers-launch-v1',
      context,
      launchUri: createNativeLaunchUri(context),
      next: 'open-native-client',
    });
  }

  return false;
}

export function createPlayerDashboardServer({ profileStore = defaultProfileStore } = {}) {
  return createServer(async (req, res) => {
    try {
      const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);
      if (url.pathname.startsWith('/api/')) {
        const handled = await handleApi(req, res, url, profileStore);
        if (handled !== false) return;
        return json(res, 404, { code: 'not_found' });
      }
      if (await serveStatic(res, url.pathname)) return;
      return json(res, 404, { code: 'not_found' });
    } catch (error) {
      if (error instanceof ProfileConflictError) return json(res, 409, { code: 'profile_conflict' });
      if (error instanceof AuthenticationError) return json(res, 401, { code: 'authentication_required' });
      if (error instanceof ProfileStoreUnavailableError) return json(res, 503, { code: 'profile_store_not_configured' });
      if (error instanceof SyntaxError || error instanceof TypeError) return json(res, 400, { code: 'invalid_request' });
      console.error('Player dashboard request failed', error);
      return json(res, 500, { code: 'internal_error' });
    }
  });
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  const host = process.env.HOST ?? '0.0.0.0';
  createPlayerDashboardServer().listen(port, host, () => {
    console.log(`World Makers Player Dashboard listening on http://${host}:${port}`);
    if (!defaultProfileStore) console.log('Persistent profile store is not configured; authenticated APIs fail closed.');
    if (identityAssertionSecret.length < 32) console.log('CTG One identity assertion verification is not configured.');
    if (!demoAuthEnabled) console.log('Demo identity is disabled.');
    if (launchSigningSecret.length < 32) console.log('Launch signing is not configured; PLAY remains fail-closed.');
  });
}
