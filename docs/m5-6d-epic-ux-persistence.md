# M5.6D — Epic UX, continuity and persistence

Status: **source implementation**. Native Unreal/device certification remains a separate M5.6E evidence boundary.

## Product intent

M5.6D turns the two source-complete vertical epics into journeys that can be left and continued without converting child play into a detailed behavioral record.

The child-facing promise is simple: **you may stop, come back, and continue from the chapter you reached**. There are no streaks, urgency timers, score-loss penalties, FOMO rewards, or pressure to finish a chapter in one sitting.

## Persistence boundary

Unreal persists a chapter checkpoint only:

- `epicId`;
- current `chapterId`;
- `chapterIndex` and `chapterCount`;
- completed/not-completed state.

It deliberately does **not** persist partial evidence, answers, free text, hint history, moral choices, personality/ideology labels, raw interaction telemetry, or session-only mastery gates.

When a journey resumes, its authoritative mission is reactivated and the current chapter begins again with empty evidence/world-state counters. This is intentionally conservative: a stale or manipulated partial result cannot be used to complete a chapter after restart.

## Unreal runtime

`UWMEpicRuntimeSubsystem` now owns a versioned `UWMEpicJourneySaveGame` checkpoint store. It can:

- start an epic fresh;
- resume a valid checkpoint;
- choose `ActivateOrResumeEpic`;
- reject checkpoints whose chapter identity/count no longer matches the current catalog;
- clear one epic checkpoint;
- save only after a fresh start or successful chapter transition.

`FWMEpicProgressModel::ResumeAtChapter` validates the index against the current epic definition and resets all partial evidence/world state before activating that chapter.

## Player Dashboard

The existing persistent player profile remains the single profile store. M5.6D adds normalized epic progress under `progress.epics` and `progress.currentEpicId`; the existing JSON-file, memory, and Supabase stores require no second persistence system.

The player read model exposes a calm `epicJourney` projection with:

- journey title;
- current chapter label and number;
- chapter count;
- checkpoint-derived progress percentage;
- the copy `Puedes continuar desde este capítulo cuando quieras.`

No score, streak, grading, wrong-answer, or scarcity vocabulary is introduced.

## Signed native handoff

`worldmakers-launch-v1` includes `epicResume` in the canonical HMAC payload. It contains only stable checkpoint fields. Changing the epic or chapter after signing invalidates the launch context.

The native client is expected to validate the launch signature through the existing handoff boundary and then request `ActivateOrResumeEpic`/`ResumeEpic`; the web dashboard itself does not award evidence or mark chapters complete.

## Parent-facing projection

The Parent Portal reuses the existing `child_dashboard_stats.adventures` aggregate JSON surface rather than introducing a new child telemetry table. Epic-shaped adventure entries are projected to a closed `epics` read model containing only:

- epic identity and authoritative label;
- in-progress/complete state;
- chapters completed / chapter count;
- checkpoint-derived percentage;
- bounded aggregate objective-group counts.

Raw evidence IDs, producer IDs, answers, free text, moral choices and personality/ideology labels are stripped and forbidden by tests.

For **The Garden at the End of Winter**, mathematics and philosophy remain session-only mastery gates. They are mechanically necessary in play but are intentionally excluded from persistent objective summaries.

## Fail-closed behavior

- unknown epic IDs are rejected;
- chapter ID/index mismatches are rejected;
- completed epic progress cannot regress;
- chapter progress cannot move backwards;
- duplicate current epic state is rejected;
- invalid Unreal save entries are ignored rather than trusted;
- partial chapter evidence is never restored;
- tampered `epicResume` invalidates the signed launch context.

## Validation

M5.6D adds source tests for:

- chapter checkpoint restart semantics;
- invalid resume indexes;
- normalized/non-regressing profile persistence;
- calm child-facing journey language;
- signed handoff tamper resistance;
- aggregate parent projection and sensitive-field stripping.

Repository Quality and Unreal project-validation must run `scripts/validate-m5-6d-epic-ux-persistence.py`.

## Certification boundary

Source completeness does not certify native save/load behavior on a packaged build, authored chapter transitions, real device performance, production gameplay-to-dashboard telemetry transport, or pedagogical/cultural review. Those remain M5.6E/native evidence work and the external Unreal runner boundary tracked by Issue #9.
