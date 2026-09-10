import { requireAuthorizedChild } from './authorization.mjs';

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
  ].some((token) => serialized.includes(token));
}
