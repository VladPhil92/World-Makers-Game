import { ProfileConflictError } from './profile-store.mjs';

function safeProfileId(value) {
  if (typeof value !== 'string' || !/^player\.[a-z0-9._:-]{3,120}$/i.test(value)) throw new TypeError('Invalid player profile ID.');
  return value;
}

// Legacy dashboard profiles remain on their established RPC contract, but those
// RPCs are no longer callable with the public anon role. This adapter runs only
// inside the Node server process and therefore requires the Supabase service-role
// credential. Never expose this key to browser code, launch tickets or the Unreal
// runtime. New CTG One runtime synchronization uses the separate signed bridge.
export class SupabaseProfileStore {
  #url;
  #serviceRoleKey;

  constructor({ url, serviceRoleKey }) {
    if (!url || !serviceRoleKey) throw new TypeError('Supabase profile store requires a url and server-only service-role key.');
    this.#url = url;
    this.#serviceRoleKey = serviceRoleKey;
  }

  async #rpc(name, args) {
    const response = await fetch(`${this.#url}/rest/v1/rpc/${name}`, {
      method: 'POST',
      headers: {
        apikey: this.#serviceRoleKey,
        Authorization: `Bearer ${this.#serviceRoleKey}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(args),
    });
    const text = await response.text();
    const body = text ? JSON.parse(text) : null;
    if (!response.ok) {
      const message = body?.message || body?.error_description || 'Supabase profile store request failed.';
      if (message.includes('profile_conflict')) throw new ProfileConflictError('Profile revision conflict.');
      throw new Error(message);
    }
    return body;
  }

  async get(playerProfileId) {
    const id = safeProfileId(playerProfileId);
    return this.#rpc('wm_get_player_profile', { p_player_profile_id: id });
  }

  async put(profile, { expectedRevision = null } = {}) {
    safeProfileId(profile?.playerProfileId);
    return this.#rpc('wm_put_player_profile', { p_profile: profile, p_expected_revision: expectedRevision });
  }
}
