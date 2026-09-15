import { createServer } from 'node:http';
import { randomUUID } from 'node:crypto';
import {
  GatewayRequestError,
  MAX_BODY_BYTES,
  PLAYER_STATE_PATH,
  normalizeUpstreamBaseUrl,
  proxyPlayerState,
} from './gateway.mjs';

const port = Number.parseInt(process.env.PORT ?? '4373', 10);
let upstreamBaseUrl = '';
let upstreamConfigError = '';
try {
  upstreamBaseUrl = normalizeUpstreamBaseUrl(process.env.CTG_ONE_API_BASE_URL ?? '');
} catch (error) {
  upstreamConfigError = error instanceof Error ? error.message : 'invalid upstream configuration';
}

function setHeaders(res) {
  res.setHeader('Cache-Control', 'no-store');
  res.setHeader('Content-Security-Policy', "default-src 'none'; frame-ancestors 'none'; base-uri 'none'");
  res.setHeader('Referrer-Policy', 'no-referrer');
  res.setHeader('X-Content-Type-Options', 'nosniff');
  res.setHeader('X-Frame-Options', 'DENY');
  res.setHeader('Permissions-Policy', 'camera=(), microphone=(), geolocation=(), payment=()');
}

function sendJson(res, status, payload) {
  setHeaders(res);
  res.statusCode = status;
  res.setHeader('Content-Type', 'application/json; charset=utf-8');
  res.end(JSON.stringify(payload));
}

async function readBody(req) {
  let raw = '';
  for await (const chunk of req) {
    raw += chunk;
    if (Buffer.byteLength(raw) > MAX_BODY_BYTES) throw new GatewayRequestError('payload_too_large', 413);
  }
  return raw;
}

const server = createServer(async (req, res) => {
  const requestId = randomUUID();
  res.setHeader('X-Request-Id', requestId);
  try {
    const url = new URL(req.url ?? '/', `http://${req.headers.host ?? 'localhost'}`);

    if (req.method === 'GET' && url.pathname === '/api/health') {
      return sendJson(res, 200, {
        status: 'ok',
        service: 'worldmakers-runtime-api',
        upstreamConfigured: Boolean(upstreamBaseUrl),
      });
    }

    if (req.method === 'GET' && url.pathname === '/api/ready') {
      if (!upstreamBaseUrl || upstreamConfigError) {
        return sendJson(res, 503, {
          status: 'not-ready',
          service: 'worldmakers-runtime-api',
          reason: upstreamConfigError || 'upstream_not_configured',
        });
      }
      return sendJson(res, 200, { status: 'ready', service: 'worldmakers-runtime-api' });
    }

    if (url.pathname !== PLAYER_STATE_PATH) return sendJson(res, 404, { error: 'not_found' });

    const rawBody = req.method === 'PUT' ? await readBody(req) : '';
    const proxied = await proxyPlayerState({
      method: req.method ?? 'GET',
      headers: req.headers,
      rawBody,
      upstreamBaseUrl,
    });

    setHeaders(res);
    res.statusCode = proxied.status;
    res.setHeader('Content-Type', proxied.contentType);
    res.end(proxied.body);
  } catch (error) {
    if (error instanceof GatewayRequestError) {
      return sendJson(res, error.status, { error: error.code, requestId });
    }
    if (error?.name === 'TimeoutError' || error?.name === 'AbortError') {
      return sendJson(res, 504, { error: 'upstream_timeout', requestId });
    }
    console.error('runtime-api request failed', { requestId, message: error instanceof Error ? error.message : String(error) });
    return sendJson(res, 502, { error: 'upstream_unavailable', requestId });
  }
});

server.listen(port, '0.0.0.0', () => {
  console.log(`World Makers runtime API listening on ${port}; upstreamConfigured=${Boolean(upstreamBaseUrl)}`);
});
