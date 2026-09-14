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
      currentEpicId: 'epic.garden-end-winter',
      epics: [
        {
          epicId: 'epic.garden-end-winter',
          chapterId: 'chapter.garden.restore-pollination',
          chapterIndex: 2,
          chapterCount: 4,
          state: 'in-progress',
          objectiveSummary: [
            { objectiveGroupId: 'biology', completedUnits: 2, totalUnits: 2 },
            { objectiveGroupId: 'chemistry', completedUnits: 2, totalUnits: 2 },
            { objectiveGroupId: 'ecology', completedUnits: 0, totalUnits: 2 },
          ],
          updatedAt: '2026-09-14T03:17:11.000Z',
        },
      ],
    },
  };
}
