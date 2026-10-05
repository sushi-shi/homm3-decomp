//! The hint solver's independent MT19937 stream and pinned MSVC shuffle.

#[derive(Clone, Debug)]
pub(super) struct Random {
    state: [u32; 624],
    index: usize,
    pub draws: u64,
}

#[cfg(test)]
mod tests {
    use super::Random;

    #[test]
    fn shuffle_rejects_only_the_incomplete_remainder_bucket() {
        // Untempered words yielding 1, MAX and zero. The three-entry shuffle
        // accepts the first draw modulo two, rejects MAX modulo three, then
        // takes zero. Exercising this by searching real seeds would be prohibitive.
        let mut random = Random {
            state: [0; 624],
            index: 0,
            draws: 0,
        };
        random.state[0] = 0x1022_44c9;
        random.state[1] = 0x12dd_9bb3;
        let mut values = [0, 1, 2];
        random.shuffle(&mut values);
        assert_eq!(values, [2, 1, 0]);
        assert_eq!(random.draws, 3);

        // MAX-1 remains in the complete bucket for modulo three.
        random.index = 0;
        random.draws = 0;
        random.state[1] = 0x02ff_df7a;
        let mut values = [0, 1, 2];
        random.shuffle(&mut values);
        assert_eq!(values, [0, 1, 2]);
        assert_eq!(random.draws, 2);

        // Powers of two have no incomplete bucket, including the maximum word.
        random.index = 1;
        random.draws = 0;
        random.state[1] = 0x12dd_9bb3;
        random.shuffle(&mut [0, 1]);
        assert_eq!(random.draws, 1);
    }
}

impl Random {
    // DLL RVA 0x1f8b30. Do not seed from the current CRT state.
    pub fn new(seed: u32) -> Self {
        let mut state = [0_u32; 624];
        state[0] = seed;
        for i in 1..624 {
            state[i] = 1_812_433_253_u32
                .wrapping_mul(state[i - 1] ^ (state[i - 1] >> 30))
                .wrapping_add(u32::try_from(i).unwrap());
        }
        Self {
            state,
            index: 624,
            draws: 0,
        }
    }

    pub fn next(&mut self) -> u32 {
        if self.index == 624 {
            // In-place refill: the wraparound reads include already updated words.
            for i in 0..624 {
                let y = (self.state[i] & 0x8000_0000) | (self.state[(i + 1) % 624] & 0x7fff_ffff);
                self.state[i] = self.state[(i + 397) % 624]
                    ^ (y >> 1)
                    ^ if y & 1 == 0 { 0 } else { 0x9908_b0df };
            }
            self.index = 0;
        }
        let mut y = self.state[self.index];
        self.index += 1;
        self.draws += 1;
        y ^= y >> 11;
        y ^= (y << 7) & 0x9d2c_5680;
        y ^= (y << 15) & 0xefc6_0000;
        y ^ (y >> 18)
    }

    // DLL RVA 0x1f7fb0. Forward shuffle, with MSVC's division-based rejection.
    // The largest solver domain is twelve; no conversion can overflow.
    pub fn shuffle(&mut self, values: &mut [u8]) {
        for next in 1..values.len() {
            let count = u32::try_from(next + 1).unwrap();
            let value = loop {
                let value = self.next();
                if value / count < u32::MAX / count || u32::MAX % count == count - 1 {
                    break value % count;
                }
            };
            values.swap(next, value as usize);
        }
    }
}
