import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, readFile, rm } from 'node:fs/promises';
import { join } from 'node:path';
import { tmpdir } from 'node:os';
import { JsonFileProfileStore, ProfileConflictError } from '../src/domain/profile-store.mjs';
import { createPlayerProfile } from '../src/domain/profile.mjs';

test('file profile store persists across store instances', async () => {
  const root = await mkdtemp(join(tmpdir(), 'wm-profile-store-'));
  const path = join(root, 'profiles.json');
  try {
    const firstStore = new JsonFileProfileStore(path);
    const profile = createPlayerProfile({
      playerProfileId: 'player.test.persist-01',
      identityLinkId: 'identity-link.test-persist-01',
      displayName: 'Persist Explorer',
    });
    const saved = await firstStore.put(profile, { expectedRevision: 0 });
    assert.equal(saved.revision, 1);

    const secondStore = new JsonFileProfileStore(path);
    const restored = await secondStore.get(profile.playerProfileId);
    assert.equal(restored.displayName, 'Persist Explorer');
    assert.equal(restored.revision, 1);

    const file = JSON.parse(await readFile(path, 'utf8'));
    assert.equal(file.schemaVersion, 1);
    assert.ok(file.profiles[profile.playerProfileId]);
  } finally {
    await rm(root, { recursive: true, force: true });
  }
});

test('optimistic revision conflict blocks stale overwrite', async () => {
  const root = await mkdtemp(join(tmpdir(), 'wm-profile-conflict-'));
  const path = join(root, 'profiles.json');
  try {
    const store = new JsonFileProfileStore(path);
    const profile = createPlayerProfile({
      playerProfileId: 'player.test.conflict-01',
      identityLinkId: 'identity-link.test-conflict-01',
      displayName: 'Conflict Explorer',
    });
    const first = await store.put(profile, { expectedRevision: 0 });
    const changed = { ...first, displayName: 'Updated Explorer' };
    const second = await store.put(changed, { expectedRevision: 1 });
    assert.equal(second.revision, 2);
    await assert.rejects(
      () => store.put({ ...first, displayName: 'Stale Explorer' }, { expectedRevision: 1 }),
      ProfileConflictError,
    );
  } finally {
    await rm(root, { recursive: true, force: true });
  }
});
