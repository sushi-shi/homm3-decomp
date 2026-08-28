//! Differential renderer checks against reconstructed C++.
#![cfg(feature = "cxx-parity")]
#![allow(unsafe_code)]

use homm3_def::{Blit, Canvas, DefType, Encoding, Rect, Sprite, DEF_HEADER_SIZE};

unsafe extern "C" {
    fn homm3_cxx_draw_frame(
        stream: *const u8,
        stream_length: i32,
        encoding: i32,
        width: i32,
        height: i32,
        cropped_width: i32,
        cropped_height: i32,
        cropped_x: i32,
        cropped_y: i32,
        destination: *mut u16,
        destination_width: i32,
        destination_height: i32,
        destination_pitch: i32,
        palette: *const u16,
        source_x: i32,
        source_y: i32,
        source_width: i32,
        source_height: i32,
        destination_x: i32,
        destination_y: i32,
        mirrored: u8,
        transparent_fills: u8,
    ) -> i32;
}

#[derive(Clone, Copy)]
struct Shape {
    width: usize,
    height: usize,
    cropped_width: usize,
    cropped_height: usize,
    cropped_x: i32,
    cropped_y: i32,
}

#[derive(Clone, Copy)]
struct Draw {
    source: Rect,
    x: i32,
    y: i32,
    mirrored: bool,
    transparent_fills: bool,
}

#[test]
fn generated_general_streams_match_reconstructed_cpp_draw() {
    let mut random = Lcg(0x4833_4445);
    let mut palette = [0u16; 256];
    for (index, color) in palette.iter_mut().enumerate() {
        *color = u16::try_from(index).unwrap().wrapping_mul(257) ^ 0x5a5a;
    }

    for case in 0..512 {
        let cropped_width = random.range(1, 40);
        let cropped_height = random.range(1, 14);
        let cropped_x = i32::try_from(random.range(0, 5)).unwrap();
        let cropped_y = i32::try_from(random.range(0, 4)).unwrap();
        let shape = Shape {
            width: cropped_width + usize::try_from(cropped_x).unwrap() + random.range(0, 5),
            height: cropped_height + usize::try_from(cropped_y).unwrap() + random.range(0, 4),
            cropped_width,
            cropped_height,
            cropped_x,
            cropped_y,
        };
        let stream = general_rle(shape.cropped_width, shape.cropped_height, &mut random);
        let image = def_image(shape, Encoding::GeneralRle, &stream);
        let sprite = Sprite::parse(&image).unwrap();
        let frame = sprite.group(0).unwrap().frame(0).unwrap();

        let source_x = i32::try_from(random.range(0, shape.width)).unwrap();
        let source_y = i32::try_from(random.range(0, shape.height)).unwrap();
        let draw = Draw {
            source: Rect::new(
                source_x,
                source_y,
                i32::try_from(
                    random.range(1, shape.width - usize::try_from(source_x).unwrap() + 1),
                )
                .unwrap(),
                i32::try_from(
                    random.range(1, shape.height - usize::try_from(source_y).unwrap() + 1),
                )
                .unwrap(),
            ),
            x: i32::try_from(random.range(0, 31)).unwrap() - 8,
            y: i32::try_from(random.range(0, 23)).unwrap() - 6,
            mirrored: random.next() & 1 != 0,
            transparent_fills: random.next() & 1 != 0,
        };

        let mut cpp = initial_canvas();
        let success = unsafe {
            homm3_cxx_draw_frame(
                stream.as_ptr(),
                i32::try_from(stream.len()).unwrap(),
                Encoding::GeneralRle as i32,
                i32::try_from(shape.width).unwrap(),
                i32::try_from(shape.height).unwrap(),
                i32::try_from(shape.cropped_width).unwrap(),
                i32::try_from(shape.cropped_height).unwrap(),
                shape.cropped_x,
                shape.cropped_y,
                cpp.as_mut_ptr(),
                24,
                17,
                24 * 2,
                palette.as_ptr(),
                draw.source.x,
                draw.source.y,
                draw.source.width,
                draw.source.height,
                draw.x,
                draw.y,
                u8::from(draw.mirrored),
                u8::from(draw.transparent_fills),
            )
        };
        assert_eq!(success, 1, "C++ adapter failed in case {case}");

        let mut rust = initial_canvas();
        let mut canvas = Canvas::new(24, 17, &mut rust).unwrap();
        Blit::new()
            .mirrored(draw.mirrored)
            .transparent_fills(draw.transparent_fills)
            .draw(&mut canvas, frame, &palette, draw.source, draw.x, draw.y)
            .unwrap();
        assert_eq!(rust, cpp, "renderer mismatch in generated case {case}");
    }
}

