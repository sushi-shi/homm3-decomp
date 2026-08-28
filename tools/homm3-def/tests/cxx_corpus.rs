//! Optional installed-corpus differential against reconstructed C++.
#![cfg(feature = "cxx-parity")]
#![allow(unsafe_code)]

use std::env;
use std::fs;
use std::io::{Read, Take};
use std::path::Path;

use flate2::read::ZlibDecoder;
use homm3_def::{Blit, Canvas, Dialect, Frame, Rect, Sprite};
use homm3_lod::{Archive, Entry, Payload};

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

const CANVAS_WIDTH: usize = 67;
const CANVAS_HEIGHT: usize = 53;

#[test]
fn installed_def_corpus_matches_reconstructed_cpp() {
    let Some(paths) = env::var_os("HOMM3_DEF_CORPUS") else {
        eprintln!("HOMM3_DEF_CORPUS is unset; installed-corpus parity skipped");
        return;
    };
    let known_interleaved = match env::var("HOMM3_DEF_DIALECT").as_deref() {
        Ok("known-interleaved" | "steam") => true,
        Ok("retail") | Err(env::VarError::NotPresent) => false,
        Ok(value) => panic!("unknown HOMM3_DEF_DIALECT {value:?}; use retail or known-interleaved"),
        Err(error) => panic!("invalid HOMM3_DEF_DIALECT: {error}"),
    };

    let palette = test_palette();
    let mut counts = [0usize; 4];
    for path in env::split_paths(&paths) {
        compare_archive(&path, known_interleaved, &palette, &mut counts);
    }
    assert_ne!(
        counts.iter().sum::<usize>(),
        0,
        "corpus contains no DEF frames"
    );
    eprintln!(
        "installed DEF parity: raw={} general={} tileset={} adventure={}",
        counts[0], counts[1], counts[2], counts[3]
    );
}

fn compare_archive(
    path: &Path,
    known_interleaved: bool,
    palette: &[u16; 256],
    counts: &mut [usize; 4],
) {
    let image = fs::read(path).unwrap_or_else(|error| panic!("{}: {error}", path.display()));
    let archive =
        Archive::parse(&image).unwrap_or_else(|error| panic!("{}: {error}", path.display()));
    for entry in archive
        .entries()
        .filter(|entry| entry.has_extension(".def"))
    {
        let bytes = member_bytes(&archive, entry);
        let dialect = if known_interleaved
            && (entry.matches("SGTWMTA.DEF") || entry.matches("SGTWMTB.DEF"))
        {
            Dialect::InterleavedCompactFrames
        } else {
            Dialect::Retail
        };
        let sprite = Sprite::parse_with_dialect(&bytes, dialect)
            .unwrap_or_else(|error| panic!("{}:{}: {error}", path.display(), name(entry)));
        for (group_index, group) in sprite.groups().enumerate() {
            for frame_index in 0..group.len() {
                let frame = group.frame(frame_index).unwrap_or_else(|error| {
                    panic!(
                        "{}:{} group {group_index} frame {frame_index}: {error}",
                        path.display(),
                        name(entry)
                    )
                });
                frame.validate().unwrap_or_else(|error| {
                    panic!(
                        "{}:{} group {group_index} frame {frame_index}: {error}",
                        path.display(),
                        name(entry)
                    )
                });
                let ordinal = counts.iter().sum::<usize>();
                compare_frame(
                    frame,
                    palette,
                    ordinal,
                    &format!(
                        "{}:{} group {group_index} frame {frame_index}",
                        path.display(),
                        name(entry)
                    ),
                );
                counts[frame.encoding() as usize] += 1;
            }
        }
    }
}

fn compare_frame(frame: Frame<'_>, palette: &[u16; 256], ordinal: usize, label: &str) {
    let width = i32::try_from(frame.width()).unwrap();
    let height = i32::try_from(frame.height()).unwrap();
    let mirrored = ordinal & 1 != 0;
    let transparent_fills = ordinal & 2 != 0;
    let destination_x = i32::try_from(ordinal % 29).unwrap() - 17;
    let destination_y = i32::try_from((ordinal / 29) % 23).unwrap() - 13;
    let source = Rect::new(0, 0, width, height);

    let mut cpp = initial_canvas();
    let success = unsafe {
        homm3_cxx_draw_frame(
            frame.stream().as_ptr(),
            i32::try_from(frame.stream().len()).unwrap(),
            frame.encoding() as i32,
            width,
            height,
            i32::try_from(frame.cropped_width()).unwrap(),
            i32::try_from(frame.cropped_height()).unwrap(),
            frame.cropped_x(),
            frame.cropped_y(),
            cpp.as_mut_ptr(),
            i32::try_from(CANVAS_WIDTH).unwrap(),
            i32::try_from(CANVAS_HEIGHT).unwrap(),
            i32::try_from(CANVAS_WIDTH * 2).unwrap(),
            palette.as_ptr(),
            source.x,
            source.y,
            source.width,
            source.height,
            destination_x,
            destination_y,
            u8::from(mirrored),
            u8::from(transparent_fills),
        )
    };
    assert_eq!(success, 1, "C++ adapter failed for {label}");

    let mut rust = initial_canvas();
    let mut canvas = Canvas::new(CANVAS_WIDTH, CANVAS_HEIGHT, &mut rust).unwrap();
    Blit::new()
        .mirrored(mirrored)
        .transparent_fills(transparent_fills)
        .draw(
            &mut canvas,
            frame,
            palette,
            source,
            destination_x,
            destination_y,
        )
        .unwrap_or_else(|error| panic!("Rust renderer failed for {label}: {error}"));
    assert_eq!(rust, cpp, "renderer mismatch for {label}");
}

fn member_bytes(archive: &Archive<'_>, entry: Entry<'_>) -> Vec<u8> {
    match archive.payload(entry).unwrap() {
        Payload::Stored(bytes) => bytes.to_vec(),
        Payload::Compressed {
            stream,
            unpacked_size,
        } => {
            let limit = u64::try_from(unpacked_size).unwrap() + 1;
            let mut decoder: Take<ZlibDecoder<&[u8]>> = ZlibDecoder::new(stream).take(limit);
            let mut bytes = Vec::with_capacity(unpacked_size);
            decoder.read_to_end(&mut bytes).unwrap();
            assert_eq!(bytes.len(), unpacked_size, "{} length", name(entry));
            bytes
        }
    }
}

fn name(entry: Entry<'_>) -> &str {
    std::str::from_utf8(entry.name).unwrap_or("<non-UTF-8>")
}

fn test_palette() -> [u16; 256] {
    let mut palette = [0u16; 256];
    for (index, color) in palette.iter_mut().enumerate() {
        *color = u16::try_from(index).unwrap().wrapping_mul(257) ^ 0x5a5a;
    }
    palette
}

fn initial_canvas() -> Vec<u16> {
    (0..CANVAS_WIDTH * CANVAS_HEIGHT)
        .map(|index| u16::try_from(index).unwrap().wrapping_mul(73) ^ 0xa55a)
        .collect()
}
