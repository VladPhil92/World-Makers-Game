// Thin fetch-based Supabase client. No npm dependency, matches this app's zero-dependency policy.
// The anon key is a publishable transport credential. Sensitive player-profile administration
// is additionally protected by WORLD_MAKERS_PROFILE_BRIDGE_SECRET and never relies on anon-key
// possession alone.

export class SupabaseAuthError extends Error {
  constructor(message, status) {
    super(message);
    this.name = 'SupabaseAuthError';
    this.status = status;
  }
}

export class SupabaseQueryError extends Error {
  constructor(message, status) {
    super(message);
    this.name = 'SupabaseQueryError';
    this.status = status;
  }
}

function requireConfig(url, anonKey) {
  if (!url || !anonKey) throw new SupabaseAuthError('Supabase is not configured.', 503);
}

async function parseJsonOrThrow(response, ErrorClass) {
  const text = await response.text();
  const body = text ? JSON.parse(text) : {};
  if (!response.ok) {
    const message = body.msg || body.message || body.error_description || body.error || 'Supabase request failed.';
    throw new ErrorClass(message, response.status);
  }
  return body;
}

export function createSupabaseClient({ url, anonKey }) {
  requireConfig(url, anonKey);
  const authBase = `${url}/auth/v1`;
  const restBase = `${url}/rest/v1`;
  const profileBridgeSecret = process.env.WORLD_MAKERS_PROFILE_BRIDGE_SECRET ?? '';

  async function signUp({ email, password, displayName }) {
    const response = await fetch(`${authBase}/signup`, {
      method: 'POST',
      headers: { apikey: anonKey, 'Content-Type': 'application/json' },
      body: JSON.stringify({ email, password, data: { display_name: displayName } }),
    });
    return parseJsonOrThrow(response, SupabaseAuthError);
  }

  async function signInWithPassword({ email, password }) {
    const response = await fetch(`${authBase}/token?grant_type=password`, {
      method: 'POST',
      headers: { apikey: anonKey, 'Content-Type': 'application/json' },
      body: JSON.stringify({ email, password }),
    });
    return parseJsonOrThrow(response, SupabaseAuthError);
  }

  async function refreshSession({ refreshToken }) {
    const response = await fetch(`${authBase}/token?grant_type=refresh_token`, {
      method: 'POST',
      headers: { apikey: anonKey, 'Content-Type': 'application/json' },
      body: JSON.stringify({ refresh_token: refreshToken }),
    });
    return parseJsonOrThrow(response, SupabaseAuthError);
  }

  async function rest(path, { accessToken, method = 'GET', body, extraHeaders = {} } = {}) {
    const response = await fetch(`${restBase}${path}`, {
      method,
      headers: {
        apikey: anonKey,
        Authorization: `Bearer ${accessToken}`,
        'Content-Type': 'application/json',
        ...extraHeaders,
      },
      body: body === undefined ? undefined : JSON.stringify(body),
    });
    return parseJsonOrThrow(response, SupabaseQueryError);
  }

  async function secureDeletePlayerProfile(args) {
    if (profileBridgeSecret.length < 48) {
      throw new SupabaseAuthError('Player profile administration is not configured.', 503);
    }
    const playerProfileId = String(args?.p_player_profile_id ?? '');
    const response = await fetch(`${restBase}/rpc/wm_secure_player_profile`, {
      method: 'POST',
      headers: {
        apikey: anonKey,
        Authorization: `Bearer ${anonKey}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        p_operation: 'delete',
        p_player_profile_id: playerProfileId,
        p_expected_revision: null,
        p_payload: null,
        p_server_secret: profileBridgeSecret,
      }),
    });
    return parseJsonOrThrow(response, SupabaseQueryError);
  }

  async function rpc(functionName, args, { accessToken = anonKey } = {}) {
    if (functionName === 'wm_delete_player_profile') {
      return secureDeletePlayerProfile(args);
    }
    const response = await fetch(`${restBase}/rpc/${functionName}`, {
      method: 'POST',
      headers: {
        apikey: anonKey,
        Authorization: `Bearer ${accessToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(args),
    });
    return parseJsonOrThrow(response, SupabaseQueryError);
  }

  return { signUp, signInWithPassword, refreshSession, rest, rpc };
}
