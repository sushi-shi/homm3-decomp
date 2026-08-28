//! Deterministic malformed-input coverage for every parser.

use homm3_resource::{
    iff::{Chunks, Xmidi},
    Bitmap, Font, Mask, Palette, Spreadsheet, Text,
};

#[test]
fn deterministic_byte_corpus_never_panics() {
    for seed in 0..32u32 {
        let mut random = seed ^ 0x5253_5243;
        for len in 0..=2048usize {
            let mut data = vec![0u8; len];
            for byte in &mut data {
                random = random.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                *byte = random.to_le_bytes()[3];
            }
            let _ = Bitmap::parse(&data);
            let _ = Palette::parse(&data);
            let _ = Font::parse(&data);
            let _ = Mask::parse(&data);
            for chunk in Chunks::new(&data) {
                let _ = chunk;
            }
            let _ = Xmidi::parse(&data);
            if let Ok(text) = Text::parse(&data) {
                for line in text.lines() {
                    let _ = line.decoded().count();
                }
            }
            if let Ok(sheet) = Spreadsheet::parse(&data) {
                for row in sheet.rows() {
                    for cell in row.cells() {
                        let _ = cell.decoded().count();
                    }
                }
            }
        }
    }
}
