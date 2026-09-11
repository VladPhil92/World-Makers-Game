import { createServer } from 'node:http';
import { randomUUID } from 'node:crypto';
import { readFile } from 'node:fs/promises';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { dashboardCatalog, validateLoadout, validateSelection } from './domain/catalog.mjs';
import { createLaunchContext } from './domain/launch.mjs';
import { createDemoPlayer } from './data/demo-player.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));
const publicRoot = join(__dirname, '..', 'public');
const port = Number.parseInt(process.env.PORT ?? '4273', 10);
const demoAuthEnabled = process.env.WORLD_MAKERS_ALLOW_DEMO_AUTH === 'true';
const launchSigningSecret = process.env.WORLD_MAKERS_LAUNCH_SIGNING_SECRET ?? '';
const cookieName = 'wm_player_session';
const sessions = new Map();

const staticFiles = new Map([
  ['/', ['index.html', 'text/html; charset=utf-8']],
  ['/app.js', ['app.js', 'text/javascript; charset=utf-8']],
  ['/app.css', ['app.css', 'text/css; charset=utf-8']],
]);

class AuthenticationError extends Error {}

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

function dashboardReadModel(session) {
  return {
    authenticated: true,
    player: structuredClone(session.player),
    catalog: dashboardCatalog(),
    storeRequests: structuredClone(session.storeRequests),
    launch: {
      ready: launchSigningSecret.length >= 32,
      ttlSeconds: 120,
      protocol: 'worldmakers-launch-v1',
    },
  };
}

function storeRequest(session, itemId) {
  const item = dashboardCatalog().storeCatalog.find((candidate) => candidate.id === itemId);
  if (!item) throw new TypeError('Store item does not exist.');
  const existing = session.storeRequests.find((request) => request.itemId === itemId && request.status === 'parent-approval-required');
  if (existing) return existing;
  const request = {
    requestId: `store-request.${randomUUID()}`,
    itemId,
    entitlementId: item.entitlementId,
    status: 'parent-approval-required',
    createdAt: new Date().toISOString(),
  };
  session.storeRequests.push(request);
  return request;
}

async function handleApi(req, res, url) {
  if (req.method === 'GET' && url.pathname === '/api/health') {
    return json(res, 200, {
      status: 'ok',
      authMode: demoAuthEnabled ? 'demo' : 'provider-required',
      launchSigningConfigured: launchSigningSecret.length >= 32,
    });
  }

  if (req.method === 'POST' && url.pathname === '/api/demo/session') {
    assertSameOrigin(req);
    if (!demoAuthEnabled) return json(res, 503, { code: 'identity_provider_required' });
    const token = randomUUID();
    const session = { player: createDemoPlayer(), storeRequests: [], createdAt: Date.now() };
    sessions.set(token, session);
    res.setHeader('Set-Cookie', cookieHeader(token));
    return json(res, 201, dashboardReadModel(session));
  }

  if (req.method === 'GET' && url.pathname === '/api/session') {
    return json(res, 200, dashboardReadModel(requireSession(req)));
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
    session.player.loadout = validateLoadout(body.loadout);
    return json(res, 200, { avatarId: session.player.avatarId, loadout: session.player.loadout });
  }

  if (req.method === 'PATCH' && url.pathname === '/api/selection') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    session.player.selection = validateSelection(body.selection);
    return json(res, 200, { selection: session.player.selection });
  }

  if (req.method === 'GET' && url.pathname === '/api/store') {
    const session = requireSession(req);
    return json(res, 200, {
      items: dashboardCatalog().storeCatalog,
      entitlements: session.player.entitlements,
      requests: session.storeRequests,
      directPurchaseEnabled: false,
    });
  }

  if (req.method === 'POST' && url.pathname === '/api/store/requests') {
    assertSameOrigin(req);
    const session = requireSession(req);
    const body = await readJson(req);
    return json(res, 202, storeRequest(session, body.itemId));
  }

  if (req.method === 'POST' && url.pathname === '/api/launch-context') {
    assertSameOrigin(req);
    const session = requireSession(req);
    if (launchSigningSecret.length < 32) return json(res, 503, { code: 'launch_signing_not_configured' });
    const context = createLaunchContext({
      player: session.player,
      selection: session.player.selection,
      secret: launchSigningSecret,
    });
    return json(res, 201, {
      protocol: 'worldmakers-launch-v1',
      context,
      next: 'handoff-to-unreal-launcher',
    });
  }

  return false;
}

export function createPlayerDashboardServer() {
  return createServer(async (req, res) => {
    try {
      const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);
      if (url.pathname.startsWith('/api/')) {
        const handled = await handleApi(req, res, url);
        if (handled !== false) return;
        return json(res, 404, { code: 'not_found' });
      }
      if (await serveStatic(res, url.pathname)) return;
      return json(res, 404, { code: 'not_found' });
    } catch (error) {
      if (error instanceof AuthenticationError) return json(res, 401, { code: 'authentication_required' });
      if (error instanceof SyntaxError || error instanceof TypeError) return json(res, 400, { code: 'invalid_request' });
      console.error('Player dashboard request failed', error);
      return json(res, 500, { code: 'internal_error' });
    }
  });
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
  createPlayerDashboardServer().listen(port, '127.0.0.1', () => {
    console.log(`World Makers Player Dashboard listening on http://127.0.0.1:${port}`);
    if (!demoAuthEnabled) console.log('Identity provider is not configured; authenticated APIs fail closed.');
    if (launchSigningSecret.length < 32) console.log('Launch signing is not configured; PLAY remains fail-closed.');
  });
}
