import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

const envExample = await readFile(new URL('../.env.example', import.meta.url), 'utf8');
assert.match(envExample, /WORLD_MAKERS_API_BASE_URL=/);
assert.doesNotMatch(envExample, /PRIVATE_KEY=/);
console.log('Parent portal scaffold tests passed.');
