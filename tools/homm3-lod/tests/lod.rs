//! Generated LOD fixtures for stored and zlib-compressed directory members.

use homm3_lod::{
    resource_name_hash, Archive, Directory, Error, HashedDirectory, HashedEntry, Header, Payload,
    ENTRY_SIZE, HEADER_SIZE,
};

#[test]
fn directory_view_does_not_require_resident_payloads() {
    let data = archive_image();
    let end = Header::parse(&data[..HEADER_SIZE]).unwrap().directory_end();
    let directory = Directory::parse(&data[..end], data.len() as u64).unwrap();
    let archive = Archive::parse(&data).unwrap();
    assert_eq!(directory.version(), archive.version());
    assert_eq!(directory.len(), archive.len());
    assert_eq!(
        directory.entries().collect::<Vec<_>>(),
        archive.entries().collect::<Vec<_>>()
    );
    assert!(matches!(
        Directory::parse(&data[..end], (data.len() - 1) as u64),
        Err(Error::PayloadOutOfBounds { index: 1, .. })
    ));
    assert!(matches!(
        Directory::parse(&data[..end], (end - 1) as u64),
        Err(Error::ShortDirectory { .. })
    ));
}

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

#[test]
fn resource_name_hash_folds_case_and_stops_at_nul() {
    // FNV-1a reference values for the lowercase names.
    assert_eq!(resource_name_hash(b""), 0x811c_9dc5);
    assert_eq!(resource_name_hash(b"a"), 0xe40c_292c);
    assert_eq!(
        resource_name_hash(b"OBJECTS.TXT"),
        resource_name_hash(b"objects.txt")
    );
    assert_eq!(
        resource_name_hash(b"objects.txt\0junk"),
        resource_name_hash(b"objects.txt")
    );
}

fn hashed_image(key: u32) -> Vec<u8> {
    let payload_at = HEADER_SIZE + 2 * ENTRY_SIZE;
    let mut data = vec![0u8; payload_at];
    data[..4].copy_from_slice(b"LOD\0");
    data[4..8].copy_from_slice(&200u32.to_le_bytes());
    data[8..12].copy_from_slice(&2u32.to_le_bytes());
    data[12..16].copy_from_slice(&key.to_le_bytes());
    let records = [
        (b"objects.txt".as_slice(), payload_at, 4, 0, 0xee),
        (b"rand_trn.txt".as_slice(), payload_at + 4, 9, 3, 3),
    ];
    for (index, (name, offset, size, compressed, codec)) in records.into_iter().enumerate() {
        let at = HEADER_SIZE + index * ENTRY_SIZE;
        let offset = u32::try_from(offset).unwrap();
        data[at..at + 4].copy_from_slice(&resource_name_hash(name).to_le_bytes());
        data[at + 4..at + 8].copy_from_slice(&(offset ^ key).to_le_bytes());
        data[at + 8..at + 12].copy_from_slice(&(size ^ key).to_le_bytes());
        data[at + 12..at + 16].copy_from_slice(&(compressed ^ key).to_le_bytes());
        data[at + 16] = codec;
    }
    data.extend_from_slice(b"TEXTzip");
    data
}

#[test]
fn hashed_directory_decodes_keyed_extents_and_rejects_the_named_reader() {
    let key = 0xb5a4_d744;
    let data = hashed_image(key);
    let header = Header::parse(&data).unwrap();
    assert_eq!(header.hashed_key(), Some(key));
    assert!(matches!(
        Directory::parse(&data, data.len() as u64),
        Err(Error::DirectoryEncoding)
    ));
    let directory = HashedDirectory::parse(&data, data.len() as u64).unwrap();
    assert_eq!(directory.len(), 2);
    let stored = directory.find(b"OBJECTS.TXT").unwrap();
    assert_eq!(
        stored,
        HashedEntry {
            name_hash: resource_name_hash(b"objects.txt"),
            offset: u32::try_from(HEADER_SIZE + 2 * ENTRY_SIZE).unwrap(),
            size: 4,
            compressed_size: 0,
            codec: 0xee,
        }
    );
    assert_eq!(stored.stored_size(), 4);
    let packed = directory.find(b"rand_trn.txt").unwrap();
    assert_eq!((packed.size, packed.stored_size(), packed.codec), (9, 3, 3));
    assert!(packed.matches(b"RAND_TRN.TXT"));
    assert!(directory.find(b"rmg.txt").is_none());
    assert!(matches!(
        HashedDirectory::parse(&data, data.len() as u64 - 1),
        Err(Error::PayloadOutOfBounds { index: 1, .. })
    ));
}

#[test]
fn named_format_markers_are_not_hashed_keys() {
    let mut data = archive_image();
    for marker in [0_u32, 0x007e_0213] {
        data[12..16].copy_from_slice(&marker.to_le_bytes());
        assert_eq!(Header::parse(&data).unwrap().hashed_key(), None);
        assert!(Archive::parse(&data).is_ok());
        assert!(matches!(
            HashedDirectory::parse(&data, data.len() as u64),
            Err(Error::DirectoryEncoding)
        ));
    }
}
