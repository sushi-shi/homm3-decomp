//! Deterministic hostile-input coverage for the save parser.

use homm3_save::SaveGame;

#[test]
fn deterministic_byte_corpus_never_panics() {
    for seed in 0..32_u32 {
        let mut random = seed ^ 0x4833_5356;
        for len in 0..=2_048_usize {
            let mut data = vec![0_u8; len];
            for byte in &mut data {
                random = random.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                *byte = random.to_le_bytes()[1];
            }
            let _ = SaveGame::parse(&data);
        }
    }
}
