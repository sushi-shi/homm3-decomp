//! Deterministic malformed-input coverage for both archive readers.

use homm3_archive::{gzip::Member as GzipMember, SoundArchive, VideoArchive};

#[test]
fn deterministic_byte_corpus_never_panics() {
    for seed in 0..32u32 {
        let mut random = seed ^ 0x534e_4456;
        for len in 0..=1024usize {
            let mut data = vec![0u8; len];
            for byte in &mut data {
                random = random.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                *byte = random.to_le_bytes()[2];
            }
            if seed % 4 == 0 && data.len() >= 4 {
                data[..4].fill(0);
            }
            exercise(&data);
        }
    }
}

fn exercise(data: &[u8]) {
    let _ = GzipMember::parse(data);
    if let Ok(archive) = SoundArchive::parse(data) {
        for entry in archive.entries() {
            let _ = archive.payload(entry);
        }
    }
    if let Ok(archive) = VideoArchive::parse(data) {
        for entry in archive.entries() {
            let _ = archive.payload(entry);
        }
    }
}
