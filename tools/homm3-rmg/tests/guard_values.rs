//! Guard strength scaling against the actual VC6 implementation.
use homm3_rmg::placement::GuardStrength;
#[test]
#[ignore = "requires HOMM3_RMG_ORACLE guard-values.txt capture"]
fn native_guard_value_thresholds_keep_separate_rounding_and_minimum() {
    let root = std::path::PathBuf::from(std::env::var_os("HOMM3_RMG_ORACLE").unwrap());
    let text =
        std::fs::read_to_string(root.join("retail-layout/case-0-candidate/guard-values.txt"))
            .unwrap();
    let mut checked = 0;
    for line in text.lines() {
        let values: Vec<i32> = line
            .split_whitespace()
            .map(|value| value.parse().unwrap())
            .collect();
        let [strength, base, expected] = values[..] else {
            panic!("invalid native checkpoint")
        };
        assert_eq!(
            GuardStrength::parse(strength).unwrap().scale(base).unwrap(),
            expected,
            "strength{strength} base{base}"
        );
        checked += 1;
    }
    assert_eq!(checked, 192);
}
