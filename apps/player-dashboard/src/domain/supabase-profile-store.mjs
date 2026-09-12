import { ProfileConflictError } from './profile-store.mjs';

function safeProfileId(value) {
  if (typeof value !== 'string' || !/^player\.[a-z0-9._:-]{3,120}$/i.test(value)) throw new TypeError('Invalid player profile ID.');
  return value;
}

// Persists player profiles through two SECURITY DEFINER Postgres RPCs (wm_get_player_profile /
// wm_put_player_profile) instead of direct table access, so this app never needs a Supabase
// service-role key: the project's anon key is enough, and RLS on player_profiles denies
// everyone else. That anon key must still be treated as a server secret and never shipped to
// the browser, since these RPCs trust any caller that holds it.
export class SupabaseProfileStore {
  #url;
  #anonKey;

  constructor({ url, anonKey }) {
    if (!url || !anonKey) throw new TypeError('Supabase profile store requires a url and anon key.');
    this.#url = url;
    this.#anonKey = anonKey;
  }

  async #rpc(name, args) {
    const response = await fetch(`${this.#url}/rest/v1/rpc/${name}`, {
      method: 'POST',
      headers: {
        apikey: this.#anonKey,
        Authorization: `Bearer ${this.#anonKey}`,
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
