import test from 'node:test';
import assert from 'node:assert/strict';
import { once } from 'node:events';
import { listAuthorizedChildren, requireAuthorizedChild, AuthorizationError } from '../src/domain/authorization.mjs';
import { createParentDashboardReadModel, containsForbiddenChildTelemetry } from '../src/domain/dashboard.mjs';
import { buildPrivacyRequest, validateLinkCode } from '../src/domain/privacy.mjs';
import { demoFamily } from '../src/data/demo-family.mjs';

const parentId = 'parent.demo.guardian-01';
const childId = 'child.demo.explorer-a';

test('M4 authorization exposes only linked child profiles', () => {
  const children = listAuthorizedChildren(demoFamily, parentId);
  assert.equal(children.length, 2);
  assert.deepEqual(Object.keys(children[0]).sort(), ['childProfileId', 'displayAlias']);
  assert.throws(() => requireAuthorizedChild(demoFamily, 'parent.demo.other', childId), AuthorizationError);
  assert.throws(() => requireAuthorizedChild(demoFamily, parentId, 'child.demo.unknown'), AuthorizationError);
});

test('M4 dashboard is a minimized read model without forbidden telemetry', () => {
  const dashboard = createParentDashboardReadModel(demoFamily, parentId, childId);
  assert.equal(dashboard.schemaVersion, 1);
  assert.equal(dashboard.child.childProfileId, childId);
  assert.equal(dashboard.wellbeing.rightToStop, true);
  assert.equal(dashboard.wellbeing.pressureLoopsDetected, false);
  assert.equal(containsForbiddenChildTelemetry(dashboard), false);
  assert.deepEqual(Object.keys(dashboard.playTime).sort(), ['last7DaysMinutes', 'sessionsLast7Days']);
});

test('M4 privacy operations require explicit guardian acknowledgement', () => {
  assert.throws(() => buildPrivacyRequest({ family: demoFamily, parentId, childProfileId: childId, operation: 'delete-child-data', acknowledged: false }));
  const request = buildPrivacyRequest({ family: demoFamily, parentId, childProfileId: childId, operation: 'export-child-data', acknowledged: true });
  assert.equal(request.status, 'pending-backend-processing');
  assert.equal(request.requestedByRole, 'guardian');
});

test('M4 family link codes are bounded and normalized', () => {
  assert.equal(validateLinkCode('ab12cd34'), 'AB12CD34');
  assert.throws(() => validateLinkCode('too-short'));
  assert.throws(() => validateLinkCode('1234567!'));
});

test('M4 HTTP boundary requires a session and prevents cross-profile access', async (t) => {
  process.env.WORLD_MAKERS_ALLOW_DEMO_AUTH = 'true';
  const { createParentPortalServer } = await import(`../src/server.mjs?test=${Date.now()}`);
  const server = createParentPortalServer();
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  t.after(() => server.close());
  const address = server.address();
  const base = `http://127.0.0.1:${address.port}`;

  const denied = await fetch(`${base}/api/dashboard?childId=${encodeURIComponent(childId)}`);
  assert.equal(denied.status, 404);

  const signIn = await fetch(`${base}/api/demo/session`, { method: 'POST', headers: { Origin: base, 'Content-Type': 'application/json' }, body: '{}' });
  assert.equal(signIn.status, 201);
  const cookie = signIn.headers.get('set-cookie');
  assert.match(cookie, /HttpOnly/);
  assert.match(cookie, /SameSite=Strict/);

  const allowed = await fetch(`${base}/api/dashboard?childId=${encodeURIComponent(childId)}`, { headers: { Cookie: cookie } });
  assert.equal(allowed.status, 200);
  const payload = await allowed.json();
  assert.equal(payload.child.childProfileId, childId);

  const unknown = await fetch(`${base}/api/dashboard?childId=child.demo.unknown`, { headers: { Cookie: cookie } });
  assert.equal(unknown.status, 404);

  const privacyDenied = await fetch(`${base}/api/privacy-requests`, {
    method: 'POST',
    headers: { Cookie: cookie, Origin: base, 'Content-Type': 'application/json' },
    body: JSON.stringify({ childProfileId: childId, operation: 'delete-child-data', acknowledged: false }),
  });
  assert.equal(privacyDenied.status, 400);
});