#[test]
fn generated_raw_streams_match_reconstructed_cpp_draw() {
    let mut random = Lcg(0x5241_5721);
    let palette = test_palette();

    for case in 0..512 {
        let cropped_width = random.range(1, 40);
        let cropped_height = random.range(1, 14);
        let cropped_x = i32::try_from(random.range(0, 5)).unwrap();
        let cropped_y = i32::try_from(random.range(0, 4)).unwrap();
        let shape = Shape {
            width: cropped_width + usize::try_from(cropped_x).unwrap() + random.range(0, 5),
            height: cropped_height + usize::try_from(cropped_y).unwrap() + random.range(0, 4),
            cropped_width,
            cropped_height,
            cropped_x,
            cropped_y,
        };
        let mut stream = vec![0; cropped_width * cropped_height];
        for pixel in &mut stream {
            *pixel = random.next().to_le_bytes()[0];
        }
        let draw = random_draw(shape, &mut random);
        compare_case(Encoding::Raw, shape, &stream, draw, &palette, case);
    }
}

#[test]
fn generated_tileset_streams_match_reconstructed_cpp_draw() {
    let mut random = Lcg(0x5449_4c45);
    let palette = test_palette();

    for case in 0..512 {
        let cropped_width = random.range(1, 40);
        let cropped_height = random.range(1, 14);
        let cropped_x = i32::try_from(random.range(0, 5)).unwrap();
        let cropped_y = i32::try_from(random.range(0, 4)).unwrap();
        let shape = Shape {
            width: cropped_width + usize::try_from(cropped_x).unwrap() + random.range(0, 5),
            height: cropped_height + usize::try_from(cropped_y).unwrap() + random.range(0, 4),
            cropped_width,
            cropped_height,
            cropped_x,
            cropped_y,
        };
        let stream = tileset_rle(cropped_width, cropped_height, &mut random);
        let draw = random_draw(shape, &mut random);
        compare_case(Encoding::TilesetRle, shape, &stream, draw, &palette, case);
    }
}

#[test]
fn generated_adventure_streams_match_reconstructed_cpp_draw() {
    let mut random = Lcg(0x4144_564f);
    let palette = test_palette();

    for case in 0..512 {
        let cropped_width = 32 * random.range(1, 4);
        let cropped_height = random.range(1, 14);
        let shape = Shape {
            width: cropped_width,
            height: cropped_height,
            cropped_width,
            cropped_height,
            cropped_x: 0,
            cropped_y: 0,
        };
        let stream = adventure_rle(cropped_width, cropped_height, &mut random);
        let draw = random_draw(shape, &mut random);
        compare_case(Encoding::AdventureRle, shape, &stream, draw, &palette, case);
    }
}

fn test_palette() -> [u16; 256] {
    let mut palette = [0u16; 256];
    for (index, color) in palette.iter_mut().enumerate() {
        *color = u16::try_from(index).unwrap().wrapping_mul(257) ^ 0x5a5a;
    }
    palette
}

fn random_draw(shape: Shape, random: &mut Lcg) -> Draw {
    let source_x = random.range(0, shape.width);
    let source_y = random.range(0, shape.height);
    Draw {
        source: Rect::new(
            i32::try_from(source_x).unwrap(),
            i32::try_from(source_y).unwrap(),
            i32::try_from(random.range(1, shape.width - source_x + 1)).unwrap(),
            i32::try_from(random.range(1, shape.height - source_y + 1)).unwrap(),
        ),
        x: i32::try_from(random.range(0, 31)).unwrap() - 8,
        y: i32::try_from(random.range(0, 23)).unwrap() - 6,
        mirrored: random.next() & 1 != 0,
        transparent_fills: random.next() & 1 != 0,
    }
}

fn compare_case(
    encoding: Encoding,
    shape: Shape,
    stream: &[u8],
    draw: Draw,
    palette: &[u16; 256],
    case: usize,
) {
    let image = def_image(shape, encoding, stream);
    let sprite = Sprite::parse(&image).unwrap();
    let frame = sprite.group(0).unwrap().frame(0).unwrap();

    let mut cpp = initial_canvas();
    let success = unsafe {
        homm3_cxx_draw_frame(
            stream.as_ptr(),
            i32::try_from(stream.len()).unwrap(),
            encoding as i32,
            i32::try_from(shape.width).unwrap(),
            i32::try_from(shape.height).unwrap(),
            i32::try_from(shape.cropped_width).unwrap(),
            i32::try_from(shape.cropped_height).unwrap(),
            shape.cropped_x,
            shape.cropped_y,
            cpp.as_mut_ptr(),
            24,
            17,
            24 * 2,
            palette.as_ptr(),
            draw.source.x,
            draw.source.y,
            draw.source.width,
            draw.source.height,
            draw.x,
            draw.y,
            u8::from(draw.mirrored),
            u8::from(draw.transparent_fills),
        )
    };
    assert_eq!(success, 1, "C++ adapter failed in case {case}");

    let mut rust = initial_canvas();
    let mut canvas = Canvas::new(24, 17, &mut rust).unwrap();
    Blit::new()
        .mirrored(draw.mirrored)
        .transparent_fills(draw.transparent_fills)
        .draw(&mut canvas, frame, palette, draw.source, draw.x, draw.y)
        .unwrap();
    assert_eq!(rust, cpp, "renderer mismatch in generated case {case}");
}

fn initial_canvas() -> Vec<u16> {
    (0..24 * 17)
        .map(|index| u16::try_from(index).unwrap().wrapping_mul(73) ^ 0xa55a)
        .collect()
}

