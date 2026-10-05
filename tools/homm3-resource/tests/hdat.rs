//! Container framing and installed-data checks, without bundling game assets.

use homm3_resource::hdat::{Container, ErrorKind};

fn word(bytes: &mut Vec<u8>, value: i32) {
    bytes.extend(value.to_le_bytes());
}
fn string(bytes: &mut Vec<u8>, value: &[u8]) {
    word(bytes, i32::try_from(value.len()).unwrap());
    bytes.extend(value);
}
fn fixture() -> Vec<u8> {
    let mut b = b"HDAT".to_vec();
    word(&mut b, 2);
    word(&mut b, 2);
    string(&mut b, b"terrain10");
    string(&mut b, b"source.str");
    word(&mut b, 2);
    string(&mut b, b"");
    string(&mut b, &[0xff, 0, 0xfe]);
    b.push(3); // Presence uses truthiness, not an enum restricted to 0/1.
    string(&mut b, &[1, 2, 3]);
    word(&mut b, 2);
    word(&mut b, -1);
    word(&mut b, i32::MIN);
    string(&mut b, b"empty");
    string(&mut b, b"");
    word(&mut b, 0);
    b.push(0);
    word(&mut b, 0);
    b
}

#[test]
fn records_borrow_binary_and_localized_data_in_source_order() {
    let bytes = fixture();
    let container = Container::parse(&bytes).unwrap();
    assert_eq!(container.len(), 2);
    assert!(!container.is_empty());
    let mut records = container.entries();
    let first = records.next().unwrap();
    assert_eq!(first.offset(), 12);
    assert_eq!(first.name(), b"terrain10");
    assert_eq!(first.source_path(), b"source.str");
    assert_eq!(
        first.strings().collect::<Vec<_>>(),
        [b"".as_slice(), &[0xff, 0, 0xfe]]
    );
    assert_eq!(first.payload_flag(), 3);
    assert_eq!(first.payload(), Some([1, 2, 3].as_slice()));
    assert_eq!(first.integers().collect::<Vec<_>>(), [-1, i32::MIN]);
    let second = records.next().unwrap();
    assert_eq!(second.name(), b"empty");
    assert!(second.payload().is_none());
    assert_eq!(second.strings().len(), 0);
    assert_eq!(second.integers().len(), 0);
    assert!(records.next().is_none());
    assert!(records.next().is_none());
    assert_eq!(container.entries().len(), 2);
}

#[test]
fn truncated_lengths_counts_and_headers_never_admit_partial_records() {
    let bytes = fixture();
    for length in 0..bytes.len() {
        assert!(
            Container::parse(&bytes[..length]).is_err(),
            "prefix {length}"
        );
    }
    for offset in [8, 12, 25, 39, 43, 47, 55, 62] {
        let mut negative = bytes.clone();
        negative[offset..offset + 4].copy_from_slice(&(-1i32).to_le_bytes());
        let error = Container::parse(&negative).err().unwrap();
        assert_eq!(error.offset, offset);
        assert_eq!(error.kind, ErrorKind::NegativeLength(-1));
        negative[offset..offset + 4].copy_from_slice(&i32::MAX.to_le_bytes());
        assert!(Container::parse(&negative).is_err());
    }
    let mut trailing = bytes.clone();
    trailing.push(0);
    assert_eq!(
        Container::parse(&trailing).err().unwrap().kind,
        ErrorKind::TrailingBytes
    );
    let mut version = bytes.clone();
    version[4] = 1;
    assert_eq!(
        Container::parse(&version).err().unwrap().kind,
        ErrorKind::Version(1)
    );
    let mut magic = bytes;
    magic[0] = 0;
    assert_eq!(
        Container::parse(&magic).err().unwrap().kind,
        ErrorKind::Signature
    );
}

#[test]
fn present_empty_payload_and_empty_container_are_distinct() {
    let mut bytes = b"HDAT\x02\0\0\0\0\0\0\0".to_vec();
    assert!(Container::parse(&bytes).unwrap().is_empty());
    bytes[8] = 1;
    string(&mut bytes, b"");
    string(&mut bytes, b"");
    word(&mut bytes, 0);
    bytes.push(1);
    word(&mut bytes, 0);
    word(&mut bytes, 0);
    assert_eq!(
        Container::parse(&bytes)
            .unwrap()
            .entries()
            .next()
            .unwrap()
            .payload(),
        Some(b"".as_slice())
    );
}

#[test]
#[ignore = "requires HOMM3_HOTA_DAT pointing to the pinned HotA 1.8.1 data file"]
fn installed_container_preserves_all_records_and_rmg_streams() {
    let bytes = std::fs::read(std::env::var_os("HOMM3_HOTA_DAT").unwrap()).unwrap();
    let data = Container::parse(&bytes).unwrap();
    assert_eq!(data.len(), 433);
    assert_eq!(
        data.entries().filter(|e| e.payload().is_some()).count(),
        151
    );
    for (name, length) in [(b"rmgobjects0", 1683), (b"rmgobjects1", 536)] {
        let record = data.entries().find(|r| r.name() == name).unwrap();
        assert_eq!(record.strings().len(), 9);
        assert_eq!(record.strings().nth(7).unwrap().len(), length);
        assert_eq!(record.integers().collect::<Vec<_>>(), [0, 0, 0, 0]);
    }
    let terrain = data.entries().find(|r| r.name() == b"terrain10").unwrap();
    assert_eq!(terrain.integers().collect::<Vec<_>>(), [0, 50, 0, 0]);
    assert!(terrain.payload().is_none());
}
