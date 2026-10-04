//! Process-local ownership checks. These tags never affect generation ordering,
//! randomness, serialization or replay diagnostics. Creating an immutable catalog
//! takes one atomic operation; copying and checking a handle allocate nothing.

use std::{
    fmt,
    sync::atomic::{AtomicU64, Ordering},
};

static NEXT_OWNER: AtomicU64 = AtomicU64::new(1);

#[derive(Clone, Copy, PartialEq, Eq, Hash)]
pub(crate) struct OwnerId(u64);
impl OwnerId {
    pub(crate) fn new() -> Option<Self> {
        NEXT_OWNER
            .fetch_update(Ordering::Relaxed, Ordering::Relaxed, |value| {
                value.checked_add(1)
            })
            .ok()
            .map(Self)
    }
}

impl fmt::Debug for OwnerId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("OwnerId(..)")
    }
}
