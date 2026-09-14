#!/usr/bin/env python3
"""M5.6B game-first Eclipse Engine vertical epic source gate."""
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EPIC_CANONICAL = ROOT / "content/epics/cross-disciplinary-epics-v1.json"
EPIC_PACKAGED = ROOT / "game/Content/WorldMakers/Epics/cross-disciplinary-epics-v1.json"
EXPERIENCE_CANONICAL = ROOT / "content/epics/eclipse-engine-player-experience-v1.json"
EXPERIENCE_PACKAGED = ROOT / "game/Content/WorldMakers/Epics/eclipse-engine-player-experience-v1.json"
EXPECTED_DISCIPLINES = {"mathematics","geometry","physics","english-language","literature","philosophy-for-children"}
FORBIDDEN_SCHOOL_TERMS = {"lesson","quiz","learning-objective","learning objective","grade","score","streak","homework","correct-answer","wrong-answer"}

def fail(message: str) -> None: raise SystemExit(message)
def load(path: Path) -> dict: return json.loads(path.read_text(encoding="utf-8"))
def read(path: str) -> str: return (ROOT / path).read_text(encoding="utf-8")
def route_key(route: dict) -> tuple[str,...]: return tuple(route.get(k,"") for k in ("objectiveId","disciplineId","producerKind","producerRefId","primitiveId","evidenceEventId"))
def state_key(route: dict) -> tuple[str,...]: return tuple(route.get(k,"") for k in ("producerKind","producerRefId","worldStateId"))

def player_facing_strings(experience: dict) -> list[str]:
    values=[]
    for chapter in experience.get("chapters",[]):
        values.extend(str(chapter.get(k,"")) for k in ("fantasyGoalKey","tensionKey","completionReactionKey"))
        for action in chapter.get("actions",[]):
            values.extend(str(action.get(k,"")) for k in ("actionId","interactionVerb","promptKey","feedbackKey"))
            values.extend(str(x) for x in action.get("hints",[]))
    return values

