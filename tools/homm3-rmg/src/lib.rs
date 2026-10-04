//! Heroes III random-map generation with explicit behavior and owned state.
//!
//! Resource values enter through parsing boundaries. Generation uses domain
//! types rather than unchecked C++ enum integers or pointer-shaped ownership.

#![forbid(unsafe_code)]

/// Source-derived raw values for callers implementing request adapters.
pub use homm3_rmg_data as raw;

pub mod behavior;
pub mod boundaries;
pub mod domain;
pub mod geometry;
pub mod hero;
mod identity;
pub mod layout;
pub mod line;
pub mod object;
mod parse;
pub mod placement;
pub mod placement_rules;
pub mod prototype;
pub mod raster;
pub mod request;
pub mod rng;
pub mod selection;
pub mod template;
pub mod terrain;
pub mod terrain_rules;
pub mod traits;
