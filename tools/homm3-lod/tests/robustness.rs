//! Deterministic malformed-input coverage for the allocation-free reader.

use homm3_lod::{Archive, ENTRY_SIZE, HEADER_SIZE};

#[test]
fn deterministic_byte_corpus_never_panics() {
    for seed in 0..64u32 {
        let mut random = seed ^ 0x4c4f_4433;
        for len in 0..=1024usize {
            let mut data = vec![0u8; len];
            for byte in &mut data {
                random = random.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                *byte = random.to_le_bytes()[1];
            }
            if seed % 2 == 0 && data.len() >= 4 {
                data[..3].copy_from_slice(b"LOD");
            }
            if seed % 4 == 0 && data.len() >= HEADER_SIZE {
                data[8..12].fill(0);
            }
            exercise(&data);
        }
    }
}

#[test]
fn every_single_byte_mutation_of_a_valid_archive_fails_closed() {
    let valid = valid_archive();
    assert!(Archive::parse(&valid).is_ok());
    for at in 0..valid.len() {
        let mut mutated = valid.clone();
        mutated[at] ^= 0x5a;
        exercise(&mutated);
    }
    for end in 0..valid.len() {
        exercise(&valid[..end]);
    }
}

fn exercise(data: &[u8]) {
    if let Ok(archive) = Archive::parse(data) {
        for entry in archive.entries() {
            let _ = archive.payload(entry);
        }
    }
}

fn valid_archive() -> Vec<u8> {
    let payload_at = HEADER_SIZE + ENTRY_SIZE;
    let mut data = vec![0u8; payload_at];
    data[..4].copy_from_slice(b"LOD\0");
    data[8..12].copy_from_slice(&1u32.to_le_bytes());
    data[HEADER_SIZE..HEADER_SIZE + 8].copy_from_slice(b"ONE.DEF\0");
    data[HEADER_SIZE + 16..HEADER_SIZE + 20]
        .copy_from_slice(&u32::try_from(payload_at).unwrap().to_le_bytes());
    data[HEADER_SIZE + 20..HEADER_SIZE + 24].copy_from_slice(&4u32.to_le_bytes());
    data.extend_from_slice(b"DATA");
    data
}
