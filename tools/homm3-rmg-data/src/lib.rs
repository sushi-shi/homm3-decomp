//! Raw definitions extracted from the canonical C++ sources.
//!
//! Values in this module have not been parsed into Rust generation domains.
//! In particular, integer aliases for C enums may contain invalid values.

#![forbid(unsafe_code)]
#![allow(non_camel_case_types, non_snake_case, non_upper_case_globals)]
#![allow(missing_docs, clippy::all, clippy::pedantic)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
include!(concat!(env!("OUT_DIR"), "/tables.rs"));

#[cfg(test)]
mod tests {
    #[test]
    fn request_matches_the_oracle_wire_record() {
        assert_eq!(std::mem::size_of::<super::TRandomMapRequest>(), 80);
        assert_eq!(std::mem::align_of::<super::TRandomMapRequest>(), 4);
        assert_eq!(
            std::mem::offset_of!(super::TRandomMapRequest, m_townChoices),
            8
        );
        assert_eq!(std::mem::offset_of!(super::TRandomMapRequest, m_width), 40);
        assert_eq!(
            std::mem::offset_of!(super::TRandomMapRequest, m_mapVersion),
            76
        );
    }
}
