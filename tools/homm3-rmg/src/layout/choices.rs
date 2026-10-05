//! Town query lifetime and terrain/faction choices for the shared layout.

use super::{hints, LayoutError};
use crate::{
    domain::{Level, Terrain},
    raw,
    request::{Request, Town},
    rng::RetailRng,
    rules::Ruleset,
    selection::{select_allowed_town, SelectedTemplate},
    template::Zone,
};
use std::num::NonZeroU32;

#[derive(Debug)]
pub(super) enum ZoneChoices {
    Complete,
    Hints(Box<HintChoices>),
}
#[derive(Debug)]
pub(super) struct HintChoices {
    solution: hints::ZoneSolution,
    selections: Vec<i32>,
}
impl ZoneChoices {
    pub fn from_selected(
        selected: &SelectedTemplate<'_>,
        request: &Request,
        seed: u32,
        clock: impl FnMut(hints::ClockRead) -> Option<i64>,
    ) -> Result<Self, LayoutError> {
        let solution = hints::ZoneSolution::from_selected(selected, request, seed, clock)?;
        let mut selections = Vec::new();
        selections.try_reserve_exact(selected.template().zones().len())?;
        selections.resize(selected.template().zones().len(), 0);
        Ok(Self::Hints(Box::new(HintChoices {
            solution,
            selections,
        })))
    }

    pub fn hints(&self) -> Option<&hints::ZoneSolution> {
        match self {
            Self::Complete => None,
            Self::Hints(state) => Some(&state.solution),
        }
    }

    // RVA 0x1c2550: count successful selector entries per slot, not placed
    // towns. No allowed town means no increment and no hint query. The first
    // player-town query is -1; later queries count neutral towns from zero.
    pub fn town(&mut self, zone: &Zone, rng: &mut RetailRng) -> Result<Option<Town>, LayoutError> {
        let Self::Hints(state) = self else {
            return Ok(select_allowed_town(zone.allowed_towns(), rng));
        };
        if !zone.allowed_towns().contains(&true) {
            return Ok(None);
        }
        let player_towns = zone.towns()[0]
            .initial_count
            .checked_add(zone.towns()[1].initial_count)
            .ok_or(LayoutError::Arithmetic)?
            > 0;
        let count = &mut state.selections[zone.id().index()];
        let instance = if player_towns {
            count.wrapping_sub(1)
        } else {
            *count
        };
        *count = count.wrapping_add(1);
        state
            .solution
            .town(zone.source_number(), instance)?
            .map(|town| {
                Town::parse_for(i32::from(town), Ruleset::HotA181).map_err(LayoutError::Request)
            })
            .transpose()
    }

    // RVA 0x1bcb50 replaces both retail terrain-draw loops. Solver terrains
    // skip water and rock. Ordinary zones have planes 0/1; the native -1
    // temporary-plane path belongs to later island work, not layout.
    pub fn terrain(
        &self,
        zone: &Zone,
        town: Option<Town>,
        level: Level,
        rng: &mut RetailRng,
    ) -> Terrain {
        let Self::Hints(state) = self else {
            return super::choose_terrain(zone, town, level, rng);
        };
        let terrain = if let Some(town) = town.filter(|_| zone.use_native_terrain()) {
            Ruleset::HotA181
                .native_terrain(town.index())
                .expect("admitted faction has a native terrain")
        } else {
            let value = state
                .solution
                .terrain(zone.source_number())
                .map_or(0, i32::from);
            let terrain =
                Terrain::parse_for(if value > 7 { value + 2 } else { value }, Ruleset::HotA181)
                    .expect("solver terrain is an admitted land terrain");
            if terrain == Terrain::Subterranean {
                Terrain::Dirt
            } else {
                terrain
            }
        };
        if level == Level::Underground && terrain != Terrain::Lava {
            Terrain::Subterranean
        } else {
            terrain
        }
    }

    // Ordered hooks at HD 0x53c457: forced neutral, hint faction, alignment,
    // then a corrected sentinel scan and the added Random(0,3) neutral draw.
    pub fn creature_town(
        &self,
        zone: &Zone,
        town: Option<Town>,
        terrain: Terrain,
        rng: &mut RetailRng,
    ) -> Option<Town> {
        let Self::Hints(state) = self else {
            return town.or_else(|| {
                // Complete's unusual scan offers all four entries, including
                // the sentinel and zero padding. Only HotA stops at -1.
                let choice = rng.below(super::CREATURE_TOWN_CHOICES);
                Town::parse(raw::TERRAIN_TOWNS[terrain.index()][choice as usize]).ok()
            });
        };
        if zone.options().force_neutral_creatures {
            return None;
        }
        let faction = state.solution.faction(zone.source_number());
        if faction > -2 {
            return Town::parse_for(faction, Ruleset::HotA181).ok();
        }
        if town.is_some() {
            return town;
        }
        neutral_creature_town(terrain, rng)
    }
}

fn neutral_creature_town(terrain: Terrain, rng: &mut RetailRng) -> Option<Town> {
    let row = TERRAIN_TOWNS[terrain.index()];
    // initializeZones passes m_mapVersion >= 0. Every admitted request version
    // (0,1,2) meets it, including Restoration, so Conflux does not end this scan.
    let count = row.iter().take_while(|&&town| town != -1).count();
    let count = NonZeroU32::new(u32::try_from(count).unwrap())?;
    if rng.below(NonZeroU32::new(4).unwrap()) == 0 {
        return None;
    }
    Town::parse_for(row[rng.below(count) as usize], Ruleset::HotA181).ok()
}

// Registrar's copy/patch sequence, recovered in hota_rmg_setup.cpp. Preserve
// canonical Complete rows and zero padding. Stop at -1 when choosing a faction.
const TERRAIN_TOWNS: [[i32; 4]; 12] = {
    let mut rows = [[-1, 0, 0, 0]; 12];
    let mut terrain = 0;
    while terrain < raw::TERRAIN_TOWNS.len() {
        rows[terrain] = raw::TERRAIN_TOWNS[terrain];
        terrain += 1;
    }
    rows[1][1] = 9;
    rows[1][2] = -1;
    rows[3][1] = 11;
    rows[3][2] = -1;
    rows[4][2] = 9;
    rows[4][3] = -1;
    rows[5][1] = -1;
    rows[5][2] = 0;
    rows[10][0] = 8;
    rows[10][1] = -1;
    rows[11][0] = 10;
    rows[11][1] = -1;
    rows
};

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn neutral_faction_scan_stops_at_sentinels_and_draws_before_singleton_choices() {
        for (terrain, faction) in [
            (Terrain::Highlands, 8),
            (Terrain::Wasteland, 10),
            (Terrain::Rough, 6),
        ] {
            let mut rng = RetailRng::new(1); // 41 % 4 != 0, followed by the singleton draw.
            assert_eq!(
                neutral_creature_town(terrain, &mut rng).unwrap().index(),
                faction
            );
            assert_eq!(rng.draws(), 2);
        }
        let mut rng = RetailRng::new(7); // 61 % 4 != 0.
        let before = rng.checkpoint();
        assert_eq!(neutral_creature_town(Terrain::Rock, &mut rng), None);
        assert_eq!(neutral_creature_town(Terrain::Water, &mut rng), None);
        assert_eq!(rng.checkpoint(), before);
        // Seed 3's first draw is 48, selecting the added neutral branch.
        let mut rng = RetailRng::new(3);
        assert_eq!(neutral_creature_town(Terrain::Highlands, &mut rng), None);
        assert_eq!(rng.draws(), 1);
    }
}
