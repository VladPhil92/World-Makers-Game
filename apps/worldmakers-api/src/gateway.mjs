const PLAYER_STATE_PATH = '/api/worldmakers/player-state';
const MAX_BODY_BYTES = 65_536;
const IDEMPOTENCY_PATTERN = /^[A-Za-z0-9._:-]{8,128}$/;

export class GatewayRequestError extends Error {
  constructor(code, status = 400) {
    super(code);
    this.name = 'GatewayRequestError';
    this.code = code;
    this.status = status;
  }
}

export function normalizeUpstreamBaseUrl(value) {
  const url = String(value ?? '').trim().replace(/\/$/, '');
  if (!url) return '';
  const parsed = new URL(url);
  if (parsed.protocol !== 'https:') throw new TypeError('CTG_ONE_API_BASE_URL must use HTTPS.');
  if (parsed.username || parsed.password || parsed.search || parsed.hash) throw new TypeError('CTG_ONE_API_BASE_URL must be an origin/base path only.');
  return url;
}

export function requireBearerAuthorization(value) {
  const header = String(value ?? '').trim();
  if (!/^Bearer\s+\S{8,}$/i.test(header)) throw new GatewayRequestError('authorization_required', 401);
  return header;
}

export function requireIdempotencyKey(value) {
  const key = String(value ?? '').trim();
  if (!IDEMPOTENCY_PATTERN.test(key)) throw new GatewayRequestError('invalid_idempotency_key', 400);
  return key;
}

export function validatePlayerStateBody(raw) {
  const text = String(raw ?? '');
  if (Buffer.byteLength(text) > MAX_BODY_BYTES) throw new GatewayRequestError('payload_too_large', 413);
  let body;
  try {
    body = text ? JSON.parse(text) : null;
  } catch {
    throw new GatewayRequestError('invalid_json', 400);
  }
  if (!body || typeof body !== 'object' || Array.isArray(body)) throw new GatewayRequestError('invalid_payload', 400);
  if (!Number.isInteger(body.expectedRevision) || body.expectedRevision < 0) throw new GatewayRequestError('invalid_revision', 400);
  for (const key of ['saves', 'missions', 'discoveries', 'achievements']) {
    if (!Array.isArray(body[key])) throw new GatewayRequestError('invalid_payload', 400);
  }
  return JSON.stringify({
    schemaVersion: 1,
    expectedRevision: body.expectedRevision,
    saves: body.saves,
    missions: body.missions,
    discoveries: body.discoveries,
    achievements: body.achievements,
  });
}

export async function proxyPlayerState({ method, headers, rawBody = '', upstreamBaseUrl, fetchImpl = fetch }) {
  if (!upstreamBaseUrl) throw new GatewayRequestError('upstream_not_configured', 503);
  if (method !== 'GET' && method !== 'PUT') throw new GatewayRequestError('method_not_allowed', 405);

  const authorization = requireBearerAuthorization(headers.authorization);
  const upstreamHeaders = {
    Accept: 'application/json',
    Authorization: authorization,
    'User-Agent': 'worldmakers-runtime-api/0.1',
  };
  let body;

  if (method === 'PUT') {
    upstreamHeaders['Content-Type'] = 'application/json';
    upstreamHeaders['Idempotency-Key'] = requireIdempotencyKey(headers['idempotency-key']);
    body = validatePlayerStateBody(rawBody);
  }

  const response = await fetchImpl(`${upstreamBaseUrl}${PLAYER_STATE_PATH}`, {
    method,
    headers: upstreamHeaders,
    body,
    redirect: 'error',
    signal: AbortSignal.timeout(10_000),
  });
  const responseText = await response.text();
  return {
    status: response.status,
    contentType: response.headers.get('content-type') || 'application/json; charset=utf-8',
    body: responseText,
  };
}

export { MAX_BODY_BYTES, PLAYER_STATE_PATH };
