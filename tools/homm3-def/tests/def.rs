//! Generated DEF fixtures covering every decoded storage encoding.

use homm3_def::{
    Blit, Canvas, DefType, Dialect, Encoding, Error, Rect, Run, Sprite, DEF_HEADER_SIZE,
    FRAME_HEADER_SIZE, GROUP_HEADER_SIZE,
};

const NAME_SIZE: usize = 13;

#[derive(Clone, Copy)]
struct FrameSpec {
    kind: DefType,
    encoding: Encoding,
    width: u32,
    height: u32,
    cropped_width: u32,
    cropped_height: u32,
    cropped_x: i32,
    cropped_y: i32,
}

fn sprite_with_frame(spec: FrameSpec, payload: &[u8]) -> Vec<u8> {
    let group_end = DEF_HEADER_SIZE + GROUP_HEADER_SIZE + NAME_SIZE + 4;
    let compact = !spec.kind.has_cropped_frames();
    let header_len = if compact { 16 } else { FRAME_HEADER_SIZE };
    let frame_at = group_end + usize::from(compact) * 16;
    let mut data = vec![0u8; frame_at + header_len * usize::from(!compact) + payload.len()];

    data[0..4].copy_from_slice(&spec.kind.0.to_le_bytes());
    data[4..8].copy_from_slice(&spec.width.to_le_bytes());
    data[8..12].copy_from_slice(&spec.height.to_le_bytes());
    data[12..16].copy_from_slice(&1u32.to_le_bytes());
    for (index, byte) in data[16..16 + 768].iter_mut().enumerate() {
        *byte = u8::try_from(index % 251).unwrap();
    }

    let group = DEF_HEADER_SIZE;
    data[group..group + 4].copy_from_slice(&7u32.to_le_bytes());
    data[group + 4..group + 8].copy_from_slice(&1u32.to_le_bytes());
    data[group + 8..group + 12].copy_from_slice(&0x1122_3344u32.to_le_bytes());
    data[group + 12..group + 16].copy_from_slice(&0x5566_7788u32.to_le_bytes());
    data[group + 16..group + 25].copy_from_slice(b"FRAME0001");

    if compact {
        let payload_at = group_end + 16;
        data[group + 16 + NAME_SIZE..group_end]
            .copy_from_slice(&u32::try_from(payload_at).unwrap().to_le_bytes());
        data[group_end..group_end + 4]
            .copy_from_slice(&u32::try_from(payload.len()).unwrap().to_le_bytes());
        data[group_end + 4..group_end + 8].copy_from_slice(&(spec.encoding as u32).to_le_bytes());
        data[group_end + 8..group_end + 12].copy_from_slice(&spec.width.to_le_bytes());
        data[group_end + 12..group_end + 16].copy_from_slice(&spec.height.to_le_bytes());
        data[payload_at..].copy_from_slice(payload);
    } else {
        data[group + 16 + NAME_SIZE..group_end]
            .copy_from_slice(&u32::try_from(group_end).unwrap().to_le_bytes());
        let header = group_end;
        data[header..header + 4]
            .copy_from_slice(&u32::try_from(payload.len()).unwrap().to_le_bytes());
        data[header + 4..header + 8].copy_from_slice(&(spec.encoding as u32).to_le_bytes());
        data[header + 8..header + 12].copy_from_slice(&spec.width.to_le_bytes());
        data[header + 12..header + 16].copy_from_slice(&spec.height.to_le_bytes());
        data[header + 16..header + 20].copy_from_slice(&spec.cropped_width.to_le_bytes());
        data[header + 20..header + 24].copy_from_slice(&spec.cropped_height.to_le_bytes());
        data[header + 24..header + 28].copy_from_slice(&spec.cropped_x.to_le_bytes());
        data[header + 28..header + 32].copy_from_slice(&spec.cropped_y.to_le_bytes());
        data[header + FRAME_HEADER_SIZE..].copy_from_slice(payload);
    }
    data
}

fn standard(encoding: Encoding, width: u32, height: u32, payload: &[u8]) -> Vec<u8> {
    sprite_with_frame(
        FrameSpec {
            kind: DefType::SPRITE,
            encoding,
            width,
            height,
            cropped_width: width,
            cropped_height: height,
            cropped_x: 0,
            cropped_y: 0,
        },
        payload,
    )
}

