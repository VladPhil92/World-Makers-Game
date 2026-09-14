#!/usr/bin/env python3
"""M5.6C Garden at the End of Winter game-first vertical epic source gate."""
from __future__ import annotations
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
EXPERIENCE=ROOT/"content/epics/garden-end-winter-player-experience-v1.json"
PACKAGED=ROOT/"game/Content/WorldMakers/Epics/garden-end-winter-player-experience-v1.json"
EPICS=ROOT/"content/epics/cross-disciplinary-epics-v1.json"
THOUGHT=ROOT/"content/thought/language-literature-thought-v1.json"
PERSISTENT={"biology","chemistry","ecology","ethics"}
MASTERY={"mathematics","philosophy-for-children"}
FORBIDDEN={"lesson","quiz","learning objective","learning-objective","grade","score","streak","homework","correct answer","wrong answer","xp"}

def fail(message:str)->None: raise SystemExit(message)
def load(path:Path)->dict: return json.loads(path.read_text(encoding="utf-8"))
def read(path:str)->str: return (ROOT/path).read_text(encoding="utf-8")
def route_key(r:dict)->tuple[str,...]: return tuple(r.get(k,"") for k in ("objectiveId","disciplineId","producerKind","producerRefId","primitiveId","evidenceEventId"))
def state_key(r:dict)->tuple[str,...]: return tuple(r.get(k,"") for k in ("producerKind","producerRefId","worldStateId"))

