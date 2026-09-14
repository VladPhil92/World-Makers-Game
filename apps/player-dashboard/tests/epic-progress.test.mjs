import test from 'node:test';
import assert from 'node:assert/strict';
import {
  buildLaunchEpicResume,
  buildParentEpicProjection,
  buildPlayerEpicReadModel,
  normalizeEpicProgressRecord,
  normalizePlayerProgress,
  updateProfileEpicProgress,
} from '../src/domain/epic-progress.mjs';

const gardenCheckpoint = {
  epicId: 'epic.garden-end-winter',
  chapterId: 'chapter.garden.restore-pollination',
  chapterIndex: 2,
  chapterCount: 4,
  state: 'in-progress',
  objectiveSummary: [
    { objectiveGroupId: 'biology', completedUnits: 2, totalUnits: 2 },
    { objectiveGroupId: 'chemistry', completedUnits: 2, totalUnits: 2 },
    { objectiveGroupId: 'ecology', completedUnits: 0, totalUnits: 2 },
  ],
  updatedAt: '2026-09-14T03:17:11.000Z',
};

test('M5.6D Garden checkpoint persists chapter state without session-only mastery profiling', () => {
  const normalized = normalizeEpicProgressRecord(gardenCheckpoint);
  assert.equal(normalized.progressPercent, 50);
  assert.deepEqual(normalized.objectiveSummary.map((item) => item.objectiveGroupId), ['biology', 'chemistry', 'ecology']);
  assert.throws(() => normalizeEpicProgressRecord({
    ...gardenCheckpoint,
    objectiveSummary: [{ objectiveGroupId: 'mathematics', completedUnits: 1, totalUnits: 1 }],
  }), /objectiveSummary/);
  assert.throws(() => normalizeEpicProgressRecord({
    ...gardenCheckpoint,
    objectiveSummary: [{ objectiveGroupId: 'philosophy-for-children', completedUnits: 1, totalUnits: 1 }],
  }), /objectiveSummary/);
});

test('M5.6F unknown epic progress states fail closed', () => {
  assert.throws(() => normalizeEpicProgressRecord({ ...gardenCheckpoint, state: 'paused' }), /state is invalid/);
  assert.throws(() => normalizeEpicProgressRecord({ ...gardenCheckpoint, state: undefined }), /state is invalid/);
});

test('M5.6D epic profile progress is monotonic and completed journeys cannot regress', () => {
  const profile = { progress: normalizePlayerProgress({ currentEpicId: 'epic.garden-end-winter', epics: [gardenCheckpoint] }) };
  assert.throws(() => updateProfileEpicProgress(profile, {
    ...gardenCheckpoint,
    chapterId: 'chapter.garden.repair-soil-water',
    chapterIndex: 1,
  }), /cannot regress/);

  const complete = updateProfileEpicProgress(profile, {
    ...gardenCheckpoint,
    chapterId: null,
    chapterIndex: 4,
    state: 'complete',
    objectiveSummary: [
      { objectiveGroupId: 'biology', completedUnits: 2, totalUnits: 2 },
      { objectiveGroupId: 'chemistry', completedUnits: 2, totalUnits: 2 },
      { objectiveGroupId: 'ecology', completedUnits: 2, totalUnits: 2 },
      { objectiveGroupId: 'ethics', completedUnits: 1, totalUnits: 1 },
    ],
  });
  assert.equal(complete.progress.currentEpicId, null);
  assert.throws(() => updateProfileEpicProgress(complete, gardenCheckpoint), /cannot regress/);
});

test('M5.6D child journey uses calm chapter language and no pressure-loop vocabulary', () => {
  const progress = normalizePlayerProgress({ currentEpicId: 'epic.garden-end-winter', epics: [gardenCheckpoint] });
  const readModel = buildPlayerEpicReadModel(progress);
  assert.equal(readModel.current.title, 'El Jardín al Final del Invierno');
  assert.equal(readModel.current.chapterLabel, 'Traer de vuelta a los mensajeros');
  assert.match(readModel.current.resumeCopy, /cuando quieras/);
  const serialized = JSON.stringify(readModel).toLowerCase();
  for (const forbidden of ['score', 'streak', 'fomo', 'lesson', 'quiz', 'wrong-answer']) assert.equal(serialized.includes(forbidden), false);
});

test('M5.6D launch and parent projections expose bounded stable checkpoint data only', () => {
  const progress = normalizePlayerProgress({ currentEpicId: 'epic.garden-end-winter', epics: [gardenCheckpoint] });
  assert.deepEqual(buildLaunchEpicResume(progress), {
    epicId: 'epic.garden-end-winter',
    chapterId: 'chapter.garden.restore-pollination',
    chapterIndex: 2,
    chapterCount: 4,
  });
  const parent = buildParentEpicProjection(progress);
  assert.equal(parent.length, 1);
  assert.equal(parent[0].progressPercent, 50);
  const serialized = JSON.stringify(parent).toLowerCase();
  for (const forbidden of ['evidenceeventid', 'producerrefid', 'answer', 'freetext', 'moralchoice', 'personalityscore', 'ideologylabel']) {
    assert.equal(serialized.includes(forbidden), false);
  }
});
