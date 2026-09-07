#!/usr/bin/env python3
"""M1.10 Trust Economy and child-safe commerce source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def require_tokens(path: str, tokens: tuple[str, ...]) -> None:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    if missing:
        fail(f"{path} missing Trust Economy tokens: {missing}")


def main() -> None:
    required_files = (
        "docs/trust-economy.md",
        "docs/compliance/child-commerce-boundaries.md",
        "content/economy/README.md",
        "content/economy/schemas/reward.schema.json",
        "content/economy/rewards/default-rewards.json",
        "services/backend/contracts/commerce-offer.schema.json",
        "services/backend/contracts/example-parent-offers.json",
        "services/backend/contracts/commerce.openapi.yaml",
        "game/Source/WorldMakers/Economy/WMTrustEconomyTypes.h",
        "game/Source/WorldMakers/Economy/WMTrustEconomyTypes.cpp",
        "game/Source/WorldMakers/Private/Tests/WMTrustEconomyTests.cpp",
    )
    missing_files = [path for path in required_files if not (ROOT / path).exists()]
    if missing_files:
        fail(f"Missing M1.10 files: {missing_files}")

    gdd = read("docs/GDD.md")
    if "[PLACEHOLDER: commercial model" in gdd or "[PLACEHOLDER: recurring characters, mission cadence, reward taxonomy]" in gdd:
        fail("GDD economy/reward placeholders must be resolved by M1.10")
    for token in ("Right to Stop", "Parent-owned commerce", "purchasable premium currency", "third-party advertising in the child experience"):
        if token not in gdd:
            fail(f"GDD missing locked Trust Economy rule: {token}")

    require_tokens(
        "docs/TDD.md",
        (
            "Commerce Catalog",
            "Parent Purchase Authorization",
            "Payment Provider Adapters",
            "Entitlements",
            "Reward Ledger / Progression",
            "Child Runtime -> Entitlement Read / Child Interest Request",
            "Child Runtime -> Payment / Receipt Validation / Provider Checkout",
            "WorldMakers.*",
        ),
    )

    reward_schema = json.loads(read("content/economy/schemas/reward.schema.json"))
    props = reward_schema.get("properties", {})
    invariants = {
        "deterministic": True,
        "purchasable": False,
        "transferable": False,
        "convertibleToMoney": False,
    }
    for field, expected in invariants.items():
        if props.get(field, {}).get("const") is not expected:
            fail(f"Reward schema must lock {field}={expected}")

    allowed_reward_types = set(props.get("type", {}).get("enum", []))
    rewards = json.loads(read("content/economy/rewards/default-rewards.json"))
    if not isinstance(rewards, list) or not rewards:
        fail("Default reward catalog must be a non-empty list")
    reward_ids: set[str] = set()
    for reward in rewards:
        reward_id = reward.get("rewardId")
        if not isinstance(reward_id, str) or not reward_id.startswith("reward."):
            fail(f"Invalid rewardId: {reward_id!r}")
        if reward_id in reward_ids:
            fail(f"Duplicate rewardId: {reward_id}")
        reward_ids.add(reward_id)
        if reward.get("type") not in allowed_reward_types:
            fail(f"Reward {reward_id} has unapproved type")
        for field, expected in invariants.items():
            if reward.get(field) is not expected:
                fail(f"Reward {reward_id} must set {field}={expected}")
        if reward.get("expiresAt") is not None:
            fail(f"Reward {reward_id} must not expire due to engagement timing")
        if reward.get("streakRequired") is not False:
            fail(f"Reward {reward_id} must not require a streak")

    offer_schema = json.loads(read("services/backend/contracts/commerce-offer.schema.json"))
    offer_props = offer_schema.get("properties", {})
    locked_offer_fields = {
        "parentAudienceOnly": True,
        "randomizedContent": False,
        "manipulativeUrgency": False,
        "consumableCurrency": False,
    }
    for field, expected in locked_offer_fields.items():
        if offer_props.get(field, {}).get("const") is not expected:
            fail(f"Commerce offer schema must lock {field}={expected}")

    offers = json.loads(read("services/backend/contracts/example-parent-offers.json"))
    if not isinstance(offers, list) or not offers:
        fail("Example parent offer catalog must be non-empty")
    allowed_products = set(offer_props.get("productType", {}).get("enum", []))
    for offer in offers:
        offer_id = offer.get("offerId", "<unknown>")
        if offer.get("productType") not in allowed_products:
            fail(f"Offer {offer_id} has unapproved productType")
        for field, expected in locked_offer_fields.items():
            if offer.get(field) is not expected:
                fail(f"Offer {offer_id} must set {field}={expected}")
        if "price" in offer or "localizedPrice" in offer:
            fail(f"Static offer source {offer_id} must not embed price data; platform pricing resolves in parent API")

    commerce_api = read("services/backend/contracts/commerce.openapi.yaml")
    for token in (
        "/v1/child/interests:",
        "/v1/family/purchases:",
        "/v1/family/transactions/verify:",
        "/v1/runtime/entitlements:",
        "localizedPrice:",
        "parentAuth:",
        "childSession:",
        "Read-only projection; contains no prices",
    ):
        if token not in commerce_api:
            fail(f"Commerce OpenAPI missing required boundary token: {token}")

    runtime_types = read("game/Source/WorldMakers/Economy/WMTrustEconomyTypes.h")
    for token in (
        "FWMGameplayRewardGrant",
        "FWMEntitlementSnapshot",
        "IsPurchasable() const { return false; }",
        "IsTransferable() const { return false; }",
        "IsConvertibleToMoney() const { return false; }",
    ):
        if token not in runtime_types:
            fail(f"Runtime Trust Economy type missing invariant: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMTrustEconomyTests.cpp")
    for test_name in (
        "WorldMakers.TrustEconomy.Reward.Invariants",
        "WorldMakers.TrustEconomy.Entitlement.ReadOnlyProjection",
    ):
        if test_name not in tests:
            fail(f"Missing Trust Economy automation test: {test_name}")

    child_paths = [ROOT / "game/Source", ROOT / "game/Config", ROOT / "content/missions"]
    forbidden_runtime_tokens = (
        "storekit",
        "billingclient",
        "revenuecat",
        "admob",
        "rewarded_ad",
        "rewardedad",
        "premiumcurrency",
        "premium_currency",
        "lootbox",
        "loot_box",
        "gacha",
        "daily_streak",
        "dailystreak",
        "battle_pass",
        "battlepass",
        "receiptvalidation",
    )
    violations: list[str] = []
    for base in child_paths:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in {".h", ".cpp", ".ini", ".json", ".yaml", ".yml", ".md"}:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore").lower()
            for token in forbidden_runtime_tokens:
                if token in text:
                    violations.append(f"{path.relative_to(ROOT)}:{token}")
    if violations:
        fail(f"Forbidden commerce/manipulation primitives found in child runtime/content: {violations}")

    policy = read("docs/trust-economy.md").lower()
    for phrase in (
        "no third-party ads in child gameplay",
        "no purchasable premium currency",
        "right to stop",
        "payment sdk",
    ):
        if phrase not in policy:
            fail(f"Trust Economy policy missing invariant: {phrase}")

    print(f"M1.10 Trust Economy passed: {len(rewards)} deterministic rewards, {len(offers)} parent-only offers, runtime payment boundary clean.")


if __name__ == "__main__":
    main()
