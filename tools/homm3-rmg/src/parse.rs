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

/// HotA.dll RVA 0x126b30: token stream used by resource integer lists.
/// Only tab, space, CR/LF and '#' delimit tokens. Other bytes survive until
/// CRT prefix conversion; a NUL ends the input. Errors identify the token start.
pub(crate) fn integer_stream(bytes: &[u8]) -> Result<Vec<i32>, usize> {
    let mut result = Vec::new();
    let mut start = None;
    let mut comment = false;
    for (offset, byte) in bytes.iter().copied().chain(std::iter::once(0)).enumerate() {
        if matches!(byte, 0 | b'\t' | b' ' | b'\r' | b'\n' | b'#') {
            if let Some(begin) = start.take() {
                result.push(integer(bytes[begin..offset].iter().copied()).ok_or(begin)?);
            }
            match byte {
                0 => break,
                b'#' => comment = true,
                b'\r' | b'\n' => comment = false,
                _ => {}
            }
        } else if !comment && start.is_none() {
            start = Some(offset);
        }
    }
    Ok(result)
}

#[cfg(test)]
mod tests {
    use super::integer_stream;

    #[test]
    fn integer_stream_retains_native_delimiters_comments_and_prefixes() {
        assert_eq!(
            integer_stream(b"12#3 4\r-5\n+6tail\tword 7\x0b8 9\0 10").unwrap(),
            [12, -5, 6, 0, 7, 9]
        );
        assert_eq!(
            integer_stream(b"# comment\n-2147483648 2147483647").unwrap(),
            [i32::MIN, i32::MAX]
        );
        assert_eq!(integer_stream(b"1 2147483648"), Err(2));
        assert_eq!(integer_stream(b"-2147483649"), Err(0));
    }
}
