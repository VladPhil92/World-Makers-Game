# World Makers

**World Makers** is a child-safe 3D sandbox for children ages 4–10 focused on building, decorating, exploration, story-driven missions, and embedded learning.

## Product pillars

- Build, decorate, explore, and complete missions.
- No combat, item loss, theft mechanics, open stranger chat, loot boxes, or pay-to-win progression.
- Curriculum by design: mathematics, science, language/narrative, and history/culture.
- Closed multiplayer by approved invitation only.
- Parent-facing progress and play-time reporting with privacy-first data separation.
- Target platforms: Windows, macOS, iPadOS, and Android; consoles later.

## Repository strategy

This project uses a **modular monorepo** so a small cross-functional team can version gameplay, curriculum, parent-facing software, contracts, documentation, and build automation together while keeping deployable boundaries clear.

```text
game/                  Unreal Engine 5 project and gameplay code
apps/parent-portal/    Parent-facing web application boundary
services/backend/      Provider-neutral backend contracts and privacy boundary
content/               Reviewable pedagogical, narrative, and biome source data
docs/                  GDD, TDD, architecture, pedagogy, art, and roadmap
scripts/               Validation and build helpers
tests/                 Cross-component test strategy
```

See [`docs/repository-tree.md`](docs/repository-tree.md) for the complete commented tree.

## Development requirements

### Unreal game

- Unreal Engine 5.x installed through Epic Games Launcher or a source build.
- Visual Studio 2022 with Desktop development with C++ and Game development with C++ on Windows.
- Xcode toolchain on macOS.
- Git LFS enabled before cloning assets: `git lfs install`.

Open `game/WorldMakers.uproject`. If prompted, generate project files and compile the `WorldMakers` editor target.

### Parent portal

The repository reserves `apps/parent-portal` for a web implementation. The initial scaffold is framework-light and can be upgraded to Next.js once product/auth requirements are confirmed.

```bash
cd apps/parent-portal
npm ci
npm run lint
npm test
```

## Privacy baseline

Child personal data must never be committed to this repository. Account identity, verified parental consent, contact details, and other PII belong behind the private identity boundary documented in `services/backend/privacy/README.md`. Gameplay services should use pseudonymous player/profile identifiers wherever possible.

Legal review is required before production launch for COPPA, GDPR-K where applicable, Colombia Law 1581 of 2012 and related regulations, platform-store child-safety requirements, retention policy, and parental-consent flows.

## Branching

`main` is the integration trunk. Use short-lived branches such as `feat/...`, `fix/...`, `art/...`, and `content/...`; open a PR early and merge only after automated checks and the required discipline review.

## License

No open-source license has been selected. See `LICENSE`.
