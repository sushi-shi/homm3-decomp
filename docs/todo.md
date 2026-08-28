# TODO

## Finish save-game oracle validation

The resource oracles are **not complete** because the save-game family still
lacks a real-file corpus. `homm3-save` currently parses generated full streams
for every save revision accepted by retail Complete (16–18 and 25–42), including
the historical field-width branches, exact-EOF checks, truncation cases, and
hostile inputs. Those generated cases do not prove behavior on saves produced
by the shipped games.

The installed Steam `Games/` directory contained no saves, and the supplied
original-media archive contains installers and game resources rather than
player-created saves. Therefore the project must not claim complete
Rust/retail/candidate save ser/de parity or 100% resource-oracle closure yet.

Remaining work:

- [ ] Acquire a provenance-recorded corpus of genuine `.GM1`–`.GM8`, `.TGM`,
  and `.CGM` files produced by Complete 4.0 and, where available, historical
  RoE/AB/SoD revisions. Keep copyrighted and personal save bytes outside the
  repository.
- [ ] Run every file through the gzip envelope and allocation-free parser:

  ```sh
  cd tools
  cargo run -p homm3-oracle --offline -- --save /path/to/Games saves
  ```

- [ ] Investigate every parse failure against the pinned retail loader and the
  reconstructed C++ load/save paths; add a minimized generated regression for
  each newly discovered rule.
- [ ] Close the family only when the real corpus reaches exact EOF, its reported
  save/map versions and ordinary/campaign kinds are correct, the existing
  generated revision and malformed-input gates remain green, and the applicable
  Rust/retail/candidate differentials agree.