fn general_rle(width: usize, height: usize, random: &mut Lcg) -> Vec<u8> {
    let mut stream = vec![0u8; height * 4];
    for row in 0..height {
        let offset = u32::try_from(stream.len()).unwrap();
        stream[row * 4..row * 4 + 4].copy_from_slice(&offset.to_le_bytes());
        let mut remaining = width;
        while remaining != 0 {
            let len = random.range(1, remaining.min(9) + 1);
            if random.next() % 3 == 0 {
                stream.push(255);
                stream.push(u8::try_from(len - 1).unwrap());
                for _ in 0..len {
                    stream.push(random.next().to_le_bytes()[0]);
                }
            } else {
                stream.push(random.next().to_le_bytes()[0] % 200);
                stream.push(u8::try_from(len - 1).unwrap());
            }
            remaining -= len;
        }
    }
    stream
}

fn tileset_rle(width: usize, height: usize, random: &mut Lcg) -> Vec<u8> {
    let mut stream = vec![0u8; height * 2];
    for row in 0..height {
        let offset = u16::try_from(stream.len()).unwrap();
        stream[row * 2..row * 2 + 2].copy_from_slice(&offset.to_le_bytes());
        packed_segment(&mut stream, width, random);
    }
    stream
}

fn adventure_rle(width: usize, height: usize, random: &mut Lcg) -> Vec<u8> {
    let cells = width / 32;
    let mut stream = vec![0u8; height * cells * 2];
    for row in 0..height {
        for cell in 0..cells {
            let offset = u16::try_from(stream.len()).unwrap();
            let at = (row * cells + cell) * 2;
            stream[at..at + 2].copy_from_slice(&offset.to_le_bytes());
            packed_segment(&mut stream, 32, random);
        }
    }
    stream
}

fn packed_segment(stream: &mut Vec<u8>, width: usize, random: &mut Lcg) {
    let mut remaining = width;
    while remaining != 0 {
        let len = random.range(1, remaining.min(32) + 1);
        if random.next() % 3 == 0 {
            stream.push((7 << 5) | u8::try_from(len - 1).unwrap());
            for _ in 0..len {
                stream.push(random.next().to_le_bytes()[0]);
            }
        } else {
            let code = random.next().to_le_bytes()[0] % 7;
            stream.push((code << 5) | u8::try_from(len - 1).unwrap());
        }
        remaining -= len;
    }
}

fn def_image(shape: Shape, encoding: Encoding, stream: &[u8]) -> Vec<u8> {
    const GROUP_BYTES: usize = 16 + 13 + 4;
    const FRAME_BYTES: usize = 32;
    let frame_at = DEF_HEADER_SIZE + GROUP_BYTES;
    let mut image = vec![0u8; frame_at + FRAME_BYTES + stream.len()];
    image[0..4].copy_from_slice(&DefType::SPRITE.0.to_le_bytes());
    image[4..8].copy_from_slice(&u32::try_from(shape.width).unwrap().to_le_bytes());
    image[8..12].copy_from_slice(&u32::try_from(shape.height).unwrap().to_le_bytes());
    image[12..16].copy_from_slice(&1u32.to_le_bytes());
    image[DEF_HEADER_SIZE + 4..DEF_HEADER_SIZE + 8].copy_from_slice(&1u32.to_le_bytes());
    image[DEF_HEADER_SIZE + 16..DEF_HEADER_SIZE + 22].copy_from_slice(b"ORACLE");
    image[DEF_HEADER_SIZE + 29..DEF_HEADER_SIZE + 33]
        .copy_from_slice(&u32::try_from(frame_at).unwrap().to_le_bytes());
    image[frame_at..frame_at + 4]
        .copy_from_slice(&u32::try_from(stream.len()).unwrap().to_le_bytes());
    image[frame_at + 4..frame_at + 8].copy_from_slice(&(encoding as u32).to_le_bytes());
    image[frame_at + 8..frame_at + 12]
        .copy_from_slice(&u32::try_from(shape.width).unwrap().to_le_bytes());
    image[frame_at + 12..frame_at + 16]
        .copy_from_slice(&u32::try_from(shape.height).unwrap().to_le_bytes());
    image[frame_at + 16..frame_at + 20]
        .copy_from_slice(&u32::try_from(shape.cropped_width).unwrap().to_le_bytes());
    image[frame_at + 20..frame_at + 24]
        .copy_from_slice(&u32::try_from(shape.cropped_height).unwrap().to_le_bytes());
    image[frame_at + 24..frame_at + 28].copy_from_slice(&shape.cropped_x.to_le_bytes());
    image[frame_at + 28..frame_at + 32].copy_from_slice(&shape.cropped_y.to_le_bytes());
    image[frame_at + FRAME_BYTES..].copy_from_slice(stream);
    image
}

struct Lcg(u32);

impl Lcg {
    fn next(&mut self) -> u32 {
        self.0 = self.0.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
        self.0
    }

    fn range(&mut self, start: usize, end: usize) -> usize {
        assert!(start < end);
        start + usize::try_from(self.next()).unwrap() % (end - start)
    }
}
