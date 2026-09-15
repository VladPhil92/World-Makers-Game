import assert from 'node:assert/strict';
import test from 'node:test';
import {
  GatewayRequestError,
  normalizeUpstreamBaseUrl,
  proxyPlayerState,
  requireBearerAuthorization,
  requireIdempotencyKey,
  validatePlayerStateBody,
} from '../src/gateway.mjs';

test('normalizes only HTTPS upstreams', () => {
  assert.equal(normalizeUpstreamBaseUrl('https://ctgone.com/'), 'https://ctgone.com');
  assert.throws(() => normalizeUpstreamBaseUrl('http://ctgone.com'), TypeError);
});

test('requires bearer auth and bounded idempotency keys', () => {
  assert.equal(requireBearerAuthorization('Bearer abcdefghijklmnop'), 'Bearer abcdefghijklmnop');
  assert.equal(requireIdempotencyKey('event.12345678'), 'event.12345678');
  assert.throws(() => requireBearerAuthorization('Basic abc'), GatewayRequestError);
  assert.throws(() => requireIdempotencyKey('bad key'), GatewayRequestError);
});

test('normalizes the writable player-state payload', () => {
  const body = validatePlayerStateBody(JSON.stringify({
    expectedRevision: 3,
    saves: [{ slot: 'primary' }],
    missions: [],
    discoveries: [],
    achievements: [],
    ignored: 'not-forwarded',
  }));
  assert.deepEqual(JSON.parse(body), {
    schemaVersion: 1,
    expectedRevision: 3,
    saves: [{ slot: 'primary' }],
    missions: [],
    discoveries: [],
    achievements: [],
  });
});

test('forwards only the runtime auth/idempotency contract', async () => {
  let captured;
  const fetchImpl = async (url, options) => {
    captured = { url, options };
    return new Response(JSON.stringify({ schemaVersion: 2, profile: { exists: false, revision: 0 } }), {
      status: 200,
      headers: { 'content-type': 'application/json' },
    });
  };
  const result = await proxyPlayerState({
    method: 'PUT',
    headers: {
      authorization: 'Bearer abcdefghijklmnop',
      'idempotency-key': 'event.12345678',
      cookie: 'must-not-forward=true',
    },
    rawBody: JSON.stringify({ expectedRevision: 0, saves: [], missions: [], discoveries: [], achievements: [] }),
    upstreamBaseUrl: 'https://ctgone.com',
    fetchImpl,
  });
  assert.equal(result.status, 200);
  assert.equal(captured.url, 'https://ctgone.com/api/worldmakers/player-state');
  assert.equal(captured.options.headers.Authorization, 'Bearer abcdefghijklmnop');
  assert.equal(captured.options.headers['Idempotency-Key'], 'event.12345678');
  assert.equal(captured.options.headers.cookie, undefined);
});
