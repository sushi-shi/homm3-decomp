//! Generated H3C header fixtures.

use homm3_map::campaign::{Campaign, StartOptions, Version};

fn push_string(bytes: &mut Vec<u8>, value: &[u8]) {
    bytes.extend_from_slice(&i32::try_from(value.len()).unwrap().to_le_bytes());
    bytes.extend_from_slice(value);
}

fn push_scenario(bytes: &mut Vec<u8>, name: &[u8], start: StartOptions) {
    push_string(bytes, name);
    bytes.extend_from_slice(&123u32.to_le_bytes());
    bytes.extend_from_slice(&[1, 2, 3]);
    push_string(bytes, b"Region");
    bytes.extend_from_slice(&[0, 0, 0x1f]);
    bytes.extend_from_slice(&[0xaa; 19]);
    bytes.extend_from_slice(&[0xbb; 18]);
    bytes.push(start as u8);
    if start == StartOptions::Bonus {
        bytes.extend_from_slice(&[4, 1, 2, 7]);
    }
}

fn fixture() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&(Version::ShadowOfDeath as u32).to_le_bytes());
    bytes.push(1); // retail region map 1 has three regions
    push_string(&mut bytes, b"Campaign");
    push_string(&mut bytes, b"Description");
    bytes.extend_from_slice(&[1, 34]);
    push_scenario(&mut bytes, b"ONE.H3M", StartOptions::None);
    push_scenario(&mut bytes, b"", StartOptions::Bonus);
    push_scenario(&mut bytes, b"THREE.H3M", StartOptions::None);
    bytes
}

fn legacy_fixture() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&(Version::LegacyRestoration as u32).to_le_bytes());
    bytes.push(4); // seven regions, four of them void
    push_string(&mut bytes, b"Campaign");
    push_string(&mut bytes, b"Description");
    for (name, size, prerequisites) in [
        (&b"ONE.H3M"[..], 100u32, 0u8),
        (&b"TWO.H3M"[..], 200, 1),
        (&b"THREE.H3M"[..], 300, 3),
    ] {
        push_string(&mut bytes, name);
        bytes.extend_from_slice(&size.to_le_bytes());
        bytes.push(prerequisites);
    }
    for _ in 0..4 {
        push_string(&mut bytes, b"");
        bytes.extend_from_slice(&0u32.to_le_bytes());
        bytes.push(0);
    }
    bytes.extend_from_slice(b"TAIL");
    bytes
}

#[test]
fn parses_scenario_records_and_typed_bonus_extents() {
    let bytes = fixture();
    let campaign = Campaign::parse(&bytes).unwrap();
    assert_eq!(campaign.scenario_count(), 3);
    assert_eq!(campaign.embedded_map_count(), 2);
    assert_eq!(campaign.scenario(0).unwrap().map_name.bytes(), b"ONE.H3M");
    assert_eq!(campaign.scenario(1).unwrap().bonus_count, 1);
    assert!(campaign.trailing().is_empty());
}

#[test]
fn rejects_every_truncated_campaign_prefix() {
    let bytes = fixture();
    for end in 0..bytes.len() {
        assert!(Campaign::parse(&bytes[..end]).is_err(), "accepted {end:#x}");
    }
}

#[test]
fn parses_original_restoration_compact_scenarios() {
    let bytes = legacy_fixture();
    let campaign = Campaign::parse(&bytes).unwrap();
    assert_eq!(campaign.version, Version::LegacyRestoration);
    assert_eq!(campaign.scenario_count(), 7);
    assert_eq!(campaign.embedded_map_count(), 3);
    assert_eq!(campaign.scenario(2).unwrap().prerequisites, 3);
    assert_eq!(campaign.scenario(2).unwrap().packed_map_size, 300);
    assert_eq!(campaign.scenario(3).unwrap().packed_map_size, 0);
    assert_eq!(campaign.trailing(), b"TAIL");
}

#[test]
fn rejects_every_truncated_original_restoration_header() {
    let bytes = legacy_fixture();
    let header_len = bytes.len() - b"TAIL".len();
    for end in 0..header_len {
        assert!(Campaign::parse(&bytes[..end]).is_err(), "accepted {end:#x}");
    }
}
