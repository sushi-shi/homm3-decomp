//! Safe clipped rendering of decoded DEF runs.

use crate::{Encoding, Error, Frame, Result, Run};

/// Integer rectangle with an exclusive right/bottom edge.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Rect {
    /// Left edge.
    pub x: i32,
    /// Top edge.
    pub y: i32,
    /// Non-negative width.
    pub width: i32,
    /// Non-negative height.
    pub height: i32,
}

impl Rect {
    /// Construct a rectangle. Invalid dimensions are reported when the value
    /// is consumed by [`Blit`] rather than hidden by saturation here.
    #[must_use]
    pub const fn new(x: i32, y: i32, width: i32, height: i32) -> Self {
        Self {
            x,
            y,
            width,
            height,
        }
    }

    fn edges(self) -> Result<(i32, i32)> {
        if self.width < 0 || self.height < 0 {
            return Err(Error::BadRectangle);
        }
        let right = self.x.checked_add(self.width).ok_or(Error::BadRectangle)?;
        let bottom = self.y.checked_add(self.height).ok_or(Error::BadRectangle)?;
        Ok((right, bottom))
    }

    /// Intersection of two valid rectangles.
    ///
    /// # Errors
    ///
    /// Returns [`Error::BadRectangle`] when either edge overflows.
    pub fn intersect(self, other: Self) -> Result<Option<Self>> {
        let (right, bottom) = self.edges()?;
        let (other_right, other_bottom) = other.edges()?;
        let x = self.x.max(other.x);
        let y = self.y.max(other.y);
        let end_x = right.min(other_right);
        let end_y = bottom.min(other_bottom);
        if end_x <= x || end_y <= y {
            Ok(None)
        } else {
            Ok(Some(Self::new(x, y, end_x - x, end_y - y)))
        }
    }
}

/// Caller-owned 16-bit destination surface.
#[derive(Debug)]
pub struct Canvas<'a> {
    width: usize,
    height: usize,
    stride: usize,
    pixels: &'a mut [u16],
}

impl<'a> Canvas<'a> {
    /// Validate a tightly packed surface.
    ///
    /// # Errors
    ///
    /// Returns [`Error::BadCanvas`] when the dimensions overflow or the pixel
    /// slice does not have exactly `width * height` entries.
    pub fn new(width: usize, height: usize, pixels: &'a mut [u16]) -> Result<Self> {
        Self::with_stride(width, height, width, pixels)
    }

    /// Validate a surface whose rows may contain trailing padding.
    ///
    /// # Errors
    ///
    /// Returns [`Error::BadCanvas`] unless `stride >= width` and the backing
    /// slice has exactly `stride * height` entries.
    pub fn with_stride(
        width: usize,
        height: usize,
        stride: usize,
        pixels: &'a mut [u16],
    ) -> Result<Self> {
        let needed = stride.checked_mul(height).ok_or(Error::BadCanvas)?;
        if width == 0 || height == 0 || stride < width || pixels.len() != needed {
            return Err(Error::BadCanvas);
        }
        if width > i32::MAX as usize || height > i32::MAX as usize {
            return Err(Error::BadCanvas);
        }
        Ok(Self {
            width,
            height,
            stride,
            pixels,
        })
    }

    /// Visible width.
    #[must_use]
    pub const fn width(&self) -> usize {
        self.width
    }

    /// Visible height.
    #[must_use]
    pub const fn height(&self) -> usize {
        self.height
    }

    /// Row stride in pixels.
    #[must_use]
    pub const fn stride(&self) -> usize {
        self.stride
    }

    /// Entire backing slice, including row padding.
    #[must_use]
    pub fn pixels(&self) -> &[u16] {
        self.pixels
    }

    /// Entire mutable backing slice, including row padding.
    #[must_use]
    pub fn pixels_mut(&mut self) -> &mut [u16] {
        self.pixels
    }

    fn bounds(&self) -> Rect {
        Rect::new(
            0,
            0,
            i32::try_from(self.width).unwrap_or(i32::MAX),
            i32::try_from(self.height).unwrap_or(i32::MAX),
        )
    }

    fn set(&mut self, x: i32, y: i32, value: u16) -> Result<()> {
        let x = usize::try_from(x).map_err(|_| Error::BadCanvas)?;
        let y = usize::try_from(y).map_err(|_| Error::BadCanvas)?;
        if x >= self.width || y >= self.height {
            return Err(Error::BadCanvas);
        }
        self.pixels[y * self.stride + x] = value;
        Ok(())
    }
}

