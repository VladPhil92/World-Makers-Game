import test from 'node:test';
import assert from 'node:assert/strict';
import { createLaunchContext, verifyLaunchContext } from '../src/domain/launch.mjs';
import { createDemoPlayer } from '../src/data/demo-player.mjs';

const secret = 'world-makers-test-signing-secret-000000000000';
const now = Date.parse('2026-09-11T16:00:00.000Z');

test('launch context is short-lived, signed and carries player configuration', () => {
  const player = createDemoPlayer();
  const context = createLaunchContext({ player, secret, now, ttlSeconds: 120 });
  assert.equal(context.playerId, player.playerId);
  assert.equal(context.avatarId, 'avatar.child-explorer.v1');
  assert.equal(context.selectedMode, 'free-explore');
  assert.equal(context.selectedWorld, 'world.rainforest');
  assert.equal(typeof context.signature, 'string');
  assert.equal(context.signature.length, 64);
  assert.equal(verifyLaunchContext(context, secret, now + 30_000), true);
  assert.equal(verifyLaunchContext(context, secret, now + 121_000), false);
});

test('tampered launch context is rejected', () => {
  const context = createLaunchContext({ player: createDemoPlayer(), secret, now });
  const tampered = { ...context, selectedWorld: 'world.moonforge' };
  assert.equal(verifyLaunchContext(tampered, secret, now + 1_000), false);
});

test('launch signing fails closed without a production-grade secret', () => {
  assert.throws(() => createLaunchContext({ player: createDemoPlayer(), secret: 'short', now }), /not configured/);
});
