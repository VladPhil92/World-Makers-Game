const EPIC_DEFINITIONS = Object.freeze({
  'epic.eclipse-engine': Object.freeze({
    title: 'El Motor del Eclipse',
    parentLabel: 'The Eclipse Engine',
    chapters: Object.freeze([
      ['chapter.eclipse.decode-orbit', 'Descifrar la órbita'],
      ['chapter.eclipse.align-mirror-lattice', 'Alinear la red de espejos'],
      ['chapter.eclipse.power-core', 'Despertar el núcleo'],
      ['chapter.eclipse.recover-language', 'Recuperar las palabras perdidas'],
      ['chapter.eclipse.decode-archive-myth', 'Leer el archivo imposible'],
      ['chapter.eclipse.revise-explanation', 'Sincronizar una nueva explicación'],
    ]),
    persistentObjectiveGroups: Object.freeze(['mathematics', 'geometry', 'physics', 'english-language', 'literature', 'philosophy-for-children']),
  }),
  'epic.garden-end-winter': Object.freeze({
    title: 'El Jardín al Final del Invierno',
    parentLabel: 'The Garden at the End of Winter',
    chapters: Object.freeze([
      ['chapter.garden.restore-cellular-balance', 'Despertar las raíces'],
      ['chapter.garden.repair-soil-water', 'Limpiar el agua dormida'],
      ['chapter.garden.restore-pollination', 'Traer de vuelta a los mensajeros'],
      ['chapter.garden.choose-shared-restoration', 'Elegir qué será del deshielo'],
    ]),
    // Mathematics and philosophy are session-only mastery gates in Garden and are
    // intentionally absent from the persistent objective summary.
    persistentObjectiveGroups: Object.freeze(['biology', 'chemistry', 'ecology', 'ethics']),
  }),
});

const STATE_IN_PROGRESS = 'in-progress';
const STATE_COMPLETE = 'complete';

function clone(value) { return structuredClone(value); }

function boundedInt(value, min, max, field) {
  if (!Number.isInteger(value) || value < min || value > max) throw new TypeError(`${field} is invalid.`);
  return value;
}

function validIso(value) {
  if (typeof value !== 'string' || !Number.isFinite(Date.parse(value))) throw new TypeError('updatedAt is invalid.');
  return value;
}

function normalizeObjectiveSummary(epicId, items = []) {
  if (!Array.isArray(items) || items.length > 12) throw new TypeError('objectiveSummary is invalid.');
  const allowed = new Set(EPIC_DEFINITIONS[epicId].persistentObjectiveGroups);
  const seen = new Set();
  return items.map((item) => {
    const objectiveGroupId = String(item?.objectiveGroupId ?? '');
    if (!allowed.has(objectiveGroupId) || seen.has(objectiveGroupId)) throw new TypeError('objectiveSummary group is invalid.');
    seen.add(objectiveGroupId);
    const totalUnits = boundedInt(item.totalUnits, 1, 100, 'totalUnits');
    const completedUnits = boundedInt(item.completedUnits, 0, totalUnits, 'completedUnits');
    return { objectiveGroupId, completedUnits, totalUnits };
  });
}

export function epicDefinition(epicId) {
  const definition = EPIC_DEFINITIONS[epicId];
  return definition ? clone(definition) : null;
}

export function normalizeEpicProgressRecord(input) {
  if (!input || typeof input !== 'object' || Array.isArray(input)) throw new TypeError('epic progress is required.');
  const epicId = String(input.epicId ?? '');
  const definition = EPIC_DEFINITIONS[epicId];
  if (!definition) throw new TypeError('epicId is invalid.');

  const state = input.state === STATE_COMPLETE ? STATE_COMPLETE : STATE_IN_PROGRESS;
  const chapterCount = definition.chapters.length;
  let chapterIndex;
  let chapterId;
  if (state === STATE_COMPLETE) {
    chapterIndex = chapterCount;
    chapterId = null;
  } else {
    chapterIndex = boundedInt(input.chapterIndex, 0, chapterCount - 1, 'chapterIndex');
    chapterId = String(input.chapterId ?? '');
    if (definition.chapters[chapterIndex][0] !== chapterId) throw new TypeError('chapterId does not match chapterIndex.');
  }

  return {
    epicId,
    chapterId,
    chapterIndex,
    chapterCount,
    state,
    progressPercent: state === STATE_COMPLETE ? 100 : Math.floor((chapterIndex / chapterCount) * 100),
    objectiveSummary: normalizeObjectiveSummary(epicId, input.objectiveSummary ?? []),
    updatedAt: validIso(input.updatedAt),
  };
}

