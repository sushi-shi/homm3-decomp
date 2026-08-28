//! Generated coverage for every variable branch in the scenario header.

use homm3_map::{
    MapBody, MapHeader, ObjectTable, Terrain, Version, WorldPrefix, COMPLETE_HERO_BYTES,
    HEADER_PADDING_SIZE, MAP_TRAILING_PADDING_SIZE, PLAYER_COUNT,
};

fn push_string(bytes: &mut Vec<u8>, value: &[u8]) {
    bytes.extend_from_slice(&i32::try_from(value.len()).unwrap().to_le_bytes());
    bytes.extend_from_slice(value);
}

fn sod_header() -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&(Version::ShadowOfDeath as i32).to_le_bytes());
    bytes.push(1);
    bytes.extend_from_slice(&36i32.to_le_bytes());
    bytes.push(1);
    push_string(&mut bytes, b"Map");
    push_string(&mut bytes, b"Description");
    bytes.extend_from_slice(&[2, 24]);
    for index in 0..PLAYER_COUNT {
        bytes.extend_from_slice(&[u8::from(index == 0), 1, 2, 0]);
        bytes.extend_from_slice(&0x01ffu16.to_le_bytes());
        bytes.extend_from_slice(&[1, 0, 1, 0xff, 0]);
        bytes.extend_from_slice(&0i32.to_le_bytes());
    }
    bytes.extend_from_slice(&[0xff, 0xff, 0]);
    bytes.extend_from_slice(&[0xaa; COMPLETE_HERO_BYTES]);
    bytes.extend_from_slice(&0i32.to_le_bytes());
    bytes.push(0);
    bytes.extend_from_slice(&[0; HEADER_PADDING_SIZE]);
    bytes.extend_from_slice(b"WORLD");
    bytes
}

#[test]
fn parses_complete_map_header_and_preserves_world() {
    let bytes = sod_header();
    let header = MapHeader::parse(&bytes).unwrap();
    assert_eq!(header.version, Version::ShadowOfDeath);
    assert_eq!(header.name.bytes(), b"Map");
    assert_eq!(header.description.bytes(), b"Description");
    assert_eq!(header.size, 36);
    assert_eq!(header.players[0].legal_alignments, 0x01ff);
    assert_eq!(header.players[0].retained_hero_count, 0);
    assert_eq!(header.available_heroes, &[0xaa; COMPLETE_HERO_BYTES]);
    assert_eq!(header.world(), b"WORLD");
}

#[test]
fn rejects_truncation_at_every_header_boundary() {
    let bytes = sod_header();
    let header_len = MapHeader::parse(&bytes).unwrap().header_len();
    for end in 0..header_len {
        assert!(
            MapHeader::parse(&bytes[..end]).is_err(),
            "accepted {end:#x}"
        );
    }
}

#[test]
fn parses_shadow_world_prefix_and_custom_hero_records() {
    let mut world = vec![0xaa; 18];
    world.extend_from_slice(&[0xbb; 9]);
    world.extend_from_slice(&[0xcc; 4]);
    world.extend_from_slice(&1i32.to_le_bytes());
    push_string(&mut world, b"unused");
    push_string(&mut world, b"rumour");
    world.push(1);
    world.extend_from_slice(&[1, 1, 0, 0, 0]);
    world.extend_from_slice(&[1, 1, 0, 0, 0]);
    world.extend_from_slice(&[7, 2]);
    world.push(0);
    world.push(1);
    push_string(&mut world, b"Hero");
    world.extend_from_slice(&[0xff, 1]);
    world.extend_from_slice(&[0xdd; 9]);
    world.push(1);
    world.extend_from_slice(&[1, 2, 3, 4]);
    world.extend_from_slice(&[0; 155]);
    world.extend_from_slice(b"TERRAIN");

    let prefix = WorldPrefix::parse(&world, Version::ShadowOfDeath).unwrap();
    assert_eq!(prefix.rumour_count, 1);
    assert_eq!(prefix.custom_hero_setups, 1);
    assert_eq!(prefix.map(), b"TERRAIN");
}

#[test]
fn parses_seven_byte_terrain_layers() {
    let mut bytes = Vec::new();
    for index in 0..8u8 {
        bytes.extend_from_slice(&[index, 1, 2, 3, 4, 5, 0x40]);
    }
    bytes.extend_from_slice(b"OBJECTS");
    let terrain = Terrain::parse(&bytes, 2, true).unwrap();
    assert_eq!(terrain.len(), 8);
    assert_eq!(terrain.layers(), 2);
    assert_eq!(terrain.cell(7).unwrap().ground_set, 7);
    assert_eq!(terrain.cell(7).unwrap().flags, 0x40);
    assert_eq!(terrain.objects(), b"OBJECTS");
}

