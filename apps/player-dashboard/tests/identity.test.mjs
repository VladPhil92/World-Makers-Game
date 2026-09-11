import test from 'node:test';
import assert from 'node:assert/strict';
import { createIdentityAssertion, deriveIdentityLinkId, verifyIdentityAssertion } from '../src/domain/identity.mjs';

const secret = 'identity-test-secret-0123456789-abcdef';
const now = Date.parse('2026-09-11T19:00:00.000Z');
const claims = {
  subject: 'ctg.user.12345',
  playerProfileId: 'player.ctg.explorer-12345',
  displayName: 'Explorer Nova',
};

test('CTG One assertion round-trips and binds expected audience', () => {
  const assertion = createIdentityAssertion(claims, secret, now);
  const verified = verifyIdentityAssertion(assertion, secret, now + 30_000);
  assert.equal(verified.issuer, 'ctg-one');
  assert.equal(verified.audience, 'world-makers');
  assert.equal(verified.subject, claims.subject);
  assert.equal(verified.playerProfileId, claims.playerProfileId);
  assert.equal(verified.displayName, claims.displayName);
});

test('tampered and expired assertions fail closed', () => {
  const assertion = createIdentityAssertion(claims, secret, now);
  const [payload, signature] = assertion.split('.');
  assert.throws(() => verifyIdentityAssertion(`${payload}.${signature.slice(0, -1)}x`, secret, now + 1_000), /signature/i);
  assert.throws(() => verifyIdentityAssertion(assertion, secret, now + 6 * 60_000), /expired|window/i);
});

test('identity link is deterministic and does not expose raw subject', () => {
  const first = deriveIdentityLinkId({ issuer: 'ctg-one', subject: claims.subject }, secret);
  const second = deriveIdentityLinkId({ issuer: 'ctg-one', subject: claims.subject }, secret);
  assert.equal(first, second);
  assert.match(first, /^identity-link\.[0-9a-f]{64}$/);
  assert.equal(first.includes(claims.subject), false);
});
