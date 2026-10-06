//! Source guard-strength scaling, shared by connections and later zone objects.
use super::PlacementError;
use crate::{raw, request::MonsterStrength};

/// Index admitted against the canonical six guard-strength table entries.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct GuardStrength(u8);
impl GuardStrength {
    /// Admit the zone/map-combined strength without indexing unchecked input.
    #[must_use]
    pub fn parse(value: i32) -> Option<Self> {
        u8::try_from(value)
            .ok()
            .filter(|&v| u32::from(v) < raw::RMG_GUARD_STRENGTH_COUNT)
            .map(Self)
    }
    /// Convert the unscaled value using each threshold's separately rounded term.
    /// Values below the source minimum suppress ordinary monster guards.
    ///
    /// # Errors
    /// Reports signed overflow at the native subtraction, product or sum, even
    /// when widening the expression would give an otherwise representable result.
    #[expect(
        clippy::missing_panics_doc,
        reason = "canonical minimum guard value fits i32"
    )]
    pub fn scale(self, value: i32) -> Result<i32, PlacementError> {
        let index = usize::from(self.0);
        let low = term(
            value,
            raw::GUARD_THRESHOLD_LOW[index],
            raw::GUARD_SCALE_LOW[index],
        )?;
        let high = term(
            value,
            raw::GUARD_THRESHOLD_HIGH[index],
            raw::GUARD_SCALE_HIGH[index],
        )?;
        let result = low.checked_add(high).ok_or(PlacementError::Arithmetic)?;
        Ok(
            if result < i32::try_from(raw::RMG_MINIMUM_GUARD_VALUE).unwrap() {
                0
            } else {
                result
            },
        )
    }
}
impl From<MonsterStrength> for GuardStrength {
    fn from(value: MonsterStrength) -> Self {
        Self(value.get())
    }
}
impl super::PlacementMap<'_> {
    pub(super) fn zone_guard_value(
        &self,
        value: i32,
        zone: crate::boundaries::BoundaryZone,
    ) -> Result<i32, PlacementError> {
        use crate::template::ZoneMonsters;
        let map = self.coverage().map();
        let Some(rules) = map.template_zone(&zone) else {
            // Added water zones explicitly have no monsters.
            return Ok(0);
        };
        let strength = match rules.monsters() {
            ZoneMonsters::None => return Ok(0),
            ZoneMonsters::Weak => raw::RMG_ZONE_MONSTERS_WEAK,
            ZoneMonsters::Average => raw::RMG_ZONE_MONSTERS_AVERAGE,
            ZoneMonsters::Strong => raw::RMG_ZONE_MONSTERS_STRONG,
        };
        let strength = i32::try_from(strength).map_err(|_| PlacementError::Arithmetic)?
            + i32::from(map.request().strength().get())
            - i32::try_from(raw::RMG_ZONE_MONSTERS_AVERAGE)
                .map_err(|_| PlacementError::Arithmetic)?;
        let maximum = i32::try_from(raw::RMG_STRONGEST_GUARD_STRENGTH)
            .map_err(|_| PlacementError::Arithmetic)?;
        GuardStrength::parse(strength.clamp(0, maximum))
            .ok_or(PlacementError::Arithmetic)?
            .scale(value)
    }
}
fn term(value: i32, threshold: i32, scale: i32) -> Result<i32, PlacementError> {
    if value <= threshold {
        return Ok(0);
    }
    value
        .checked_sub(threshold)
        .and_then(|v| v.checked_mul(scale))
        .map(|v| v / 4)
        .ok_or(PlacementError::Arithmetic)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn undefined_native_product_faults_even_when_widened_result_would_fit() {
        assert!(GuardStrength::parse(-1).is_none());
        assert!(GuardStrength::parse(6).is_none());
        let normal = GuardStrength::parse(3).unwrap();
        // Native multiplies (536871912 - 1000) by four before dividing.
        // Both widened terms and their sum would fit after division.
        assert!(matches!(
            normal.scale(536_871_912),
            Err(PlacementError::Arithmetic)
        ));
        assert_eq!(GuardStrength::parse(0).unwrap().scale(i32::MAX).unwrap(), 0);
        assert_eq!(normal.scale(i32::MIN).unwrap(), 0);
    }
}
