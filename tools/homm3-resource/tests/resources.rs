//! Generated fixtures for every engine-owned resource surface in the crate.

use homm3_resource::{
    iff::{Chunks, Xmidi},
    Bitmap, BitmapKind, Font, Mask, Palette, Spreadsheet, Text, BITMAP_HEADER_SIZE,
    BITMAP_PALETTE_SIZE, FONT_SPEC_SIZE, MASK_SIZE, PALETTE_FILE_SIZE,
};

fn iff_chunk(id: [u8; 4], data: &[u8]) -> Vec<u8> {
    let mut chunk = Vec::from(id);
    chunk.extend_from_slice(&u32::try_from(data.len()).unwrap().to_be_bytes());
    chunk.extend_from_slice(data);
    if data.len() & 1 != 0 {
        chunk.push(0);
    }
    chunk
}

fn xmidi_track(events: &[u8]) -> Vec<u8> {
    let mut form = Vec::from(*b"XMID");
    form.extend(iff_chunk(*b"TIMB", &[1, 0, 7, 0]));
    form.extend(iff_chunk(*b"EVNT", events));
    iff_chunk(*b"FORM", &form)
}

fn xmidi_fixture(tracks: &[&[u8]]) -> Vec<u8> {
    let mut xdir = Vec::from(*b"XDIR");
    xdir.extend(iff_chunk(
        *b"INFO",
        &u16::try_from(tracks.len()).unwrap().to_le_bytes(),
    ));
    let mut data = iff_chunk(*b"FORM", &xdir);
    let mut catalog = Vec::from(*b"XMID");
    for events in tracks {
        catalog.extend(xmidi_track(events));
    }
    data.extend(iff_chunk(*b"CAT ", &catalog));
    data
}

#[test]
fn parses_indexed_and_packed24_bitmaps() {
    let mut indexed = vec![0u8; BITMAP_HEADER_SIZE];
    indexed[0..4].copy_from_slice(&4u32.to_le_bytes());
    indexed[4..8].copy_from_slice(&2u32.to_le_bytes());
    indexed[8..12].copy_from_slice(&2u32.to_le_bytes());
    indexed.extend_from_slice(&[1, 2, 3, 4]);
    indexed.extend((0..BITMAP_PALETTE_SIZE).map(|value| u8::try_from(value % 256).unwrap()));
    let bitmap = Bitmap::parse(&indexed).unwrap();
    assert_eq!(bitmap.kind(), BitmapKind::Indexed8);
    assert_eq!(bitmap.pixels(), &[1, 2, 3, 4]);
    assert_eq!(bitmap.palette_rgb(1), Some([3, 4, 5]));

    let mut packed = vec![0u8; BITMAP_HEADER_SIZE];
    packed[0..4].copy_from_slice(&6u32.to_le_bytes());
    packed[4..8].copy_from_slice(&2u32.to_le_bytes());
    packed[8..12].copy_from_slice(&1u32.to_le_bytes());
    packed.extend_from_slice(&[1, 2, 3, 4, 5, 6]);
    assert_eq!(Bitmap::parse(&packed).unwrap().kind(), BitmapKind::Packed24);
}

#[test]
fn parses_palette_and_mask_records() {
    let mut palette = vec![0u8; PALETTE_FILE_SIZE];
    palette[24..28].copy_from_slice(&[1, 2, 3, 4]);
    let palette = Palette::parse(&palette).unwrap();
    assert_eq!(palette.header().len(), 24);
    assert_eq!(palette.rgba(0), [1, 2, 3, 4]);
    assert!(palette.trailing().is_empty());

    let mut tailed = vec![0u8; PALETTE_FILE_SIZE + 4];
    tailed[PALETTE_FILE_SIZE..].copy_from_slice(b"TAIL");
    assert_eq!(Palette::parse(&tailed).unwrap().trailing(), b"TAIL");

    let mut mask = [0u8; MASK_SIZE];
    mask[0] = 3;
    mask[1] = 2;
    mask[2] = 0b10;
    mask[8] = 0b100;
    let mask = Mask::parse(&mask).unwrap();
    assert_eq!((mask.width(), mask.height()), (3, 2));
    assert_eq!(mask.draw(1), Some(true));
    assert_eq!(mask.shadow(2), Some(true));
    assert_eq!(mask.draw(48), None);
}

#[test]
fn validates_font_glyph_extents() {
    let mut font = vec![0u8; FONT_SPEC_SIZE];
    font[1] = u8::MAX;
    font[5] = 2;
    let abc = 0x20 + usize::from(b'A') * 12;
    font[abc..abc + 4].copy_from_slice(&(-1i32).to_le_bytes());
    font[abc + 4..abc + 8].copy_from_slice(&2u32.to_le_bytes());
    font[abc + 8..abc + 12].copy_from_slice(&1i32.to_le_bytes());
    let offset = 0xc20 + usize::from(b'A') * 4;
    font[offset..offset + 4].copy_from_slice(&1u32.to_le_bytes());
    font.extend_from_slice(&[0, 1, 2, 3, 4]);

    let font = Font::parse(&font).unwrap();
    let glyph = font.glyph(b'A').unwrap();
    assert_eq!((glyph.left, glyph.width, glyph.right), (-1, 2, 1));
    assert_eq!(glyph.pixels, &[1, 2, 3, 4]);
}

#[test]
fn text_and_spreadsheet_match_retail_quote_rules() {
    let text = Text::parse(b"plain\t\t\r\n\"say \"\"\"hi\"\"\"\"\r\nignored").unwrap();
    assert_eq!(text.len(), 2);
    let decoded: Vec<Vec<u8>> = text.lines().map(|line| line.decoded().collect()).collect();
    assert_eq!(decoded[0], b"plain");
    assert_eq!(decoded[1], b"say \"hi\"");

    let sheet = Spreadsheet::parse(b"a\t\"b\"\t\r\n\"x\"\"y\"\tz\r\n").unwrap();
    assert_eq!(sheet.len(), 2);
    let rows: Vec<Vec<Vec<u8>>> = sheet
        .rows()
        .map(|row| row.cells().map(|cell| cell.decoded().collect()).collect())
        .collect();
    assert_eq!(rows[0], [b"a".to_vec(), b"b".to_vec(), Vec::new()]);
    assert_eq!(rows[1], [b"x\"y".to_vec(), b"z".to_vec()]);
}

#[test]
fn validates_iff_padding_and_xmidi_tracks() {
    let odd = iff_chunk(*b"ODD!", &[1, 2, 3]);
    let chunk = Chunks::new(&odd).next().unwrap().unwrap();
    assert_eq!(chunk.id(), *b"ODD!");
    assert_eq!(chunk.data(), &[1, 2, 3]);

    let image = xmidi_fixture(&[&[0x90, 60, 64], &[0xff, 0x2f, 0]]);
    let xmidi = Xmidi::parse(&image).unwrap();
    assert_eq!(xmidi.track_count(), 2);
    let tracks: Vec<_> = xmidi.tracks().collect();
    assert_eq!(tracks[0].events(), &[0x90, 60, 64]);
    assert_eq!(tracks[1].events(), &[0xff, 0x2f, 0]);
    assert_eq!(tracks[0].timbres(), Some(&[1, 0, 7, 0][..]));

    let mut truncated = image;
    truncated.pop();
    assert!(Xmidi::parse(&truncated).is_err());
}
