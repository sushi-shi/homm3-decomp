use super::*;
use std::fmt::Write;

fn spells(enabled: Option<usize>) -> SpellCatalog {
    let enabled_row = enabled.map(|id| {
        5 + id
            + if id < 10 {
                0
            } else if id < 70 {
                3
            } else {
                6
            }
    });
    let mut text = String::new();
    for row in 0..92 {
        for column in 0..33 {
            if column > 0 {
                text.push('\t');
            }
            let value = match column {
                2 => "1",
                6 if Some(row) == enabled_row => "x",
                3..=6 => " ",
                _ => "0",
            };
            write!(text, "{value}").unwrap();
        }
        text.push_str("\r\n");
    }
    SpellCatalog::parse(text.as_bytes()).unwrap()
}

#[test]
fn scroll_empty_and_singleton_pools_both_consume_the_native_draw() {
    let mut rng = RetailRng::new(1);
    let mut expected = rng.clone();
    expected.draw();
    assert!(matches!(
        select_scroll(&spells(None), 1, &mut rng),
        Err(TreasureGenerationError::EmptyScroll)
    ));
    assert_eq!(rng, expected);
    let id = raw::SPELL_FLAGS
        .iter()
        .take(raw::HERO_SPELL_COUNT as usize)
        .position(|&flags| flags & raw::RMG_SPELL_DISABLED_BY_DEFAULT == 0)
        .unwrap();
    assert_eq!(
        select_scroll(&spells(Some(id)), 1, &mut rng)
            .unwrap()
            .index(),
        id
    );
    expected.draw();
    assert_eq!(rng, expected);
    // A creature ability with a level and school is still outside the hero range.
    assert!(matches!(
        select_scroll(&spells(Some(raw::HERO_SPELL_COUNT as usize)), 1, &mut rng),
        Err(TreasureGenerationError::EmptyScroll)
    ));
    expected.draw();
    assert_eq!(rng, expected);
}
