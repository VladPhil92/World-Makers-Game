// Issuer-side half of the ctg-one-identity-v1 protocol. Must stay wire-compatible with
// apps/player-dashboard/src/domain/identity.mjs (the verifier). Kept as a separate small copy
// rather than a shared package because each app under apps/ ships and deploys independently.
import { createHmac } from 'node:crypto';

const PROTOCOL = 'ctg-one-identity-v1';
const ISSUER = 'ctg-one';
const AUDIENCE = 'world-makers';

function encode(value) {
  return Buffer.from(value).toString('base64url');
}

function hmac(value, secret) {
  return createHmac('sha256', secret).update(value).digest('base64url');
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
  const payload = {
    protocol: PROTOCOL,
    issuer: ISSUER,
    audience: AUDIENCE,
    subject: claims.subject,
    playerProfileId: claims.playerProfileId,
    displayName: claims.displayName.trim(),
    issuedAt: new Date(nowMs).toISOString(),
    expiresAt: new Date(nowMs + 5 * 60_000).toISOString(),
  };
  const encoded = encode(JSON.stringify(payload));
  return `${encoded}.${hmac(encoded, secret)}`;
}
