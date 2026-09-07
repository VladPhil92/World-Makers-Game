# Commented repository tree

```text
World-Makers-Game/
├── .github/                         # GitHub collaboration and automation configuration.
│   ├── workflows/                   # CI workflows for repository, web, and Unreal validation/build.
│   ├── ISSUE_TEMPLATE/              # Structured issue intake for gameplay, pedagogy, and art.
│   ├── CODEOWNERS                   # Initial review ownership; expand as the team grows.
│   └── PULL_REQUEST_TEMPLATE.md     # Cross-discipline PR safety/quality checklist.
├── apps/                            # Independently deployable user-facing applications.
│   └── parent-portal/               # Parent dashboard boundary; web stack can evolve independently.
├── content/                         # Human-reviewable source content outside Unreal binaries.
│   ├── missions/                    # Curriculum-aligned mission definitions and JSON schema.
│   ├── learning-objectives/         # Learning-objective vocabulary and mapping rules.
│   ├── narrative/                   # Localizable branching story source and writing conventions.
│   └── biomes/                      # Biome metadata, cultural provenance, learning hooks, budgets.
├── game/                            # Unreal Engine 5 project root.
│   ├── Config/                      # Versioned project configuration; never secrets.
│   ├── Content/WorldMakers/         # Unreal assets organized by domain and tracked with Git LFS.
│   └── Source/WorldMakers/          # Native C++ gameplay module and future subsystem modules.
├── services/                        # Network/service boundaries independent from presentation clients.
│   └── backend/                     # Provider-neutral identity, progress, invites, and parental APIs.
│       ├── contracts/               # OpenAPI/event contracts shared by game and parent portal.
│       └── privacy/                 # PII segregation, consent, retention, and legal-review boundary.
├── docs/                            # Product/design/technical source of truth.
│   ├── GDD.md                       # Game Design Document.
│   ├── TDD.md                       # Technical Design Document.
│   ├── architecture.md              # Monorepo, service boundaries, data and deployment decisions.
│   ├── pedagogical-framework.md     # Four curriculum axes mapped to mechanics and evidence.
│   ├── art-style-guide.md           # Visual identity and tablet optimization constraints.
│   └── roadmap.md                   # Initial milestone and issue plan.
├── scripts/                         # Repeatable local/CI validation and build helpers.
├── tests/                           # Cross-component test strategy and future fixtures.
├── .gitattributes                   # Git LFS policy for Unreal, DCC, image, audio, and packages.
├── .gitignore                       # Unreal/web/editor/build/generated exclusions.
├── CONTRIBUTING.md                  # Branch, commit, code, Blueprint, asset, and review standards.
├── SECURITY.md                      # Child-safety security baseline and vulnerability process.
├── CODE_OF_CONDUCT.md               # Contributor behavior and child-data hygiene.
├── LICENSE                          # Conservative all-rights-reserved status pending final license.
└── README.md                        # Product vision, setup, architecture, privacy, and workflow entrypoint.
```

Git does not retain empty directories, so each important future asset/data boundary contains a README, schema, config, or example file.
