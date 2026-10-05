//! Native header and team assignment, including format-specific defaults.
use super::{expansion, OutputFault, Writer};
use crate::{
    domain::WorldPosition,
    generation::GeneratedMap,
    hero::HeroId,
    raw,
    request::{MapVersion, PerColour, Player, Town, TownChoice, Water, PLAYER_COUNT},
    rng::RetailRng,
    template::{Zone, ZoneRole},
};
use std::io::{Cursor, Write};

#[expect(
    clippy::too_many_lines,
    reason = "keep format gates and header field order together for comparison with the native writer"
)]
pub(super) fn write(
    map: &GeneratedMap<'_>,
    out: &mut Writer<'_, impl Write>,
    rng: &mut RetailRng,
) -> Result<(), OutputFault> {
    let boundary = map.map().coverage().map();
    let request = boundary.request();
    let version = request.version();
    out.u32(match version {
        MapVersion::Restoration => raw::MAP_FORMAT_RESTORATION_OF_ERATHIA,
        MapVersion::ArmageddonsBlade => raw::MAP_FORMAT_ARMAGEDDONS_BLADE,
        MapVersion::ShadowOfDeath => raw::MAP_FORMAT_SHADOW_OF_DEATH,
    })?;
    out.byte(1)?; // playable
    out.u32(request.size().dimension())?;
    out.byte(u8::from(request.levels().count() > 1))?;
    out.string(b"Random Map")?;
    description(map, out)?;
    out.byte(1)?; // difficulty
    if expansion(version) {
        out.byte(0)?;
    } // hero level limit
    let mut seats: PerColour<Option<Seat>> = PerColour::new([None; PLAYER_COUNT]);
    let mut alignments = PerColour::new([0_u32; PLAYER_COUNT]);
    for zone in boundary.zones() {
        let Some(role) = boundary.template_zone(zone).map(Zone::role) else {
            continue;
        };
        let Some(player) = role.owner().and_then(|s| boundary.player(s)) else {
            continue;
        };
        let Some(town) = zone.primary_town() else {
            continue;
        };
        if let Some(kind) = SeatKind::of(role) {
            Seat::occupy(&mut seats[player], kind, town);
        }
        // Retail SHL masks the shift count to five bits: neutral (-1) sets
        // bit31, which is then discarded by the u8/u16 on-disk field.
        alignments[player] |= 1 << zone.alignment().map_or(31, Town::index);
    }
    let is_human = |seat: &Option<Seat>| seat.is_some_and(|seat| seat.kind.is_human());
    let mut surplus = seats
        .iter()
        .filter(|(_, seat)| is_human(seat))
        .count()
        .saturating_sub(usize::from(request.human_players().get()));
    for player in Player::all().rev() {
        if let Some(seat) = &mut seats[player] {
            if seat.kind.is_human() && !request.human_seats()[player] && surplus > 0 {
                seat.kind = SeatKind::Computer;
                surplus -= 1;
            }
        }
    }
    if request.behavior().is_hotfix() {
        for player in Player::all() {
            if let Some(seat) = &mut seats[player] {
                seat.kind = seat.kind.hotfix();
            }
        }
    }
    let mut human_count = 0;
    let mut computer_count = 0;
    for (player, seat) in seats.iter() {
        let human = is_human(seat);
        out.bytes(&[u8::from(human), u8::from(seat.is_some()), 0])?;
        if version == MapVersion::ShadowOfDeath {
            out.byte(0)?;
        }
        if expansion(version) {
            out.u16(alignments[player] as u16)?;
        } else {
            out.byte(alignments[player] as u8)?;
        }
        out.byte(0)?; // random alignment
        out.byte(u8::from(seat.is_some()))?;
        if let Some(seat) = seat {
            if human {
                human_count += 1;
            } else {
                computer_count += 1;
            }
            if expansion(version) {
                out.bytes(&[1, 255])?;
            } // generate hero, neutral town type
            out.position(seat.main_town)?;
        }
        out.bytes(&[0, 255])?; // random hero, custom hero sentinel
        if expansion(version) {
            out.zero(5)?;
        } // placeholder heroes and hero count
    }
    out.bytes(&[255, 255])?; // ordinary victory/loss
    let mut human_teams = usize::from(request.human_teams().get());
    let mut computer_teams = usize::from(request.computer_teams().get());
    if human_teams == 0 {
        human_teams = human_count;
    }
    if computer_teams == 0 {
        computer_teams = computer_count;
    }
    if computer_count == 0 {
        human_teams = human_teams.max(2);
    }
    if human_teams >= human_count && computer_teams >= computer_count {
        out.byte(0)?;
    } else {
        human_teams = human_teams.max(1).min(human_count);
        computer_teams = computer_teams.max(1).min(computer_count);
        // A retail human-and-computer seat is assigned in both passes.
        let human = seats.map(|seat| seat.is_some_and(|seat| seat.kind.is_human()));
        let computer = seats.map(|seat| seat.is_some_and(|seat| seat.kind.is_computer()));
        let mut teams = PerColour::new([0; PLAYER_COUNT]);
        assign_teams(human_teams, human_count, 0, human, &mut teams, rng)?;
        assign_teams(
            computer_teams,
            computer_count,
            human_teams,
            computer,
            &mut teams,
            rng,
        )?;
        out.byte((human_teams + computer_teams) as u8)?;
        out.bytes(teams.values())?;
    }
    let heroes = if expansion(version) {
        raw::RMG_HERO_COUNT
    } else {
        raw::RMG_ROE_HERO_COUNT
    } as usize;
    out.bits(
        HeroId::all()
            .take(heroes)
            .map(|hero| !map.treasures().hero_disabled(hero)),
    )?;
    if expansion(version) {
        out.u32(0)?;
    }
    if version == MapVersion::ShadowOfDeath {
        out.byte(0)?;
    }
    out.zero(31)?;
    if expansion(version) {
        let count = if version == MapVersion::ShadowOfDeath {
            raw::ARTIFACT_COUNT
        } else {
            raw::ARTIFACT_ANGELIC_ALLIANCE
        } as usize;
        out.bits(
            map.treasures().artifacts().entries()[..count]
                .iter()
                .enumerate()
                .map(|(id, artifact)| {
                    artifact.combination().is_some()
                        || id == raw::ARTIFACT_ARMAGEDDONS_BLADE as usize
                        || id == raw::RMG_ARTIFACT_VIAL_OF_DRAGON_BLOOD as usize
                }),
        )?;
    }
    if version == MapVersion::ShadowOfDeath {
        out.zero((raw::HERO_SPELL_COUNT as usize).div_ceil(8))?;
        out.zero((raw::kNumSecSkills as usize).div_ceil(8))?;
        out.zero(raw::RMG_HERO_COUNT as usize)?;
    }
    Ok(())
}
/// Lobby seat kinds a colour's starting zones claim.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum SeatKind {
    Human,
    Computer,
    /// The colour owns both a human and a computer starting zone. Retail
    /// writes it as human but assigns it a team in both passes; hotfix
    /// resolves it to `Human`.
    HumanAndComputer,
}
impl SeatKind {
    const fn of(role: ZoneRole) -> Option<Self> {
        match role {
            ZoneRole::Human(_) => Some(Self::Human),
            ZoneRole::Computer(_) => Some(Self::Computer),
            ZoneRole::Treasure(_) | ZoneRole::Junction(_) => None,
        }
    }
    const fn is_human(self) -> bool {
        matches!(self, Self::Human | Self::HumanAndComputer)
    }
    const fn is_computer(self) -> bool {
        matches!(self, Self::Computer | Self::HumanAndComputer)
    }
    const fn hotfix(self) -> Self {
        match self {
            Self::HumanAndComputer => Self::Human,
            kind => kind,
        }
    }
}

