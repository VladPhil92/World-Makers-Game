# World Makers — Game Design Document

> Working design source of truth. Product decisions should be recorded here before they become expensive implementation assumptions.

## 1. Vision

A premium-stylized 3D sandbox for children ages 4–10 where creativity, exploration, storytelling, and learning reinforce each other without combat, punitive loss, manipulative engagement, or aggressive monetization.

## 2. Audience

- Primary players: children ages 4–10.
- Secondary users: parents/guardians who approve social access, purchases, and review progress.
- Internal stakeholders: game design, engineering, 3D art, level design, pedagogy/content, security/privacy, trust/safety, commerce operations.

## 3. Core pillars

### Build
Place, modify, combine, and personalize structures and spaces.

### Decorate
Express identity and aesthetic choices through safe, non-pay-to-win customization.

### Explore
Discover biomes, environmental systems, characters, stories, and learning opportunities.

### Missions
Interactive stories and challenges that map learning objectives to meaningful play.

### Trust
The product must protect a child's attention, data, social environment, and relationship with money. Monetization may fund the experience but must never be designed to exploit developmental vulnerability.

## 4. Player character

- Child avatar selectable/personalizable from the start.
- No default imposed gender.
- Inclusive customization without exposing identity attributes to strangers.
- [PLACEHOLDER: customization catalog and accessibility requirements]

## 5. Core loop

Explore → discover goal/story → plan → build/solve/create → receive non-punitive feedback/reward → unlock new creative possibilities → reflect/progress → stop naturally or continue by choice.

### Right to Stop

World Makers must preserve natural exit points. Leaving must never destroy streaks, rewards, creations, progress, or access already earned. The game may say that progress is saved; it must not guilt, pressure, countdown, or threaten loss to prevent exit.

## 6. Building system

- Placement, move, rotate, recolor/decorate.
- Resource budgets can support mathematical reasoning without destructive scarcity.
- No theft of another player's items.
- No irreversible loss as a core loop.
- [PLACEHOLDER: grid/freeform hybrid, undo/redo, blueprint sharing rules]

## 7. Missions, narrative, and reward taxonomy

- Story missions can branch based on creative choices.
- Avoid detached worksheet-style quizzes where gameplay can demonstrate understanding.
- Narrative source should use localization keys and age-band readability review.
- Rewards must be deterministic, non-purchasable, non-convertible to money, and must expand possibility rather than manufacture compulsion.

Reward classes:

1. **Intrinsic feedback** — the world, NPCs, systems, or creation visibly respond to what the player accomplished.
2. **Creative unlock** — new building piece, material, decorative capability, tool, or customization option.
3. **Mastery recognition** — a badge or record acknowledging demonstrated learning or craft; never a public ranking of children.
4. **Narrative unlock** — new story branch, character interaction, location, or project opportunity.
5. **Discovery recognition** — collections, observations, ecosystem discoveries, or cultural/contextual knowledge.

Numerical XP may exist only where it improves comprehension; it must not become the primary psychological reward loop.

## 8. Pedagogical integration

Four curriculum axes:

1. Mathematics — proportion, measurement, spatial reasoning, resource budgeting.
2. Science — natural cycles, systems, ecosystems, observation, cause/effect.
3. Language / narrative — comprehension, sequencing, dialogue, storytelling, branching choices.
4. History / culture — curated biomes, material culture, stories, context, provenance, respectful representation.

See `docs/pedagogical-framework.md`.

## 9. Biomes

Each biome combines visual identity, exploration affordances, building materials, ecological systems, cultural/contextual references, and learning hooks.

Initial vertical-slice candidate: **Caribbean Rainforest / Selva Caribeña**.

## 10. Multiplayer and social safety

- Closed multiplayer only.
- Parent/guardian-approved relationships or invitations.
- No public lobby discovery for children.
- No open stranger chat.
- All multiplayer permissions server-authoritative.
- [PLACEHOLDER: emotes/preset communication, session size, persistence model]

## 11. Parent experience

Parent portal should provide:

- play time summary;
- builds/creative activity summaries;
- learning-objective progress;
- social/invite controls;
- account/privacy controls;
- purchase/entitlement history;
- store enable/disable and child-interest controls;
- subscription management and restore-purchase workflows;
- export/delete workflows where required.

It must not become an invasive surveillance dashboard or expose child data to third-party advertisers.

## 12. Trust Economy and monetization

World Makers monetizes **content value**, not child attention or behavioral vulnerability.

### Allowed commercial model

- premium/base game purchase where commercially appropriate;
- curated biome/expansion packs;
- creative/building packs;
- cosmetic packs with no gameplay advantage;
- optional family membership/subscription for an expanded library and family services;
- institutional/education licensing.

All commercial products are non-consumable content or transparent subscriptions. Cancellation must not delete a child's worlds, achievements, or progress.

### Parent-owned commerce

- The child runtime never executes a payment.
- The child may save an item to a family interest list; this must not trigger nagging, urgency, or repeated parent notifications.
- Prices, checkout, platform billing, receipt validation, refund state, and subscriptions are parent-facing/server-side concerns.
- Purchase opportunities must be behind appropriate parental authorization and platform purchase confirmation.
- A parent may disable all commercial surfaces for the child profile.

### Gameplay economy

Gameplay rewards/currencies, if used, are:

- earned only through play/learning/creation;
- not purchasable;
- not transferable between users;
- not saleable;
- not convertible to fiat, crypto, store credit, or paid content currency;
- deterministic rather than randomized for paid value.

### Product-level prohibitions

World Makers does not ship:

- third-party advertising in the child experience;
- rewarded ads or ad walls;
- loot boxes, gacha, paid random rewards, wheels, mystery purchases;
- pay-to-win, paid learning boosts, or paid stat advantages;
- purchasable premium currency or obscured real-money conversion;
- daily-login rewards designed around loss aversion;
- streaks that punish missed days;
- expiring battle passes or paid FOMO tracks;
- energy/life timers that can be bypassed with payment;
- pay-to-skip friction intentionally introduced for monetization;
- fake scarcity, countdown sales, emotional pressure, or manipulative urgency aimed at children;
- commercial push notifications addressed to child profiles;
- public wealth/status rankings of children.

See `docs/trust-economy.md`.

## 13. Platforms and input

Phase 1: Windows, macOS, iPadOS, Android.

Future: consoles after platform, certification, performance, and account-system implications are assessed.

[PLACEHOLDER: controller support and keyboard/mouse accessibility; M1.7 establishes the first touch interaction shell]

## 14. Accessibility

[PLACEHOLDER: text size, color dependence, motor accessibility, reading assistance, subtitles, audio cues, dyslexia-friendly options, reduced motion]

## 15. Art direction

Premium stylized / low-poly-informed visual language with stronger fidelity than voxel sandboxes while preserving clarity and mobile scalability. References are inspirational only; World Makers requires an original visual identity.

See `docs/art-style-guide.md`.

## 16. Audio

[PLACEHOLDER: music identity, ambient systems, child-safe voice strategy, SFX readability, volume/accessibility controls]

## 17. Success metrics

Prefer healthy product metrics over compulsion metrics:

- creative completion and diversity;
- mission completion and learning evidence;
- safe social sessions;
- parent trust/retention;
- crash-free sessions and device performance;
- age-appropriate session health;
- healthy exit rate (sessions ending at natural stopping points without coercive retention prompts);
- purchase satisfaction/refund and dispute health at family-account level.

DAU, retention, session length, ARPU, conversion, and similar business metrics may be observed, but must not be optimized through child-targeted dark patterns or compulsive loops.
