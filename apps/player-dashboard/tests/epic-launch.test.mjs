import test from 'node:test';
import assert from 'node:assert/strict';
import { createLaunchContext, verifyLaunchContext } from '../src/domain/launch.mjs';
import { createDemoPlayer } from '../src/data/demo-player.mjs';

const secret = 'world-makers-epic-launch-test-secret-000000000';
const now = Date.parse('2026-09-14T11:00:00.000Z');

test('M5.6D signed launch context carries the resumable epic chapter', () => {
  const context = createLaunchContext({ player: createDemoPlayer(), secret, now });
  assert.deepEqual(context.epicResume, {
    epicId: 'epic.garden-end-winter',
    chapterId: 'chapter.garden.restore-pollination',
    chapterIndex: 2,
    chapterCount: 4,
  });
  assert.equal(verifyLaunchContext(context, secret, now + 1_000), true);
});

test('M5.6D tampering with epic resume invalidates the launch signature', () => {
  const context = createLaunchContext({ player: createDemoPlayer(), secret, now });
  const tampered = {
    ...context,
    epicResume: { ...context.epicResume, chapterIndex: 3, chapterId: 'chapter.garden.choose-shared-restoration' },
  };
  assert.equal(verifyLaunchContext(tampered, secret, now + 1_000), false);
});
