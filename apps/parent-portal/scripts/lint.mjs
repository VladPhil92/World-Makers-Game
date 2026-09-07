import { readFile } from 'node:fs/promises';

const pkg = JSON.parse(await readFile(new URL('../package.json', import.meta.url), 'utf8'));
if (pkg.private !== true) {
  throw new Error('Parent portal package must remain private.');
}
if (!pkg.name?.startsWith('@world-makers/')) {
  throw new Error('Unexpected package namespace.');
}
console.log('Parent portal scaffold lint passed.');
