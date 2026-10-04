//! Complete's resource precedence with disk-backed LOD payloads.
//!
//! Text uses loose Data files first, then base/expansion bitmap archives.
//! Masks use base/expansion sprite archives only. This is resource context 3
//! in resourcemanager.cpp, irrespective of the output map's format.

use flate2::{Decompress, DecompressError, FlushDecompress, Status};
use homm3_lod::{Directory, Entry, Header, HEADER_SIZE, NAME_SIZE};
use homm3_rmg::prototype::ImageMask;
use std::{
    collections::TryReserveError,
    error::Error,
    fmt,
    fs::{self, File},
    io::{self, Read, Seek, SeekFrom},
    path::{Path, PathBuf},
};

/// Resource loading failure, with the source path retained only on failure.
#[derive(Debug)]
pub struct ResourceError {
    /// Loose file, archive or resource name identifying the failure.
    pub path: PathBuf,
    /// Failure before a resource can enter a parser.
    pub fault: ResourceFault,
}

/// A file or compressed member could not be read completely.
#[derive(Debug)]
pub enum ResourceFault {
    /// File operation failed.
    Io(io::Error),
    /// Malformed LOD header, directory or payload extent.
    Archive(homm3_lod::Error),
    /// Invalid zlib stream.
    Inflate(DecompressError),
    /// The stream did not finish at its advertised uncompressed size.
    Length {
        /// Directory-advertised size.
        expected: u64,
        /// Bytes produced before completion or exhaustion of output space.
        actual: u64,
    },
    /// Resource cannot fit the host's address domain.
    Size,
    /// Output or directory storage could not be reserved.
    Allocation(TryReserveError),
    /// Multiple file names collide under the game's ASCII case folding.
    AmbiguousName,
    /// A required text resource was not found.
    Missing,
    /// The mask header is shorter than the required native record.
    Mask(homm3_resource::Error),
}
impl fmt::Display for ResourceError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}: ", self.path.display())?;
        match &self.fault {
            ResourceFault::Io(error) => error.fmt(f),
            ResourceFault::Archive(error) => error.fmt(f),
            ResourceFault::Inflate(error) => error.fmt(f),
            ResourceFault::Allocation(error) => error.fmt(f),
            ResourceFault::Mask(error) => error.fmt(f),
            ResourceFault::Length { expected, actual } => write!(f, "incomplete zlib stream or length mismatch (expected {expected}, produced {actual})"),
            ResourceFault::Size => f.write_str("resource exceeds addressable storage"),
            ResourceFault::AmbiguousName => f.write_str("ambiguous case-insensitive file name"),
            ResourceFault::Missing => f.write_str("required resource not found"),
        }
    }
}
impl Error for ResourceError {}
impl From<io::Error> for ResourceFault {
    fn from(error: io::Error) -> Self {
        Self::Io(error)
    }
}
impl From<homm3_lod::Error> for ResourceFault {
    fn from(error: homm3_lod::Error) -> Self {
        Self::Archive(error)
    }
}
impl From<TryReserveError> for ResourceFault {
    fn from(error: TryReserveError) -> Self {
        Self::Allocation(error)
    }
}

/// Owned lookup metadata decoded by the shared format reader. Directory bytes
/// are dropped after indexing; archive payloads remain on disk.
#[derive(Clone, Copy, Debug)]
struct IndexedEntry {
    name: [u8; NAME_SIZE],
    offset: u32,
    size: u32,
    compressed_size: u32,
}
impl IndexedEntry {
    fn from_entry(entry: Entry<'_>) -> Self {
        let mut name = [0; NAME_SIZE];
        name[..entry.name.len()].copy_from_slice(entry.name);
        Self {
            name,
            offset: entry.offset,
            size: entry.size,
            compressed_size: entry.compressed_size,
        }
    }
    fn matches(self, name: &[u8]) -> bool {
        let end = self.name.iter().position(|&b| b == 0).unwrap_or(NAME_SIZE);
        self.name[..end].eq_ignore_ascii_case(name)
    }
}

