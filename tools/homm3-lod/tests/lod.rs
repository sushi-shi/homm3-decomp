//! Generated LOD fixtures for stored and zlib-compressed directory members.

use homm3_lod::{Archive, Error, Payload, ENTRY_SIZE, HEADER_SIZE};

fn archive_image() -> Vec<u8> {
    let count = 2usize;
    let payload_at = HEADER_SIZE + count * ENTRY_SIZE;
    let mut data = vec![0u8; payload_at];
    data[..4].copy_from_slice(b"LOD\0");
    data[4..8].copy_from_slice(&500u32.to_le_bytes());
    data[8..12].copy_from_slice(&u32::try_from(count).unwrap().to_le_bytes());

    let first = HEADER_SIZE;
    data[first..first + 8].copy_from_slice(b"ONE.DEF\0");
    data[first + 16..first + 20].copy_from_slice(&u32::try_from(payload_at).unwrap().to_le_bytes());
    data[first + 20..first + 24].copy_from_slice(&4u32.to_le_bytes());

    let second = HEADER_SIZE + ENTRY_SIZE;
    data[second..second + 8].copy_from_slice(b"TWO.DEF\0");
    data[second + 16..second + 20]
        .copy_from_slice(&u32::try_from(payload_at + 4).unwrap().to_le_bytes());
    data[second + 20..second + 24].copy_from_slice(&9u32.to_le_bytes());
    data[second + 24..second + 28].copy_from_slice(&1u32.to_le_bytes());
    data[second + 28..second + 32].copy_from_slice(&3u32.to_le_bytes());

    data.extend_from_slice(b"DATA");
    data.extend_from_slice(b"zip");
    data
}

#[test]
fn parses_stored_and_compressed_members_without_allocating() {
    let data = archive_image();
    let archive = Archive::parse(&data).unwrap();
    assert_eq!(archive.version(), 500);
    assert_eq!(archive.len(), 2);
    assert_eq!(archive.entries().count(), 2);

    let one = archive.find("one.def").unwrap();
    assert_eq!(one.name_str(), Some("ONE.DEF"));
    assert!(one.has_extension(".def"));
    assert_eq!(archive.payload(one).unwrap(), Payload::Stored(b"DATA"));

    let two = archive.find("TWO.DEF").unwrap();
    assert!(two.is_compressed());
    assert_eq!(
        archive.payload(two).unwrap(),
        Payload::Compressed {
            stream: b"zip",
            unpacked_size: 9,
        }
    );
}

#[test]
fn rejects_bad_signature_and_payload_extent() {
    let mut data = archive_image();
    data[0] = b'X';
    assert!(matches!(Archive::parse(&data), Err(Error::BadSignature(_))));

    let mut data = archive_image();
    let offset = HEADER_SIZE + 16;
    data[offset..offset + 4].copy_from_slice(&u32::MAX.to_le_bytes());
    assert!(matches!(
        Archive::parse(&data),
        Err(Error::PayloadOutOfBounds { index: 0, .. })
    ));
}

#[test]
fn directory_must_fit() {
    let mut data = vec![0u8; HEADER_SIZE];
    data[..4].copy_from_slice(b"LOD\0");
    data[8..12].copy_from_slice(&2u32.to_le_bytes());
    assert!(matches!(
        Archive::parse(&data),
        Err(Error::ShortDirectory { .. })
    ));
}
