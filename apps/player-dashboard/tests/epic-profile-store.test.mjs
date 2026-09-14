import test from 'node:test';
import assert from 'node:assert/strict';
import { MemoryProfileStore, ProfileConflictError } from '../src/domain/profile-store.mjs';
import { createPlayerProfile, profilePlayerReadModel, updateProfileEpicCheckpoint } from '../src/domain/profile.mjs';

const checkpoint = {
  epicId: 'epic.eclipse-engine',
  chapterId: 'chapter.eclipse.power-core',
  chapterIndex: 2,
  chapterCount: 6,
  state: 'in-progress',
  objectiveSummary: [
    { objectiveGroupId: 'mathematics', completedUnits: 3, totalUnits: 3 },
    { objectiveGroupId: 'geometry', completedUnits: 3, totalUnits: 3 },
    { objectiveGroupId: 'physics', completedUnits: 0, totalUnits: 2 },
  ],
  updatedAt: '2026-09-14T11:00:00.000Z',
};

test('M5.6D existing profile store durably carries normalized epic progress', async () => {
  const store = new MemoryProfileStore();
  let profile = createPlayerProfile({
    playerProfileId: 'player.test.epic-01',
    identityLinkId: 'identity-link.test.epic-01',
    displayName: 'Explorer',
  });
  profile = await store.put(profile, { expectedRevision: 0 });
  const changed = updateProfileEpicCheckpoint(profile, checkpoint);
  const saved = await store.put(changed, { expectedRevision: profile.revision });
  const restored = await store.get(saved.playerProfileId);
  const readModel = profilePlayerReadModel(restored);
  assert.equal(readModel.epicJourney.current.epicId, 'epic.eclipse-engine');
  assert.equal(readModel.epicJourney.current.chapterNumber, 3);
  assert.equal(readModel.progress.currentEpicId, 'epic.eclipse-engine');
});

test('M5.6D profile store retains optimistic revision protection for epic checkpoint writes', async () => {
  const store = new MemoryProfileStore();
  let profile = createPlayerProfile({
    playerProfileId: 'player.test.epic-02',
    identityLinkId: 'identity-link.test.epic-02',
    displayName: 'Explorer',
  });
  profile = await store.put(profile, { expectedRevision: 0 });
  const changed = updateProfileEpicCheckpoint(profile, checkpoint);
  await store.put(changed, { expectedRevision: profile.revision });
  await assert.rejects(() => store.put(changed, { expectedRevision: profile.revision }), ProfileConflictError);
});
