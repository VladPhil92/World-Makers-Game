import { createHash, randomBytes } from 'node:crypto';

const DEFAULT_LAUNCH_TTL_MS = 120_000;
const DEFAULT_SYNC_TTL_MS = 8 * 60 * 60 * 1000;
const TOKEN_BYTES = 32;

function clone(value) { return structuredClone(value); }

function opaqueToken() {
  return randomBytes(TOKEN_BYTES).toString('base64url');
}

function tokenKey(value) {
  if (typeof value !== 'string' || value.length < 32 || value.length > 256 || !/^[A-Za-z0-9_-]+$/.test(value)) {
    throw new TypeError('Native launch token is invalid.');
  }
  return createHash('sha256').update(value, 'utf8').digest('hex');
}

function iso(ms) { return new Date(ms).toISOString(); }

export class NativeLaunchSessionStore {
  #launchTickets = new Map();
  #syncTokens = new Map();
  #launchTtlMs;
  #syncTtlMs;

  constructor({ launchTtlMs = DEFAULT_LAUNCH_TTL_MS, syncTtlMs = DEFAULT_SYNC_TTL_MS } = {}) {
    if (!Number.isInteger(launchTtlMs) || launchTtlMs < 10_000 || launchTtlMs > 10 * 60_000) throw new TypeError('launchTtlMs is invalid.');
    if (!Number.isInteger(syncTtlMs) || syncTtlMs < 60_000 || syncTtlMs > 24 * 60 * 60 * 1000) throw new TypeError('syncTtlMs is invalid.');
    this.#launchTtlMs = launchTtlMs;
    this.#syncTtlMs = syncTtlMs;
  }

  issue({ context, profileRevision, now = Date.now() }) {
    if (!context || typeof context !== 'object' || Array.isArray(context) || typeof context.playerId !== 'string') {
      throw new TypeError('Signed launch context is required.');
    }
    if (!Number.isInteger(profileRevision) || profileRevision < 1) throw new TypeError('profileRevision is invalid.');
    const ticket = opaqueToken();
    const expiresAtMs = now + this.#launchTtlMs;
    this.#launchTickets.set(tokenKey(ticket), {
      context: clone(context),
      playerId: context.playerId,
      profileRevision,
      epicId: context.epicResume?.epicId ?? null,
      expiresAtMs,
    });
    return { ticket, expiresAt: iso(expiresAtMs) };
  }

  redeem(ticket, { now = Date.now() } = {}) {
    const key = tokenKey(ticket);
    const session = this.#launchTickets.get(key);
    // Launch tickets are strictly single use even when the stored record has expired.
    this.#launchTickets.delete(key);
    if (!session || session.expiresAtMs <= now) throw new TypeError('Native launch ticket is invalid or expired.');

    const progressSyncToken = opaqueToken();
    const syncExpiresAtMs = now + this.#syncTtlMs;
    this.#syncTokens.set(tokenKey(progressSyncToken), {
      playerId: session.playerId,
      epicId: session.epicId,
      issuedProfileRevision: session.profileRevision,
      expiresAtMs: syncExpiresAtMs,
    });

    return {
      context: clone(session.context),
      progressSync: {
        token: progressSyncToken,
        scope: 'epic-checkpoint:write',
        expiresAt: iso(syncExpiresAtMs),
      },
    };
  }

  authorizeProgressSync(token, epicId, { now = Date.now() } = {}) {
    const key = tokenKey(token);
    const session = this.#syncTokens.get(key);
    if (!session || session.expiresAtMs <= now) {
      if (session) this.#syncTokens.delete(key);
      throw new TypeError('Progress sync token is invalid or expired.');
    }
    const normalizedEpicId = String(epicId ?? '');
    if (!normalizedEpicId) throw new TypeError('epicId is required.');
    if (session.epicId !== null && session.epicId !== normalizedEpicId) throw new TypeError('Progress sync token is bound to another epic.');
    if (session.epicId === null) session.epicId = normalizedEpicId;
    return clone(session);
  }

  revokeProgressSync(token) {
    return this.#syncTokens.delete(tokenKey(token));
  }
}
