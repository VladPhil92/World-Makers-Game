import { cloneDefaultLoadout, cloneDefaultSelection } from '../domain/catalog.mjs';

export function createDemoPlayer() {
  return {
    playerId: 'player.demo.explorer-01',
    displayName: 'Explorer',
    avatarId: 'avatar.child-explorer.v1',
    loadout: cloneDefaultLoadout(),
    selection: cloneDefaultSelection(),
    entitlements: [
      'hair.explorer-a',
      'top.field-jacket-a',
      'bottom.utility-a',
      'footwear.trail-a',
      'back.explorer-pack-a',
    ],
    progress: {
      level: 4,
      discoveries: 18,
      builds: 11,
      currentAdventure: {
        missionId: 'mission.thousand-voices',
        worldId: 'world.rainforest',
        progressPercent: 38,
      },
    },
  };
}
