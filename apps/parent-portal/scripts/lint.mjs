import { execFileSync } from 'node:child_process';
import { readdir, readFile } from 'node:fs/promises';
import { join } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('..', import.meta.url));
const pkg = JSON.parse(await readFile(new URL('../package.json', import.meta.url), 'utf8'));
if (pkg.private !== true) throw new Error('Parent portal package must remain private.');
if (!pkg.name?.startsWith('@world-makers/')) throw new Error('Unexpected package namespace.');
if (pkg.dependencies && Object.keys(pkg.dependencies).length) throw new Error('M4 MVP must remain dependency-minimized until production identity/backend selection.');

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

const appJs = await readFile(join(root, 'public', 'app.js'), 'utf8');
for (const forbidden of ['innerHTML', 'outerHTML', 'eval(', 'localStorage', 'sessionStorage']) {
  if (appJs.includes(forbidden)) throw new Error(`Unsafe browser primitive in parent portal: ${forbidden}`);
}

const html = await readFile(join(root, 'public', 'index.html'), 'utf8');
for (const required of ['Skip to main content', 'aria-live="polite"', 'Family Space', 'privacy-ack']) {
  if (!html.includes(required)) throw new Error(`Accessibility/product marker missing: ${required}`);
}

const server = await readFile(join(root, 'src', 'server.mjs'), 'utf8');
for (const required of ['Content-Security-Policy', 'HttpOnly', 'SameSite=Strict', 'Permissions-Policy', 'WORLD_MAKERS_ALLOW_DEMO_AUTH']) {
  if (!server.includes(required)) throw new Error(`Server security boundary missing: ${required}`);
}
for (const forbidden of ['PRIVATE_KEY', 'SUPABASE_SERVICE_ROLE', 'STRIPE_SECRET', 'database.query(', 'prisma.']) {
  if (server.includes(forbidden)) throw new Error(`Parent portal must not couple to privileged infrastructure: ${forbidden}`);
}

console.log('Parent portal M4 lint passed.');
