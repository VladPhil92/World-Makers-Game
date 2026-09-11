import test from 'node:test';
import assert from 'node:assert/strict';
import {
  avatarSlots,
  defaultLoadout,
  gameModes,
  worlds,
  missions,
  storeCatalog,
  validateLoadout,
  validateSelection,
} from '../src/domain/catalog.mjs';

test('D1 exposes exactly three playable modes plus locked cooperative', () => {
  assert.deepEqual(gameModes.filter((mode) => mode.playable).map((mode) => mode.id), ['free-explore', 'missions', 'laboratory']);
  assert.equal(gameModes.find((mode) => mode.id === 'cooperative')?.playable, false);
});

test('avatar loadout covers stable P3 cosmetic slots', () => {
  assert.deepEqual(Object.keys(avatarSlots), ['hair', 'top', 'bottom', 'footwear', 'head-accessory', 'back-accessory', 'hand-prop']);
  assert.deepEqual(validateLoadout(defaultLoadout), defaultLoadout);
  assert.throws(() => validateLoadout({ ...defaultLoadout, top: 'top.not-real' }), /Invalid cosmetic/);
});

test('world selection is constrained by mode compatibility', () => {
  assert.deepEqual(validateSelection({ modeId: 'free-explore', worldId: 'world.rainforest', missionId: null }), {
    modeId: 'free-explore', worldId: 'world.rainforest', missionId: null,
  });
  assert.throws(() => validateSelection({ modeId: 'laboratory', worldId: 'world.impossible-city', missionId: null }), /incompatible/);
});

test('mission mode requires a mission in the selected world', () => {
  const mission = missions.find((item) => item.worldId === 'world.rainforest');
  assert.ok(mission);
  assert.deepEqual(validateSelection({ modeId: 'missions', worldId: 'world.rainforest', missionId: mission.id }), {
    modeId: 'missions', worldId: 'world.rainforest', missionId: mission.id,
  });
  assert.throws(() => validateSelection({ modeId: 'missions', worldId: 'world.rainforest', missionId: 'mission.moonforge' }), /Mission is required/);
});

test('store is cosmetic-only and parent approval gated', () => {
  assert.ok(storeCatalog.length >= 4);
  assert.ok(storeCatalog.every((item) => item.purchasePolicy === 'parent-approval-required'));
  assert.ok(storeCatalog.every((item) => ['top', 'back-accessory', 'head-accessory', 'hand-prop'].includes(item.type)));
  assert.ok(worlds.every((world) => world.modes.length >= 2));
});
