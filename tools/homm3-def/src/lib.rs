#![no_std]
#![forbid(unsafe_code)]
//! Allocation-free parser, decoder, and blitter for Heroes III DEF sprites.
//!
//! The model is independent of the matching C++ source tree. Its container
//! layout is checked against retail `ResourceManager::GetSprite` at
//! `0x55c7b0`. Raw, general-RLE, tileset-RLE, and adventure-RLE drawing are
//! checked against `CSpriteFrame::Draw`, `DrawAdvObjImpl`, and `DrawTile` at
//! `0x47c570`, `0x47d0a0`, and `0x47dd40`. Every API borrows input and writes
//! into caller-owned storage, so the library needs neither `std` nor an
//! allocator.

mod decode;
mod error;
mod render;
mod sprite;

pub use decode::{RowRuns, Run};
pub use error::Error;
pub use render::{Blit, Canvas, Rect};
pub use sprite::{
    DefType, Dialect, Encoding, Frame, Group, Groups, Palette, Sprite, SpriteHeader,
    DEF_HEADER_SIZE, FRAME_HEADER_SIZE, GROUP_HEADER_SIZE,
};

/// Result type used by this crate.
pub type Result<T> = core::result::Result<T, Error>;
