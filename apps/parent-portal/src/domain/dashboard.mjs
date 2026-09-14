import { requireAuthorizedChild } from './authorization.mjs';

const objectiveLabels = Object.freeze({
  mathematics: 'Mathematical reasoning',
  geometry: 'Spatial reasoning',
  physics: 'Physical systems',
  'english-language': 'Contextual English',
  literature: 'Interpretation and narrative',
  'philosophy-for-children': 'Reasoning and revision',
  biology: 'Living systems',
  chemistry: 'Matter and transformations',
  ecology: 'Ecological interdependence',
  ethics: 'Perspective and tradeoffs',
});

const epicLabels = Object.freeze({
  'epic.eclipse-engine': 'The Eclipse Engine',
  'epic.garden-end-winter': 'The Garden at the End of Winter',
});

const safeProgress = (items = []) => items.map((item) => ({
  objectiveId: item.objectiveId,
  label: item.label,
  progressPercent: Math.max(0, Math.min(100, Number(item.progressPercent) || 0)),
  state: item.state,
}));

const safeBuilds = (items = []) => items.slice(0, 6).map((item) => ({
  creationId: item.creationId,
  label: item.label,
  pieceCount: Math.max(0, Number.parseInt(item.pieceCount, 10) || 0),
  biomeLabel: item.biomeLabel,
}));

const safeAdventures = (items = []) => items.slice(0, 8).map((item) => ({
  adventureId: item.adventureId,
  label: item.label,
  state: item.state,
}));

function safeEpicObjectives(items = []) {
  if (!Array.isArray(items)) return [];
  return items.slice(0, 12).flatMap((item) => {
    const objectiveGroupId = typeof item?.objectiveGroupId === 'string' ? item.objectiveGroupId : '';
    const label = objectiveLabels[objectiveGroupId];
    if (!label) return [];
    const totalUnits = Math.max(1, Math.min(100, Number.parseInt(item.totalUnits, 10) || 1));
    const completedUnits = Math.max(0, Math.min(totalUnits, Number.parseInt(item.completedUnits, 10) || 0));
    return [{
      objectiveGroupId,
      label,
      completedUnits,
      totalUnits,
      progressPercent: Math.round((completedUnits / totalUnits) * 100),
    }];
  });
}

const safeEpics = (items = []) => items.slice(0, 8).flatMap((item) => {
  const epicId = typeof item?.epicId === 'string' ? item.epicId : '';
  if (!epicLabels[epicId]) return [];
  const chapterCount = Math.max(1, Math.min(24, Number.parseInt(item.chapterCount, 10) || 1));
  const state = item.state === 'complete' ? 'complete' : 'in-progress';
  const chapterIndex = state === 'complete'
    ? chapterCount
    : Math.max(0, Math.min(chapterCount - 1, Number.parseInt(item.chapterIndex, 10) || 0));
  return [{
    epicId,
    label: epicLabels[epicId],
    state,
    chaptersCompleted: state === 'complete' ? chapterCount : chapterIndex,
    chapterCount,
    progressPercent: state === 'complete' ? 100 : Math.round((chapterIndex / chapterCount) * 100),
    objectives: safeEpicObjectives(item.objectiveSummary),
  }];
});

export function createParentDashboardReadModel(family, parentId, childProfileId) {
  const child = requireAuthorizedChild(family, parentId, childProfileId);
  const source = child.dashboard ?? {};

  return {
    schemaVersion: 1,
    child: {
      childProfileId: child.childProfileId,
      displayAlias: child.displayAlias,
    },
    playTime: {
      last7DaysMinutes: Math.max(0, Number.parseInt(source.playTime?.last7DaysMinutes, 10) || 0),
      sessionsLast7Days: Math.max(0, Number.parseInt(source.playTime?.sessionsLast7Days, 10) || 0),
    },
    learning: safeProgress(source.learning),
    recentBuilds: safeBuilds(source.recentBuilds),
    adventures: safeAdventures(source.adventures),
    epics: safeEpics(source.adventures),
    wellbeing: {
      rightToStop: true,
      pressureLoopsDetected: false,
    },
  };
}

export function containsForbiddenChildTelemetry(value) {
  const serialized = JSON.stringify(value).toLowerCase();
  return [
    'rawtelemetry',
    'chatmessage',
    'learninganswer',
    'advertisingid',
    'preciselocation',
    'paymenttoken',
    'purchasehistory',
    'rawevidence',
    'evidenceeventid',
    'producerrefid',
    'childfreetext',
    'moralchoice',
    'personalityscore',
    'ideologylabel',
  ].some((token) => serialized.includes(token));
}
