//! DLL RVAs 0x208170, 0x2074a0 and 0x206220: ordered byte-oriented hint patterns.

use super::{
    unique,
    zones::{Diagnostic, DiagnosticKind as Kind, ZoneFault},
    Relation,
};
use regex::bytes::{Captures, Regex, RegexBuilder};
use std::sync::OnceLock;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(super) enum Who {
    Player,
    Neutral,
    All,
    Faction,
    Index(i32),
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub(super) enum Condition {
    Pattern(i32),
    Towns {
        zone: i32,
        who: Who,
        target: Who,
        relation: Relation,
    },
    Restrict {
        who: Who,
        allowed: [bool; 12],
    },
    TownTerrain {
        town_zone: i32,
        terrain_zone: i32,
        who: Who,
        relation: Relation,
    },
    Terrain {
        zone: i32,
        relation: Relation,
    },
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub(super) struct Rule {
    pub own: i32,
    pub condition: Condition,
}

pub(super) fn town_set(text: &[u8]) -> [bool; 12] {
    let mut allowed = std::array::from_fn(|i| text.get(i).is_some_and(|&b| b != b'0'));
    if text.len() < 12 && [0, 1, 3, 5, 6].iter().all(|&i| allowed[i]) {
        allowed[text.len()..].fill(true);
    }
    allowed
}

fn patterns() -> &'static [Regex; 10] {
    static PATTERNS: OnceLock<[Regex; 10]> = OnceLock::new();
    PATTERNS.get_or_init(|| {
        [
            r"^s([0-9]+)$",
            r"^([0-9]+|[pP]|[nN]|[^\r\n]*)(s)([0-9]+|[xX])_([0-9]+|[pP])$",
            r"^([0-9]+|[pP]|[nN]|[^\r\n]*)(d)([0-9]+|[xX])(?:_([0-9]+|[pP]|[nN]))?$",
            r"^([0-9]+|[pP]|[nN]|[^\r\n]*)([sd])t([0-9]+|[xX])$",
            r"^([0-9]+|[pP]|[nN])i([01]+)$",
            r"^([sd])([0-9xX]+)$",
            r"^(s)t([0-9]+|[xX])_([0-9]+|[pP])$",
            r"^(d)t([0-9]+|[xX])(?:_([0-9]+|[pP]|[nN]))?$",
            r"^([sd])([0-9]+)$",
            r"^([sd])r([0-9]+|[xX])$",
        ]
        .map(|pattern| {
            RegexBuilder::new(pattern)
                .unicode(false)
                .build()
                .expect("fixed native hint pattern")
        })
    })
}
fn group<'a>(captures: &Captures<'a>, index: usize) -> &'a [u8] {
    captures.get(index).map_or(b"", |value| value.as_bytes())
}
fn integer(text: &[u8]) -> Result<i32, ZoneFault> {
    let mut prefix = text
        .iter()
        .copied()
        .skip_while(u8::is_ascii_whitespace)
        .peekable();
    if matches!(prefix.peek(), Some(b'+' | b'-')) {
        prefix.next();
    }
    if !prefix.peek().is_some_and(u8::is_ascii_digit) {
        return Err(ZoneFault::Integer(text.to_vec()));
    }
    crate::parse::integer(text.iter().copied()).ok_or_else(|| ZoneFault::Integer(text.to_vec()))
}
fn who(text: &[u8]) -> Result<Who, ZoneFault> {
    Ok(match text {
        b"p" | b"P" => Who::Player,
        b"n" | b"N" => Who::Neutral,
        b"" => Who::All,
        _ => Who::Index(integer(text)?),
    })
}
fn zone(text: &[u8], own: i32) -> Result<i32, ZoneFault> {
    if matches!(text, b"x" | b"X") {
        Ok(own)
    } else {
        integer(text)
    }
}
fn relation(text: &[u8]) -> Relation {
    if text == b"s" {
        Relation::Same
    } else {
        Relation::Different
    }
}
fn self_diagnostic(errors: &mut Vec<Diagnostic>, kind: Kind, own: i32, relation: Relation) {
    unique(
        errors,
        Diagnostic::new(
            kind,
            relation == Relation::Different,
            [own, i32::from(relation == Relation::Same), 0, 0],
        ),
    );
}

