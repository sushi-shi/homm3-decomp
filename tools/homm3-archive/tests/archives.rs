//! Generated SND and VID fixtures covering their different size models.

use homm3_archive::{
    Error, SoundArchive, VideoArchive, HEADER_SIZE, SND_ENTRY_SIZE, VID_ENTRY_SIZE,
};

fn snd_image() -> Vec<u8> {
    let payload_at = HEADER_SIZE + 2 * SND_ENTRY_SIZE;
    let mut data = vec![0u8; payload_at];
    data[..4].copy_from_slice(&2u32.to_le_bytes());
    data[4..12].copy_from_slice(b"ONE.WAV\0");
    data[44..48].copy_from_slice(&u32::try_from(payload_at).unwrap().to_le_bytes());
    data[48..52].copy_from_slice(&4u32.to_le_bytes());
    let second = HEADER_SIZE + SND_ENTRY_SIZE;
    data[second..second + 8].copy_from_slice(b"TWO.WAV\0");
    data[second + 40..second + 44]
        .copy_from_slice(&u32::try_from(payload_at + 4).unwrap().to_le_bytes());
    data[second + 44..second + 48].copy_from_slice(&3u32.to_le_bytes());
    data.extend_from_slice(b"RIFFxyz");
    data
}

fn vid_image() -> Vec<u8> {
    let payload_at = HEADER_SIZE + 2 * VID_ENTRY_SIZE;
    let mut data = vec![0u8; payload_at];
    data[..4].copy_from_slice(&2u32.to_le_bytes());
    data[4..12].copy_from_slice(b"ONE.SMK\0");
    data[44..48].copy_from_slice(&u32::try_from(payload_at).unwrap().to_le_bytes());
    let second = HEADER_SIZE + VID_ENTRY_SIZE;
    data[second..second + 8].copy_from_slice(b"TWO.BIK\0");
    data[second + 40..second + 44]
        .copy_from_slice(&u32::try_from(payload_at + 4).unwrap().to_le_bytes());
    data.extend_from_slice(b"SMK2BIK");
    data
}

#[test]
fn sound_records_have_explicit_sizes() {
    let data = snd_image();
    let archive = SoundArchive::parse(&data).unwrap();
    assert_eq!(archive.len(), 2);
    let first = archive.find("one.wav").unwrap();
    assert_eq!(first.name_str(), Some("ONE.WAV"));
    assert!(first.has_extension(".wav"));
    assert_eq!(archive.payload(first).unwrap(), b"RIFF");
    let second = archive.find("TWO.WAV").unwrap();
    assert_eq!(archive.payload(second).unwrap(), b"xyz");
}

#[test]
fn video_sizes_are_derived_from_offsets_and_eof() {
    let data = vid_image();
    let archive = VideoArchive::parse(&data).unwrap();
    assert_eq!(archive.len(), 2);
    let first = archive.find("one.smk").unwrap();
    assert_eq!(first.size, 4);
    assert_eq!(archive.payload(first).unwrap(), b"SMK2");
    let second = archive.find("TWO.BIK").unwrap();
    assert_eq!(second.size, 3);
    assert_eq!(archive.payload(second).unwrap(), b"BIK");
}

#[test]
fn both_formats_reject_bad_extents() {
    let mut sound = snd_image();
    sound[44..48].copy_from_slice(&0u32.to_le_bytes());
    assert!(matches!(
        SoundArchive::parse(&sound),
        Err(Error::PayloadBeforeDirectory { .. })
    ));

    let mut video = vid_image();
    let second_offset = HEADER_SIZE + VID_ENTRY_SIZE + 40;
    video[second_offset..second_offset + 4].copy_from_slice(&1u32.to_le_bytes());
    assert!(matches!(
        VideoArchive::parse(&video),
        Err(Error::OffsetsOutOfOrder { .. })
    ));
}
