import test from 'node:test';
import assert from 'node:assert/strict';
import { createParentDashboardReadModel, containsForbiddenChildTelemetry } from '../src/domain/dashboard.mjs';
import { demoFamily } from '../src/data/demo-family.mjs';

const parentId = 'parent.demo.guardian-01';
const childId = 'child.demo.explorer-a';

function familyWithEpicProgress() {
  const family = structuredClone(demoFamily);
  const child = family.children.find((item) => item.childProfileId === childId);
  child.dashboard.adventures.push({
    adventureId: 'epic.garden-end-winter',
    epicId: 'epic.garden-end-winter',
    label: 'caller-controlled-label-is-ignored',
    state: 'in-progress',
    chapterIndex: 2,
    chapterCount: 4,
    progressPercent: 99,
    objectiveSummary: [
      { objectiveGroupId: 'biology', completedUnits: 2, totalUnits: 2, evidenceEventId: 'must-not-leak' },
      { objectiveGroupId: 'chemistry', completedUnits: 1, totalUnits: 2, producerRefId: 'must-not-leak' },
      { objectiveGroupId: 'unknown-profile-dimension', completedUnits: 99, totalUnits: 99 },
    ],
    rawEvidence: [{ answer: 'must-not-leak' }],
  });
  return family;
}

test('M5.6D parent epic summary is aggregate, bounded and label-authoritative', () => {
  const dashboard = createParentDashboardReadModel(familyWithEpicProgress(), parentId, childId);
  assert.equal(dashboard.epics.length, 1);
  const epic = dashboard.epics[0];
  assert.equal(epic.epicId, 'epic.garden-end-winter');
  assert.equal(epic.label, 'The Garden at the End of Winter');
  assert.equal(epic.chaptersCompleted, 2);
  assert.equal(epic.chapterCount, 4);
  assert.equal(epic.progressPercent, 50);
  assert.deepEqual(epic.objectives.map((item) => item.objectiveGroupId), ['biology', 'chemistry']);
  assert.equal(epic.objectives[0].progressPercent, 100);
  assert.equal(epic.objectives[1].progressPercent, 50);
});

test('M5.6D parent projection strips raw evidence and sensitive child profiling fields', () => {
  const dashboard = createParentDashboardReadModel(familyWithEpicProgress(), parentId, childId);
  assert.equal(containsForbiddenChildTelemetry(dashboard), false);
  const serialized = JSON.stringify(dashboard).toLowerCase();
  for (const forbidden of ['evidenceeventid', 'producerrefid', 'rawtelemetry', 'rawevidence', 'answer', 'moralchoice', 'personalityscore', 'ideologylabel']) {
    assert.equal(serialized.includes(forbidden), false);
  }
});