def main()->None:
    required=[EXPERIENCE,PACKAGED,EPICS,THOUGHT,
        ROOT/"docs/m5-6c-garden-end-winter-vertical-epic.md",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenEndWinterExperience.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenEndWinterExperience.cpp",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.cpp",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenEndWinterInteractableActor.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenEndWinterInteractableActor.cpp",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenSystemsRuntime.h",
        ROOT/"game/Source/WorldMakers/Adventure/WMGardenSystemsRuntime.cpp",
        ROOT/"game/Source/WorldMakers/Private/Tests/WMGardenEndWinterExperienceTests.cpp"]
    missing=[str(p.relative_to(ROOT)) for p in required if not p.exists()]
    if missing: fail(f"Missing M5.6C files: {missing}")

    experience=load(EXPERIENCE)
    if experience!=load(PACKAGED): fail("Packaged Garden experience must match canonical source")
    expected_header={
        "schemaVersion":1,"experienceId":"experience.garden-end-winter-v1","epicId":"epic.garden-end-winter","prototypeOnly":True,
        "playerPromise":"bring-a-frozen-living-garden-back-into-motion","presentationRule":"world-first-no-school-ui",
        "rewardModel":"living-world-transformation-and-new-access","failureModel":"reversible-ecosystem-experimentation",
        "hintModel":"player-requested-progressive-environmental-hints","privacyModel":"stable-ids-no-child-free-text"}
    for key,value in expected_header.items():
        if experience.get(key)!=value: fail(f"Garden experience contract drift: {key}")

    epic=next((e for e in load(EPICS).get("epics",[]) if e.get("epicId")=="epic.garden-end-winter"),None)
    if not epic: fail("Garden epic contract missing")
    if set(epic.get("disciplines",[]))!=PERSISTENT|MASTERY: fail("Garden must retain exactly six intended knowledge streams")
    chapters=experience.get("chapters",[]); epic_chapters=epic.get("chapters",[])
    if len(chapters)!=4 or [c.get("chapterId") for c in chapters]!=[c.get("chapterId") for c in epic_chapters]: fail("Garden must preserve its four-act epic order")

    persistent=set(); mastery=set(); executable=[]
    player_surface=[]
    for ec,pc in zip(epic_chapters,chapters,strict=True):
        actions=pc.get("actions",[])
        if len(actions)<3 or "worldState" not in actions[-1]: fail(f"Garden act {pc.get('chapterId')} must end in a visible world consequence")
        epic_evidence={route_key(r) for r in ec.get("evidenceRequirements",[])}
        player_evidence={route_key(a["evidence"]) for a in actions if "evidence" in a}
        if epic_evidence!=player_evidence: fail(f"Garden diegetic evidence mismatch: {pc.get('chapterId')}")
        persistent.update(r[1] for r in player_evidence)
        epic_states={state_key(r) for r in ec.get("worldStateRequirements",[])}
        player_states={state_key(a["worldState"]) for a in actions if "worldState" in a}
        if epic_states!=player_states or len(player_states)!=1: fail(f"Garden world-state mismatch: {pc.get('chapterId')}")
        for action in actions:
            route_count=sum(key in action for key in ("evidence","worldState","masteryGate"))
            if route_count!=1: fail(f"Garden action must have exactly one authority route: {action.get('actionId')}")
            if "masteryGate" in action:
                mastery.add(action["masteryGate"].get("disciplineId")); executable.append(action["actionId"])
            if "evidence" in action: executable.append(action["actionId"])
            if not 1<=len(action.get("hints",[]))<=3: fail(f"Garden hints must remain bounded: {action.get('actionId')}")
            player_surface.extend(str(action.get(k,"")) for k in ("actionId","interactionVerb","promptKey","feedbackKey")); player_surface.extend(action.get("hints",[]))
        player_surface.extend(str(pc.get(k,"")) for k in ("fantasyGoalKey","tensionKey","completionReactionKey"))
    if persistent!=PERSISTENT: fail(f"Garden persistent evidence streams drifted: {sorted(persistent)}")
    if mastery!=MASTERY: fail(f"Garden minimized mastery gates must be math + philosophy: {sorted(mastery)}")

    surface="\n".join(player_surface).lower()
    for term in FORBIDDEN:
        if term in surface: fail(f"School-style surface leaked into Garden: {term}")
    for subject in PERSISTENT|MASTERY:
        if subject in surface: fail(f"Subject label leaked into Garden player surface: {subject}")
    serialized=json.dumps(experience,sort_keys=True).lower()
    for token in ("lootbox","battle-pass","daily-streak","fomo","chance-reward","childauthoredtext","voicetranscript","personalityscore","ideologylabel"):
        if token in serialized: fail(f"Garden manipulation/privacy boundary violated: {token}")

    exp_h=read("game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.h")
    exp_cpp=read("game/Source/WorldMakers/Adventure/WMGardenEndWinterExperienceSubsystem.cpp")
    systems_h=read("game/Source/WorldMakers/Adventure/WMGardenSystemsRuntime.h")
    systems_cpp=read("game/Source/WorldMakers/Adventure/WMGardenSystemsRuntime.cpp")
    actor_cpp=read("game/Source/WorldMakers/Adventure/WMGardenEndWinterInteractableActor.cpp")
    focus_cpp=read("game/Source/WorldMakers/Environment/WMInteractionComponent.cpp")
    thought_cpp=read("game/Source/WorldMakers/Thought/WMLanguageThoughtSubsystem.cpp")
    for token in ("AdvancePrototypeMechanic","ResolveTrustedAction","ResolveValidatedEvidence","ResolveMasteryGate","AreCurrentMasteryGatesSatisfied"):
        if token not in exp_h and token not in exp_cpp: fail(f"Garden authority layer missing {token}")
    for action_id in executable:
        if action_id not in exp_cpp: fail(f"Garden world interaction has no executable mechanic: {action_id}")
    for token in ("FWMCellSystemState","DissolveInWater","ExecuteReaction","FWMPlantState","IsIrrigationRatioBalanced"):
        if token not in systems_cpp: fail(f"Garden does not reuse causal simulation primitive: {token}")
    if "UFUNCTION(BlueprintCallable" in systems_h: fail("Trusted Garden science/mastery adapters must not be Blueprint completion shortcuts")
    if "AdvancePrototypeMechanic(ActionId,PrototypeInteractionStep)" not in actor_cpp.replace(" ",""): fail("Garden artefacts are not wired from world interaction into mechanics")
    if "TActorIterator<AWMGardenEndWinterInteractableActor>" not in focus_cpp: fail("Garden must participate in normal IWMInteractable focus loop")
    if "epic.garden-end-winter" not in thought_cpp or "ResolveValidatedEvidence" not in thought_cpp: fail("Garden ethics must route through epic authority")

    thought=load(THOUGHT)
    problems={p.get("problemId") for p in thought.get("philosophyProblems",[])}
    if "philosophy.garden-restoration-causality" not in problems: fail("Garden needs its own causal-revision philosophy problem")

    tests=read("game/Source/WorldMakers/Private/Tests/WMGardenEndWinterExperienceTests.cpp")
    for name in ("WorldMakers.Garden.PlayerExperience.WorldFirstContract","WorldMakers.Garden.PlayerExperience.MathAndPhilosophyAreNecessaryNotProfiled","WorldMakers.Garden.Gameplay.LivingSystemsRequireCausalModeling"):
        if name not in tests: fail(f"Missing Garden automation test: {name}")

    docs=read("docs/m5-6c-garden-end-winter-vertical-epic.md").lower()
    for idea in ("living system","intrinsic","reversible","mastery gate","no school","visible consequence"):
        if idea not in docs: fail(f"Garden design documentation missing principle: {idea}")
    repo=read(".github/workflows/repo-quality.yml"); unreal=read(".github/workflows/unreal-ci.yml")
    if "python scripts/validate-m5-6c-garden-end-winter.py" not in repo: fail("Repository Quality must execute M5.6C gate")
    if "python scripts/validate-m5-6c-garden-end-winter.py" not in unreal: fail("Unreal CI must execute M5.6C gate")
    print("M5.6C passed: Garden is a four-act living-system adventure; biology/chemistry/ecology/ethics evidence plus math/philosophy mastery are causally necessary without school UI or expanded child profiling.")

if __name__=="__main__": main()
