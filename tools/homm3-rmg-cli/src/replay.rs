//! Small versioned replay records. Keep raw request values, including repaired
//! lobby inputs, separate from human-readable diagnostics appended after them.
use homm3_rmg::{
    behavior::{Behavior, RetailProfile, TownMask},
    raw,
};
use std::{
    error::Error,
    fmt,
    io::{self, Write},
    str::{FromStr, SplitWhitespace},
};

/// Complete deterministic input to the generator. Resource files are supplied
/// separately; replay requires the same installation assets as the original run.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Replay {
    /// Initial CRT seed.
    pub seed: u32,
    /// Runtime semantics and all currently modeled retail residue inputs.
    pub behavior: Behavior,
    /// Original lobby record, before admission and repairs.
    pub request: raw::TRandomMapRequest,
}
/// A replay record is missing a field, has an unknown version, or is malformed.
#[derive(Clone, Copy, Debug)]
pub struct ReplayError;
impl fmt::Display for ReplayError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("invalid RMG replay record (expected homm3-rmg-replay 1 or 2)")
    }
}
impl Error for ReplayError {}
impl Replay {
    /// Parse the complete input section, ignoring only an explicit diagnostics section.
    ///
    /// # Errors
    /// Rejects unknown modes/versions, malformed fields, and extra input tokens.
    pub fn parse(text: &str) -> Result<Self, ReplayError> {
        let mut input = text;
        let mut offset = 0;
        for line in text.split_inclusive('\n') {
            if line.trim_end_matches(['\r', '\n']) == "[diagnostics]" {
                input = &text[..offset];
                break;
            }
            offset += line.len();
        }
        let mut words = input.split_whitespace();
        label(&mut words, "homm3-rmg-replay")?;
        let version = match words.next() {
            Some("1") => 1,
            Some("2") => 2,
            _ => return Err(ReplayError),
        };
        label(&mut words, "seed")?;
        let seed = number(&mut words)?;
        label(&mut words, "behavior")?;
        let behavior = match words.next() {
            Some("hotfix") => Behavior::Hotfix,
            Some("retail") => Behavior::Retail(retail_profile(&mut words, version)?),
            _ => return Err(ReplayError),
        };
        label(&mut words, "seats")?;
        let mut seats = [0; homm3_rmg::request::PLAYER_COUNT];
        for seat in &mut seats {
            *seat = number(&mut words)?;
        }
        label(&mut words, "towns")?;
        let mut towns = [0; homm3_rmg::request::PLAYER_COUNT];
        for town in &mut towns {
            *town = number(&mut words)?;
        }
        label(&mut words, "shape")?;
        let width = number(&mut words)?;
        let height = number(&mut words)?;
        let levels = number(&mut words)?;
        label(&mut words, "players")?;
        let humans = number(&mut words)?;
        let human_teams = number(&mut words)?;
        let computers = number(&mut words)?;
        let computer_teams = number(&mut words)?;
        label(&mut words, "settings")?;
        let water = number(&mut words)?;
        let monsters = number(&mut words)?;
        let version = number(&mut words)?;
        if words.next().is_some() {
            return Err(ReplayError);
        }
        Ok(Self {
            seed,
            behavior,
            request: raw::TRandomMapRequest {
                m_isHumanSeat: seats,
                m_townChoices: towns,
                m_width: width,
                m_height: height,
                m_levels: levels,
                m_humanPlayerCount: humans,
                m_humanTeamCount: human_teams,
                m_computerPlayerCount: computers,
                m_computerTeamCount: computer_teams,
                m_waterContent: water,
                m_monsterStrength: monsters,
                m_mapVersion: version,
            },
        })
    }
    /// Write deterministic input before attempting resource loading or generation.
    ///
    /// # Errors
    /// Returns destination IO errors.
    pub fn write(&self, out: &mut impl Write) -> io::Result<()> {
        writeln!(out, "homm3-rmg-replay 2\nseed {}", self.seed)?;
        match self.behavior {
            Behavior::Hotfix => writeln!(out, "behavior hotfix")?,
            Behavior::Retail(p) => {
                write!(
                    out,
                    "behavior retail {} {} {} ",
                    p.stack_word,
                    p.heap_byte,
                    u8::from(p.water_guards_match_alignment)
                )?;
                if let Some(color) = p.initial_key_tent_color {
                    write!(out, "{color} ")?;
                } else {
                    write!(out, "none ")?;
                }
                if let Some(mask) = p.water_zone_towns {
                    writeln!(out, "{}", mask.bits())?;
                } else {
                    writeln!(out, "none")?;
                }
            }
        }
        let r = &self.request;
        write!(out, "seats")?;
        for seat in r.m_isHumanSeat {
            write!(out, " {seat}")?;
        }
        writeln!(out)?;
        write!(out, "towns")?;
        for town in r.m_townChoices {
            write!(out, " {town}")?;
        }
        writeln!(out)?;
        writeln!(out, "shape {} {} {}", r.m_width, r.m_height, r.m_levels)?;
        writeln!(
            out,
            "players {} {} {} {}",
            r.m_humanPlayerCount,
            r.m_humanTeamCount,
            r.m_computerPlayerCount,
            r.m_computerTeamCount
        )?;
        writeln!(
            out,
            "settings {} {} {}",
            r.m_waterContent, r.m_monsterStrength, r.m_mapVersion
        )
    }
}
fn retail_profile(
    words: &mut SplitWhitespace<'_>,
    version: u8,
) -> Result<RetailProfile, ReplayError> {
    let stack_word = number(words)?;
    let heap_byte = number(words)?;
    let water_guards_match_alignment = match words.next() {
        Some("0") => false,
        Some("1") => true,
        _ => return Err(ReplayError),
    };
    let initial_key_tent_color = optional_number(words)?;
    let water_zone_towns = if version == 1 {
        // v1 followed authored C++: all towns, even with a zero heap fill.
        Some(TownMask::ALL)
    } else {
        optional_number(words)?
            .map(|bits| TownMask::parse(bits).ok_or(ReplayError))
            .transpose()?
    };
    Ok(RetailProfile {
        stack_word,
        heap_byte,
        water_guards_match_alignment,
        initial_key_tent_color,
        water_zone_towns,
    })
}
fn optional_number<T: FromStr>(words: &mut SplitWhitespace<'_>) -> Result<Option<T>, ReplayError> {
    match words.next() {
        Some("none") => Ok(None),
        Some(n) => n.parse().map(Some).map_err(|_| ReplayError),
        None => Err(ReplayError),
    }
}
fn label(words: &mut SplitWhitespace<'_>, expected: &str) -> Result<(), ReplayError> {
    if words.next() == Some(expected) {
        Ok(())
    } else {
        Err(ReplayError)
    }
}
fn number<T: FromStr>(words: &mut SplitWhitespace<'_>) -> Result<T, ReplayError> {
    words
        .next()
        .ok_or(ReplayError)?
        .parse()
        .map_err(|_| ReplayError)
}

