# Binary upload boundary

The four PNG masters listed in `manifest.json` are intentionally not represented by compressed or re-encoded substitutes. They must be transferred as the original binary files and accepted only when their SHA-256 and Git blob SHA match the manifest.

The public website integration is already designed to prefer these v3 masters and fail soft to the previous v2 images until each exact binary is present on `main`.