/// A playing colour's header seat: its kind and the main town written for it.
#[derive(Clone, Copy, Debug)]
struct Seat {
    kind: SeatKind,
    main_town: WorldPosition,
}
impl Seat {
    /// Native records the first zone of each kind; a colour's main town is
    /// the primary town of whichever of those came last in zone order.
    fn occupy(seat: &mut Option<Self>, kind: SeatKind, town: WorldPosition) {
        match seat {
            None => {
                *seat = Some(Self {
                    kind,
                    main_town: town,
                });
            }
            Some(seat) if seat.kind != kind && seat.kind != SeatKind::HumanAndComputer => {
                seat.kind = SeatKind::HumanAndComputer;
                seat.main_town = town;
            }
            Some(_) => {}
        }
    }
}

fn assign_teams(
    count: usize,
    players: usize,
    first: usize,
    mask: PerColour<bool>,
    teams: &mut PerColour<u8>,
    rng: &mut RetailRng,
) -> Result<(), OutputFault> {
    let mut quotas = [0; PLAYER_COUNT];
    for (team, quota) in quotas.iter_mut().take(count).enumerate() {
        *quota = players / count + usize::from(players % count > team);
    }
    for (player, &member) in mask.iter() {
        if !member {
            continue;
        }
        let nonempty = quotas[..count].iter().filter(|&&q| q > 0).count();
        let draw = rng.draw(); // draw precedes retail's possible zero-divisor fault
        if nonempty == 0 {
            return Err(OutputFault::EmptyTeamPool);
        }
        let selected = draw as usize % nonempty;
        let (team, quota) = quotas[..count]
            .iter_mut()
            .enumerate()
            .filter(|(_, q)| **q > 0)
            .nth(selected)
            .unwrap();
        teams[player] = (first + team) as u8;
        *quota -= 1;
    }
    Ok(())
}
fn description(
    map: &GeneratedMap<'_>,
    out: &mut Writer<'_, impl Write>,
) -> Result<(), OutputFault> {
    let boundary = map.map().coverage().map();
    let request = boundary.request();
    let mut bytes = [0; 1024];
    // Keep room for native sprintf/strcat's terminating NUL. Byte-oriented
    // formatting preserves legacy resource encodings and embedded-NUL behavior.
    let capacity = if request.behavior().is_hotfix() {
        1023
    } else {
        499
    };
    let mut text = Cursor::new(&mut bytes[..capacity]);
    let result = (|| -> std::io::Result<()> {
        text.write_all(b"Map created by the Random Map Generator.  Template was ")?;
        let mut name = boundary.template().name();
        if request.behavior().is_hotfix() {
            name = &name[..name.len().min(255)];
        }
        name = &name[..name.iter().position(|&b| b == 0).unwrap_or(name.len())];
        text.write_all(name)?;
        let water = match boundary.water() {
            Water::None => 0,
            Water::Normal => 1,
            Water::Islands => 2,
        };
        write!(
            text,
            ", Random seed was {}, size {}, levels {}, humans {}, computers {}, water ",
            i32::from_ne_bytes(map.report().seed().to_ne_bytes()),
            request.size().dimension(),
            request.levels().count(),
            request.human_players().get(),
            request.computer_players().get()
        )?;
        text.write_all(raw::WATER_NAMES[water])?;
        write!(text, ", monsters {}", request.strength().get())?;
        text.write_all(match request.version() {
            MapVersion::Restoration => b", original map",
            MapVersion::ArmageddonsBlade => b", first expansion map",
            MapVersion::ShadowOfDeath => b", second expansion map",
        })?;
        for player in Player::all() {
            let p = player.index();
            if request.human_seats()[player] {
                text.write_all(b", ")?;
                text.write_all(raw::PLAYER_NAMES[p])?;
                text.write_all(b" is human")?;
            }
            if matches!(request.towns()[player], TownChoice::Fixed(_)) {
                text.write_all(b", ")?;
                text.write_all(raw::PLAYER_NAMES[p])?;
                text.write_all(b" town choice is ")?;
                text.write_all(raw::TOWN_NAMES[p])?; // Native bug in both modes: player index, not selected faction.
            }
        }
        Ok(())
    })();
    result.map_err(|_| OutputFault::DescriptionOverflow)?;
    let len = text.position() as usize;
    out.string(&bytes[..len])
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{domain::Level, geometry::Point};

    // The native writer's independent first-human/first-computer flags,
    // with the main town overwritten by whichever flag is set later.
    fn native_flags(roles: &[SeatKind]) -> (bool, bool, Option<i32>) {
        let (mut human, mut computer, mut town) = (false, false, None);
        for (x, &kind) in (0..).zip(roles) {
            if kind == SeatKind::Human && !human {
                human = true;
                town = Some(x);
            }
            if kind == SeatKind::Computer && !computer {
                computer = true;
                town = Some(x);
            }
        }
        (human, computer, town)
    }

    #[test]
    fn seats_match_native_flags_for_every_role_order() {
        let kinds = [SeatKind::Human, SeatKind::Computer];
        for length in 0..=4 {
            for bits in 0..1_u32 << length {
                let roles: Vec<_> = (0..length)
                    .map(|i| kinds[(bits >> i & 1) as usize])
                    .collect();
                let mut seat = None;
                for (x, &kind) in (0..).zip(&roles) {
                    let town = WorldPosition {
                        point: Point::new(x, 0),
                        level: Level::Surface,
                    };
                    Seat::occupy(&mut seat, kind, town);
                }
                let actual = seat.map_or((false, false, None), |seat: Seat| {
                    (
                        seat.kind.is_human(),
                        seat.kind.is_computer(),
                        Some(seat.main_town.point.x),
                    )
                });
                assert_eq!(actual, native_flags(&roles), "{roles:?}");
            }
        }
        assert_eq!(SeatKind::HumanAndComputer.hotfix(), SeatKind::Human);
    }
}
