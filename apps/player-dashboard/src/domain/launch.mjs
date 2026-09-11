import { createHmac, timingSafeEqual } from 'node:crypto';
import { validateLoadout, validateSelection } from './catalog.mjs';

const launchVersion = 1;
const defaultTtlSeconds = 120;

function canonicalPayload(payload) {
  return JSON.stringify({
    version: payload.version,
    playerId: payload.playerId,
    avatarId: payload.avatarId,
    avatarLoadout: payload.avatarLoadout,
    selectedMode: payload.selectedMode,
    selectedWorld: payload.selectedWorld,
    missionId: payload.missionId,
    inventoryEntitlements: payload.inventoryEntitlements,
    issuedAt: payload.issuedAt,
    expiresAt: payload.expiresAt,
  });
}

function sign(payload, secret) {
  return createHmac('sha256', secret).update(canonicalPayload(payload)).digest('hex');
}

export function createLaunchContext({ player, selection = player?.selection, secret, now = Date.now(), ttlSeconds = defaultTtlSeconds }) {
  if (!secret || typeof secret !== 'string' || secret.length < 32) throw new TypeError('Launch signing secret is not configured.');
  if (!player?.playerId || !player?.avatarId) throw new TypeError('Player identity is incomplete.');
  const loadout = validateLoadout(player.loadout);
  const normalizedSelection = validateSelection(selection);
  const issuedAt = new Date(now).toISOString();
  const expiresAt = new Date(now + ttlSeconds * 1000).toISOString();
  const payload = {
    version: launchVersion,
    playerId: player.playerId,
    avatarId: player.avatarId,
    avatarLoadout: loadout,
    selectedMode: normalizedSelection.modeId,
    selectedWorld: normalizedSelection.worldId,
    missionId: normalizedSelection.missionId,
    inventoryEntitlements: [...new Set(player.entitlements ?? [])].sort(),
    issuedAt,
    expiresAt,
  };
  return { ...payload, signature: sign(payload, secret) };
}

export function verifyLaunchContext(context, secret, now = Date.now()) {
  if (!context || typeof context !== 'object' || typeof context.signature !== 'string') return false;
  if (!secret || secret.length < 32) return false;
  const { signature, ...payload } = context;
  const expected = sign(payload, secret);
  const actualBuffer = Buffer.from(signature, 'hex');
  const expectedBuffer = Buffer.from(expected, 'hex');
  if (actualBuffer.length !== expectedBuffer.length || !timingSafeEqual(actualBuffer, expectedBuffer)) return false;
  const expiresAt = Date.parse(payload.expiresAt);
  return Number.isFinite(expiresAt) && expiresAt > now;
}