struct DiskArchive {
    path: PathBuf,
    file: File,
    entries: Vec<IndexedEntry>,
}
impl DiskArchive {
    fn open(path: &Path) -> Result<Self, ResourceError> {
        let load = || -> Result<(File, Vec<IndexedEntry>), ResourceFault> {
            let mut file = File::open(path)?;
            let file_len = file.metadata()?.len();
            let mut header_bytes = [0; HEADER_SIZE];
            file.read_exact(&mut header_bytes)?;
            let header = Header::parse(&header_bytes)?;
            if header.directory_end() as u64 > file_len {
                return Err(ResourceFault::Archive(homm3_lod::Error::ShortDirectory {
                    needed: header.directory_end(),
                    available: usize::try_from(file_len).unwrap_or(usize::MAX),
                }));
            }
            let mut bytes = Vec::new();
            resize(&mut bytes, header.directory_end())?;
            bytes[..HEADER_SIZE].copy_from_slice(&header_bytes);
            file.read_exact(&mut bytes[HEADER_SIZE..])?;
            let directory = Directory::parse(&bytes, file_len)?;
            let mut entries = Vec::new();
            entries.try_reserve_exact(directory.len())?;
            entries.extend(directory.entries().map(IndexedEntry::from_entry));
            Ok((file, entries))
        };
        let (file, entries) = load().map_err(|fault| ResourceError {
            path: path.into(),
            fault,
        })?;
        Ok(Self {
            path: path.into(),
            file,
            entries,
        })
    }
    fn read(
        &mut self,
        name: &[u8],
        output: &mut Vec<u8>,
        scratch: &mut InflateWorkspace,
    ) -> Result<bool, ResourceError> {
        let Some(entry) = self
            .entries
            .iter()
            .find(|entry| entry.matches(name))
            .copied()
        else {
            return Ok(false);
        };
        let mut read = || -> Result<(), ResourceFault> {
            self.file.seek(SeekFrom::Start(u64::from(entry.offset)))?;
            resize(
                output,
                usize::try_from(entry.size).map_err(|_| ResourceFault::Size)?,
            )?;
            if entry.compressed_size == 0 {
                self.file.read_exact(output)?;
            } else {
                resize(
                    &mut scratch.compressed,
                    usize::try_from(entry.compressed_size).map_err(|_| ResourceFault::Size)?,
                )?;
                self.file.read_exact(&mut scratch.compressed)?;
                scratch.decoder.reset(true);
                let status = scratch
                    .decoder
                    .decompress(&scratch.compressed, output, FlushDecompress::Finish)
                    .map_err(ResourceFault::Inflate)?;
                if status != Status::StreamEnd
                    || scratch.decoder.total_out() != u64::from(entry.size)
                {
                    return Err(ResourceFault::Length {
                        expected: u64::from(entry.size),
                        actual: scratch.decoder.total_out(),
                    });
                }
            }
            Ok(())
        };
        read().map_err(|fault| ResourceError {
            path: self.path.clone(),
            fault,
        })?;
        Ok(true)
    }
}

fn resize(buffer: &mut Vec<u8>, size: usize) -> Result<(), ResourceFault> {
    buffer.clear();
    buffer.try_reserve(size)?;
    buffer.resize(size, 0);
    Ok(())
}

struct InflateWorkspace {
    compressed: Vec<u8>,
    decoder: Decompress,
}

/// Installation resource reader. Only directories and resources actually used
/// by RMG are read; mask output and zlib state are reused across all image names.
pub struct Installation {
    files: Vec<PathBuf>,
    bitmaps: [Option<DiskArchive>; 2],
    sprites: [Option<DiskArchive>; 2],
    inflate: InflateWorkspace,
    mask_bytes: Vec<u8>,
}
impl Installation {
    /// Open a Complete installation's Data directory, with ASCII-insensitive
    /// filenames. Missing archives are permitted until a required lookup fails.
    ///
    /// # Errors
    /// Reports directory/file errors, ambiguous case-folded names and malformed archives.
    pub fn open(data_directory: &Path) -> Result<Self, ResourceError> {
        let scan = || -> Result<Vec<PathBuf>, ResourceFault> {
            fs::read_dir(data_directory)?
                .map(|entry| entry.map(|entry| entry.path()))
                .collect::<Result<_, _>>()
                .map_err(ResourceFault::Io)
        };
        let files = scan().map_err(|fault| ResourceError {
            path: data_directory.into(),
            fault,
        })?;
        let archive = |name: &[u8]| -> Result<Option<DiskArchive>, ResourceError> {
            find_file(&files, name).and_then(|path| path.map(DiskArchive::open).transpose())
        };
        let bitmaps = [archive(b"h3bitmap.lod")?, archive(b"h3ab_bmp.lod")?];
        let sprites = [archive(b"h3sprite.lod")?, archive(b"h3ab_spr.lod")?];
        Ok(Self {
            files,
            bitmaps,
            sprites,
            inflate: InflateWorkspace {
                compressed: Vec::new(),
                decoder: Decompress::new(true),
            },
            mask_bytes: Vec::new(),
        })
    }
    /// Load a required spreadsheet/text member, reusing the caller's output buffer.
    /// Loose Data files take precedence over the archives.
    ///
    /// # Errors
    /// Reports a missing member, file error, invalid stream or allocation failure.
    pub fn text(&mut self, name: &str, output: &mut Vec<u8>) -> Result<(), ResourceError> {
        if let Some(path) = find_file(&self.files, name.as_bytes())? {
            let mut read = || -> Result<(), ResourceFault> {
                let mut file = File::open(path)?;
                let size =
                    usize::try_from(file.metadata()?.len()).map_err(|_| ResourceFault::Size)?;
                resize(output, size)?;
                file.read_exact(output)?;
                Ok(())
            };
            return read().map_err(|fault| ResourceError {
                path: path.into(),
                fault,
            });
        }
        for archive in self.bitmaps.iter_mut().flatten() {
            if archive.read(name.as_bytes(), output, &mut self.inflate)? {
                return Ok(());
            }
        }
        output.clear();
        Err(ResourceError {
            path: name.into(),
            fault: ResourceFault::Missing,
        })
    }
    /// Read an image mask from sprite archives. Missing masks return `None` so
    /// the prototype parser can apply its original default.msk fallback.
    ///
    /// # Errors
    /// Reports file/decompression errors or a truncated mask header.
    pub fn mask(&mut self, name: &[u8]) -> Result<Option<ImageMask>, ResourceError> {
        for archive in self.sprites.iter_mut().flatten() {
            if archive.read(name, &mut self.mask_bytes, &mut self.inflate)? {
                return ImageMask::parse(&self.mask_bytes)
                    .map(Some)
                    .map_err(|error| ResourceError {
                        path: archive.path.clone(),
                        fault: ResourceFault::Mask(error),
                    });
            }
        }
        Ok(None)
    }
}

