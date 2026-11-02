# PR #192: final Mac-preservation review

Recommendation: retain the original `teleportTo(current_hero, path[destination_index].m_point, ...)` argument and reject #192's final `start_point` copy. Do not add a platform fork or a Mac abstraction waiver. The change adds no recovered helper or independently proven source fact. Its Windows gain alone does not justify the Mac regression.

Measured results for `attemptTeleport` (Windows VA `0x00430ab0`, Mac section offset `0x0340a4`):

| Source spelling | Windows | Mac |
| --- | ---: | ---: |
| Original direct path argument | 99.9755% existing baseline | 79.3785% freshly reproduced |
| #192 separate assignment to existing `start_point`, then argument | 100% coordinator's integrated verification | 78.4819% coordinator's full Mac gate |
| Assignment expression inside the argument | 100% fresh targeted VC6 build | 77.4373% fresh full-TU Mac build |

The last candidate was tested only in `/tmp/homm3-cleanup-flow-20261102`; it was restored to the original direct argument afterward. No integration source was edited, no score was banked, and no candidate was committed.

## Native evidence

Mac `0x34594..0x34598` stores the earlier selected intermediate point into `start_point` at `sp+0x130`. The final teleport sequence instead computes the destination index at `0x345bc` and directly loads the path point into argument register `r5` with `lwzx` at `0x345d4`, before calling `advManager::teleportTo` at `0x345e4`. There is no final store to the earlier point home.

Dreamcast line 4054 initializes `start_point` at `r14+4/+6` (`0x347a8`, `0x347b0`; `r14` is 32 bytes above the final stack pointer). Line 4145 reads `path[destination_index]` and stages the by-value argument at different offsets `r14+36/+38` (`0x34a38`, `0x34a44`) before `TeleportTo`. The line table has no separate row 4144. An absent row is not proof of absent source, and an optimizer can remove a dead local assignment; these observations support the direct expression without uniquely proving its original spelling.

The argument-assignment candidate retains an extra copy path in CodeWarrior: two halfword loads/stores into its `start_point` home, followed by a word reload for the argument (candidate offsets `+0x544`, `+0x554`, `+0x558`, `+0x55c`, `+0x564`). This differs structurally from the native direct load, not merely a shifted byte score. It adds no helper and supplies no evidence of platform-specific original source. The relocation-masked shape report has 271 aligned identical instructions, 359 candidate instructions versus 349 native, with the existing unresolved differences still present; this is diagnostic shape evidence, not an exact verdict.

## Reproduction artifacts

Under the isolated tree's `build/cleanup-flow/`:

- `pr192-mac-native.txt`: native Mac disassembly.
- `pr192-dc-asm.txt`, `pr192-dc-show.txt`, `pr192-dc-lines.txt`: previously captured native DC evidence.
- `pr192-direct-mac.log`: freshly reproduced unchanged direct-argument control.
- `pr192-argument-mac.log`, `pr192-argument-win.log`: candidate compiler results.
- `pr192-argument-diff.txt`, `pr192-argument-shape.txt`: candidate/native Mac comparison.
- `pr192-original-ai_player.cpp`: pre-experiment source copy used for restoration.

Coordinator integration failure: `build/cleanup-mac.log` reports this as its only Mac preservation regression. The earlier static review accepted the copy's runtime semantics but did not establish Mac preservation or prove the copy was original; this final compiled review supersedes that acceptance for integration.

Restoration check: `pr192-restored-mac.log` reproduces 79.3785% again, and `cmp` confirms the source matches the pre-experiment copy byte for byte. The already-started restoration check completed without further variants.
