import { ProfileConflictError } from './profile-store.mjs';

function safeProfileId(value) {
  if (typeof value !== 'string' || !/^player\.[a-z0-9._:-]{3,120}$/i.test(value)) throw new TypeError('Invalid player profile ID.');
  return value;
}

function requireBridgeSecret(value) {
  if (typeof value !== 'string' || value.length < 48) {
    throw new TypeError('Supabase profile store requires WORLD_MAKERS_PROFILE_BRIDGE_SECRET.');
  }
  return value;
}

// Dashboard persistence uses an anon-key reachable SECURITY DEFINER wrapper,
// but the wrapper itself requires a high-entropy server-only bridge secret.
// Neither the secret nor any privileged Supabase credential is exposed to the
// browser, launch tickets, or the game runtime.
export class SupabaseProfileStore {
  #url;
  #anonKey;
  #bridgeSecret;

  constructor({ url, anonKey, bridgeSecret = process.env.WORLD_MAKERS_PROFILE_BRIDGE_SECRET ?? '' }) {
    if (!url || !anonKey) throw new TypeError('Supabase profile store requires a url and anon key.');
    this.#url = url.replace(/\/$/, '');
    this.#anonKey = anonKey;
    this.#bridgeSecret = requireBridgeSecret(bridgeSecret);
  }

  async #rpc(operation, { playerProfileId, expectedRevision = null, payload = null }) {
    const response = await fetch(`${this.#url}/rest/v1/rpc/wm_secure_player_profile`, {
      method: 'POST',
      headers: {
        apikey: this.#anonKey,
        Authorization: `Bearer ${this.#anonKey}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        p_operation: operation,
        p_player_profile_id: playerProfileId,
        p_expected_revision: expectedRevision,
        p_payload: payload,
        p_server_secret: this.#bridgeSecret,
      }),
    });
    const text = await response.text();
    const body = text ? JSON.parse(text) : null;
    if (!response.ok) {
      const message = body?.message || body?.error_description || body?.error || 'Supabase profile store request failed.';
      if (message.includes('profile_conflict')) throw new ProfileConflictError('Profile revision conflict.');
      throw new Error(message);
    }
    return body;
  }

  async get(playerProfileId) {
    const id = safeProfileId(playerProfileId);
    return this.#rpc('get', { playerProfileId: id });
  }

  async put(profile, { expectedRevision = null } = {}) {
    const id = safeProfileId(profile?.playerProfileId);
    if (!Number.isInteger(expectedRevision) || expectedRevision < 0) throw new TypeError('expectedRevision must be a non-negative integer.');
    return this.#rpc('put', { playerProfileId: id, expectedRevision, payload: profile });
  }
}