def main() -> None:
    required=[EPIC_CANONICAL,EPIC_PACKAGED,EXPERIENCE_CANONICAL,EXPERIENCE_PACKAGED,
        ROOT/"docs/m5-6b-eclipse-engine-vertical-epic.md",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseEngineExperience.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseEngineExperience.cpp",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseEngineInteractableActor.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseEngineInteractableActor.cpp",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseOpticsRuntime.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseOpticsRuntime.cpp",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseSystemsRuntime.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMEclipseSystemsRuntime.cpp",
        ROOT/"game/Source/WorldMakers/Private/Tests/WMEclipseEngineExperienceTests.cpp"]
    missing=[str(p.relative_to(ROOT)) for p in required if not p.exists()]
    if missing: fail(f"Missing M5.6B files: {missing}")

    epic_catalog=load(EPIC_CANONICAL)
    if epic_catalog != load(EPIC_PACKAGED): fail("Packaged epic catalog diverges from canonical source")
    experience=load(EXPERIENCE_CANONICAL)
    if experience != load(EXPERIENCE_PACKAGED): fail("Packaged Eclipse experience diverges from canonical source")
    if experience.get("schemaVersion") != 1 or experience.get("prototypeOnly") is not True: fail("M5.6B must remain prototype schema v1")
    if experience.get("experienceId") != "experience.eclipse-engine-v1" or experience.get("epicId") != "epic.eclipse-engine": fail("Unexpected Eclipse identity")
    if experience.get("presentationRule") != "world-first-no-school-ui": fail("Eclipse must remain world-first")
    if experience.get("rewardModel") != "world-transformation-and-new-capability": fail("Rewards must remain intrinsic/world-transforming")
    if experience.get("failureModel") != "reversible-experimentation-with-visible-consequence": fail("Failure must remain reversible experimentation")
    if experience.get("hintModel") != "player-requested-progressive-environmental-hints": fail("Hints must remain player-requested")
    if experience.get("privacyModel") != "stable-ids-no-child-free-text": fail("Privacy model drifted")

    eclipse=next((e for e in epic_catalog.get("epics",[]) if e.get("epicId")=="epic.eclipse-engine"),None)
    if not eclipse: fail("Eclipse epic missing")
    if set(eclipse.get("disciplines",[])) != EXPECTED_DISCIPLINES: fail("Eclipse discipline set drifted")
    epic_chapters=eclipse.get("chapters",[]); exp_chapters=experience.get("chapters",[])
    if len(epic_chapters)!=6 or len(exp_chapters)!=6: fail("Eclipse must contain six acts")
    if [c.get("chapterId") for c in epic_chapters] != [c.get("chapterId") for c in exp_chapters]: fail("Chapter order drifted")

    evidence_disciplines=set(); evidence_action_ids=[]
    for epic_chapter, player_chapter in zip(epic_chapters,exp_chapters,strict=True):
        actions=player_chapter.get("actions",[])
        if len(actions)<2 or "worldState" not in actions[-1]: fail(f"Thin/non-causal act: {player_chapter.get('chapterId')}")
        epic_evidence={route_key(x) for x in epic_chapter.get("evidenceRequirements",[])}
        player_evidence={route_key(a["evidence"]) for a in actions if "evidence" in a}
        if epic_evidence != player_evidence: fail(f"Hidden evidence parity failed for {player_chapter.get('chapterId')}")
        evidence_disciplines.update(r[1] for r in player_evidence)
        evidence_action_ids.extend(a["actionId"] for a in actions if "evidence" in a)
        epic_states={state_key(x) for x in epic_chapter.get("worldStateRequirements",[])}
        player_states={state_key(a["worldState"]) for a in actions if "worldState" in a}
        if epic_states != player_states or len(player_states)!=1: fail(f"World-state parity failed for {player_chapter.get('chapterId')}")
        for action in actions:
            if bool(action.get("evidence")) == bool(action.get("worldState")): fail(f"Invalid action route {action.get('actionId')}")
            if not 1 <= len(action.get("hints",[])) <= 3: fail(f"Invalid hint count {action.get('actionId')}")
    if evidence_disciplines != EXPECTED_DISCIPLINES: fail("Every declared discipline must be mechanically necessary")

    surface="\n".join(player_facing_strings(experience)).lower()
    for term in FORBIDDEN_SCHOOL_TERMS:
        if term in surface: fail(f"School language leaked: {term}")
    for subject in EXPECTED_DISCIPLINES:
        if subject in surface: fail(f"Subject label leaked: {subject}")
    serialized=json.dumps(experience,sort_keys=True).lower()
    for token in ("lootbox","daily-streak","fomo","battle-pass","chance-reward","artificial-scarcity"):
        if token in serialized: fail(f"Manipulative progression forbidden: {token}")

    subsystem_h=read("game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.h")
    subsystem_cpp=read("game/Source/WorldMakers/Adventure/WMEclipseEngineExperienceSubsystem.cpp")
    interactable_h=read("game/Source/WorldMakers/Adventure/WMEclipseEngineInteractableActor.h")
    interactable_cpp=read("game/Source/WorldMakers/Adventure/WMEclipseEngineInteractableActor.cpp")
    optics_h=read("game/Source/WorldMakers/Adventure/WMEclipseOpticsRuntime.h")
    optics_cpp=read("game/Source/WorldMakers/Adventure/WMEclipseOpticsRuntime.cpp")
    systems_h=read("game/Source/WorldMakers/Adventure/WMEclipseSystemsRuntime.h")
    systems_cpp=read("game/Source/WorldMakers/Adventure/WMEclipseSystemsRuntime.cpp")
    interaction_cpp=read("game/Source/WorldMakers/Environment/WMInteractionComponent.cpp")
    thought_cpp=read("game/Source/WorldMakers/Thought/WMLanguageThoughtSubsystem.cpp")
    player_h=read("game/Source/WorldMakers/Player/WMPlayerCharacter.h")
    game_ini=read("game/Config/DefaultGame.ini")
    tests=read("game/Source/WorldMakers/Private/Tests/WMEclipseEngineExperienceTests.cpp")

    for token in ("BeginAction","AdvancePrototypeMechanic","ResolveTrustedAction","ResolveValidatedEvidence","RequestHint","RefreshPrototypeTargetAvailability","OnWorldBeginPlay"):
        if token not in subsystem_h and token not in subsystem_cpp: fail(f"Experience subsystem missing {token}")
    if "Mission->GetMissionState() != EWMMissionRuntimeState::Completed" not in subsystem_cpp: fail("World consequence can bypass mission completion")
    if "bStartEclipseEngineVerticalSlice" not in subsystem_cpp or "bStartEclipseEngineVerticalSlice=True" not in game_ini: fail("Automatic prototype start path missing")
    if "AdvancePrototypeMechanic(ActionId, PrototypeInteractionStep)" not in interactable_cpp: fail("World interaction is not wired to executable mechanics")
    if "The click itself is never evidence" not in interactable_cpp: fail("Interaction/evidence boundary is undocumented")
    if "PrototypeInteractionStep" not in interactable_h or "ChapterId" not in interactable_h or "SetAvailable" not in interactable_h: fail("Prototype world-state actor contract incomplete")

    for action_id in evidence_action_ids:
        if action_id not in subsystem_cpp: fail(f"Evidence action has no executable prototype mechanic wiring: {action_id}")
    for token in ("IsSymmetrySolved","IsReflectionSolved","IsPathStable","ResolveTrustedAction"):
        if token not in optics_cpp: fail(f"Optics gameplay missing {token}")
    for token in ("InferArithmeticPattern","IsEquivalentRatio","IsOptimizedRoute","IsForcePredictionConsistent","IsCircuitModelConsistent","IsContextInferenceSupported","ResolveTrustedAction"):
        if token not in systems_cpp: fail(f"Systems gameplay missing {token}")

    if "UFUNCTION(BlueprintCallable" in systems_h or "UFUNCTION(BlueprintCallable" in optics_h: fail("Trusted evidence adapters must not be directly Blueprint-callable")
    if "bool SubmitContextInference(FName SelectedMeaningId);" not in systems_h: fail("Context inference must derive required meaning internally")
    if "RequiredMeaningId" in systems_h.split("bool SubmitContextInference",1)[1].split(";",1)[0]: fail("Caller still controls required context answer")
    for token in ("ContextAllowedClues","ObservedContextClues","ObserveContextClue"):
        if token not in systems_cpp and token not in systems_h: fail(f"Authoritative context clue gate missing {token}")

    if "TActorIterator<AWMEclipseEngineInteractableActor>" not in interaction_cpp: fail("Eclipse targets missing normal focus loop")
    if "FirstPersonInteractionComponent" not in player_h: fail("First-person bridge missing")
    if "ResolveValidatedEvidence" not in thought_cpp or "Never fall back directly to Mission Runtime while the epic is active" not in thought_cpp: fail("Thought evidence can bypass epic authority")

    for test_name in ("WorldMakers.Eclipse.PlayerExperience.WorldFirstContract","WorldMakers.Eclipse.PlayerExperience.HiddenEvidenceParity","WorldMakers.Eclipse.PlayerExperience.ProgressivePlayerRequestedHints","WorldMakers.Eclipse.Gameplay.OpticsRequiresSpatialReasoning","WorldMakers.Eclipse.Gameplay.SystemsRequireModeling"):
        if test_name not in tests: fail(f"Missing automation test {test_name}")
    if "Invented caller-controlled clue is rejected" not in tests: fail("Context spoof regression test missing")

    docs=read("docs/m5-6b-eclipse-engine-vertical-epic.md").lower()
    for idea in ("discover","manipulate","visible consequence","intrinsic","no school","reversible"):
        if idea not in docs: fail(f"Design doc missing {idea}")
    workflow=read(".github/workflows/repo-quality.yml"); unreal=read(".github/workflows/unreal-ci.yml")
    if "python scripts/validate-m5-6b-eclipse-player-experience.py" not in workflow: fail("Repository Quality missing M5.6B gate")
    if "python scripts/validate-m5-6b-eclipse-player-experience.py" not in unreal: fail("Unreal CI missing M5.6B gate")
    print("M5.6B passed: every diegetic evidence action is wired to executable C++ mechanics, context inference is authoritative, and the player surface remains game-first.")

if __name__ == "__main__": main()
