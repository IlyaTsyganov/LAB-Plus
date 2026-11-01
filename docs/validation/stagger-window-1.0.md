# Stagger deadline validation on 1.0

The initial reader incorrectly required `PAIN_STAGGER_VULNERABLE` (17). The user's live demonstration showed a blinking, glory-kill-ready Imp in `PAIN_STAGGER` (16), with a five-second engine duration. The reader now accepts both 16 and 17 and rejects flinch, falter and stun.

The vanilla 1.0 stagger/recovery routine at RVA `0xe83980` explicitly accepts reaction 16 or 17. Its rejected-state diagnostic is `CheckForRetaliation - not PAIN_STAGGER`. A separate player helper containing a comparison with 17 was initially interpreted too narrowly; that comparison is only one branch, not a complete eligibility predicate.

The countdown reads `aiPainState_t.finishStaggerTime` and the audited scaled game clock (domain 1, 999960 ticks per second). It does not start a Windows stopwatch when damage is observed. See `stagger-deadline-layouts.json` for the verified 1.0 and 5.1 getter/field layouts.

## Recorded positive inputs

The bounded read-only watch `build/stagger-positive-watch-retry.tsv` recorded 937 positionable-AI samples. There were 916 reaction-16 samples, including 36 during gameplay, and 12 reaction-7 falter samples. The user confirmed that the Imp was blinking and available for a glory kill.

| Imp | Reaction | Game-clock ticks | Deadline ticks | Duration ticks | Recalculated remaining seconds |
| --- | --- | --- | --- | --- | --- |
| imp_1733 | 16 | 1160981410 | 1163496338 | 4999800 | 2.5150286011 |
| imp_1733 | 16 | 1161973904 | 1163496338 | 4999800 | 1.5224948998 |
| imp_2368 | 16 | 1165069948 | 1170013637 | 4999800 | 4.9438867555 |
| imp_2368 | 16 | 1166177609 | 1170013637 | 4999800 | 3.8361814473 |

These remaining values are derived from the recorded raw fields: `(deadline - gameClock) / 999960`. The old watch's `remainingSeconds` column was zero because of the incorrect 17-only predicate; it is not evidence of a zero-length window. Paused samples retained the same game clock and therefore the same remaining time.

The five captured clock/deadline pairs in `enemy_telemetry_tests.cpp` replay those real inputs through the corrected production reader. Additional cases cover reaction 17, expiration, a paused clock, an unavailable clock, changed getter bytes, dead/removed actors and rejection of reactions 1, 3, 7 and 13.

The corrected live watch was prepared as `build/StaggerWatchV2.exe`. No additional human demonstration was requested merely to repeat the already recorded action.
