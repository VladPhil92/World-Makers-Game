const title = document.getElementById('handoff-title');
const message = document.getElementById('handoff-message');

function fail(text) {
  title.textContent = 'We could not sign you in';
  message.textContent = text;
}

async function run() {
  const assertion = new URLSearchParams(window.location.search).get('assertion');
  if (!assertion) return fail('This link is missing its sign-in token. Go back to the family portal and try again.');

  try {
    const response = await fetch('/api/identity/session', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ assertion }),
    });
    if (!response.ok) {
      const payload = await response.json().catch(() => ({}));
      return fail(payload.code === 'identity_provider_required'
        ? 'Sign-in is not configured on this deployment yet.'
        : 'This sign-in link expired or is no longer valid. Ask a guardian to launch again.');
    }
    window.location.replace('/');
  } catch {
    fail('Could not reach the World Makers dashboard. Check your connection and try again.');
  }
}

run();
