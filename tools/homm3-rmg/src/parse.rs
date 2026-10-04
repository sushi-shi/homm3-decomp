//! Shared CRT text conversions at resource parsing boundaries.

/// CRT `atoi` prefix semantics; overflow remains a typed caller error.
pub(crate) fn integer(bytes: impl Iterator<Item = u8>) -> Option<i32> {
    let mut bytes = bytes.skip_while(u8::is_ascii_whitespace).peekable();
    let negative = bytes.peek() == Some(&b'-');
    if negative || bytes.peek() == Some(&b'+') {
        bytes.next();
    }
    let mut value = 0_i32;
    for digit in bytes.take_while(u8::is_ascii_digit) {
        let digit = i32::from(digit - b'0');
        value = value.checked_mul(10).and_then(|value| {
            if negative {
                value.checked_sub(digit)
            } else {
                value.checked_add(digit)
            }
        })?;
    }
    Some(value)
}