#[test]
fn parses_object_templates_and_placed_count() {
    let mut bytes = 1i32.to_le_bytes().to_vec();
    push_string(&mut bytes, b"AVWATTA0.DEF");
    bytes.extend_from_slice(&[1; 6]);
    bytes.extend_from_slice(&[2; 6]);
    bytes.extend_from_slice(&[0; 4]);
    bytes.extend_from_slice(&17u32.to_le_bytes());
    bytes.extend_from_slice(&3u32.to_le_bytes());
    bytes.extend_from_slice(&[4, 1]);
    bytes.extend_from_slice(&[0; 16]);
    bytes.extend_from_slice(&2i32.to_le_bytes());
    bytes.extend_from_slice(b"PLACED");

    let table = ObjectTable::parse(&bytes).unwrap();
    assert_eq!(table.type_count(), 1);
    assert_eq!(table.object_count(), 2);
    let object_type = table.types().next().unwrap();
    assert_eq!(object_type.image_name.bytes(), b"AVWATTA0.DEF");
    assert_eq!(object_type.object_class, 17);
    assert!(object_type.suppress_draw);
    assert_eq!(table.objects(), b"PLACED");
}

fn push_object_type(bytes: &mut Vec<u8>, object_class: u32, extra: u32) {
    push_string(bytes, b"OBJECT.DEF");
    bytes.extend_from_slice(&[0; 6 + 6 + 4]);
    bytes.extend_from_slice(&object_class.to_le_bytes());
    bytes.extend_from_slice(&extra.to_le_bytes());
    bytes.extend_from_slice(&[0; 2 + 16]);
}

fn push_object_prefix(bytes: &mut Vec<u8>, type_index: u32) {
    bytes.extend_from_slice(&[1, 2, 0]);
    bytes.extend_from_slice(&type_index.to_le_bytes());
    bytes.extend_from_slice(&[0; 5]);
}

#[test]
fn parses_complete_object_and_event_tail_with_retail_remaps() {
    let mut bytes = 3i32.to_le_bytes().to_vec();
    push_object_type(&mut bytes, 219, 0); // retail remaps Garrison II to 33
    push_object_type(&mut bytes, 220, 7); // retail remaps Abandoned Mine to 53
    push_object_type(&mut bytes, 83, 0);
    bytes.extend_from_slice(&3i32.to_le_bytes());

    push_object_prefix(&mut bytes, 0);
    bytes.extend_from_slice(&[0; 4]);
    bytes.extend_from_slice(&[0; 7 * 4]);
    bytes.push(1);
    bytes.extend_from_slice(&[0; 8]);

    push_object_prefix(&mut bytes, 1);
    bytes.extend_from_slice(&[0xff, 0, 0, 0]);

    push_object_prefix(&mut bytes, 2);
    bytes.extend_from_slice(&[0, 0, 0, 0]); // no quest, no reward, two-byte pad

    bytes.extend_from_slice(&1i32.to_le_bytes());
    push_string(&mut bytes, b"event");
    push_string(&mut bytes, b"message");
    bytes.extend_from_slice(&[0; 7 * 4]);
    bytes.extend_from_slice(&[0xff, 1, 0]);
    bytes.extend_from_slice(&4u16.to_le_bytes());
    bytes.extend_from_slice(&7u16.to_le_bytes());
    bytes.extend_from_slice(&[0; 16]);
    bytes.extend_from_slice(&[0; MAP_TRAILING_PADDING_SIZE]);

    let table = ObjectTable::parse(&bytes).unwrap();
    let body = MapBody::parse(table, Version::ShadowOfDeath).unwrap();
    assert!(body.trailing().is_empty());
    assert_eq!(body.objects().count(), 3);
    let mut objects = body.objects();
    let garrison = objects.next().unwrap();
    assert_eq!(garrison.serialized_class, 219);
    assert_eq!(garrison.object_class, 33);
    let mine = objects.next().unwrap();
    assert_eq!(mine.serialized_class, 220);
    assert_eq!(mine.object_class, 53);

    let event = body.timed_events().iter().next().unwrap();
    assert_eq!(event.name.bytes(), b"event");
    assert_eq!(event.message.bytes(), b"message");
    assert_eq!(event.stored_first_day, 4);
    assert_eq!(event.first_day, 5);
    assert_eq!(event.interval, 7);
}
