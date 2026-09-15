import assert from 'node:assert/strict';
import test from 'node:test';
import { createSupabaseClient } from '../src/domain/supabase-client.mjs';

test('privacy deletion routes through server-only secure profile bridge', async (t) => {
  const originalFetch = global.fetch;
  const originalSecret = process.env.WORLD_MAKERS_PROFILE_BRIDGE_SECRET;
  t.after(() => {
    global.fetch = originalFetch;
    if (originalSecret === undefined) delete process.env.WORLD_MAKERS_PROFILE_BRIDGE_SECRET;
    else process.env.WORLD_MAKERS_PROFILE_BRIDGE_SECRET = originalSecret;
  });

  process.env.WORLD_MAKERS_PROFILE_BRIDGE_SECRET = 'x'.repeat(64);
  let captured;
  global.fetch = async (url, options) => {
    captured = { url, options, body: JSON.parse(options.body) };
    return new Response(JSON.stringify({ deleted: true }), {
      status: 200,
      headers: { 'content-type': 'application/json' },
    });
  };

  const client = createSupabaseClient({ url: 'https://example.supabase.co', anonKey: 'anon-key' });
  const result = await client.rpc('wm_delete_player_profile', { p_player_profile_id: 'player.test.child' });

  assert.equal(result.deleted, true);
  assert.equal(captured.url, 'https://example.supabase.co/rest/v1/rpc/wm_secure_player_profile');
  assert.equal(captured.body.p_operation, 'delete');
  assert.equal(captured.body.p_player_profile_id, 'player.test.child');
  assert.equal(captured.body.p_server_secret, 'x'.repeat(64));
  assert.equal(captured.options.headers.apikey, 'anon-key');
});
