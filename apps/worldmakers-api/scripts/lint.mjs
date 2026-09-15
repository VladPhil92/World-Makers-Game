import { spawnSync } from 'node:child_process';
import { readdirSync } from 'node:fs';
import { join } from 'node:path';

for (const dir of ['src', 'tests']) {
  for (const file of readdirSync(dir).filter((name) => name.endsWith('.mjs')).sort()) {
    const path = join(dir, file);
    const result = spawnSync(process.execPath, ['--check', path], { stdio: 'inherit' });
    if (result.status !== 0) process.exit(result.status ?? 1);
  }
}
console.log('runtime-api lint: PASS');
