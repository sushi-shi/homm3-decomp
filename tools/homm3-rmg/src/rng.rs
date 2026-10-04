//! The retail CRT random stream, owned by one generation.

use std::num::NonZeroU32;

/// RNG diagnostics at a completed generation stage.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct RngCheckpoint {
    /// Complete CRT state, rather than the last 15-bit result.
    pub state: u32,
    /// Draws since the original seed.
    pub draws: u64,
}

/// Seed and draw position of the pinned Complete CRT's `rand` stream.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RetailRng {
    state: u32,
    draws: u64,
}

impl RetailRng {
    /// Seed exactly as retail `srand` at 0x617e4f; zero is a valid seed.
    #[must_use]
    pub const fn new(seed: u32) -> Self {
        Self {
            state: seed,
            draws: 0,
        }
    }

    /// One 15-bit draw, including the original 32-bit wrapping state update.
    ///
    /// Retail 0x617e64 multiplies by 0x343fd, 0x617e6a adds 0x269ec3,
    /// and 0x617e75..0x617e78 extracts `(state >> 16) & 0x7fff`.
    pub fn draw(&mut self) -> u32 {
        self.state = self
            .state
            .wrapping_mul(0x0003_43fd)
            .wrapping_add(0x0026_9ec3);
        self.draws += 1;
        (self.state >> 16) & 0x7fff
    }

    /// Retail's biased `rand() % count`, always consuming exactly one draw.
    ///
    /// Even a singleton consumes a draw. Do not substitute an unbiased sampler:
    /// rejection sampling changes every subsequent generation decision.
    pub fn below(&mut self, count: NonZeroU32) -> u32 {
        self.draw() % count.get()
    }

    /// One modulo-biased draw centred on zero; even ranges are asymmetric.
    /// A singleton still consumes its draw and returns zero.
    #[expect(
        clippy::missing_panics_doc,
        reason = "a u32 range centred at floor(range/2) fits i32"
    )]
    pub fn centered_offset(&mut self, range: NonZeroU32) -> i32 {
        i32::try_from(i64::from(self.below(range)) - i64::from(range.get() / 2)).unwrap()
    }

    /// CRT state compared by the whole-map oracle, not the last returned draw.
    #[must_use]
    pub const fn state(&self) -> u32 {
        self.state
    }

    /// Number of draws since construction, useful for stage comparisons.
    #[must_use]
    pub const fn draws(&self) -> u64 {
        self.draws
    }

    /// Record a stage without consuming a draw.
    #[must_use]
    pub const fn checkpoint(&self) -> RngCheckpoint {
        RngCheckpoint {
            state: self.state,
            draws: self.draws,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn published_crt_seed_one_sequence() {
        let mut rng = RetailRng::new(1);
        assert_eq!(
            std::array::from_fn::<_, 10, _>(|_| rng.draw()),
            [41, 18467, 6334, 26500, 19169, 15724, 11478, 29358, 26962, 24464]
        );
        assert_eq!(rng.draws(), 10);
    }

    #[test]
    fn zero_seed_and_singletons_still_advance() {
        let mut rng = RetailRng::new(0);
        assert_eq!(rng.below(NonZeroU32::MIN), 0);
        assert_eq!(rng.state(), 0x0026_9ec3);
        assert_eq!(rng.draws(), 1);
    }

    #[test]
    fn separate_generations_do_not_share_state() {
        let mut first = RetailRng::new(u32::MAX);
        let second = RetailRng::new(u32::MAX);
        first.draw();
        assert_eq!(second.state(), u32::MAX);
        assert_eq!(
            first.state(),
            u32::MAX.wrapping_mul(0x0003_43fd).wrapping_add(0x0026_9ec3)
        );
    }
}