fn interleaved_compact_frame(
    encoding: Encoding,
    width: u32,
    height: u32,
    payload: &[u8],
) -> Vec<u8> {
    let frame_at = DEF_HEADER_SIZE + GROUP_HEADER_SIZE + NAME_SIZE + 4;
    let mut data = vec![0u8; frame_at + 16 + payload.len()];

    data[0..4].copy_from_slice(&DefType::SPRITE.0.to_le_bytes());
    data[4..8].copy_from_slice(&width.to_le_bytes());
    data[8..12].copy_from_slice(&height.to_le_bytes());
    data[12..16].copy_from_slice(&1u32.to_le_bytes());

    let group = DEF_HEADER_SIZE;
    data[group..group + 4].copy_from_slice(&7u32.to_le_bytes());
    data[group + 4..group + 8].copy_from_slice(&1u32.to_le_bytes());
    data[group + 16..group + 25].copy_from_slice(b"FRAME0001");
    data[group + 16 + NAME_SIZE..frame_at]
        .copy_from_slice(&u32::try_from(frame_at).unwrap().to_le_bytes());

    data[frame_at..frame_at + 4]
        .copy_from_slice(&u32::try_from(payload.len()).unwrap().to_le_bytes());
    data[frame_at + 4..frame_at + 8].copy_from_slice(&(encoding as u32).to_le_bytes());
    data[frame_at + 8..frame_at + 12].copy_from_slice(&width.to_le_bytes());
    data[frame_at + 12..frame_at + 16].copy_from_slice(&height.to_le_bytes());
    data[frame_at + 16..].copy_from_slice(payload);
    data
}

fn first_frame(data: &[u8]) -> homm3_def::Frame<'_> {
    Sprite::parse(data)
        .unwrap()
        .group(0)
        .unwrap()
        .frame(0)
        .unwrap()
}

#[test]
fn parses_container_groups_names_palette_and_crop() {
    let payload = [0u8; 12];
    let data = sprite_with_frame(
        FrameSpec {
            kind: DefType::CREATURE,
            encoding: Encoding::Raw,
            width: 8,
            height: 7,
            cropped_width: 4,
            cropped_height: 3,
            cropped_x: 2,
            cropped_y: -1,
        },
        &payload,
    );
    let sprite = Sprite::parse(&data).unwrap();
    assert_eq!(sprite.header().kind, DefType::CREATURE);
    assert_eq!(sprite.group_count(), 1);
    assert_eq!(sprite.total_frames(), 1);
    assert_eq!(sprite.palette().rgb(1), [3, 4, 5]);

    let group = sprite.find_group(7).unwrap();
    assert_eq!(
        (group.unknown_a(), group.unknown_b()),
        (0x1122_3344, 0x5566_7788)
    );
    let frame = group.frame(0).unwrap();
    assert_eq!(frame.name_str(), Some("FRAME0001"));
    assert_eq!((frame.width(), frame.height()), (8, 7));
    assert_eq!((frame.cropped_width(), frame.cropped_height()), (4, 3));
    assert_eq!((frame.cropped_x(), frame.cropped_y()), (2, -1));
    assert_eq!(frame.stream(), payload);
}

#[test]
fn decodes_raw_and_compact_frames() {
    let payload = [1, 2, 3, 4, 5, 6];
    let data = standard(Encoding::Raw, 3, 2, &payload);
    let frame = first_frame(&data);
    let mut out = [0u8; 6];
    frame.decode_into(&mut out).unwrap();
    assert_eq!(out, payload);

    let compact = sprite_with_frame(
        FrameSpec {
            kind: DefType::SPRITE_DEFINITION,
            encoding: Encoding::Raw,
            width: 3,
            height: 2,
            cropped_width: 3,
            cropped_height: 2,
            cropped_x: 0,
            cropped_y: 0,
        },
        &payload,
    );
    let compact_frame = first_frame(&compact);
    assert_eq!(compact_frame.encoding(), Encoding::Raw);
    let mut out = [0u8; 6];
    compact_frame.decode_into(&mut out).unwrap();
    assert_eq!(out, payload);
}

#[test]
fn explicit_interleaved_compact_dialect_does_not_weaken_retail() {
    let payload = [1, 2, 3, 4, 5, 6];
    let data = interleaved_compact_frame(Encoding::Raw, 3, 2, &payload);

    let retail = Sprite::parse(&data).unwrap();
    assert!(retail.group(0).unwrap().frame(0).is_err());

    let sprite = Sprite::parse_with_dialect(&data, Dialect::InterleavedCompactFrames).unwrap();
    let frame = sprite.group(0).unwrap().frame(0).unwrap();
    assert_eq!((frame.width(), frame.height()), (3, 2));
    assert_eq!(frame.stream(), payload);
    let mut decoded = [0u8; 6];
    frame.decode_into(&mut decoded).unwrap();
    assert_eq!(decoded, payload);
}