/// Configurable counterpart of retail's ordinary `CSpriteFrame::Draw` path.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Blit {
    mirrored: bool,
    transparent_fills: bool,
    clip: Option<Rect>,
}

impl Blit {
    /// Opaque forward renderer.
    #[must_use]
    pub const fn new() -> Self {
        Self {
            mirrored: false,
            transparent_fills: false,
            clip: None,
        }
    }

    /// Select horizontal reversal.
    #[must_use]
    pub const fn mirrored(mut self, mirrored: bool) -> Self {
        self.mirrored = mirrored;
        self
    }

    /// Match retail's general-RLE `tblit`: literal bytes still paint, while
    /// general-RLE fill runs advance without touching the surface. Packed
    /// tileset/adventure control runs are always transparent in the ordinary
    /// renderer.
    #[must_use]
    pub const fn transparent_fills(mut self, transparent: bool) -> Self {
        self.transparent_fills = transparent;
        self
    }

    /// Restrict drawing further than the canvas extent.
    #[must_use]
    pub const fn clip(mut self, clip: Rect) -> Self {
        self.clip = Some(clip);
        self
    }

    /// Render the complete uncropped frame at `(x, y)`.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for malformed encoded rows, invalid rectangles, or
    /// inconsistent canvas storage.
    pub fn draw_full(
        self,
        canvas: &mut Canvas<'_>,
        frame: Frame<'_>,
        palette: &[u16; 256],
        x: i32,
        y: i32,
    ) -> Result<()> {
        let width = i32::try_from(frame.width()).map_err(|_| Error::BadRectangle)?;
        let height = i32::try_from(frame.height()).map_err(|_| Error::BadRectangle)?;
        self.draw(canvas, frame, palette, Rect::new(0, 0, width, height), x, y)
    }