export function normalizePlayerProgress(progress = {}) {
  const source = progress && typeof progress === 'object' && !Array.isArray(progress) ? progress : {};
  const epics = Array.isArray(source.epics) ? source.epics.map(normalizeEpicProgressRecord) : [];
  if (new Set(epics.map((item) => item.epicId)).size !== epics.length) throw new TypeError('Duplicate epic progress.');
  const currentEpicId = source.currentEpicId == null ? null : String(source.currentEpicId);
  if (currentEpicId !== null && !epics.some((item) => item.epicId === currentEpicId && item.state === STATE_IN_PROGRESS)) {
    throw new TypeError('currentEpicId must reference an in-progress epic.');
  }
  return {
    level: Math.max(1, Number.parseInt(source.level, 10) || 1),
    discoveries: Math.max(0, Number.parseInt(source.discoveries, 10) || 0),
    builds: Math.max(0, Number.parseInt(source.builds, 10) || 0),
    currentAdventure: source.currentAdventure && typeof source.currentAdventure === 'object' ? clone(source.currentAdventure) : null,
    currentEpicId,
    epics,
  };
}

export function updateProfileEpicProgress(profile, input) {
  const next = clone(profile);
  next.progress = normalizePlayerProgress(next.progress);
  const incoming = normalizeEpicProgressRecord(input);
  const existingIndex = next.progress.epics.findIndex((item) => item.epicId === incoming.epicId);
  const existing = existingIndex >= 0 ? next.progress.epics[existingIndex] : null;

  if (existing?.state === STATE_COMPLETE && incoming.state !== STATE_COMPLETE) throw new TypeError('Completed epic progress cannot regress.');
  if (existing && incoming.chapterIndex < existing.chapterIndex) throw new TypeError('Epic chapter progress cannot regress.');

  if (existingIndex >= 0) next.progress.epics[existingIndex] = incoming;
  else next.progress.epics.push(incoming);
  next.progress.currentEpicId = incoming.state === STATE_COMPLETE ? null : incoming.epicId;
  return next;
}

function playerJourney(record) {
  const definition = EPIC_DEFINITIONS[record.epicId];
  const completed = record.state === STATE_COMPLETE;
  const chapter = completed ? null : definition.chapters[record.chapterIndex];
  return {
    epicId: record.epicId,
    title: definition.title,
    state: record.state,
    chapterId: record.chapterId,
    chapterLabel: chapter?.[1] ?? 'Viaje completado',
    chapterNumber: completed ? record.chapterCount : record.chapterIndex + 1,
    chapterCount: record.chapterCount,
    progressPercent: record.progressPercent,
    resumeCopy: completed ? 'Este viaje ya transformó tu mundo.' : 'Puedes continuar desde este capítulo cuando quieras.',
  };
}

export function buildPlayerEpicReadModel(progress = {}) {
  const normalized = normalizePlayerProgress(progress);
  const journeys = normalized.epics.map(playerJourney);
  const current = normalized.currentEpicId ? journeys.find((item) => item.epicId === normalized.currentEpicId) ?? null : null;
  return { current, journeys };
}

export function buildLaunchEpicResume(progress = {}) {
  const normalized = normalizePlayerProgress(progress);
  if (!normalized.currentEpicId) return null;
  const record = normalized.epics.find((item) => item.epicId === normalized.currentEpicId && item.state === STATE_IN_PROGRESS);
  if (!record) return null;
  return {
    epicId: record.epicId,
    chapterId: record.chapterId,
    chapterIndex: record.chapterIndex,
    chapterCount: record.chapterCount,
  };
}

export function buildParentEpicProjection(progress = {}) {
  const normalized = normalizePlayerProgress(progress);
  return normalized.epics.map((record) => ({
    adventureId: record.epicId,
    epicId: record.epicId,
    label: EPIC_DEFINITIONS[record.epicId].parentLabel,
    state: record.state,
    chapterIndex: record.chapterIndex,
    chapterCount: record.chapterCount,
    progressPercent: record.progressPercent,
    objectiveSummary: clone(record.objectiveSummary),
  }));
}
