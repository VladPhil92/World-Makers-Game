#!/usr/bin/env python3
"""Fail-closed validation for World Makers pre-Unreal source readiness."""
from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class ValidationError(RuntimeError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValidationError(message)


def load_json(path: str) -> dict:
    target = ROOT / path
    require(target.is_file(), f"missing {path}")
    with target.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def read(path: str) -> str:
    target = ROOT / path
    require(target.is_file(), f"missing {path}")
    return target.read_text(encoding="utf-8")


def validate_contracts(audio: dict, inputs: dict, ui: dict, accessibility: dict, platforms: dict, pre: dict) -> None:
    require(audio.get("schema") == "worldmakers.audio-design.v1", "audio schema mismatch")
    require(audio.get("status") == "source-ready-native-audio-authoring-required", "audio status mismatch")
    buses = set(audio.get("mix", {}).get("buses", []))
    require(buses == {"master", "music", "ambience", "sfx", "ui", "voice"}, "audio buses incomplete")
    cue_ids = {
        item.get("cueId") for item in audio.get("semanticCues", [])
    } | {
        item.get("cueId") for item in audio.get("rainforestSoundscape", {}).get("layers", [])
    } | {
        item.get("cueId") for item in audio.get("rainforestSoundscape", {}).get("music", [])
    }
    require(len(cue_ids) >= 17, "semantic audio catalog too small")
    require(audio.get("childSafety", {}).get("noUserVoiceCapture") is True, "user voice capture must remain disabled")
    require(audio.get("childSafety", {}).get("noOpenVoiceChat") is True, "open voice chat must remain disabled")
    require(audio.get("nativeAuthoring", {}).get("nativeAudioAssetsPresent") is False, "source gate must not fabricate native audio assets")

    require(inputs.get("schema") == "worldmakers.input-readiness.v1", "input schema mismatch")
    require(inputs.get("strategy", {}).get("primary") == "Enhanced Input", "Enhanced Input must be primary")
    required_actions = {
        "input.move", "input.look", "input.jump", "input.interact", "input.observe",
        "input.build.place", "input.build.rotate", "input.build.remove", "input.build.move",
        "input.build.cycle", "input.cancel", "input.undo", "input.redo", "input.measure",
        "input.mission.cycle", "input.pause"
    }
    require(set(inputs.get("actions", [])) == required_actions, "canonical input action set drift")
    devices = inputs.get("devices", {})
    require(devices.get("keyboardMouse", {}).get("required") is True, "keyboard/mouse required")
    require(devices.get("gamepad", {}).get("required") is True, "gamepad required")
    require(devices.get("touch", {}).get("required") is True, "touch required")
    require(devices.get("touch", {}).get("landscape") is True, "tablet touch must be landscape")
    require(devices.get("touch", {}).get("minimumTouchTargetDp", 0) >= 48, "touch targets below 48dp")
    require(inputs.get("nativeAuthoring", {}).get("nativeEnhancedInputAssetsPresent") is False, "source gate must not fabricate Enhanced Input assets")

    require(ui.get("schema") == "worldmakers.ui-production-readiness.v1", "UI schema mismatch")
    surfaces = {item.get("id") for item in ui.get("requiredSurfaces", [])}
    require(len(surfaces) >= 8, "UI surface inventory incomplete")
    require("ui.touch.controls" in surfaces and "ui.panel.settings" in surfaces, "touch/settings UI missing")
    require(ui.get("design", {}).get("safeAreaAware") is True, "UI must be safe-area aware")
    require(ui.get("design", {}).get("noCriticalMeaningByColorAlone") is True, "UI cannot rely on color alone")
    require(ui.get("motion", {}).get("reducedMotionSupported") is True, "UI reduced motion missing")
    require(ui.get("nativeAuthoring", {}).get("nativeWidgetAssetsPresent") is False, "source gate must not fabricate UMG assets")

    require(accessibility.get("schema") == "worldmakers.accessibility-localization.v1", "accessibility schema mismatch")
    cultures = accessibility.get("cultures", {})
    require(cultures.get("native") == "en", "English must remain native source culture")
    require(set(cultures.get("required", [])) == {"en", "es-419"}, "required cultures must be en and es-419")
    acc = accessibility.get("accessibility", {})
    for flag in ("subtitlesDefault", "reducedMotion", "highContrastUI", "holdToggleChoice", "colorIndependentCriticalState", "screenReaderFriendlyLabelsRequired", "focusOrderRequired"):
        require(acc.get(flag) is True, f"accessibility flag {flag} must remain enabled")
    policy = accessibility.get("copyPolicy", {})
    require(policy.get("noDarkPatterns") is True, "dark patterns prohibited")
    require(policy.get("noRealMoneyPrompts") is True, "real money prompts prohibited")
    require(policy.get("noOpenChat") is True, "open chat prohibited")
    require(policy.get("noChildPiiPrompts") is True, "child PII prompts prohibited")
    require(accessibility.get("nativeAuthoring", {}).get("nativeLocalizationAssetsPresent") is False, "source gate must not fabricate localization assets")

    require(platforms.get("schema") == "worldmakers.platform-readiness.v1", "platform schema mismatch")
    targets = {item.get("unrealPlatform"): item for item in platforms.get("targets", [])}
    require(set(targets) == {"Win64", "Android", "IOS"}, "target platform set must be Win64/Android/IOS")
    require(targets["Android"].get("architecture") == "arm64", "Android must target arm64")
    require(targets["IOS"].get("architecture") == "arm64", "iPadOS must target arm64")
    require(targets["Android"].get("textureCookFlavor") == "ASTC", "Android texture cook flavor must be ASTC")
    shared = platforms.get("sharedRequirements", {})
    require(shared.get("offlineFirstVerticalSlice") is True, "vertical slice must remain offline-first")
    require(shared.get("backgroundResumeRequiredOnMobile") is True, "mobile background/resume required")
    require(shared.get("noRuntimeSecretsInClient") is True, "runtime secrets in client prohibited")
    require(shared.get("runtimeApiHttpsOnly") is True, "runtime API must remain HTTPS-only")
    require(platforms.get("nativeValidation", {}).get("nativePlatformCertified") is False, "source gate must not claim native platform certification")

    require(pre.get("schema") == "worldmakers.pre-unreal-content-platform-readiness.v1", "pre-Unreal schema mismatch")
    require(pre.get("engine", {}).get("requiredVersion") == "5.8.2", "engine lock must remain 5.8.2")
    domains = {item.get("id") for item in pre.get("requiredDomains", []) if item.get("required")}
    expected_domains = {
        "gameplay-runtime", "mission-science-content", "visual-animation-vfx", "audio", "input", "ui",
        "accessibility-localization", "platforms", "cloud-runtime", "packaging-certification"
    }
    require(domains == expected_domains, "required pre-Unreal domain inventory drift")
    source_gate = pre.get("sourceGate", {})
    require(source_gate.get("enhancedInputEnabled") is True, "Enhanced Input gate disabled")
    require(source_gate.get("nativeBinaryAssetsMustNotBeFabricated") is True, "native fabrication boundary missing")
    require(source_gate.get("mapMustBeCreatedByUnreal") is True, "map authoring boundary missing")
    require(pre.get("exitCriteria", {}).get("nextPhase") == "native-unreal-materialization", "next phase mismatch")


def validate_source_wiring(audio: dict, inputs: dict) -> None:
    uproject = json.loads(read("game/WorldMakers.uproject"))
    plugins = {item.get("Name"): item.get("Enabled") for item in uproject.get("Plugins", [])}
    require(plugins.get("EnhancedInput") is True, "EnhancedInput plugin not enabled")

    build_cs = read("game/Source/WorldMakers/WorldMakers.Build.cs")
    require('"EnhancedInput"' in build_cs, "EnhancedInput module not linked")

    audio_h = read("game/Source/WorldMakers/Audio/WMAudioReadinessSubsystem.h")
    audio_cpp = read("game/Source/WorldMakers/Audio/WMAudioReadinessSubsystem.cpp")
    require("UWMAudioReadinessSubsystem" in audio_h, "audio subsystem missing")
    require("OnSemanticAudioCueRequested" in audio_h, "semantic audio event boundary missing")
    for cue in [item.get("cueId") for item in audio.get("semanticCues", [])]:
        require(cue in audio_cpp, f"semantic audio cue not wired in runtime: {cue}")

    accessibility_h = read("game/Source/WorldMakers/Accessibility/WMAccessibilitySubsystem.h")
    accessibility_cpp = read("game/Source/WorldMakers/Accessibility/WMAccessibilitySubsystem.cpp")
    require("bReducedMotion" in accessibility_h and "bHighContrastUI" in accessibility_h, "accessibility runtime incomplete")
    require("Settings.CameraShakeScale = 0.0f" in accessibility_cpp, "reduced motion must disable camera shake")

    input_h = read("game/Source/WorldMakers/Input/WMInputReadinessLibrary.h")
    input_cpp = read("game/Source/WorldMakers/Input/WMInputReadinessLibrary.cpp")
    require("GetDefaultTabletTouchLayout" in input_h, "touch layout runtime missing")
    for action in inputs.get("actions", []):
        require(action in input_cpp, f"canonical action not wired in runtime: {action}")

    default_game = read("game/Config/DefaultGame.ini")
    for staged in ("Audio", "Input", "UI", "Accessibility", "Platforms"):
        require(f'Path="WorldMakers/{staged}"' in default_game, f"{staged} content not staged for packaging")

    localization = read("game/Config/Localization/Game.ini")
    require("NativeCulture=en" in localization, "localization native culture missing")
    require("CulturesToGenerate=es-419" in localization, "es-419 localization target missing")

    device_profiles = read("game/Config/DefaultDeviceProfiles.ini")
    for profile in ("WMAndroidLow", "WMAndroidMid", "WMAndroidHigh", "WMiPadLow", "WMiPadMid", "WMiPadHigh"):
        require(f"[{profile} DeviceProfile]" in device_profiles, f"device profile missing: {profile}")

    default_engine = read("game/Config/DefaultEngine.ini")
    require("UIScaleRule=ShortestSide" in default_engine, "responsive UI scaling baseline missing")
    require("RenderFocusRule=NavigationOnly" in default_engine, "controller/keyboard focus baseline missing")

    copy_contract = load_json("game/Content/WorldMakers/Accessibility/localized-copy-en-es419-v1.json")
    entries = copy_contract.get("entries", [])
    require(len(entries) >= 30, "localized core UI copy is too small")
    for entry in entries:
        require(entry.get("key") and entry.get("en") and entry.get("es-419"), "localized copy contains incomplete entry")

    require(read("game/UNREAL_ENGINE_VERSION").strip() == "5.8.2", "UNREAL_ENGINE_VERSION drift")
    require((ROOT / "apps/worldmakers-api").is_dir(), "worldmakers-api cloud boundary missing")
    require((ROOT / "content/production/production-release-readiness-v1.json").is_file(), "production release gate missing")


def run_validation() -> None:
    audio = load_json("game/Content/WorldMakers/Audio/audio-design-v1.json")
    inputs = load_json("game/Content/WorldMakers/Input/input-readiness-v1.json")
    ui = load_json("game/Content/WorldMakers/UI/ui-production-readiness-v1.json")
    accessibility = load_json("game/Content/WorldMakers/Accessibility/accessibility-localization-v1.json")
    platforms = load_json("game/Content/WorldMakers/Platforms/platform-readiness-v1.json")
    pre = load_json("content/production/pre-unreal-content-platform-readiness-v1.json")
    validate_contracts(audio, inputs, ui, accessibility, platforms, pre)
    validate_source_wiring(audio, inputs)


def expect_contract_failure(audio: dict, inputs: dict, ui: dict, accessibility: dict, platforms: dict, pre: dict, label: str) -> None:
    try:
        validate_contracts(audio, inputs, ui, accessibility, platforms, pre)
    except ValidationError:
        return
    raise ValidationError(f"self-test failed closed check: {label}")


def self_test() -> None:
    audio = load_json("game/Content/WorldMakers/Audio/audio-design-v1.json")
    inputs = load_json("game/Content/WorldMakers/Input/input-readiness-v1.json")
    ui = load_json("game/Content/WorldMakers/UI/ui-production-readiness-v1.json")
    accessibility = load_json("game/Content/WorldMakers/Accessibility/accessibility-localization-v1.json")
    platforms = load_json("game/Content/WorldMakers/Platforms/platform-readiness-v1.json")
    pre = load_json("content/production/pre-unreal-content-platform-readiness-v1.json")

    bad_audio = copy.deepcopy(audio)
    bad_audio["childSafety"]["noUserVoiceCapture"] = False
    expect_contract_failure(bad_audio, inputs, ui, accessibility, platforms, pre, "voice capture enabled")

    bad_input = copy.deepcopy(inputs)
    bad_input["devices"]["touch"]["required"] = False
    expect_contract_failure(audio, bad_input, ui, accessibility, platforms, pre, "touch removed")

    bad_accessibility = copy.deepcopy(accessibility)
    bad_accessibility["cultures"]["required"] = ["en"]
    expect_contract_failure(audio, inputs, ui, bad_accessibility, platforms, pre, "es-419 removed")

    bad_platforms = copy.deepcopy(platforms)
    bad_platforms["targets"] = [item for item in bad_platforms["targets"] if item["unrealPlatform"] != "IOS"]
    expect_contract_failure(audio, inputs, ui, accessibility, bad_platforms, pre, "iPadOS removed")

    print("Pre-Unreal validator self-test passed: unsafe audio, missing touch, missing es-419 and missing iPadOS all fail closed.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        if args.self_test:
            self_test()
        else:
            run_validation()
            print("Pre-Unreal Content & Platform source readiness: PASS")
            print("PASS certifies source readiness only; native Unreal assets, maps and device evidence remain intentionally unclaimed.")
    except (ValidationError, json.JSONDecodeError) as exc:
        raise SystemExit(f"Pre-Unreal Content & Platform validation FAILED: {exc}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