    /// Render a full-frame-coordinate source rectangle at `(x, y)`.
    ///
    /// This follows the clipping order in `CSpriteFrame::Clip`: mirror the
    /// source coordinate, clip against the destination, then clip against the
    /// frame's stored crop and finally translate to crop-local scanlines.
    ///
    /// # Errors
    ///
    /// Returns [`Error`] for malformed encoded rows, invalid rectangles, or
    /// inconsistent canvas storage.
    #[allow(clippy::too_many_lines)]
    pub fn draw(
        self,
        canvas: &mut Canvas<'_>,
        frame: Frame<'_>,
        palette: &[u16; 256],
        source: Rect,
        mut dx: i32,
        mut dy: i32,
    ) -> Result<()> {
        source.edges()?;
        let mut sx = source.x;
        let mut sy = source.y;
        let mut sw = source.width;
        let mut sh = source.height;
        if sw <= 0 || sh <= 0 {
            return Ok(());
        }

        let full_width = i32::try_from(frame.width()).map_err(|_| Error::BadRectangle)?;
        if self.mirrored {
            sx = full_width
                .checked_sub(sw.checked_add(sx).ok_or(Error::BadRectangle)?)
                .ok_or(Error::BadRectangle)?;
        }

        let clip = match self.clip {
            Some(extra) => match canvas.bounds().intersect(extra)? {
                Some(clip) => clip,
                None => return Ok(()),
            },
            None => canvas.bounds(),
        };
        let (clip_right, clip_bottom) = clip.edges()?;

        if dx < clip.x {
            let amount = clip.x - dx;
            if !self.mirrored {
                sx = sx.checked_add(amount).ok_or(Error::BadRectangle)?;
            }
            sw -= amount;
            dx = clip.x;
        }
        if dy < clip.y {
            let amount = clip.y - dy;
            sy = sy.checked_add(amount).ok_or(Error::BadRectangle)?;
            sh -= amount;
            dy = clip.y;
        }
        let draw_right = dx.checked_add(sw).ok_or(Error::BadRectangle)?;
        if draw_right > clip_right {
            let amount = draw_right - clip_right;
            if self.mirrored {
                sx = sx.checked_add(amount).ok_or(Error::BadRectangle)?;
            }
            sw -= amount;
        }
        let draw_bottom = dy.checked_add(sh).ok_or(Error::BadRectangle)?;
        if draw_bottom > clip_bottom {
            sh -= draw_bottom - clip_bottom;
        }

        let crop_x = frame.cropped_x();
        let crop_y = frame.cropped_y();
        let crop_width = i32::try_from(frame.cropped_width()).map_err(|_| Error::BadRectangle)?;
        let crop_height = i32::try_from(frame.cropped_height()).map_err(|_| Error::BadRectangle)?;
        let crop_right = crop_x.checked_add(crop_width).ok_or(Error::BadRectangle)?;
        let crop_bottom = crop_y.checked_add(crop_height).ok_or(Error::BadRectangle)?;

        if sx < crop_x {
            let amount = crop_x - sx;
            if !self.mirrored {
                dx = dx.checked_add(amount).ok_or(Error::BadRectangle)?;
            }
            sw -= amount;
            sx = crop_x;
        }
        if sy < crop_y {
            let amount = crop_y - sy;
            dy = dy.checked_add(amount).ok_or(Error::BadRectangle)?;
            sh -= amount;
            sy = crop_y;
        }
        let source_right = sx.checked_add(sw).ok_or(Error::BadRectangle)?;
        if source_right > crop_right {
            let amount = source_right - crop_right;
            if self.mirrored {
                dx = dx.checked_add(amount).ok_or(Error::BadRectangle)?;
            }
            sw -= amount;
        }
        let source_bottom = sy.checked_add(sh).ok_or(Error::BadRectangle)?;
        if source_bottom > crop_bottom {
            sh -= source_bottom - crop_bottom;
        }
        if sw <= 0 || sh <= 0 {
            return Ok(());
        }

        sx -= crop_x;
        sy -= crop_y;
        debug_assert!(sx >= 0 && sy >= 0 && sw > 0 && sh > 0);
        let source_left = usize::try_from(sx).map_err(|_| Error::BadRectangle)?;
        let source_width = usize::try_from(sw).map_err(|_| Error::BadRectangle)?;
        let first_row = usize::try_from(sy).map_err(|_| Error::BadRectangle)?;
        let row_count = usize::try_from(sh).map_err(|_| Error::BadRectangle)?;

        for row_delta in 0..row_count {
            let row = first_row + row_delta;
            let mut cursor = 0usize;
            for run in frame.runs(row)? {
                let run = run?;
                let run_start = cursor;
                let run_end = cursor + run.len();
                cursor = run_end;
                let wanted_start = source_left;
                let wanted_end = source_left + source_width;
                let overlap_start = run_start.max(wanted_start);
                let overlap_end = run_end.min(wanted_end);
                if overlap_start >= overlap_end {
                    continue;
                }
                let within_run = overlap_start - run_start;
                let count = overlap_end - overlap_start;
                let within_source = overlap_start - wanted_start;

                match run {
                    Run::Fill { color, .. }
                        if frame.encoding() == Encoding::GeneralRle && !self.transparent_fills =>
                    {
                        self.paint_span(
                            canvas,
                            palette,
                            dx,
                            dy,
                            sw,
                            row_delta,
                            within_source,
                            count,
                            |_| color,
                        )?;
                    }
                    Run::Fill { .. } => {}
                    Run::Literal(bytes) => {
                        let literal = &bytes[within_run..within_run + count];
                        self.paint_span(
                            canvas,
                            palette,
                            dx,
                            dy,
                            sw,
                            row_delta,
                            within_source,
                            count,
                            |index| literal[index],
                        )?;
                    }
                }
            }
            if cursor != frame.cropped_width() {
                return Err(Error::ShortRow {
                    row,
                    decoded: cursor,
                    expected: frame.cropped_width(),
                });
            }
        }
        Ok(())
    }

    #[allow(clippy::too_many_arguments)]
    fn paint_span(
        self,
        canvas: &mut Canvas<'_>,
        palette: &[u16; 256],
        dx: i32,
        dy: i32,
        sw: i32,
        row_delta: usize,
        within_source: usize,
        count: usize,
        color: impl Fn(usize) -> u8,
    ) -> Result<()> {
        let y = dy
            .checked_add(i32::try_from(row_delta).map_err(|_| Error::BadRectangle)?)
            .ok_or(Error::BadRectangle)?;
        for index in 0..count {
            let source_offset = within_source + index;
            let source_offset = i32::try_from(source_offset).map_err(|_| Error::BadRectangle)?;
            let x = if self.mirrored {
                dx.checked_add(sw - 1 - source_offset)
            } else {
                dx.checked_add(source_offset)
            }
            .ok_or(Error::BadRectangle)?;
            canvas.set(x, y, palette[usize::from(color(index))])?;
        }
        Ok(())
    }
}
