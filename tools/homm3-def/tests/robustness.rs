//! Deterministic malformed-input and renderer-canary coverage.

use homm3_def::{Blit, Canvas, Encoding, Sprite, DEF_HEADER_SIZE};

#[test]
fn deterministic_byte_corpus_never_panics_or_overwrites_canaries() {
    let palette = [0x6b5au16; 256];
    for seed in 0..64u32 {
        let mut random = seed ^ 0x4445_4633;
        for len in 0..=1024usize {
            let mut data = vec![0u8; len];
            for byte in &mut data {
                random = random.wrapping_mul(1_103_515_245).wrapping_add(12_345);
                *byte = random.to_le_bytes()[2];
            }
            if seed % 4 == 0 && data.len() >= DEF_HEADER_SIZE {
                data[12..16].fill(0);
            }
            exercise_def(&data, &palette);
        }
    }
}

#[test]
fn every_single_byte_mutation_of_a_valid_frame_fails_closed() {
    let palette = [0x1234u16; 256];
    let valid = valid_def();
    exercise_def(&valid, &palette);
    for at in 0..valid.len() {
        let mut mutated = valid.clone();
        mutated[at] ^= 0xa5;
        exercise_def(&mutated, &palette);
    }
    for end in 0..valid.len() {
        exercise_def(&valid[..end], &palette);
    }
}

fn exercise_def(data: &[u8], palette: &[u16; 256]) {
    let Ok(sprite) = Sprite::parse(data) else {
        return;
    };
    for group in sprite.groups() {
        for index in 0..group.len() {
            let Ok(frame) = group.frame(index) else {
                continue;
            };
            let _ = frame.validate();
            let mut guarded = [0xfeedu16; 102];
            {
                let mut canvas = Canvas::new(10, 10, &mut guarded[1..101]).unwrap();
                let _ = Blit::new().draw_full(&mut canvas, frame, palette, -3, -4);
                let _ = Blit::new()
                    .mirrored(true)
                    .transparent_fills(true)
                    .draw_full(&mut canvas, frame, palette, 7, 8);
            }
            assert_eq!(guarded[0], 0xfeed);
            assert_eq!(guarded[101], 0xfeed);
        }
    }
}

fn valid_def() -> Vec<u8> {
    const GROUP_BYTES: usize = 16 + 13 + 4;
    const FRAME_BYTES: usize = 32;
    let frame_at = DEF_HEADER_SIZE + GROUP_BYTES;
    let stream = [4, 0, 0, 0, 255, 2, 10, 11, 12];
    let mut image = vec![0u8; frame_at + FRAME_BYTES + stream.len()];
    image[0..4].copy_from_slice(&64u32.to_le_bytes());
    image[4..8].copy_from_slice(&3u32.to_le_bytes());
    image[8..12].copy_from_slice(&1u32.to_le_bytes());
    image[12..16].copy_from_slice(&1u32.to_le_bytes());
    image[DEF_HEADER_SIZE + 4..DEF_HEADER_SIZE + 8].copy_from_slice(&1u32.to_le_bytes());
    image[DEF_HEADER_SIZE + 29..DEF_HEADER_SIZE + 33]
        .copy_from_slice(&u32::try_from(frame_at).unwrap().to_le_bytes());
    image[frame_at..frame_at + 4]
        .copy_from_slice(&u32::try_from(stream.len()).unwrap().to_le_bytes());
    image[frame_at + 4..frame_at + 8].copy_from_slice(&(Encoding::GeneralRle as u32).to_le_bytes());
    image[frame_at + 8..frame_at + 12].copy_from_slice(&3u32.to_le_bytes());
    image[frame_at + 12..frame_at + 16].copy_from_slice(&1u32.to_le_bytes());
    image[frame_at + 16..frame_at + 20].copy_from_slice(&3u32.to_le_bytes());
    image[frame_at + 20..frame_at + 24].copy_from_slice(&1u32.to_le_bytes());
    image[frame_at + FRAME_BYTES..].copy_from_slice(&stream);
    image
}