#[cfg(test)]
mod tests {
    use super::*;
    use homm3_rmg::request::{default_record, Levels, MapSize};

    #[test]
    fn round_trip_preserves_raw_inputs_and_line_endings() {
        let mut request = default_record(MapSize::ExtraLarge, Levels::Underground);
        request.m_isHumanSeat[3] = 255; // Keep original noncanonical truth bytes.
        request.m_humanPlayerCount = 0; // Admission repairs must not rewrite replay.
        request.m_computerPlayerCount = 0;
        request.m_townChoices[5] = raw::TOWN_CONFLUX;
        for behavior in [
            Behavior::Hotfix,
            Behavior::Retail(RetailProfile::default()),
            Behavior::Retail(RetailProfile {
                stack_word: u32::MAX,
                heap_byte: 0xcd,
                water_guards_match_alignment: true,
                initial_key_tent_color: Some(-7),
                water_zone_towns: Some(TownMask::parse(0b1_0000_0010).unwrap()),
            }),
        ] {
            let replay = Replay {
                seed: u32::MAX,
                behavior,
                request,
            };
            let mut bytes = Vec::new();
            replay.write(&mut bytes).unwrap();
            let text = String::from_utf8(bytes).unwrap();
            assert_eq!(Replay::parse(&text).unwrap(), replay);
            assert!(Replay::parse(&format!("{text}unexpected")).is_err());
            let report = format!("{text}\n[diagnostics]\nfailure arbitrary text\n");
            assert_eq!(Replay::parse(&report).unwrap(), replay);
            assert_eq!(
                Replay::parse(&report.replace('\n', "\r\n")).unwrap(),
                replay
            );
            assert!(
                Replay::parse(&text.replacen("homm3-rmg-replay 2", "homm3-rmg-replay 3", 1))
                    .is_err()
            );
        }
    }

    #[test]
    fn legacy_replay_preserves_authored_water_draws_and_v2_checks_masks() {
        let input = "homm3-rmg-replay 1\nseed 100\nbehavior retail 0 0 0 none\nseats 0 0 0 0 0 0 0 0\ntowns -1 -1 -1 -1 -1 -1 -1 -1\nshape 108 108 1\nplayers 2 2 0 8\nsettings 2 0 0\n";
        let old = Replay::parse(input).unwrap();
        let Behavior::Retail(profile) = old.behavior else {
            panic!("retail replay")
        };
        assert_eq!(profile.water_zone_towns, Some(TownMask::ALL));
        let mut bytes = Vec::new();
        old.write(&mut bytes).unwrap();
        assert_eq!(
            Replay::parse(std::str::from_utf8(&bytes).unwrap()).unwrap(),
            old
        );

        let v2 = input.replacen("replay 1", "replay 2", 1);
        assert!(Replay::parse(&v2).is_err()); // The new field is required.
        for mask in ["none", "0", "1", "256", "511"] {
            let replay =
                Replay::parse(&v2.replace("0 0 0 none", &format!("0 0 0 none {mask}"))).unwrap();
            let mut bytes = Vec::new();
            replay.write(&mut bytes).unwrap();
            assert_eq!(
                Replay::parse(std::str::from_utf8(&bytes).unwrap()).unwrap(),
                replay
            );
        }
        for mask in ["512", "65535", "65536", "-1"] {
            assert!(
                Replay::parse(&v2.replace("0 0 0 none", &format!("0 0 0 none {mask}"))).is_err()
            );
        }
        assert_eq!(
            Replay::parse(&input.replace("retail 0 0 0 none", "hotfix"))
                .unwrap()
                .behavior,
            Behavior::Hotfix
        );
    }
}
