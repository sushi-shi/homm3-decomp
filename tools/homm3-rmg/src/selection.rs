//! Ordered template selection and player-to-template assignment.

use crate::{
    raw,
    request::{Request, Water, WaterChoice, PLAYER_COUNT},
    rng::RetailRng,
    template::{PlayerSlot, RetailTemplateFault, Template, TemplateCandidate},
};
use std::{error::Error, fmt, num::NonZeroU32};

const WATER_CHOICES: NonZeroU32 = match NonZeroU32::new(raw::RMG_WATER_RANDOM) {
    Some(count) => count,
    None => panic!("the source water domain must not be empty"),
};

/// A playable colour, distinct from a template's player slot.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Player(u8);
impl Player {
    /// Index into request colour arrays.
    #[must_use]
    pub const fn index(self) -> usize {
        self.0 as usize
    }
}

/// Failure to select and seat the requested players.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SelectionError {
    /// No templates are eligible. Does not consume a selection draw.
    NoTemplates,
    /// Candidate count exceeds the original 32-bit selection domain.
    TooManyTemplates,
    /// A selected retail candidate cannot be executed safely.
    RetailTemplate(RetailTemplateFault),
    /// Retail counts repeated player zones but then runs out of distinct slots.
    InsufficientSlots,
}
impl fmt::Display for SelectionError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::NoTemplates => f.write_str("no eligible random-map templates"),
            Self::TooManyTemplates => f.write_str("template count exceeds 32-bit selection domain"),
            Self::RetailTemplate(fault) => fault.fmt(f),
            Self::InsufficientSlots => {
                f.write_str("selected template has too few distinct player slots")
            }
        }
    }
}
impl Error for SelectionError {}

/// Resolve random water once, before template filtering, as in the constructor.
#[must_use]
pub fn resolve_water(choice: WaterChoice, rng: &mut RetailRng) -> Water {
    match choice {
        WaterChoice::Fixed(water) => water,
        WaterChoice::Random => match rng.below(WATER_CHOICES) {
            raw::RMG_WATER_NONE => Water::None,
            raw::RMG_WATER_NORMAL => Water::Normal,
            _ => Water::Islands,
        },
    }
}

/// A borrowed template with all requested players assigned to valid slots.
#[derive(Debug)]
pub struct SelectedTemplate<'a> {
    template: &'a Template<'a>,
    players: [Option<Player>; PLAYER_COUNT],
    source_index: usize,
}
impl<'a> SelectedTemplate<'a> {
    /// Select in source order with one retail RNG draw, including singletons.
    ///
    /// Fixed human colours come first in ascending order, followed by all
    /// other colours. Human slots are assigned before computer-only slots.
    ///
    /// # Errors
    /// Reports no candidates, an unsupported selected retail candidate, or
    /// insufficient distinct player slots. A selected fault still consumes
    /// the selection draw.
    pub fn select(
        candidates: &'a [TemplateCandidate<'a>],
        request: &Request,
        rng: &mut RetailRng,
    ) -> Result<Self, SelectionError> {
        let count =
            u32::try_from(candidates.len()).map_err(|_| SelectionError::TooManyTemplates)?;
        let count = NonZeroU32::new(count).ok_or(SelectionError::NoTemplates)?;
        let source_index = rng.below(count) as usize;
        let template = match &candidates[source_index] {
            TemplateCandidate::Ready(template) => template,
            TemplateCandidate::RetailFault { fault, .. } => {
                return Err(SelectionError::RetailTemplate(*fault))
            }
        };
        let (humans, mut available) = template.player_slots();
        let mut player_order = [Player(0); PLAYER_COUNT];
        let mut order = 0;
        for fixed in [true, false] {
            for (index, &is_fixed) in (0_u8..).zip(request.human_seats()) {
                if is_fixed == fixed {
                    player_order[order] = Player(index);
                    order += 1;
                }
            }
        }
        let mut players = [None; PLAYER_COUNT];
        let mut human_slots = humans
            .iter()
            .enumerate()
            .filter_map(|(slot, &used)| used.then_some(slot));
        let human_count = usize::from(request.human_players().get());
        let total = human_count + usize::from(request.computer_players().get());
        for &player in &player_order[..human_count] {
            let slot = human_slots
                .next()
                .ok_or(SelectionError::InsufficientSlots)?;
            players[slot] = Some(player);
            available[slot] = false;
        }
        let mut computer_slots = available
            .iter()
            .enumerate()
            .filter_map(|(slot, &used)| used.then_some(slot));
        for &player in &player_order[human_count..total] {
            let slot = computer_slots
                .next()
                .ok_or(SelectionError::InsufficientSlots)?;
            players[slot] = Some(player);
        }
        Ok(Self {
            template,
            players,
            source_index,
        })
    }

    /// Template with validated zone domains.
    #[must_use]
    pub const fn template(&self) -> &'a Template<'a> {
        self.template
    }

    /// Original candidate index, for deterministic replay diagnostics.
    #[must_use]
    pub const fn source_index(&self) -> usize {
        self.source_index
    }

    /// Player assigned to a template slot, or no owner for an unused slot.
    #[must_use]
    pub fn player(&self, slot: PlayerSlot) -> Option<Player> {
        self.players[slot.index()]
    }

    /// Complete assignment in template slot order.
    #[must_use]
    pub const fn players(&self) -> &[Option<Player>; PLAYER_COUNT] {
        &self.players
    }
}
