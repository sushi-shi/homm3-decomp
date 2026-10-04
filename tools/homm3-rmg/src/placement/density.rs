//! Native weighted category scheduling, with no uninitialized inactive slots.

use super::PlacementError;
use crate::template::Placement;

#[derive(Clone, Copy)]
enum Category {
    Finished,
    Active { stride: i32, weighted_count: i32 },
}

pub(super) struct Density<const N: usize> {
    categories: [Category; N],
    total: i32,
}
impl<const N: usize> Density<N> {
    pub(super) fn new(placements: &[Placement; N]) -> Result<Self, PlacementError> {
        let mut total = 0_i32;
        let mut product = 1_i32;
        for placement in placements {
            if let Some(density) = placement.density {
                let density =
                    i32::try_from(density.get()).map_err(|_| PlacementError::Arithmetic)?;
                total = total
                    .checked_add(density)
                    .ok_or(PlacementError::Arithmetic)?;
                product = product
                    .checked_mul(density)
                    .ok_or(PlacementError::Arithmetic)?;
            }
        }
        let mut categories = [Category::Finished; N];
        for (category, placement) in categories.iter_mut().zip(placements) {
            if let Some(density) = placement.density {
                let stride = product
                    / i32::try_from(density.get()).map_err(|_| PlacementError::Arithmetic)?;
                *category = Category::Active {
                    stride,
                    weighted_count: placement
                        .initial_count
                        .checked_mul(stride)
                        .ok_or(PlacementError::Arithmetic)?,
                };
            }
        }
        Ok(Self { categories, total })
    }
    /// Squared distances are four times tile areas: truncate division before root.
    pub(super) fn spacing(&self, area: i32) -> Option<i32> {
        if self.total == 0 {
            return None;
        }
        // Area is a positive shared source constant; the quotient fits i32.
        #[expect(
            clippy::cast_possible_truncation,
            reason = "nonnegative i32 square root fits i32"
        )]
        let spacing = f64::from(area / self.total).sqrt() as i32;
        Some(spacing)
    }
    pub(super) fn next(&mut self) -> Result<Option<usize>, PlacementError> {
        let selected = self
            .categories
            .iter()
            .enumerate()
            .filter_map(|(index, category)| match category {
                Category::Finished => None,
                Category::Active { weighted_count, .. } => Some((index, *weighted_count)),
            })
            .min_by_key(|&(_, count)| count)
            .map(|(index, _)| index);
        if let Some(index) = selected {
            if let Category::Active {
                stride,
                weighted_count,
            } = &mut self.categories[index]
            {
                *weighted_count = weighted_count
                    .checked_add(*stride)
                    .ok_or(PlacementError::Arithmetic)?;
            }
        }
        Ok(selected)
    }
    pub(super) fn finish(&mut self, index: usize) {
        self.categories[index] = Category::Finished;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::num::NonZeroU32;

    #[test]
    fn weighted_categories_keep_first_ties_and_retire_failed_categories() {
        let category = |initial_count, density| Placement {
            initial_count,
            density: NonZeroU32::new(density),
        };
        let mut schedule =
            Density::new(&[category(1, 2), category(0, 1), category(i32::MAX, 0)]).unwrap();
        assert_eq!(schedule.spacing(100), Some(5)); // floor(sqrt(100 / 3)).
        for expected in [1, 0, 0, 1, 0, 0] {
            assert_eq!(schedule.next().unwrap(), Some(expected));
        }
        schedule.finish(0);
        assert_eq!(schedule.next().unwrap(), Some(1));
        schedule.finish(1);
        assert_eq!(schedule.next().unwrap(), None);
        let disabled = Density::new(&[category(i32::MAX, 0)]).unwrap();
        assert_eq!(disabled.spacing(100), None);
    }

    #[test]
    fn negative_initial_counts_keep_their_priority_and_overflow_is_a_fault() {
        let category = |initial_count, density| Placement {
            initial_count,
            density: NonZeroU32::new(density),
        };
        let mut schedule = Density::new(&[category(-2, 1), category(0, 1)]).unwrap();
        for expected in [0, 0, 0, 1] {
            assert_eq!(schedule.next().unwrap(), Some(expected));
        }
        assert!(matches!(
            Density::new(&[category(i32::MAX, 2), category(0, 3)]),
            Err(PlacementError::Arithmetic)
        ));
        assert!(matches!(
            Density::new(&[category(0, 50_000), category(0, 50_000)]),
            Err(PlacementError::Arithmetic)
        ));
    }
}
