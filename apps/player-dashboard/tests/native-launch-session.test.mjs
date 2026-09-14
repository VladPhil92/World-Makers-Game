import test from 'node:test';
import assert from 'node:assert/strict';
import { NativeLaunchSessionStore } from '../src/domain/native-launch-session.mjs';

const context = {
  version: 1,
  playerId: 'player.test.native-01',
  avatarId: 'avatar.child-explorer.v1',
  avatarLoadout: {},
  selectedMode: 'missions',
  selectedWorld: 'world.moonforge',
  missionId: 'mission.test',
  inventoryEntitlements: [],
  epicResume: {
    epicId: 'epic.eclipse-engine',
    chapterId: 'chapter.eclipse.power-core',
    chapterIndex: 2,
    chapterCount: 6,
  },
  issuedAt: '2026-09-14T12:00:00.000Z',
  expiresAt: '2026-09-14T12:02:00.000Z',
  signature: 'deadbeef',
};

test('M5.6F launch ticket is opaque, single-use and yields a scoped sync token', () => {
  const now = Date.parse('2026-09-14T12:00:00.000Z');
  const store = new NativeLaunchSessionStore({ launchTtlMs: 120_000, syncTtlMs: 60 * 60 * 1000 });
  const issued = store.issue({ context, profileRevision: 7, now });
  assert.match(issued.ticket, /^[A-Za-z0-9_-]{40,}$/);
  assert.equal(JSON.stringify(issued).includes(context.playerId), false);

  const redeemed = store.redeem(issued.ticket, { now: now + 1_000 });
  assert.equal(redeemed.context.playerId, context.playerId);
  assert.equal(redeemed.progressSync.scope, 'epic-checkpoint:write');
  assert.match(redeemed.progressSync.token, /^[A-Za-z0-9_-]{40,}$/);
  assert.throws(() => store.redeem(issued.ticket, { now: now + 2_000 }), /invalid or expired/);

  const auth = store.authorizeProgressSync(redeemed.progressSync.token, 'epic.eclipse-engine', { now: now + 3_000 });
  assert.equal(auth.playerId, context.playerId);
  assert.equal(auth.epicId, 'epic.eclipse-engine');
  assert.throws(
    () => store.authorizeProgressSync(redeemed.progressSync.token, 'epic.garden-end-winter', { now: now + 4_000 }),
    /another epic/,
  );
});

test('M5.6F launch and sync tokens fail closed after expiry', () => {
  const now = Date.parse('2026-09-14T12:00:00.000Z');
  const store = new NativeLaunchSessionStore({ launchTtlMs: 10_000, syncTtlMs: 60_000 });
  const expiredTicket = store.issue({ context, profileRevision: 7, now });
  assert.throws(() => store.redeem(expiredTicket.ticket, { now: now + 10_001 }), /invalid or expired/);

  const live = store.issue({ context: { ...context, epicResume: null }, profileRevision: 7, now });
  const redeemed = store.redeem(live.ticket, { now: now + 1_000 });
  const first = store.authorizeProgressSync(redeemed.progressSync.token, 'epic.garden-end-winter', { now: now + 2_000 });
  assert.equal(first.epicId, 'epic.garden-end-winter');
  assert.throws(
    () => store.authorizeProgressSync(redeemed.progressSync.token, 'epic.garden-end-winter', { now: now + 61_001 }),
    /invalid or expired/,
  );
});
