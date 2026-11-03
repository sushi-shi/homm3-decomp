# Save-call result lifetimes with recovered packed writers

The recovered `writePackedBits` calls remain in `game::save` and
`NewSMapHeader::save`. Their canonical definition in `include/packed_bits.h`
is unchanged. This review uses the `043806c7b` source context; the earlier
`f6657771a` source scored 92.81% and 95.4451% respectively before the shared
writer recovery.

## Adopted source and evidence

`NewSMapHeader::save` now assigns the results of its nineteen checked stream
writes and three `saveString` calls to the existing `int count`, then tests
that result. Dreamcast records this local at `sp+0x18`, which is `r14+4`
after the outgoing argument area. For example, the first write at
`0xb01a2` stores both its temporary at `r14+8` and `count` at `r14+4`
(`0xb01a8`/`0xb01aa`); the next source row tests it. The same distinction
appears at lines 6808/6809 through 6966/6967 and the three string writes at
6824/6825, 6829/6830, and 6922/6923. The older alignment write has become a
Complete `writeValue<unsigned short>` operation, leaving nineteen direct
checked writes in this source. The unsigned `sizeof` comparisons remain
ordinary C++; no casts are added.

These assignments recover the nested Windows string inlining boundary:
**88.3414% → 99.1594%** before the additional length correction below.
Assignments for only the three string calls are byte-flat; they remain
because they are independently recorded source operations.

The Complete custom-hero name payload now obtains `length()` after writing
its length prefix. Retail reloads the string length from `[edi+0x1c]` after
the virtual write, rather than retaining the earlier length in a register.
The cached form achieves 99.1594%, but omits that observed reload. Keeping
the reload produces **97.8992%**, still above both the 88.3414% starting
implementation and the pre-helper 95.4451%. Direct argument evaluation,
reassigning the existing `count`, and giving the first length directly to
`writeValue<int>` all emit the same Windows object. The retained form keeps
the existing first length staging and uses the second `length()` directly
at the payload write.

Native Mac instead retains the first length in `r25` at `0xdbe08` through
the payload argument at `0xdbe38`. This is a real source-evidence distinction;
it does not justify inventing a platform conditional. Windows retail remains
the reconstruction target. Neither platform uniquely proves the original
C++ spelling. The local fixed-name string construction remains supported by
Dreamcast line 6920 and the Mac constructor call at `0xdbc10`.

`game::save` also drops the unnecessary explicit `char` cast when assigning
`m_gameVersion` to its existing narrow buffer. The resulting Windows score is
unchanged at **92.2229%**. Its other helper-result tests remain direct:
Dreamcast stores those results only to an unnamed temporary at `sp+28`,
whereas the explicitly assigned scalar-write results also store to the named
`count` at `sp+32`. Adding assignments to the helper tests would not follow
that evidence.

## Remaining Windows differences

The adopted `NewSMapHeader::save` has 82 paired CFG blocks, 81 exact and one
size-only; all 50 conditional branches agree. Its call comparison has 54
matching sites and one differing nested string call: candidate
`char_traits::assign` versus retail `_Eos` at candidate `+0x6ca`. Buffer,
register and temporary placement still differ. The register model reports
matching first-definition order and cannot attribute this remainder to the
simple first-definition allocation rule. No synthetic locals or declarations
were introduced to change it.

`game::save` remains an unresolved cleanup/inlining boundary. A live C2 trace
passes its byte-identity control: caller cost 1685, initial budget 3370,
207 recorded budget tests. The `writePackedBits` call expands at trace site
91, budget 1510, callee cost 122. The failure cleanup before the vector loops
expands `SavedGameHeader::~SavedGameHeader` at site 191 with budget 115 and
cost 67, leaving budget 48. All six later `saveVector` sites (cost 94) remain
calls; the later destructor sites (cost 67) remain calls as well. The
point/long `saveVector` instances have folded equivalent bodies, so the
aggregate named-call mismatch is not proof that either source helper was
removed or expanded. The native/candidate cleanup state transcripts have
76 versus 74 stores and different shared failure placement. Earlier
error-return bracing and index relocation probes are already documented
beside the function; repeating them supplies no new source evidence.

This is a measured unresolved boundary, not a proof that no further recovery
is possible. No fabricated mass, pragma pins, duplicate helper bodies, or
alternate declarations were used to cross it.

## Verification and coverage

The first source family exhausts eight combinations of result staging and
cast removal: eight successful states, four distinct objects, and four
reproduced elites. The length-reload family exhausts two states/two objects;
the follow-up length-lifetime family exhausts three states/one object.
All runs preserve the driver's unchanged-source and opposite-corner
reproduction controls. No observed source-owned MAX falls accompany the
chosen change relative to `043806c7b`.

A targeted VC6 `homm3 build --fast game -j2` verifies the adopted source.
Mac control and candidate full-TU builds retain all 78 available game pairs
without a score change or lost comparison; 80 emitted pairs remain
unavailable and four claims lack full-TU bodies. The two edited targets are
among the unavailable pairs: `game::save` needs the unreviewed `g_grailOwner`
data reference; `NewSMapHeader::save` has an unresolved `writeValue<char>`
relocation. Thus the available Mac preservation result is not an exact-byte
verdict for these targets. No linking manifests were added to hide this gap.

Disposable evidence is retained in the SSD worker's
`build/game-save-match/`. Source-family IDs are
`bece7193ae6dcd3bb9d2` (result staging), `4848d075e09c31cb88c9`
(length reload), and `2f237aec1a5c3517c513` (length lifetimes).
The identity-checked C2 trace is
`build/vc6/inline-trace/game/a14bbff0a8c5/trace.json`.
