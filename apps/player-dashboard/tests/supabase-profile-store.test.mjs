import assert from 'node:assert/strict';
import test from 'node:test';
import { SupabaseProfileStore } from '../src/domain/supabase-profile-store.mjs';
import { ProfileConflictError } from '../src/domain/profile-store.mjs';

const secret = 'x'.repeat(64);

test('profile store uses anon transport plus server-only secure bridge', async (t) => {
  const originalFetch = global.fetch;
  t.after(() => { global.fetch = originalFetch; });
  let captured;
  global.fetch = async (url, options) => {
    captured = { url, options, body: JSON.parse(options.body) };
    return new Response(JSON.stringify({ playerProfileId: 'player.test.user', revision: 1 }), {
      status: 200,
      headers: { 'content-type': 'application/json' },
    });
  };

  const store = new SupabaseProfileStore({
    url: 'https://example.supabase.co',
    anonKey: 'anon-key',
    bridgeSecret: secret,
  });
  const profile = await store.get('player.test.user');

  assert.equal(profile.revision, 1);
  assert.equal(captured.url, 'https://example.supabase.co/rest/v1/rpc/wm_secure_player_profile');
  assert.equal(captured.options.headers.apikey, 'anon-key');
  assert.equal(captured.options.headers.Authorization, 'Bearer anon-key');
  assert.equal(captured.body.p_operation, 'get');
  assert.equal(captured.body.p_server_secret, secret);
});

test('profile store preserves optimistic conflict semantics', async (t) => {
  const originalFetch = global.fetch;
  t.after(() => { global.fetch = originalFetch; });
  global.fetch = async () => new Response(JSON.stringify({ message: 'profile_conflict' }), {
    status: 400,
    headers: { 'content-type': 'application/json' },
  });

  const store = new SupabaseProfileStore({
    url: 'https://example.supabase.co',
    anonKey: 'anon-key',
    bridgeSecret: secret,
  });

  await assert.rejects(
    () => store.put({ playerProfileId: 'player.test.user' }, { expectedRevision: 1 }),
    ProfileConflictError,
  );
});

test('profile store rejects missing server-only bridge secret', () => {
  assert.throws(
    () => new SupabaseProfileStore({ url: 'https://example.supabase.co', anonKey: 'anon-key', bridgeSecret: '' }),
    /WORLD_MAKERS_PROFILE_BRIDGE_SECRET/,
  );
});
