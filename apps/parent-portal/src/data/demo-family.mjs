export const demoFamily = Object.freeze({
  familyId: 'family.demo.01',
  authorizedParentIds: ['parent.demo.guardian-01'],
  children: [
    {
      childProfileId: 'child.demo.explorer-a',
      displayAlias: 'Explorer A',
      authorizedParentIds: ['parent.demo.guardian-01'],
      dashboard: {
        playTime: { last7DaysMinutes: 96, sessionsLast7Days: 4 },
        learning: [
          { objectiveId: 'objective.science.ecosystem-evidence', label: 'Ecosystem observation', progressPercent: 100, state: 'complete' },
          { objectiveId: 'objective.math.spatial-measurement', label: 'Spatial measurement', progressPercent: 67, state: 'in-progress' },
          { objectiveId: 'objective.design.ecological-building', label: 'Ecological design', progressPercent: 50, state: 'in-progress' },
        ],
        recentBuilds: [
          { creationId: 'creation.demo.shade-shelter', label: 'Shade Shelter', pieceCount: 7, biomeLabel: 'Caribbean Rainforest' },
          { creationId: 'creation.demo.soil-buffer', label: 'Forest Floor Buffer', pieceCount: 4, biomeLabel: 'Caribbean Rainforest' },
        ],
        adventures: [
          { adventureId: 'mission.science.rainforest-ecosystem-01', label: 'Discover the Rainforest', state: 'complete' },
          { adventureId: 'intervention.eco.shade-shelter', label: 'Make a Shady Shelter', state: 'complete' },
          { adventureId: 'intervention.eco.habitat-garden', label: 'Create a Habitat Garden', state: 'ready' },
        ],
      },
    },
    {
      childProfileId: 'child.demo.explorer-b',
      displayAlias: 'Explorer B',
      authorizedParentIds: ['parent.demo.guardian-01'],
      dashboard: {
        playTime: { last7DaysMinutes: 42, sessionsLast7Days: 2 },
        learning: [
          { objectiveId: 'objective.science.ecosystem-evidence', label: 'Ecosystem observation', progressPercent: 67, state: 'in-progress' },
          { objectiveId: 'objective.math.spatial-measurement', label: 'Spatial measurement', progressPercent: 33, state: 'in-progress' },
        ],
        recentBuilds: [
          { creationId: 'creation.demo.first-platform', label: 'First Platform', pieceCount: 5, biomeLabel: 'Caribbean Rainforest' },
        ],
        adventures: [
          { adventureId: 'mission.science.rainforest-ecosystem-01', label: 'Discover the Rainforest', state: 'in-progress' },
          { adventureId: 'intervention.eco.soil-buffer', label: 'Protect the Forest Floor', state: 'ready' },
        ],
      },
    },
  ],
});
