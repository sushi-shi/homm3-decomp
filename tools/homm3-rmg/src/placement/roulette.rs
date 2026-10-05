//! Native weighted roulette: one draw reduced modulo the total weight.
//!
//! Candidates keep their own weights, so selection never re-reads a catalog
//! and the running total cannot drift from the candidate list.
use super::PlacementError;
use crate::rng::RetailRng;
use std::num::NonZeroU32;

/// Positively weighted candidates in insertion order. Retains capacity.
#[derive(Debug)]
pub(crate) struct Roulette<T> {
    items: Vec<(T, NonZeroU32)>,
    total: u32,
}
impl<T> Default for Roulette<T> {
    fn default() -> Self {
        Self {
            items: Vec::new(),
            total: 0,
        }
    }
}
impl<T> Roulette<T> {
    pub(crate) fn clear(&mut self) {
        self.items.clear();
        self.total = 0;
    }
    /// Add a candidate. Native sums weights as signed 32-bit integers; a total
    /// beyond that domain is an arithmetic fault before any reservation.
    pub(crate) fn push(&mut self, item: T, weight: NonZeroU32) -> Result<(), PlacementError> {
        let total = self
            .total
            .checked_add(weight.get())
            .filter(|&total| i32::try_from(total).is_ok())
            .ok_or(PlacementError::Arithmetic)?;
        self.items.try_reserve(1)?;
        self.items.push((item, weight));
        self.total = total;
        Ok(())
    }
    /// Draw `rand() % total` and return the first candidate whose running
    /// weight exceeds it. An empty roulette returns `None` without drawing.
    pub(crate) fn choose(&self, rng: &mut RetailRng) -> Option<&T> {
        let mut choice = rng.below(NonZeroU32::new(self.total)?);
        self.items.iter().find_map(|(item, weight)| {
            if choice < weight.get() {
                Some(item)
            } else {
                choice -= weight.get();
                None
            }
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn roulette_matches_signed_running_subtraction() {
        let weights = [3, 1, 5];
        let mut roulette = Roulette::default();
        assert!(roulette.choose(&mut RetailRng::new(1)).is_none());
        for (item, weight) in weights.into_iter().enumerate() {
            roulette
                .push(item, NonZeroU32::new(weight).unwrap())
                .unwrap();
        }
        for seed in 0..64 {
            let mut native = RetailRng::new(seed);
            let mut remaining = i32::try_from(native.draw() % 9).unwrap();
            let expected = weights.iter().position(|&weight| {
                remaining -= i32::try_from(weight).unwrap();
                remaining < 0
            });
            let mut rng = RetailRng::new(seed);
            assert_eq!(roulette.choose(&mut rng).copied(), expected);
            assert_eq!(rng.state(), native.state());
        }
        let mut full = Roulette::default();
        full.push((), NonZeroU32::new(i32::MAX.unsigned_abs()).unwrap())
            .unwrap();
        assert!(matches!(
            full.push((), NonZeroU32::MIN),
            Err(PlacementError::Arithmetic)
        ));
    }
}
