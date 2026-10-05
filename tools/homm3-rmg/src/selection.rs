//! Ordered template selection and player-to-template assignment.

pub use crate::request::Player;
use crate::{
    raw,
    request::{Request, Water, WaterChoice, PLAYER_COUNT},
    rng::RetailRng,
    template::{PerSlot, PlayerSlot, RetailTemplateFault, SlotUse, Template, TemplateCandidate},
};
use std::{error::Error, fmt, num::NonZeroU32};

const WATER_CHOICES: NonZeroU32 = match NonZeroU32::new(raw::RMG_WATER_RANDOM) {
    Some(count) => count,
    None => panic!("the source water domain must not be empty"),
};

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
    players: PerSlot<Option<Player>>,
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
        let slots = template.player_slots();
        let seats = request.human_seats();
        let colours = |human: bool| {
            seats
                .iter()
                .filter(move |&(_, &seat)| seat == human)
                .map(|(player, _)| player)
        };
        let mut player_order = colours(true).chain(colours(false));
        let mut players = PerSlot::new([None; PLAYER_COUNT]);
        let mut human_slots = slots
            .iter()
            .filter(|(_, use_)| **use_ == Some(SlotUse::Human))
            .map(|(slot, _)| slot);
        let humans = usize::from(request.human_players().get());
        let computers = usize::from(request.computer_players().get());
        for player in player_order.by_ref().take(humans) {
            let slot = human_slots
                .next()
                .ok_or(SelectionError::InsufficientSlots)?;
            players[slot] = Some(player);
        }
        // Unused human-capable slots remain available to computer players.
        let seated_humans = players;
        let mut computer_slots = slots
            .iter()
            .filter(|&(slot, use_)| use_.is_some() && seated_humans[slot].is_none())
            .map(|(slot, _)| slot);
        for player in player_order.take(computers) {
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
        self.players[slot]
    }

    /// Complete assignment in template slot order.
    #[must_use]
    pub const fn players(&self) -> &PerSlot<Option<Player>> {
        &self.players
    }
}
