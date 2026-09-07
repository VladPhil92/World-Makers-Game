# Economy content boundary

This directory contains **gameplay progression/reward definitions**, not paid commerce state.

## Rules

- Rewards are deterministic.
- Rewards are earned, never purchased.
- Rewards cannot be transferred or converted to money/store credit/crypto.
- No reward expires because the player stopped playing.
- No streak-loss requirement.
- No price, platform SKU, receipt, transaction, checkout, subscription, or payment-provider data belongs here.

Paid offers and entitlements belong to the parent/backend trust zone under `services/backend/contracts`.

## Files

- `schemas/reward.schema.json` — invariant contract for gameplay rewards.
- `rewards/default-rewards.json` — initial M1.10 reward examples tied to learning/exploration.

Future mission-runtime work should reference stable `rewardId` values and emit reward grants through a deterministic progression subsystem.