fn town_token(
    own: i32,
    token: &[u8],
    errors: &mut Vec<Diagnostic>,
) -> Result<Option<Condition>, ZoneFault> {
    let patterns = patterns();
    if let Some(c) = patterns[0].captures(token) {
        let other = integer(group(&c, 1))?;
        if other == own {
            unique(
                errors,
                Diagnostic::new(Kind::PatternTargetsItself, false, [own, 0, 0, 0]),
            );
            return Ok(None);
        }
        return Ok(Some(Condition::Pattern(other)));
    }
    for pattern in &patterns[1..3] {
        if let Some(c) = pattern.captures(token) {
            let who = who(group(&c, 1))?;
            let relation = relation(group(&c, 2));
            let zone = zone(group(&c, 3), own)?;
            let target = self::who(group(&c, 4))?;
            if zone == own
                && who == target
                && (relation == Relation::Same || !matches!(who, Who::Neutral | Who::All))
            {
                self_diagnostic(errors, Kind::TownRelatedToItself, own, relation);
                return Ok(None);
            }
            return Ok(Some(Condition::Towns {
                zone,
                who,
                target,
                relation,
            }));
        }
    }
    if let Some(c) = patterns[3].captures(token) {
        return Ok(Some(Condition::TownTerrain {
            town_zone: own,
            terrain_zone: zone(group(&c, 3), own)?,
            who: who(group(&c, 1))?,
            relation: relation(group(&c, 2)),
        }));
    }
    if let Some(c) = patterns[4].captures(token) {
        return Ok(Some(Condition::Restrict {
            who: who(group(&c, 1))?,
            allowed: town_set(group(&c, 2)),
        }));
    }
    syntax(errors, own, token);
    Ok(None)
}
fn syntax(errors: &mut Vec<Diagnostic>, own: i32, token: &[u8]) {
    let mut error = Diagnostic::new(Kind::SyntaxError, false, [own, 0, 0, 0]);
    error.text = token.to_vec();
    unique(errors, error);
}
fn terrain_token(
    own: i32,
    token: &[u8],
    errors: &mut Vec<Diagnostic>,
) -> Result<Option<Condition>, ZoneFault> {
    if let Some(c) = patterns()[5].captures(token) {
        let relation = relation(group(&c, 1));
        let zone = zone(group(&c, 2), own)?;
        if zone == own {
            self_diagnostic(errors, Kind::TerrainRelatedToItself, own, relation);
            return Ok(None);
        }
        return Ok(Some(Condition::Terrain { zone, relation }));
    }
    for pattern in &patterns()[6..8] {
        if let Some(c) = pattern.captures(token) {
            return Ok(Some(Condition::TownTerrain {
                town_zone: zone(group(&c, 2), own)?,
                terrain_zone: own,
                who: who(group(&c, 3))?,
                relation: relation(group(&c, 1)),
            }));
        }
    }
    syntax(errors, own, token);
    Ok(None)
}
fn faction_token(
    own: i32,
    token: &[u8],
    errors: &mut Vec<Diagnostic>,
) -> Result<Option<Condition>, ZoneFault> {
    if let Some(c) = patterns()[8].captures(token) {
        let relation = relation(group(&c, 1));
        let zone = integer(group(&c, 2))?;
        if zone == own {
            self_diagnostic(errors, Kind::FactionRelatedToItself, own, relation);
            return Ok(None);
        }
        return Ok(Some(Condition::Towns {
            zone,
            who: Who::Faction,
            target: Who::Faction,
            relation,
        }));
    }
    for pattern in &patterns()[6..8] {
        if let Some(c) = pattern.captures(token) {
            return Ok(Some(Condition::Towns {
                zone: zone(group(&c, 2), own)?,
                who: Who::Faction,
                target: who(group(&c, 3))?,
                relation: relation(group(&c, 1)),
            }));
        }
    }
    if let Some(c) = patterns()[9].captures(token) {
        return Ok(Some(Condition::TownTerrain {
            town_zone: own,
            terrain_zone: zone(group(&c, 2), own)?,
            who: Who::Faction,
            relation: relation(group(&c, 1)),
        }));
    }
    Ok(None)
}

pub(super) fn parse(
    own: i32,
    texts: [&[u8]; 3],
    errors: &mut Vec<Diagnostic>,
    output: &mut Vec<Rule>,
) -> Result<(), ZoneFault> {
    for (text, parser) in texts
        .into_iter()
        .zip([town_token, terrain_token, faction_token])
    {
        for token in text.split(|&b| b == b' ').filter(|token| !token.is_empty()) {
            if let Some(condition) = parser(own, token, errors)? {
                output.push(Rule { own, condition });
            }
        }
    }
    Ok(())
}
