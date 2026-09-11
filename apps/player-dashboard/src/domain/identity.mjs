import { createHmac, timingSafeEqual } from 'node:crypto';

const PROTOCOL = 'ctg-one-identity-v1';
const ISSUER = 'ctg-one';
const AUDIENCE = 'world-makers';

function encode(value) {
  return Buffer.from(value).toString('base64url');
}

function decode(value) {
  return Buffer.from(value, 'base64url').toString('utf8');
}

function hmac(value, secret) {
  return createHmac('sha256', secret).update(value).digest('base64url');
}

function canonicalClaims(claims) {
  return {
    protocol: PROTOCOL,
    issuer: claims.issuer,
    audience: claims.audience,
    subject: claims.subject,
    playerProfileId: claims.playerProfileId,
    displayName: claims.displayName,
    issuedAt: claims.issuedAt,
    expiresAt: claims.expiresAt,
  };
}

function assertSafeId(value, label) {
  if (typeof value !== 'string' || !/^[a-z0-9][a-z0-9._:-]{2,127}$/i.test(value)) {
    throw new TypeError(`${label} is invalid.`);
  }
}

export function createIdentityAssertion(claims, secret, nowMs = Date.now()) {
  if (typeof secret !== 'string' || secret.length < 32) throw new TypeError('Identity assertion secret must be at least 32 characters.');
  assertSafeId(claims.subject, 'subject');
  assertSafeId(claims.playerProfileId, 'playerProfileId');
  if (typeof claims.displayName !== 'string' || claims.displayName.trim().length < 1 || claims.displayName.trim().length > 40) {
    throw new TypeError('displayName is invalid.');
  }
  const payload = canonicalClaims({
    ...claims,
    issuer: ISSUER,
    audience: AUDIENCE,
    displayName: claims.displayName.trim(),
    issuedAt: new Date(nowMs).toISOString(),
    expiresAt: new Date(nowMs + 5 * 60_000).toISOString(),
  });
  const encoded = encode(JSON.stringify(payload));
  return `${encoded}.${hmac(encoded, secret)}`;
}

export function verifyIdentityAssertion(assertion, secret, nowMs = Date.now()) {
  if (typeof secret !== 'string' || secret.length < 32) throw new TypeError('Identity provider is not configured.');
  if (typeof assertion !== 'string' || assertion.length > 4096) throw new TypeError('Identity assertion is malformed.');
  const [encoded, signature, extra] = assertion.split('.');
  if (!encoded || !signature || extra !== undefined) throw new TypeError('Identity assertion is malformed.');
  const expected = hmac(encoded, secret);
  const providedBuffer = Buffer.from(signature);
  const expectedBuffer = Buffer.from(expected);
  if (providedBuffer.length !== expectedBuffer.length || !timingSafeEqual(providedBuffer, expectedBuffer)) {
    throw new TypeError('Identity assertion signature is invalid.');
  }
  let claims;
  try { claims = JSON.parse(decode(encoded)); } catch { throw new TypeError('Identity assertion payload is invalid.'); }
  const canonical = canonicalClaims(claims);
  const actualKeys = Object.keys(claims).sort();
  const expectedKeys = Object.keys(canonical).sort();
  if (JSON.stringify(actualKeys) !== JSON.stringify(expectedKeys)) throw new TypeError('Identity assertion contains unsupported claims.');
  if (claims.protocol !== PROTOCOL || claims.issuer !== ISSUER || claims.audience !== AUDIENCE) throw new TypeError('Identity assertion audience is invalid.');
  assertSafeId(claims.subject, 'subject');
  assertSafeId(claims.playerProfileId, 'playerProfileId');
  if (typeof claims.displayName !== 'string' || claims.displayName.length < 1 || claims.displayName.length > 40) throw new TypeError('Identity display name is invalid.');
  const issued = Date.parse(claims.issuedAt);
  const expires = Date.parse(claims.expiresAt);
  if (!Number.isFinite(issued) || !Number.isFinite(expires) || issued > nowMs + 30_000 || expires <= nowMs || expires - issued > 5 * 60_000 + 1_000) {
    throw new TypeError('Identity assertion is expired or outside the accepted time window.');
  }
  return canonical;
}

export function deriveIdentityLinkId({ issuer, subject }, secret) {
  if (typeof secret !== 'string' || secret.length < 32) throw new TypeError('Identity link secret is not configured.');
  assertSafeId(subject, 'subject');
  const digest = createHmac('sha256', secret).update(`${issuer}:${subject}`).digest('hex');
  return `identity-link.${digest}`;
}

export const identityProtocol = Object.freeze({ protocol: PROTOCOL, issuer: ISSUER, audience: AUDIENCE });
