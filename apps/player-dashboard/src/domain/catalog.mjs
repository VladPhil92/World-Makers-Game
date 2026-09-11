export const avatarSlots = Object.freeze({
  hair: ['hair.explorer-a', 'hair.curl-a', 'hair.short-a'],
  top: ['top.field-jacket-a', 'top.science-hoodie-a', 'top.eco-vest-a'],
  bottom: ['bottom.utility-a', 'bottom.trail-a', 'bottom.lab-a'],
  footwear: ['footwear.trail-a', 'footwear.sneaker-a', 'footwear.boot-a'],
  'head-accessory': ['none', 'head.goggles-a', 'head.cap-a'],
  'back-accessory': ['none', 'back.explorer-pack-a', 'back.botany-pack-a'],
  'hand-prop': ['none', 'hand.scanner-skin-a', 'hand.field-notebook-a'],
});

export const defaultLoadout = Object.freeze({
  hair: 'hair.explorer-a',
  top: 'top.field-jacket-a',
  bottom: 'bottom.utility-a',
  footwear: 'footwear.trail-a',
  'head-accessory': 'none',
  'back-accessory': 'back.explorer-pack-a',
  'hand-prop': 'none',
});

export const gameModes = Object.freeze([
  {
    id: 'free-explore',
    name: 'Exploración Libre',
    eyebrow: 'Crea sin límites',
    description: 'Explora, construye, destruye, observa y experimenta sin una misión obligatoria.',
    playable: true,
    icon: '◇',
  },
  {
    id: 'missions',
    name: 'Aventuras & Misiones',
    eyebrow: 'Aprende dentro de la historia',
    description: 'Vive aventuras fantásticas en las que el conocimiento modifica lo que puedes hacer en el mundo.',
    playable: true,
    icon: '✦',
  },
  {
    id: 'laboratory',
    name: 'Laboratorio',
    eyebrow: 'Prueba ideas',
    description: 'Experimenta con física, química, biología, ecología, medición y construcción en espacios controlados.',
    playable: true,
    icon: '⌬',
  },
  {
    id: 'cooperative',
    name: 'Cooperativo',
    eyebrow: 'Próximamente',
    description: 'Construcción y descubrimiento compartidos con controles sociales y de seguridad dedicados.',
    playable: false,
    icon: '∞',
  },
]);

export const worlds = Object.freeze([
  {
    id: 'world.rainforest',
    name: 'Rainforest World',
    description: 'Selva eco-futurista con cascadas, domos científicos y ecosistemas reactivos.',
    modes: ['free-explore', 'missions'],
    accent: 'growth',
  },
  {
    id: 'world.research-island',
    name: 'Research Island',
    description: 'Isla de investigación con invernaderos, energía renovable y zonas de experimentación.',
    modes: ['free-explore', 'laboratory'],
    accent: 'science',
  },
  {
    id: 'world.impossible-city',
    name: 'Impossible City',
    description: 'Arquitectura curva, geometría y construcción creativa en una ciudad que desafía la intuición.',
    modes: ['free-explore', 'missions'],
    accent: 'build',
  },
  {
    id: 'world.moonforge',
    name: 'The Moonforge',
    description: 'Escenario de física y aventura donde masa, fuerza y movimiento importan para avanzar.',
    modes: ['missions', 'laboratory'],
    accent: 'physics',
  },
]);

export const missions = Object.freeze([
  { id: 'mission.moonforge', name: 'The Moonforge', worldId: 'world.moonforge', subject: 'Physics' },
  { id: 'mission.cell-city', name: 'The City Inside a Cell', worldId: 'world.research-island', subject: 'Biology' },
  { id: 'mission.alchemist', name: "The Alchemist's Archipelago", worldId: 'world.research-island', subject: 'Chemistry' },
  { id: 'mission.thousand-voices', name: 'The Forest of a Thousand Voices', worldId: 'world.rainforest', subject: 'Ecology' },
]);

export const storeCatalog = Object.freeze([
  {
    id: 'store.top.rain-jacket',
    name: 'Rainfinder Jacket',
    type: 'top',
    entitlementId: 'top.rainfinder-jacket',
    previewTone: 'growth',
    purchasePolicy: 'parent-approval-required',
  },
  {
    id: 'store.back.botany-pack',
    name: 'Botany Field Pack',
    type: 'back-accessory',
    entitlementId: 'back.botany-pack-a',
    previewTone: 'biology',
    purchasePolicy: 'parent-approval-required',
  },
  {
    id: 'store.head.science-goggles',
    name: 'Research Goggles',
    type: 'head-accessory',
    entitlementId: 'head.goggles-a',
    previewTone: 'science',
    purchasePolicy: 'parent-approval-required',
  },
  {
    id: 'store.hand.scanner-skin',
    name: 'Aurora Scanner Shell',
    type: 'hand-prop',
    entitlementId: 'hand.scanner-skin-a',
    previewTone: 'discovery',
    purchasePolicy: 'parent-approval-required',
  },
]);

export const defaultSelection = Object.freeze({
  modeId: 'free-explore',
  worldId: 'world.rainforest',
  missionId: null,
});

export function cloneDefaultLoadout() {
  return structuredClone(defaultLoadout);
}

export function cloneDefaultSelection() {
  return structuredClone(defaultSelection);
}

export function validateLoadout(loadout) {
  if (!loadout || typeof loadout !== 'object' || Array.isArray(loadout)) throw new TypeError('Invalid loadout.');
  const normalized = {};
  for (const [slot, allowed] of Object.entries(avatarSlots)) {
    const value = loadout[slot];
    if (typeof value !== 'string' || !allowed.includes(value)) throw new TypeError(`Invalid cosmetic for slot ${slot}.`);
    normalized[slot] = value;
  }
  return normalized;
}

export function validateSelection(selection) {
  if (!selection || typeof selection !== 'object' || Array.isArray(selection)) throw new TypeError('Invalid selection.');
  const mode = gameModes.find((item) => item.id === selection.modeId);
  if (!mode || !mode.playable) throw new TypeError('Game mode is unavailable.');
  const world = worlds.find((item) => item.id === selection.worldId);
  if (!world || !world.modes.includes(mode.id)) throw new TypeError('World is incompatible with selected mode.');

  let missionId = null;
  if (mode.id === 'missions') {
    const mission = missions.find((item) => item.id === selection.missionId);
    if (!mission || mission.worldId !== world.id) throw new TypeError('Mission is required and must belong to selected world.');
    missionId = mission.id;
  }

  return { modeId: mode.id, worldId: world.id, missionId };
}

export function dashboardCatalog() {
  return {
    avatarSlots,
    gameModes,
    worlds,
    missions,
    storeCatalog,
  };
}
