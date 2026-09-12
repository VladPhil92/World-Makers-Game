import test from 'node:test';
import assert from 'node:assert/strict';
import { createNativeLaunchUri, decodeNativeLaunchUri } from '../src/domain/native-handoff.mjs';

test('native handoff round-trips the signed launch context', () => {
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

test('native handoff rejects unrelated URI schemes', () => {
  assert.throws(() => decodeNativeLaunchUri('https://example.com/launch?payload=x&protocol=y'), /Unsupported World Makers launch URI/);
});
