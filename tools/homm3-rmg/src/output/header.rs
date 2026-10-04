//! Native header and team assignment, including format-specific defaults.
use super::{expansion, OutputFault, Writer};
use crate::{
    boundaries::ZoneOrigin,
    generation::GeneratedMap,
    hero::HeroId,
    raw,
    request::{MapVersion, Town, TownChoice, Water, PLAYER_COUNT},
    rng::RetailRng,
    template::ZoneRole,
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
    let mut human = [false; PLAYER_COUNT];
    let mut computer = [false; PLAYER_COUNT];
    let mut alignments = [0_u32; PLAYER_COUNT];
    let mut towns = [None; PLAYER_COUNT];
    for zone in boundary.zones() {
        let ZoneOrigin::Template(id) = zone.origin() else {
            continue;
        };
        let role = boundary.template().zones()[id.index()].role();
        let slot = match role {
            ZoneRole::Human(s) | ZoneRole::Computer(s) => Some(s),
            ZoneRole::Treasure(s) | ZoneRole::Junction(s) => s,
        };
        let Some(player) = slot.and_then(|s| boundary.player(s)) else {
            continue;
        };
        let Some(town) = zone.primary_town() else {
            continue;
        };
        let p = player.index();
        if matches!(role, ZoneRole::Human(_)) && !human[p] {
            human[p] = true;
            towns[p] = Some(town);
        }
        if matches!(role, ZoneRole::Computer(_)) && !computer[p] {
            computer[p] = true;
            towns[p] = Some(town);
        }
        // Retail SHL masks the shift count to five bits: neutral (-1) sets
        // bit31, which is then discarded by the u8/u16 on-disk field.
        alignments[p] |= 1 << zone.alignment().map_or(31, Town::index);
    }
    let mut surplus = human
        .iter()
        .filter(|&&b| b)
        .count()
        .saturating_sub(usize::from(request.human_players().get()));
    for p in (0..PLAYER_COUNT).rev() {
        if human[p] && !request.human_seats()[p] && surplus > 0 {
            computer[p] = true;
            human[p] = false;
            surplus -= 1;
        }
    }
    if request.behavior().is_hotfix() {
        for p in 0..PLAYER_COUNT {
            if human[p] {
                computer[p] = false;
            }
        }
    }
    let mut human_count = 0;
    let mut computer_count = 0;
    for p in 0..PLAYER_COUNT {
        let playing = human[p] || computer[p];
        out.bytes(&[u8::from(human[p]), u8::from(playing), 0])?;
        if version == MapVersion::ShadowOfDeath {
            out.byte(0)?;
        }
        if expansion(version) {
            out.u16(alignments[p] as u16)?;
        } else {
            out.byte(alignments[p] as u8)?;
        }
        out.byte(0)?; // random alignment
        out.byte(u8::from(playing))?;
        if playing {
            if human[p] {
                human_count += 1;
            } else {
                computer_count += 1;
            }
            if expansion(version) {
                out.bytes(&[1, 255])?;
            } // generate hero, neutral town type
            out.position(towns[p].expect("player admission records its main town"))?;
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
        let mut teams = [0; PLAYER_COUNT];
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
        out.bytes(&teams)?;
    }
    let heroes = if expansion(version) {
        raw::RMG_HERO_COUNT
    } else {
        raw::RMG_ROE_HERO_COUNT
    } as usize;
    out.bits(heroes, |id| {
        !map.treasures()
            .hero_disabled(HeroId::parse(i32::try_from(id).unwrap()).unwrap())
    })?;
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
        out.bits(count, |id| {
            map.treasures().artifacts().entries()[id]
                .combination()
                .is_some()
                || id == raw::ARTIFACT_ARMAGEDDONS_BLADE as usize
                || id == raw::RMG_ARTIFACT_VIAL_OF_DRAGON_BLOOD as usize
        })?;
    }
    if version == MapVersion::ShadowOfDeath {
        out.zero((raw::HERO_SPELL_COUNT as usize).div_ceil(8))?;
        out.zero((raw::kNumSecSkills as usize).div_ceil(8))?;
        out.zero(raw::RMG_HERO_COUNT as usize)?;
    }
    Ok(())
}
fn assign_teams(
    count: usize,
    players: usize,
    first: usize,
    mask: [bool; PLAYER_COUNT],
    teams: &mut [u8; PLAYER_COUNT],
    rng: &mut RetailRng,
) -> Result<(), OutputFault> {
    let mut quotas = [0; PLAYER_COUNT];
    for (team, quota) in quotas.iter_mut().take(count).enumerate() {
        *quota = players / count + usize::from(players % count > team);
    }
    for player in 0..PLAYER_COUNT {
        if !mask[player] {
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
        for p in 0..PLAYER_COUNT {
            if request.human_seats()[p] {
                text.write_all(b", ")?;
                text.write_all(raw::PLAYER_NAMES[p])?;
                text.write_all(b" is human")?;
            }
            if matches!(request.towns()[p], TownChoice::Fixed(_)) {
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
