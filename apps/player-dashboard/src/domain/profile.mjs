import { cloneDefaultLoadout, cloneDefaultSelection, validateLoadout, validateSelection } from './catalog.mjs';

function clone(value) { return structuredClone(value); }

export function createPlayerProfile({ playerProfileId, identityLinkId, displayName, seed = {} }) {
  if (typeof identityLinkId !== 'string' || !identityLinkId.startsWith('identity-link.')) throw new TypeError('Identity link is invalid.');
  return {
    schemaVersion: 1,
    playerProfileId,
    identityLinkId,
    displayName: String(displayName || 'Explorer').trim().slice(0, 40) || 'Explorer',
    avatarId: 'avatar.child-explorer.v1',
    loadout: validateLoadout(seed.loadout ?? cloneDefaultLoadout()),
    selection: validateSelection(seed.selection ?? cloneDefaultSelection()),
    entitlements: Array.isArray(seed.entitlements) ? [...new Set(seed.entitlements.filter((item) => typeof item === 'string'))] : [],
    progress: clone(seed.progress ?? { level: 1, discoveries: 0, builds: 0, currentAdventure: null }),
    preferences: {
      reducedMotion: seed.preferences?.reducedMotion === true,
      preferredModeId: seed.preferences?.preferredModeId ?? null,
      preferredWorldId: seed.preferences?.preferredWorldId ?? null,
    },
    storeRequests: Array.isArray(seed.storeRequests) ? clone(seed.storeRequests) : [],
    revision: 0,
    createdAt: new Date().toISOString(),
    updatedAt: new Date().toISOString(),
  };
}

export function profilePlayerReadModel(profile) {
  return {
    playerId: profile.playerProfileId,
    displayName: profile.displayName,
    avatarId: profile.avatarId,
    loadout: clone(profile.loadout),
    selection: clone(profile.selection),
    entitlements: clone(profile.entitlements),
    progress: clone(profile.progress),
    preferences: clone(profile.preferences),
    profileRevision: profile.revision,
    profileUpdatedAt: profile.updatedAt,
  };
}

export function updateProfileLoadout(profile, loadout) {
  return { ...clone(profile), loadout: validateLoadout(loadout) };
}

export function updateProfileSelection(profile, selection) {
  return { ...clone(profile), selection: validateSelection(selection) };
}

export function updateProfilePreferences(profile, patch) {
  const next = clone(profile);
  if ('reducedMotion' in patch) {
    if (typeof patch.reducedMotion !== 'boolean') throw new TypeError('reducedMotion must be boolean.');
    next.preferences.reducedMotion = patch.reducedMotion;
  }
  if ('preferredModeId' in patch) next.preferences.preferredModeId = patch.preferredModeId === null ? null : String(patch.preferredModeId);
  if ('preferredWorldId' in patch) next.preferences.preferredWorldId = patch.preferredWorldId === null ? null : String(patch.preferredWorldId);
  return next;
}

export function appendStoreRequest(profile, request) {
  const next = clone(profile);
  const existing = next.storeRequests.find((item) => item.itemId === request.itemId && item.status === 'parent-approval-required');
  if (existing) return { profile: next, request: existing, changed: false };
  next.storeRequests.push(clone(request));
  return { profile: next, request: clone(request), changed: true };
}
