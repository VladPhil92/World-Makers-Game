const state = { dashboard: null, activeView: 'home' };

const $ = (selector) => document.querySelector(selector);
const $$ = (selector) => [...document.querySelectorAll(selector)];

function node(tag, className, text) {
  const element = document.createElement(tag);
  if (className) element.className = className;
  if (text !== undefined) element.textContent = text;
  return element;
}

async function api(path, options = {}) {
  const response = await fetch(path, {
    ...options,
    headers: options.body ? { 'Content-Type': 'application/json', ...(options.headers ?? {}) } : options.headers,
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    const error = new Error(payload.code ?? 'request_failed');
    error.code = payload.code;
    throw error;
  }
  return payload;
}

function toast(message) {
  const target = $('#global-status');
  target.textContent = message;
  target.classList.add('is-visible');
  window.setTimeout(() => target.classList.remove('is-visible'), 2600);
}

function setProfileRevision(revision, updatedAt) {
  if (!Number.isInteger(revision)) return;
  state.dashboard.player.profileRevision = revision;
  state.dashboard.profile.revision = revision;
  if (updatedAt) {
    state.dashboard.player.profileUpdatedAt = updatedAt;
    state.dashboard.profile.updatedAt = updatedAt;
  }
  const sync = $('#profile-sync-status');
  if (sync) sync.textContent = `Sincronizado · r${revision}`;
}

async function recoverConflict() {
  try {
    state.dashboard = await api('/api/profile');
    renderAll();
    toast('Tu perfil cambió en otra sesión. Sincronizamos la versión más reciente.');
  } catch {
    toast('No fue posible resincronizar tu perfil.');
  }
}

function titleForView(view) {
  return ({ home: 'Inicio', avatar: 'Mi personaje', modes: 'Modos de juego', worlds: 'Mis mundos', store: 'Store' })[view] ?? 'Player Hub';
}

function showView(view) {
  state.activeView = view;
  $$('.view').forEach((section) => section.classList.toggle('is-visible', section.id === `view-${view}`));
  $$('.nav-item[data-view]').forEach((button) => button.classList.toggle('is-active', button.dataset.view === view));
  $('#view-title').textContent = titleForView(view);
  window.scrollTo({ top: 0, behavior: 'smooth' });
}

function currentMode() {
  return state.dashboard.catalog.gameModes.find((mode) => mode.id === state.dashboard.player.selection.modeId);
}

function currentWorld() {
  return state.dashboard.catalog.worlds.find((world) => world.id === state.dashboard.player.selection.worldId);
}

function renderAvatarPreview() {
  const loadout = state.dashboard.player.loadout;
  const hueSeed = [...Object.values(loadout).join('')].reduce((sum, char) => sum + char.charCodeAt(0), 0) % 360;
  $$('.avatar-preview').forEach((preview) => {
    preview.style.setProperty('--avatar-hue', String(hueSeed));
    preview.dataset.top = loadout.top;
    preview.dataset.back = loadout['back-accessory'];
    preview.dataset.prop = loadout['hand-prop'];
  });
}

function modeCard(mode, compact = false) {
  const card = node('button', `mode-card glass-panel${mode.playable ? '' : ' is-locked'}${mode.id === state.dashboard.player.selection.modeId ? ' is-selected' : ''}`);
  card.type = 'button';
  card.disabled = !mode.playable;
  card.dataset.modeId = mode.id;
  const icon = node('span', 'mode-icon', mode.icon);
  const eyebrow = node('span', 'mode-eyebrow', mode.eyebrow);
  const title = node('strong', 'mode-title', mode.name);
  const description = node('span', 'mode-description', mode.description);
  card.append(icon, eyebrow, title);
  if (!compact) card.append(description);
  if (!mode.playable) card.append(node('span', 'mode-lock', 'EN DESARROLLO'));
  if (mode.playable) card.addEventListener('click', () => selectMode(mode.id));
  return card;
}

function renderModes() {
  const home = $('#home-mode-grid');
  const full = $('#mode-grid');
  home.replaceChildren();
  full.replaceChildren();
  state.dashboard.catalog.gameModes.filter((mode) => mode.playable).forEach((mode) => home.append(modeCard(mode, true)));
  state.dashboard.catalog.gameModes.forEach((mode) => full.append(modeCard(mode, false)));

  const missionsMode = state.dashboard.player.selection.modeId === 'missions';
  $('#mission-picker').hidden = !missionsMode;
  const list = $('#mission-list');
  list.replaceChildren();
  if (missionsMode) {
    const worldId = state.dashboard.player.selection.worldId;
    state.dashboard.catalog.missions.filter((mission) => mission.worldId === worldId).forEach((mission) => {
      const button = node('button', `mission-option${mission.id === state.dashboard.player.selection.missionId ? ' is-selected' : ''}`);
      button.type = 'button';
      button.append(node('span', 'mission-subject', mission.subject), node('strong', '', mission.name));
      button.addEventListener('click', () => selectMission(mission.id));
      list.append(button);
    });
  }
}

function renderWorlds() {
  const grid = $('#world-grid');
  grid.replaceChildren();
  const modeId = state.dashboard.player.selection.modeId;
  state.dashboard.catalog.worlds.forEach((world) => {
    const compatible = world.modes.includes(modeId);
    const card = node('button', `world-card glass-panel tone-${world.accent}${world.id === state.dashboard.player.selection.worldId ? ' is-selected' : ''}${compatible ? '' : ' is-disabled'}`);
    card.type = 'button';
    card.disabled = !compatible;
    card.append(
      node('span', 'world-orbit', '◎'),
      node('span', 'world-label', compatible ? 'DISPONIBLE' : 'OTRO MODO'),
      node('strong', 'world-title', world.name),
      node('span', 'world-description', world.description),
    );
    if (compatible) card.addEventListener('click', () => selectWorld(world.id));
    grid.append(card);
  });
}

function humanizeSlot(slot) {
  return ({ hair: 'Cabello', top: 'Parte superior', bottom: 'Parte inferior', footwear: 'Calzado', 'head-accessory': 'Accesorio de cabeza', 'back-accessory': 'Mochila / espalda', 'hand-prop': 'Objeto de mano' })[slot] ?? slot;
}

function humanizeCosmetic(id) {
  if (id === 'none') return 'Ninguno';
  return id.split('.').slice(1).join(' · ').replaceAll('-', ' ');
}

function renderAvatarControls() {
  const panel = $('#avatar-controls');
  panel.replaceChildren();
  for (const [slot, options] of Object.entries(state.dashboard.catalog.avatarSlots)) {
    const group = node('label', 'custom-control');
    group.append(node('span', 'control-label', humanizeSlot(slot)));
    const select = node('select', 'cosmetic-select');
    select.dataset.slot = slot;
    options.forEach((id) => {
      const option = node('option', '', humanizeCosmetic(id));
      option.value = id;
      option.selected = state.dashboard.player.loadout[slot] === id;
      select.append(option);
    });
    select.addEventListener('change', () => saveCosmetic(slot, select.value));
    group.append(select);
    panel.append(group);
  }
}

function renderStore() {
  const grid = $('#store-grid');
  grid.replaceChildren();
  const requested = new Set(state.dashboard.storeRequests.map((request) => request.itemId));
  const owned = new Set(state.dashboard.player.entitlements);
  state.dashboard.catalog.storeCatalog.forEach((item) => {
    const card = node('article', `store-card glass-panel tone-${item.previewTone}`);
    const preview = node('div', 'store-preview');
    preview.append(node('span', '', item.type === 'top' ? '◫' : item.type === 'back-accessory' ? '⬡' : item.type === 'head-accessory' ? '⌁' : '⌖'));
    const meta = node('div', 'store-meta');
    meta.append(node('span', 'store-type', item.type.replaceAll('-', ' ').toUpperCase()), node('strong', '', item.name));
    const button = node('button', 'secondary-button store-request-button');
    button.type = 'button';
    if (owned.has(item.entitlementId)) {
      button.textContent = 'En tu inventario';
      button.disabled = true;
    } else if (requested.has(item.id)) {
      button.textContent = 'Solicitud enviada';
      button.disabled = true;
    } else {
      button.textContent = 'Pedir a un adulto';
      button.addEventListener('click', () => requestStoreItem(item.id));
    }
    card.append(preview, meta, button);
    grid.append(card);
  });
}

function renderProgress() {
  const progress = state.dashboard.player.progress.currentAdventure;
  const mission = progress ? state.dashboard.catalog.missions.find((item) => item.id === progress.missionId) : null;
  const world = progress ? state.dashboard.catalog.worlds.find((item) => item.id === progress.worldId) : null;
  $('#continue-title').textContent = mission?.name ?? 'Tu próxima aventura';
  $('#continue-meta').textContent = world?.name ?? 'World Makers';
  const percent = progress?.progressPercent ?? 0;
  $('#continue-progress').style.width = `${percent}%`;
  $('#continue-percent').textContent = `${percent}%`;
}

function renderLaunch() {
  const mode = currentMode();
  const world = currentWorld();
  $('#launch-summary').textContent = `${mode?.name ?? 'Modo'} · ${world?.name ?? 'Mundo'}`;
  const ready = state.dashboard.launch.ready;
  $('#launch-readiness').textContent = ready ? `Perfil r${state.dashboard.player.profileRevision} confirmado · contexto seguro listo.` : 'El servidor de lanzamiento todavía no tiene firma configurada.';
  $('#play-button').classList.toggle('is-not-ready', !ready);
}

function renderAll() {
  $('#player-name').textContent = state.dashboard.player.displayName;
  setProfileRevision(state.dashboard.player.profileRevision, state.dashboard.player.profileUpdatedAt);
  renderAvatarPreview();
  renderAvatarControls();
  renderModes();
  renderWorlds();
  renderStore();
  renderProgress();
  renderLaunch();
}

async function persistSelection(selection) {
  const payload = await api('/api/selection', {
    method: 'PATCH',
    body: JSON.stringify({ selection, profileRevision: state.dashboard.player.profileRevision }),
  });
  state.dashboard.player.selection = payload.selection;
  setProfileRevision(payload.profileRevision, payload.profileUpdatedAt);
  renderModes();
  renderWorlds();
  renderLaunch();
}

async function selectMode(modeId) {
  const mode = state.dashboard.catalog.gameModes.find((item) => item.id === modeId);
  if (!mode?.playable) return;
  let world = state.dashboard.catalog.worlds.find((item) => item.id === state.dashboard.player.selection.worldId && item.modes.includes(modeId));
  if (!world) world = state.dashboard.catalog.worlds.find((item) => item.modes.includes(modeId));
  let missionId = null;
  if (modeId === 'missions') missionId = state.dashboard.catalog.missions.find((mission) => mission.worldId === world.id)?.id ?? null;
  try {
    await persistSelection({ modeId, worldId: world.id, missionId });
    toast(`${mode.name} seleccionado.`);
  } catch (error) {
    if (error.code === 'profile_conflict') return recoverConflict();
    toast('No se pudo cambiar el modo.');
  }
}

async function selectWorld(worldId) {
  const modeId = state.dashboard.player.selection.modeId;
  let missionId = null;
  if (modeId === 'missions') missionId = state.dashboard.catalog.missions.find((mission) => mission.worldId === worldId)?.id ?? null;
  try {
    await persistSelection({ modeId, worldId, missionId });
    toast('Mundo seleccionado.');
  } catch (error) {
    if (error.code === 'profile_conflict') return recoverConflict();
    toast('Ese mundo no está disponible para el modo actual.');
  }
}

async function selectMission(missionId) {
  const selection = { ...state.dashboard.player.selection, missionId };
  try {
    await persistSelection(selection);
    toast('Aventura preparada.');
  } catch (error) {
    if (error.code === 'profile_conflict') return recoverConflict();
    toast('No se pudo preparar esa aventura.');
  }
}

async function saveCosmetic(slot, value) {
  const previous = state.dashboard.player.loadout[slot];
  state.dashboard.player.loadout[slot] = value;
  renderAvatarPreview();
  $('#avatar-save-status').textContent = 'Guardando…';
  try {
    const payload = await api('/api/avatar/loadout', {
      method: 'PATCH',
      body: JSON.stringify({ loadout: state.dashboard.player.loadout, profileRevision: state.dashboard.player.profileRevision }),
    });
    state.dashboard.player.loadout = payload.loadout;
    setProfileRevision(payload.profileRevision, payload.profileUpdatedAt);
    $('#avatar-save-status').textContent = 'Guardado y sincronizado';
  } catch (error) {
    state.dashboard.player.loadout[slot] = previous;
    if (error.code === 'profile_conflict') return recoverConflict();
    renderAvatarControls();
    renderAvatarPreview();
    $('#avatar-save-status').textContent = 'No se pudo guardar';
  }
}

async function requestStoreItem(itemId) {
  try {
    const payload = await api('/api/store/requests', {
      method: 'POST',
      body: JSON.stringify({ itemId, profileRevision: state.dashboard.player.profileRevision }),
    });
    if (!state.dashboard.storeRequests.some((request) => request.requestId === payload.request.requestId)) state.dashboard.storeRequests.push(payload.request);
    setProfileRevision(payload.profileRevision, payload.profileUpdatedAt);
    renderStore();
    toast('Solicitud persistida y enviada para aprobación parental.');
  } catch (error) {
    if (error.code === 'profile_conflict') return recoverConflict();
    toast('No se pudo enviar la solicitud.');
  }
}

async function launchGame() {
  try {
    const payload = await api('/api/launch-context', { method: 'POST', body: '{}' });
    const dialog = $('#launch-dialog');
    $('#launch-dialog-copy').textContent = `${currentMode().name} · ${currentWorld().name}. El launcher nativo debe consumir este contexto firmado.`;
    const safePreview = {
      protocol: payload.protocol,
      playerId: payload.context.playerId,
      profileRevision: payload.context.profileRevision,
      avatarId: payload.context.avatarId,
      selectedMode: payload.context.selectedMode,
      selectedWorld: payload.context.selectedWorld,
      missionId: payload.context.missionId,
      expiresAt: payload.context.expiresAt,
      signature: `${payload.context.signature.slice(0, 12)}…`,
    };
    $('#launch-context-preview').textContent = JSON.stringify(safePreview, null, 2);
    dialog.showModal();
  } catch (error) {
    if (error.code === 'launch_signing_not_configured') return toast('PLAY está fail-closed hasta configurar la firma de lanzamiento.');
    toast('No se pudo preparar el lanzamiento.');
  }
}

async function signInDemo() {
  $('#auth-status').textContent = 'Abriendo tu perfil persistente…';
  try {
    state.dashboard = await api('/api/demo/session', { method: 'POST', body: '{}' });
    $('#auth-view').hidden = true;
    $('#app-shell').hidden = false;
    $('#auth-status').textContent = '';
    renderAll();
  } catch (error) {
    $('#auth-status').textContent = error.code === 'identity_provider_required'
      ? 'El proveedor de identidad todavía no está conectado. El modo demo está desactivado.'
      : error.code === 'profile_store_not_configured'
        ? 'El almacén persistente de perfiles no está configurado.'
        : 'No fue posible iniciar sesión.';
  }
}

async function hydrate() {
  try {
    state.dashboard = await api('/api/session');
    $('#auth-view').hidden = true;
    $('#app-shell').hidden = false;
    renderAll();
  } catch {
    $('#auth-view').hidden = false;
    $('#app-shell').hidden = true;
  }
}

$('#demo-login').addEventListener('click', signInDemo);
$('#logout').addEventListener('click', async () => {
  await api('/api/session', { method: 'DELETE' }).catch(() => {});
  state.dashboard = null;
  $('#app-shell').hidden = true;
  $('#auth-view').hidden = false;
});
$('#play-button').addEventListener('click', launchGame);
$$('[data-view]').forEach((button) => button.addEventListener('click', () => showView(button.dataset.view)));

hydrate();
