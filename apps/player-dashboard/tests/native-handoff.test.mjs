import test from 'node:test';
import assert from 'node:assert/strict';
import {
  createNativeLaunchTicketUri,
  createNativeLaunchUri,
  decodeNativeLaunchTicketUri,
  decodeNativeLaunchUri,
} from '../src/domain/native-handoff.mjs';

test('native handoff round-trips the legacy signed launch context', () => {
  const context = {
    version: 1,
    playerId: 'player.demo',
    avatarId: 'avatar.child-explorer.v1',
    selectedMode: 'free-exploration',
    selectedWorld: 'rainforest-world',
    missionId: null,
    issuedAt: '2026-09-11T20:00:00.000Z',
    expiresAt: '2026-09-11T20:02:00.000Z',
    signature: 'abc123',
  };

  const uri = createNativeLaunchUri(context);
  assert.match(uri, /^worldmakers:\/\/launch\?/);
  assert.deepEqual(decodeNativeLaunchUri(uri), {
    protocol: 'worldmakers-launch-v1',
    context,
  });
});

test('M5.6F ticket handoff carries no player context and round-trips the opaque ticket', () => {
  const ticket = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-';
  const uri = createNativeLaunchTicketUri(ticket);
  assert.match(uri, /^worldmakers:\/\/launch\?protocol=worldmakers-launch-v2&ticket=/);
  assert.equal(uri.includes('playerId'), false);
  assert.equal(uri.includes('payload='), false);
  assert.deepEqual(decodeNativeLaunchTicketUri(uri), { protocol: 'worldmakers-launch-v2', ticket });
});

test('native handoff rejects unrelated URI schemes and payload-bearing v2 tickets', () => {
  assert.throws(() => decodeNativeLaunchUri('https://example.com/launch?payload=x&protocol=y'), /Unsupported World Makers launch URI/);
  assert.throws(
    () => decodeNativeLaunchTicketUri('worldmakers://launch?protocol=worldmakers-launch-v2&ticket=abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-&payload=secret'),
    /must not embed player context/,
  );
});
