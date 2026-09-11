import { execFileSync } from 'node:child_process';
import { readdir, readFile } from 'node:fs/promises';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('..', import.meta.url));
const pkg = JSON.parse(await readFile(new URL('../package.json', import.meta.url), 'utf8'));
if (pkg.private !== true) throw new Error('Player dashboard package must remain private.');
if (pkg.name !== '@world-makers/player-dashboard') throw new Error('Unexpected package name.');
if (pkg.dependencies && Object.keys(pkg.dependencies).length) throw new Error('Player dashboard must remain dependency-minimized while identity/profile contracts stabilize.');

async function collect(dir) {
  const entries = await readdir(dir, { withFileTypes: true });
  const paths = [];
  for (const entry of entries) {
    const path = join(dir, entry.name);
    if (entry.isDirectory()) paths.push(...await collect(path));
    else if (entry.name.endsWith('.mjs')) paths.push(path);
  }
  return paths;
}

for (const source of await collect(join(root, 'src'))) {
  execFileSync(process.execPath, ['--check', source], { stdio: 'inherit' });
}
execFileSync(process.execPath, ['--check', join(root, 'public', 'app.js')], { stdio: 'inherit' });

const browser = await readFile(join(root, 'public', 'app.js'), 'utf8');
for (const forbidden of ['innerHTML', 'outerHTML', 'eval(', 'localStorage', 'sessionStorage', 'document.cookie']) {
  if (browser.includes(forbidden)) throw new Error(`Unsafe browser primitive in player dashboard: ${forbidden}`);
}
for (const required of ['profileRevision', 'profile_conflict', '/api/profile']) {
  if (!browser.includes(required)) throw new Error(`D2 browser persistence marker missing: ${required}`);
}

const html = await readFile(join(root, 'public', 'index.html'), 'utf8');
for (const marker of ['Saltar al contenido principal', 'PLAYER HUB', 'AVATAR STUDIO', 'GAME MODES', 'WORLD SELECT', 'STORE', 'JUGAR', 'aria-live="polite"']) {
  if (!html.includes(marker)) throw new Error(`Player dashboard product/accessibility marker missing: ${marker}`);
}

const server = await readFile(join(root, 'src', 'server.mjs'), 'utf8');
for (const required of [
  'Content-Security-Policy', 'HttpOnly', 'SameSite=Strict', 'WORLD_MAKERS_ALLOW_DEMO_AUTH',
  'WORLD_MAKERS_LAUNCH_SIGNING_SECRET', 'WORLD_MAKERS_IDENTITY_ASSERTION_SECRET', 'WORLD_MAKERS_IDENTITY_LINK_SECRET',
  'WORLD_MAKERS_PROFILE_STORE_PATH', '/api/identity/session', '/api/profile', '/api/preferences',
  '/api/launch-context', '/api/avatar/loadout', '/api/selection', '/api/store/requests', 'ProfileConflictError',
]) {
  if (!server.includes(required)) throw new Error(`Player dashboard server boundary missing: ${required}`);
}
for (const forbidden of ['STRIPE_SECRET', 'SUPABASE_SERVICE_ROLE', 'PRIVATE_KEY', '/api/store/purchase', 'database.query(', 'prisma.']) {
  if (server.includes(forbidden)) throw new Error(`Player dashboard must not couple to privileged/direct-purchase infrastructure: ${forbidden}`);
}

const identity = await readFile(join(root, 'src', 'domain', 'identity.mjs'), 'utf8');
for (const required of ["const ISSUER = 'ctg-one'", "const AUDIENCE = 'world-makers'", "const PROTOCOL = 'ctg-one-identity-v1'", 'timingSafeEqual', 'deriveIdentityLinkId']) {
  if (!identity.includes(required)) throw new Error(`D2 identity boundary missing: ${required}`);
}
if (identity.includes('localStorage') || identity.includes('password')) throw new Error('Identity contract may not persist browser credentials/passwords.');

const store = await readFile(join(root, 'src', 'domain', 'profile-store.mjs'), 'utf8');
for (const required of ['JsonFileProfileStore', 'ProfileConflictError', 'rename(', 'expectedRevision', 'mode: 0o600']) {
  if (!store.includes(required)) throw new Error(`D2 persistent profile store marker missing: ${required}`);
}

const catalog = await readFile(join(root, 'src', 'domain', 'catalog.mjs'), 'utf8');
for (const mode of ['free-explore', 'missions', 'laboratory', 'cooperative']) {
  if (!catalog.includes(`id: '${mode}'`)) throw new Error(`Game mode missing from catalog: ${mode}`);
}
if (!catalog.includes('playable: false')) throw new Error('Future cooperative mode must remain locked.');
if (!catalog.includes("purchasePolicy: 'parent-approval-required'")) throw new Error('Store must preserve parental approval boundary.');

console.log('World Makers Player Dashboard D2 identity/persistence lint passed.');
