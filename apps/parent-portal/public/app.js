const $ = (selector) => document.querySelector(selector);

const signedOut = $('#signed-out');
const portal = $('#portal');
const signOutButton = $('#sign-out');
const childSelect = $('#child-select');
const status = $('#status');
let session = null;
let activeDashboard = null;

function setStatus(message = '') {
  status.textContent = message;
}

function setAuthenticated(authenticated) {
  signedOut.hidden = authenticated;
  portal.hidden = !authenticated;
  signOutButton.hidden = !authenticated;
}

function clearChildren(node) {
  while (node.firstChild) node.removeChild(node.firstChild);
}

function text(tag, value, className) {
  const node = document.createElement(tag);
  node.textContent = value;
  if (className) node.className = className;
  return node;
}

function renderChildren(children) {
  clearChildren(childSelect);
  for (const child of children) {
    const option = document.createElement('option');
    option.value = child.childProfileId;
    option.textContent = child.displayAlias;
    childSelect.append(option);
  }
}

function formatMinutes(minutes) {
  if (minutes < 60) return `${minutes} min`;
  const hours = Math.floor(minutes / 60);
  const remainder = minutes % 60;
  return remainder ? `${hours} h ${remainder} min` : `${hours} h`;
}

function renderLearning(items) {
  const root = $('#learning-list');
  clearChildren(root);
  for (const item of items) {
    const card = document.createElement('div');
    card.className = 'item';
    const top = document.createElement('div');
    top.className = 'item-top';
    top.append(text('strong', item.label), text('small', `${item.progressPercent}%`));
    const track = document.createElement('div');
    track.className = 'progress-track';
    track.setAttribute('role', 'progressbar');
    track.setAttribute('aria-label', item.label);
    track.setAttribute('aria-valuemin', '0');
    track.setAttribute('aria-valuemax', '100');
    track.setAttribute('aria-valuenow', String(item.progressPercent));
    const fill = document.createElement('div');
    fill.className = 'progress-fill';
    fill.style.width = `${item.progressPercent}%`;
    track.append(fill);
    card.append(top, track);
    root.append(card);
  }
}

function renderAdventures(items) {
  const root = $('#adventure-list');
  clearChildren(root);
  for (const item of items) {
    const card = document.createElement('div');
    card.className = 'item item-top';
    card.append(text('strong', item.label), text('span', item.state.replaceAll('-', ' '), 'state-pill'));
    root.append(card);
  }
}

function renderBuilds(items) {
  const root = $('#build-list');
  clearChildren(root);
  for (const item of items) {
    const card = document.createElement('div');
    card.className = 'item';
    card.append(text('strong', item.label), text('small', `${item.pieceCount} pieces · ${item.biomeLabel}`));
    root.append(card);
  }
}

function renderDashboard(dashboard) {
  activeDashboard = dashboard;
  $('#play-minutes').textContent = formatMinutes(dashboard.playTime.last7DaysMinutes);
  $('#session-count').textContent = `${dashboard.playTime.sessionsLast7Days} play sessions`;
  $('#learning-count').textContent = String(dashboard.learning.length);
  $('#creation-count').textContent = String(dashboard.recentBuilds.length);
  renderLearning(dashboard.learning);
  renderAdventures(dashboard.adventures);
  renderBuilds(dashboard.recentBuilds);
}

async function api(path, options = {}) {
  const response = await fetch(path, {
    ...options,
    headers: { 'Content-Type': 'application/json', ...(options.headers ?? {}) },
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) {
    const error = new Error(payload.code ?? 'request_failed');
    error.status = response.status;
    throw error;
  }
  return payload;
}

async function loadDashboard(childProfileId) {
  setStatus('Loading family summary…');
  try {
    const dashboard = await api(`/api/dashboard?childId=${encodeURIComponent(childProfileId)}`);
    renderDashboard(dashboard);
    setStatus(`Showing ${dashboard.child.displayAlias}.`);
  } catch {
    setStatus('This profile is not available to the signed-in guardian.');
  }
}

async function hydrateSession() {
  try {
    session = await api('/api/session');
    setAuthenticated(true);
    renderChildren(session.children);
    if (session.children.length) await loadDashboard(session.children[0].childProfileId);
  } catch {
    session = null;
    setAuthenticated(false);
  }
}

$('#demo-sign-in').addEventListener('click', async () => {
  const button = $('#demo-sign-in');
  button.disabled = true;
  $('#auth-note').textContent = 'Opening the guardian demo…';
  try {
    session = await api('/api/demo/session', { method: 'POST', body: '{}' });
    setAuthenticated(true);
    renderChildren(session.children);
    if (session.children.length) await loadDashboard(session.children[0].childProfileId);
  } catch (error) {
    $('#auth-note').textContent = error.status === 503
      ? 'Demo access is disabled. Configure an approved identity provider for production.'
      : 'Could not open the guardian session.';
  } finally {
    button.disabled = false;
  }
});

signOutButton.addEventListener('click', async () => {
  await api('/api/session', { method: 'DELETE' }).catch(() => null);
  session = null;
  activeDashboard = null;
  setAuthenticated(false);
});

childSelect.addEventListener('change', () => loadDashboard(childSelect.value));

$('#link-form').addEventListener('submit', async (event) => {
  event.preventDefault();
  const output = $('#link-result');
  output.textContent = 'Submitting verification request…';
  try {
    const result = await api('/api/link-requests', {
      method: 'POST',
      body: JSON.stringify({ linkCode: $('#link-code').value }),
    });
    output.textContent = `Request received (${result.codeFingerprint}). Access remains pending backend verification.`;
    event.currentTarget.reset();
  } catch {
    output.textContent = 'That code could not be submitted. Check all 8 characters.';
  }
});

for (const button of document.querySelectorAll('.privacy-action')) {
  button.addEventListener('click', async () => {
    const output = $('#privacy-result');
    const acknowledged = $('#privacy-ack').checked;
    if (!activeDashboard) return;
    if (!acknowledged) {
      output.textContent = 'Please acknowledge the profile scope before submitting a privacy request.';
      return;
    }
    output.textContent = 'Submitting guardian request…';
    try {
      const result = await api('/api/privacy-requests', {
        method: 'POST',
        body: JSON.stringify({
          childProfileId: activeDashboard.child.childProfileId,
          operation: button.dataset.operation,
          acknowledged,
        }),
      });
      output.textContent = `Request accepted: ${result.operation}. Status: ${result.status}.`;
      $('#privacy-ack').checked = false;
    } catch {
      output.textContent = 'The request was not accepted. Guardian authorization is required.';
    }
  });
}

hydrateSession();