#[test]
fn decodes_general_rle_packets() {
    let payload = [
        8, 0, 0, 0, 14, 0, 0, 0, // row offsets
        0xff, 1, 10, 11, 3, 2, // literal 2, fill 3
        4, 4, // fill 5
    ];
    let data = standard(Encoding::GeneralRle, 5, 2, &payload);
    let frame = first_frame(&data);
    let runs: Vec<_> = frame.runs(0).unwrap().collect::<Result<_, _>>().unwrap();
    assert_eq!(
        runs,
        [Run::Literal(&[10, 11]), Run::Fill { color: 3, len: 3 }]
    );
    let mut out = [0u8; 10];
    frame.decode_into(&mut out).unwrap();
    assert_eq!(out, [10, 11, 3, 3, 3, 4, 4, 4, 4, 4]);
    frame.validate().unwrap();
}

#[test]
fn decodes_tileset_and_adventure_packet_tables() {
    let tileset = [
        4,
        0,
        8,
        0,            // row offsets
        (2 << 5) | 2, // fill 3
        (7 << 5) | 1,
        9,
        8,            // literal 2
        (6 << 5) | 4, // fill 5
    ];
    let data = standard(Encoding::TilesetRle, 5, 2, &tileset);
    let mut out = [0u8; 10];
    first_frame(&data).decode_into(&mut out).unwrap();
    assert_eq!(out, [2, 2, 2, 9, 8, 6, 6, 6, 6, 6]);

    let mut adventure = vec![4, 0, 5, 0];
    adventure.push((2 << 5) | 31); // first 32-pixel segment
    adventure.push((7 << 5) | 31); // second segment: 32 literals
    adventure.extend(20..52);
    let data = standard(Encoding::AdventureRle, 64, 1, &adventure);
    let mut out = [0u8; 64];
    first_frame(&data).decode_into(&mut out).unwrap();
    assert_eq!(&out[..32], &[2; 32]);
    assert_eq!(&out[32..], &(20..52).collect::<Vec<_>>());

    let data = standard(Encoding::AdventureRle, 40, 1, &[0, 0]);
    assert!(matches!(
        first_frame(&data).validate(),
        Err(Error::BadDimension {
            what: "adventure row width",
            value: 40
        })
    ));
}

#[test]
fn clipped_mirrored_blit_matches_fill_and_literal_semantics() {
    let payload = [
        4, 0, 0, 0, // row offset
        2, 1, // fill colour 2 x2
        0xff, 2, 5, 6, 7, // literal x3
    ];
    let data = standard(Encoding::GeneralRle, 5, 1, &payload);
    let frame = first_frame(&data);
    let mut palette = [0u16; 256];
    for (index, color) in palette.iter_mut().enumerate() {
        *color = u16::try_from(index).unwrap() * 10;
    }

    let mut pixels = [9u16; 7];
    let mut canvas = Canvas::new(7, 1, &mut pixels).unwrap();
    Blit::new()
        .draw_full(&mut canvas, frame, &palette, 1, 0)
        .unwrap();
    assert_eq!(pixels, [9, 20, 20, 50, 60, 70, 9]);

    let mut pixels = [9u16; 7];
    let mut canvas = Canvas::new(7, 1, &mut pixels).unwrap();
    Blit::new()
        .transparent_fills(true)
        .draw_full(&mut canvas, frame, &palette, 1, 0)
        .unwrap();
    assert_eq!(pixels, [9, 9, 9, 50, 60, 70, 9]);

    let mut pixels = [9u16; 7];
    let mut canvas = Canvas::new(7, 1, &mut pixels).unwrap();
    Blit::new()
        .mirrored(true)
        .clip(Rect::new(2, 0, 3, 1))
        .draw_full(&mut canvas, frame, &palette, 1, 0)
        .unwrap();
    assert_eq!(pixels, [9, 9, 60, 50, 20, 9, 9]);
}

#[test]
fn malformed_rows_and_destinations_fail_closed() {
    let payload = [4, 0, 0, 0, 3, 9]; // fill 10 into a five-pixel row
    let data = standard(Encoding::GeneralRle, 5, 1, &payload);
    let frame = first_frame(&data);
    assert!(matches!(frame.validate(), Err(Error::RunOverrun { .. })));
    assert!(matches!(
        frame.decode_into(&mut [0u8; 4]),
        Err(Error::BadDestination {
            needed: 5,
            available: 4
        })
    ));

    let mut data = standard(Encoding::Raw, 1, 1, &[0]);
    let offset_at = DEF_HEADER_SIZE + GROUP_HEADER_SIZE + NAME_SIZE;
    data[offset_at..offset_at + 4].copy_from_slice(&u32::MAX.to_le_bytes());
    let sprite = Sprite::parse(&data).unwrap();
    assert!(matches!(
        sprite.group(0).unwrap().frame(0),
        Err(Error::FrameOffset { .. })
    ));
}
