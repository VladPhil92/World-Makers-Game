// Maps this app's family/child domain shape (see domain/authorization.mjs and demo-family.mjs)
// onto the real Supabase-backed tables created for World Makers accounts.
import { randomUUID } from 'node:crypto';

function mapChild(row) {
  const stats = row.child_dashboard_stats?.[0] ?? row.child_dashboard_stats ?? {};
  return {
    childProfileId: row.child_profile_id,
    displayAlias: row.display_alias,
    authorizedParentIds: [],
    dashboard: {
      playTime: {
        last7DaysMinutes: stats.play_time_last7_minutes ?? 0,
        sessionsLast7Days: stats.sessions_last7 ?? 0,
      },
      learning: stats.learning ?? [],
      recentBuilds: stats.recent_builds ?? [],
      adventures: stats.adventures ?? [],
    },
  };
}

function mapFamily(row) {
  const authorizedParentIds = (row.family_parents ?? []).map((fp) => fp.parent_id);
  const children = (row.children ?? []).map((child) => ({ ...mapChild(child), authorizedParentIds }));
  return { familyId: row.family_id, authorizedParentIds, children };
}

export async function ensureFamilyForParent(client, accessToken, parentId) {
  const existing = await client.rest(`/family_parents?parent_id=eq.${encodeURIComponent(parentId)}&select=family_id`, { accessToken });
  if (existing.length > 0) return existing[0].family_id;

  // The new family is invisible under families_member_select until family_parents links this
  // parent to it, so we generate the id client-side and skip `return=representation` on the
  // families insert (RETURNING is governed by the SELECT policy too, and would fail here).
  const familyId = `family.${randomUUID()}`;
  await client.rest('/families', {
    accessToken,
    method: 'POST',
    body: { family_id: familyId },
    extraHeaders: { Prefer: 'return=minimal' },
  });
  await client.rest('/family_parents', {
    accessToken,
    method: 'POST',
    body: { family_id: familyId, parent_id: parentId },
    extraHeaders: { Prefer: 'return=minimal' },
  });
  return familyId;
}

export async function fetchFamilyReadModel(client, accessToken, familyId) {
  const select = 'family_id,family_parents(parent_id),children(child_profile_id,display_alias,child_dashboard_stats(play_time_last7_minutes,sessions_last7,learning,recent_builds,adventures))';
  const [row] = await client.rest(`/families?family_id=eq.${encodeURIComponent(familyId)}&select=${select}`, { accessToken });
  if (!row) throw new Error('Family not found.');
  return mapFamily(row);
}

export async function createChildProfile(client, accessToken, familyId, displayAlias) {
  const trimmed = String(displayAlias || '').trim().slice(0, 40);
  if (trimmed.length < 1) throw new TypeError('displayAlias is required.');
  const [row] = await client.rest('/children', {
    accessToken,
    method: 'POST',
    body: { family_id: familyId, display_alias: trimmed },
    extraHeaders: { Prefer: 'return=representation' },
  });
  return { childProfileId: row.child_profile_id, displayAlias: row.display_alias };
}

export function derivePlayerProfileId(childProfileId) {
  return `player.${childProfileId.replace(/^child\./, '')}`;
}

export async function exportChildData(client, accessToken, childProfileId) {
  const select = 'child_profile_id,display_alias,created_at,child_dashboard_stats(play_time_last7_minutes,sessions_last7,learning,recent_builds,adventures,updated_at)';
  const [child] = await client.rest(`/children?child_profile_id=eq.${encodeURIComponent(childProfileId)}&select=${select}`, { accessToken });
  if (!child) throw new Error('Child not found.');
  const stats = child.child_dashboard_stats?.[0] ?? child.child_dashboard_stats ?? {};
  let playerProfile = null;
  try {
    playerProfile = await client.rpc('wm_get_player_profile', { p_player_profile_id: derivePlayerProfileId(childProfileId) });
  } catch {
    // The child may never have launched into player-dashboard yet; export without it.
  }
  return {
    exportedAt: new Date().toISOString(),
    child: {
      childProfileId: child.child_profile_id,
      displayAlias: child.display_alias,
      createdAt: child.created_at,
    },
    dashboard: {
      playTime: {
        last7DaysMinutes: stats.play_time_last7_minutes ?? 0,
        sessionsLast7Days: stats.sessions_last7 ?? 0,
      },
      learning: stats.learning ?? [],
      recentBuilds: stats.recent_builds ?? [],
      adventures: stats.adventures ?? [],
    },
    playerProfile: playerProfile ?? null,
  };
}

export async function deleteChildProfile(client, accessToken, childProfileId) {
  await client.rest(`/children?child_profile_id=eq.${encodeURIComponent(childProfileId)}`, {
    accessToken,
    method: 'DELETE',
    extraHeaders: { Prefer: 'return=minimal' },
  });
}

export async function createFamilyInvite(client, accessToken, familyId) {
  const row = await client.rpc('wm_create_family_invite', { p_family_id: familyId }, { accessToken });
  return { inviteCode: row.inviteCode, expiresAt: row.expiresAt };
}

export async function redeemFamilyInvite(client, accessToken, inviteCode) {
  const trimmed = String(inviteCode || '').trim().toUpperCase();
  if (trimmed.length < 1) throw new TypeError('inviteCode is required.');
  const row = await client.rpc('wm_redeem_family_invite', { p_invite_code: trimmed }, { accessToken });
  return { familyId: row.familyId };
}
