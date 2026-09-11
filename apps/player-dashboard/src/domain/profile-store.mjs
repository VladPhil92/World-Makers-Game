import { mkdir, readFile, rename, writeFile } from 'node:fs/promises';
import { dirname } from 'node:path';

function clone(value) {
  return structuredClone(value);
}

function safeProfileId(value) {
  if (typeof value !== 'string' || !/^player\.[a-z0-9._:-]{3,120}$/i.test(value)) throw new TypeError('Invalid player profile ID.');
  return value;
}

export class ProfileConflictError extends Error {}

export class JsonFileProfileStore {
  #filePath;
  #queue = Promise.resolve();

  constructor(filePath) {
    if (typeof filePath !== 'string' || !filePath.trim()) throw new TypeError('Profile store path is required.');
    this.#filePath = filePath;
  }

  async #readDatabase() {
    try {
      const raw = await readFile(this.#filePath, 'utf8');
      const parsed = JSON.parse(raw);
      if (parsed.schemaVersion !== 1 || typeof parsed.profiles !== 'object' || parsed.profiles === null || Array.isArray(parsed.profiles)) {
        throw new TypeError('Profile store file is invalid.');
      }
      return parsed;
    } catch (error) {
      if (error?.code === 'ENOENT') return { schemaVersion: 1, profiles: {} };
      throw error;
    }
  }

  async #writeDatabase(database) {
    await mkdir(dirname(this.#filePath), { recursive: true });
    const temporary = `${this.#filePath}.tmp-${process.pid}-${Date.now()}`;
    await writeFile(temporary, `${JSON.stringify(database, null, 2)}\n`, { encoding: 'utf8', mode: 0o600 });
    await rename(temporary, this.#filePath);
  }

  async get(playerProfileId) {
    const id = safeProfileId(playerProfileId);
    const database = await this.#readDatabase();
    return database.profiles[id] ? clone(database.profiles[id]) : null;
  }

  async put(profile, { expectedRevision = null } = {}) {
    safeProfileId(profile?.playerProfileId);
    return this.#enqueue(async () => {
      const database = await this.#readDatabase();
      const current = database.profiles[profile.playerProfileId] ?? null;
      if (expectedRevision !== null) {
        const currentRevision = current?.revision ?? 0;
        if (currentRevision !== expectedRevision) throw new ProfileConflictError('Profile revision conflict.');
      }
      const next = clone(profile);
      next.revision = (current?.revision ?? 0) + 1;
      next.updatedAt = new Date().toISOString();
      database.profiles[next.playerProfileId] = next;
      await this.#writeDatabase(database);
      return clone(next);
    });
  }

  async #enqueue(operation) {
    const result = this.#queue.then(operation, operation);
    this.#queue = result.then(() => undefined, () => undefined);
    return result;
  }
}

export class MemoryProfileStore {
  #profiles = new Map();

  async get(playerProfileId) {
    safeProfileId(playerProfileId);
    return this.#profiles.has(playerProfileId) ? clone(this.#profiles.get(playerProfileId)) : null;
  }

  async put(profile, { expectedRevision = null } = {}) {
    safeProfileId(profile?.playerProfileId);
    const current = this.#profiles.get(profile.playerProfileId) ?? null;
    if (expectedRevision !== null && (current?.revision ?? 0) !== expectedRevision) throw new ProfileConflictError('Profile revision conflict.');
    const next = clone(profile);
    next.revision = (current?.revision ?? 0) + 1;
    next.updatedAt = new Date().toISOString();
    this.#profiles.set(next.playerProfileId, next);
    return clone(next);
  }
}
