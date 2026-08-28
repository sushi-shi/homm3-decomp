//! Generated gzip-envelope fixtures.

use homm3_archive::gzip::{Error, Member};

fn fixture(flags: u8, optional: &[u8]) -> Vec<u8> {
    let mut data = vec![0x1f, 0x8b, 8, flags, 1, 2, 3, 4, 0, 11];
    data.extend_from_slice(optional);
    data.extend_from_slice(&[0x03, 0x00]);
    data.extend_from_slice(&0x1234_5678u32.to_le_bytes());
    data.extend_from_slice(&9u32.to_le_bytes());
    data
}

#[test]
fn parses_fixed_and_optional_gzip_fields() {
    let image = fixture(0, &[]);
    let member = Member::parse(&image).unwrap();
    assert_eq!(member.modified_time(), 0x0403_0201);
    assert_eq!(member.operating_system(), 11);
    assert_eq!(member.deflate(), &[0x03, 0]);
    assert_eq!(member.crc32(), 0x1234_5678);
    assert_eq!(member.uncompressed_size(), 9);

    let optional = [3, 0, 1, 2, 3, b'n', 0, b'c', 0, 0xaa, 0xbb];
    let image = fixture(0x1e, &optional);
    let member = Member::parse(&image).unwrap();
    assert_eq!(member.flags(), 0x1e);
    assert_eq!(member.deflate(), &[0x03, 0]);
}

#[test]
fn rejects_bad_gzip_extents_and_flags() {
    assert!(matches!(
        Member::parse(&[]),
        Err(Error::Short {
            needed: 18,
            available: 0
        })
    ));
    assert!(matches!(
        Member::parse(&fixture(0xe0, &[])),
        Err(Error::ReservedFlags(0xe0))
    ));
    let mut unterminated = vec![0x1f, 0x8b, 8, 0x08, 0, 0, 0, 0, 0, 0];
    unterminated.extend_from_slice(&[1; 8]);
    assert!(matches!(
        Member::parse(&unterminated),
        Err(Error::MissingTerminator { .. })
    ));
    let mut image = fixture(0, &[]);
    image[2] = 9;
    assert!(matches!(
        Member::parse(&image),
        Err(Error::UnsupportedCompression(9))
    ));
}