fn find_file<'a>(files: &'a [PathBuf], name: &[u8]) -> Result<Option<&'a Path>, ResourceError> {
    let mut matches = files.iter().filter(|path| {
        path.file_name()
            .is_some_and(|file| file.as_encoded_bytes().eq_ignore_ascii_case(name))
    });
    let found = matches.next();
    if let Some(ambiguous) = matches.next() {
        return Err(ResourceError {
            path: ambiguous.clone(),
            fault: ResourceFault::AmbiguousName,
        });
    }
    Ok(found.map(PathBuf::as_path))
}

#[cfg(test)]
mod tests {
    use super::*;
    use flate2::{write::ZlibEncoder, Compression};
    use std::{
        io::Write,
        sync::atomic::{AtomicU64, Ordering},
    };

    struct Fixture(PathBuf);
    impl Fixture {
        fn new() -> Self {
            static SERIAL: AtomicU64 = AtomicU64::new(0);
            let path = std::env::temp_dir().join(format!(
                "homm3-rmg-resources-{}-{}",
                std::process::id(),
                SERIAL.fetch_add(1, Ordering::Relaxed)
            ));
            fs::create_dir(&path).unwrap();
            Self(path)
        }
        fn archive(&self, name: &str, members: &[(&[u8], &[u8], bool)]) {
            let mut bytes = vec![0; HEADER_SIZE + members.len() * homm3_lod::ENTRY_SIZE];
            bytes[..4].copy_from_slice(b"LOD\0");
            bytes[8..12].copy_from_slice(&u32::try_from(members.len()).unwrap().to_le_bytes());
            for (index, &(name, data, compressed)) in members.iter().enumerate() {
                let start = HEADER_SIZE + index * homm3_lod::ENTRY_SIZE;
                bytes[start..start + name.len()].copy_from_slice(name);
                let offset = u32::try_from(bytes.len()).unwrap();
                bytes[start + 16..start + 20].copy_from_slice(&offset.to_le_bytes());
                bytes[start + 20..start + 24]
                    .copy_from_slice(&u32::try_from(data.len()).unwrap().to_le_bytes());
                if compressed {
                    let mut encoder = ZlibEncoder::new(Vec::new(), Compression::default());
                    encoder.write_all(data).unwrap();
                    let packed = encoder.finish().unwrap();
                    bytes[start + 28..start + 32]
                        .copy_from_slice(&u32::try_from(packed.len()).unwrap().to_le_bytes());
                    bytes.extend_from_slice(&packed);
                } else {
                    bytes.extend_from_slice(data);
                }
            }
            fs::write(self.0.join(name), bytes).unwrap();
        }
    }
    impl Drop for Fixture {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.0);
        }
    }

    #[test]
    fn loose_text_then_base_then_expansion_and_masks_ignore_loose_files() {
        let fixture = Fixture::new();
        fixture.archive(
            "H3bitmap.lod",
            &[(b"a.txt", b"base", true), (b"b.txt", b"base-b", false)],
        );
        fixture.archive(
            "h3AB_BMP.lod",
            &[
                (b"a.txt", b"expansion", true),
                (b"c.txt", b"only-expansion", true),
            ],
        );
        fs::write(fixture.0.join("B.TxT"), b"loose").unwrap();
        let mask = [1_u8; 14];
        let other = [2_u8; 14];
        fixture.archive("H3sprite.lod", &[(b"a.msk", &mask, true)]);
        fixture.archive(
            "H3ab_spr.lod",
            &[(b"a.msk", &other, false), (b"b.msk", &other, false)],
        );
        fs::write(fixture.0.join("a.msk"), [3; 14]).unwrap();
        let mut installation = Installation::open(&fixture.0).unwrap();
        let mut bytes = Vec::new();
        for (name, expected) in [
            ("A.TXT", &b"base"[..]),
            ("b.txt", b"loose"),
            ("c.txt", b"only-expansion"),
        ] {
            installation.text(name, &mut bytes).unwrap();
            assert_eq!(bytes, expected);
        }
        assert_eq!(
            installation.mask(b"A.MSK").unwrap(),
            Some(ImageMask::parse(&mask).unwrap())
        );
        assert_eq!(
            installation.mask(b"b.msk").unwrap(),
            Some(ImageMask::parse(&other).unwrap())
        );
        assert!(installation.mask(b"missing.msk").unwrap().is_none());
        assert!(matches!(
            installation
                .text("missing.txt", &mut bytes)
                .unwrap_err()
                .fault,
            ResourceFault::Missing
        ));
    }

    #[test]
    fn compressed_reads_reuse_storage_and_reject_corrupt_or_wrong_length_streams() {
        let fixture = Fixture::new();
        fixture.archive(
            "h3bitmap.lod",
            &[(b"data.txt", &[42; 8192], true), (b"empty.txt", b"", true)],
        );
        let mut installation = Installation::open(&fixture.0).unwrap();
        let mut bytes = Vec::new();
        installation.text("data.txt", &mut bytes).unwrap();
        let storage = (bytes.as_ptr(), installation.inflate.compressed.as_ptr());
        for _ in 0..3 {
            installation.text("data.txt", &mut bytes).unwrap();
            assert_eq!(bytes, [42; 8192]);
            assert_eq!(
                storage,
                (bytes.as_ptr(), installation.inflate.compressed.as_ptr())
            );
        }
        installation.text("empty.txt", &mut bytes).unwrap();
        assert!(bytes.is_empty());
        drop(installation);
        let path = fixture.0.join("h3bitmap.lod");
        let original = fs::read(&path).unwrap();
        for size in [8191_u32, 8193] {
            let mut bad = original.clone();
            bad[HEADER_SIZE + 20..HEADER_SIZE + 24].copy_from_slice(&size.to_le_bytes());
            fs::write(&path, &bad).unwrap();
            let mut installation = Installation::open(&fixture.0).unwrap();
            assert!(installation.text("data.txt", &mut bytes).is_err());
        }
        let mut bad = original;
        bad[HEADER_SIZE + 2 * homm3_lod::ENTRY_SIZE] ^= 0xff;
        fs::write(&path, &bad).unwrap();
        let mut installation = Installation::open(&fixture.0).unwrap();
        assert!(matches!(
            installation.text("data.txt", &mut bytes).unwrap_err().fault,
            ResourceFault::Inflate(_)
        ));
    }

    #[test]
    fn directory_extents_and_case_collisions_fail_before_lookup() {
        let fixture = Fixture::new();
        fixture.archive("h3bitmap.lod", &[(b"a.txt", b"abcd", false)]);
        let path = fixture.0.join("h3bitmap.lod");
        let mut bytes = fs::read(&path).unwrap();
        bytes.pop();
        fs::write(&path, bytes).unwrap();
        assert!(matches!(
            Installation::open(&fixture.0).err().unwrap().fault,
            ResourceFault::Archive(homm3_lod::Error::PayloadOutOfBounds { .. })
        ));
        fs::remove_file(&path).unwrap();
        fs::write(fixture.0.join("a.txt"), b"one").unwrap();
        fs::write(fixture.0.join("A.TXT"), b"two").unwrap();
        // A case-insensitive filesystem already treats these as one file.
        if fs::read_dir(&fixture.0).unwrap().count() == 2 {
            let mut installation = Installation::open(&fixture.0).unwrap();
            assert!(matches!(
                installation
                    .text("a.txt", &mut Vec::new())
                    .unwrap_err()
                    .fault,
                ResourceFault::AmbiguousName
            ));
        }
    }
}
