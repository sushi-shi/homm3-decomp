"""Convert the pinned VCMI Cove artwork into a native LOD and loose sounds.

Requires Pillow, the pinned upstream checkout, and a licensed Complete Data
folder. Existing game files are read only; output must be a separate directory.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path

from .cove import BUILDINGS, ROOT


def pcx(image):
    """Heroes' PCX container (12-byte header), not ZSoft PCX."""
    width, height = image.size
    if image.mode == 'P':
        pixels = image.tobytes()
        palette = bytes(image.getpalette()[:768]).ljust(768, b'\0')
        return struct.pack('<III', len(pixels), width, height) + pixels + palette
    pixels = image.convert('RGB').tobytes('raw', 'BGR')
    return struct.pack('<III', len(pixels), width, height) + pixels


def read_resource(data_dir, name):
    files = {p.name.lower(): p for p in data_dir.iterdir() if p.is_file()}
    if name.lower() in files:
        return files[name.lower()].read_bytes()
    for archive_name in ('h3ab_spr.lod', 'h3sprite.lod', 'h3ab_bmp.lod', 'h3bitmap.lod'):
        if archive_name not in files:
            continue
        with files[archive_name].open('rb') as f:
            header = f.read(92)
            if header[:4] != b'LOD\0':
                raise ValueError(f'Invalid LOD: {archive_name}')
            count = struct.unpack_from('<I', header, 8)[0]
            directory = f.read(count * 32)
            for offset in range(0, len(directory), 32):
                key, pos, size, flags, compressed = struct.unpack_from('<16sIIII', directory, offset)
                if key.split(b'\0')[0].decode('ascii').lower() == name.lower():
                    f.seek(pos)
                    content = f.read(compressed or size)
                    content = zlib.decompress(content) if compressed else content
                    if len(content) != size:
                        raise ValueError(f'Truncated resource: {name}')
                    return content
    raise FileNotFoundError(f'{name} is absent from {data_dir}')


def extend_def(base, images, prefix):
    """Append interface frames without altering any original encoded frames."""
    from PIL import Image
    if not prefix.isascii() or not prefix.isalnum() or len(prefix) > 8:
        raise ValueError('Frame namespace must contain at most eight ASCII letters/digits')
    kind, width, height, groups = struct.unpack_from('<4I', base)
    palette = base[16:784]
    pos = 784
    sequences = []
    for _ in range(groups):
        seq, count, unused1, unused2 = struct.unpack_from('<4I', base, pos)
        pos += 16
        names = [base[pos + i * 13:pos + (i + 1) * 13] for i in range(count)]
        pos += count * 13
        offsets = struct.unpack_from(f'<{count}I', base, pos)
        pos += count * 4
        sequences.append([seq, names, list(offsets)])
    if len(sequences) != 1 or sequences[0][0] != 0:
        raise ValueError('Expected one interface sequence')
    _, names, offsets = sequences[0]
    original_count = len(names)
    if images and min(images) < original_count:
        raise ValueError('Appending must not overwrite original frames')
    count = max([original_count - 1, *images]) + 1
    if count > 65536:
        raise ValueError('Frame index exceeds the four-digit name suffix')
    if set(images) != set(range(original_count, count)):
        raise ValueError('New interface frames must be contiguous')
    new_header_size = 784 + 16 + 17 * count
    delta = new_header_size - pos
    offsets = [v + delta for v in offsets]
    payload = bytearray(base[pos:])
    palette_image = Image.new('P', (1, 1))
    palette_image.putpalette(palette)
    for index in range(original_count, count):
        img = images[index]
        width = max(width, img.width)
        height = max(height, img.height)
        if img.mode == 'P' and bytes(img.getpalette()[:768]).ljust(768, b'\0') == palette:
            indexed = img
        else:
            indexed = img.convert('RGB').quantize(palette=palette_image, dither=Image.Dither.NONE)
        pixels = indexed.tobytes()
        offsets.append(new_header_size + len(payload))
        names.append(f'{prefix}{index:04x}'.encode().ljust(13, b'\0'))
        payload += struct.pack('<8I', len(pixels), 0, img.width, img.height, img.width, img.height, 0, 0)
        payload += pixels
    return (struct.pack('<4I', kind, width, height, 1) + palette
            + struct.pack('<4I', 0, count, 0, 0) + b''.join(names)
            + struct.pack(f'<{count}I', *offsets) + payload)


def interface_def(images):
    first = next(iter(images.values()))
    palette = bytes(first.getpalette()[:768]).ljust(768, b'\0')
    empty = struct.pack('<4I', 71, 0, 0, 1) + palette + struct.pack('<4I', 0, 0, 0, 0)
    return extend_def(empty, images, "hallcove")


def adventure_mask(sprite):
    """Native MSK draw/shadow coverage, aligned to the lower-right map cell.

    As in Complete's town masks, cover the sprite's full rectangle. Passability
    and visitable cells remain separate in objects.txt and serialized H3M.
    """
    _, width, height, _ = struct.unpack_from('<4I', sprite)
    columns, rows = (width + 31) // 32, (height + 31) // 32
    if not 1 <= columns <= 8 or not 1 <= rows <= 6:
        raise ValueError('Adventure sprite exceeds the native 8 by 6 mask')
    bits = bytes(6 - rows) + bytes([(255 << (8 - columns)) & 255]) * rows
    return bytes([columns, rows]) + bits + bits


def package(upstream, base_data, output):
    from PIL import Image
    data = json.loads((ROOT / 'extensions/cove/definition.json').read_text())
    for name, digest in data['provenance']['sha256'].items():
        if hashlib.sha256((upstream / name).read_bytes()).hexdigest() != digest:
            raise ValueError(f'Upstream differs from the pinned source: {name}')
    if output.resolve() == base_data.resolve() or base_data.resolve() in output.resolve().parents:
        raise ValueError('Output must be separate from the original game Data directory')
    # Check every required original before creating output.
    bases = {name: read_resource(base_data, name) for name in
             ('cprsmall.def', 'twcrport.def', 'itpa.def', 'itpt.def', 'un32.def', 'un44.def', 'artifact.def')}
    roots = [upstream / 'mods' / mod / 'Content' for mod in ('cove', 'cannon')]
    index = {}
    for root in roots:
        for file in root.rglob('*'):
            if file.is_file():
                relative = file.relative_to(root)
                index[str(Path(*relative.parts[1:])).lower()] = file
    def source(key):
        key = key.lower()
        for candidate in (key, key + '.png', key + '.def'):
            if candidate in index:
                return index[candidate]
        # Shared projectile is outside Cove's submod.
        matches = [p for p in (upstream / 'content').rglob('*') if p.is_file() and p.name.lower() == Path(key).name.lower()]
        if len(matches) == 1:
            return matches[0]
        raise FileNotFoundError(key)
    def picture(key):
        with Image.open(source(key)) as image:
            return image.copy()
    resources = {}
    def bitmap(name, key):
        resources[name] = pcx(picture(key))
    # Extend the shared template table used by both map loading and RMG.
    object_lines = read_resource(base_data, 'objects.txt').decode('cp1252').splitlines()
    rows = [line.split() for line in object_lines[1:] if line.strip()]
    def object_row(kind, subtype, sprite, native=None):
        row = next(r[:] for r in rows if len(r) == 9 and r[5:7] == [str(kind), '0'])
        row[0], row[6] = sprite, str(subtype)
        if native is not None:
            row[4] = format(1 << native, '09b')
        return ' '.join(row)
    additions = [object_row(98, 9, 'AVCcovf0.def')]
    additions += [object_row(54, 151 + i, Path(data['creatures'][key]['graphics']['map']).name, 4)
                  for i, key in enumerate(k for row in data['town']['creatures'] for k in row)]
    resources['objects.txt'] = ('\r\n'.join([str(int(object_lines[0]) + len(additions)),
                                               *object_lines[1:], *additions]) + '\r\n').encode('cp1252')
    animations = [v['graphics']['animation'] for v in [data['cannon'], *data['creatures'].values()]]
    animations += [v['graphics']['map'] for v in data['creatures'].values()]
    animations += [v['animation'] for k, v in data['structures'].items() if k in BUILDINGS]
    animations += [v['animation'] for v in data['town']['mapObject']['templates'].values()]
    for hero_class in data['heroClasses'].values():
        animations += list(hero_class['animation']['battle'].values())
        animations += [v['animation'] for v in hero_class['mapObject']['templates'].values()]
    for animation in animations:
        file = source(animation)
        resources[file.name.lower()] = file.read_bytes()
    map_animations = [v['graphics']['map'] for v in data['creatures'].values()]
    map_animations += [v['animation'] for v in data['town']['mapObject']['templates'].values()]
    for hero_class in data['heroClasses'].values():
        map_animations += [v['animation'] for v in hero_class['mapObject']['templates'].values()]
    for animation in map_animations:
        file = source(animation)
        resources[file.with_suffix('.msk').name.lower()] = adventure_mask(file.read_bytes())
    for creature in [data['cannon'], *data['creatures'].values()]:
        missile = creature['graphics'].get('missile', {}).get('projectile')
        if missile:
            resources[Path(missile).name] = source(missile).read_bytes()
        for key in creature['sound'].values():
            resources[Path(key).with_suffix('.82m').name] = source(key).read_bytes()
    bitmap('TBCVBack.pcx', data['town']['townBackground'])
    bitmap('TPMageCv.pcx', data['town']['guildWindow'])
    bitmap('TPCasCv.pcx', data['faction']['creatureBackground']['120px'])
    bitmap('CrBkgCov.pcx', data['faction']['creatureBackground']['130px'])
    for key, structure in data['structures'].items():
        if key in BUILDINGS and 'border' in structure:
            bitmap(f'TOCV{BUILDINGS[key]:02d}.pcx', structure['border'])
            bitmap(f'TZCV{BUILDINGS[key]:02d}.pcx', structure['area'])
        if 'campaignBonus' in structure:
            bitmap(Path(structure['campaignBonus']).name + '.pcx', structure['campaignBonus'])
    hall = json.loads(source(data['town']['buildingsIcons'] + '.json').read_text())
    resources['hallcove.def'] = interface_def({(44 if entry['frame'] == 46 else entry['frame']): picture(hall['basepath'] + entry['file']) for entry in hall['images']})
    for file in (roots[0] / 'data/hota/cove/town/siege').glob('*.png'):
        resources[file.with_suffix('.pcx').name] = pcx(Image.open(file))
    for i in range(48):
        bitmap(f'puzCov{i:02d}.pcx', f'hota/cove/puzzleMap/{i:02d}.png')
    creatures = [data['cannon']] + [data['creatures'][k] for row in data['town']['creatures'] for k in row]
    for name, size in (('cprsmall.def', 'iconSmall'), ('twcrport.def', 'iconLarge')):
        images = {152 + i: picture(c['graphics'][size]) for i, c in enumerate(creatures)}
        # Frame = creature ID + 2. Retail has no portrait for arrow towers (149).
        count = struct.unpack_from('<I', bases[name], 788)[0]
        images.update({i: Image.new('P', images[152].size) for i in range(count, 152)})
        resources[name] = extend_def(bases[name], images, Path(name).stem)
    # Preserve legacy town frames; Cove uses four appended frames in each sheet.
    for name, size in (('itpa.def', 'small'), ('itpt.def', 'large')):
        count = struct.unpack_from('<I', bases[name], 788)[0]
        if count != (39 if name == 'itpa.def' else 36):
            raise ValueError('Unexpected Complete town portrait frame layout')
        images = [picture(data['town']['icons'][kind][state][size]) for kind in ('fort', 'village') for state in ('normal', 'built')]
        resources[name] = extend_def(bases[name], {count + i: img for i, img in enumerate(images)}, Path(name).stem)
    for i, hero in enumerate(data['heroes'].values()):
        bitmap(f'HPS{163 + i}CV.pcx', hero['images']['small'])
        bitmap(f'HPL{163 + i}CV.pcx', hero['images']['large'])
    for name, size in (('un32.def', 'specialtySmall'), ('un44.def', 'specialtyLarge')):
        count = struct.unpack_from('<I', bases[name], 788)[0]
        if count > 163:
            raise ValueError('Unexpected Complete specialty sheet')
        images = {i: Image.new('P', (32 if name == 'un32.def' else 44, 32)) for i in range(count, 163)}
        images.update({163 + i: picture(h['images'][size]) for i, h in enumerate(data['heroes'].values())})
        resources[name] = extend_def(bases[name], images, Path(name).stem)
    artifact = bytearray(extend_def(bases['artifact.def'], {146: picture('hota/iconsLarge/cannonArtifact.png')}, 'artifact'))
    # Insert Cannon before the two UI overlay frames; artifact frames equal IDs.
    names = [bytes(artifact[800 + i * 13:813 + i * 13]) for i in range(147)]
    offsets = list(struct.unpack_from('<147I', artifact, 800 + 147 * 13))
    order = list(range(144)) + [146, 144, 145]
    artifact[800:800 + 147 * 13] = b''.join(names[i] for i in order)
    struct.pack_into('<147I', artifact, 800 + 147 * 13, *(offsets[i] for i in order))
    resources['artifact.def'] = bytes(artifact)
    for name in resources:
        if len(name) > 12:
            raise ValueError(f'Resource exceeds native name limit: {name}')
    output.mkdir(parents=True, exist_ok=True)
    for name, content in resources.items():
        if len(name) > 12:
            raise ValueError(f'Resource exceeds native name limit: {name}')
        # Native loose PCX uses ZSoft encoding; the compact bitmap containers
        # belong in the LOD. Sounds use the engine's loose WAV/82M loader.
        if name.lower().endswith('.82m'):
            (output / name).write_bytes(content)
        elif (output / name).exists() and not (output / name).is_symlink():
            (output / name).unlink()
    # Sprite loading is archive-only in the native engine. The Cove archive
    # also owns the extended shared interface sheets, before the base LODs.
    names = sorted(resources, key=str.lower)
    offset = 92 + 32 * len(names)
    directory = bytearray()
    payload = bytearray()
    for name in names:
        content = resources[name]
        encoded = zlib.compress(content)
        directory += struct.pack('<16sIIII', name.encode('ascii').ljust(16, b'\0'),
                                 offset, len(content), 1, len(encoded))
        payload += encoded
        offset += len(encoded)
    header = b'LOD\0' + struct.pack('<II', 200, len(names)) + bytes(80)
    (output / 'cove.lod').write_bytes(header + directory + payload)
    (output / 'cove-source.json').write_text(json.dumps(data['provenance'], indent=2) + '\n')
    music = roots[0] / 'music/factions/CoveTown.mp3'
    music_output = output.parent / 'mp3'
    music_output.mkdir(parents=True, exist_ok=True)
    (music_output / 'CoveTown.mp3').write_bytes(music.read_bytes())
    return len(resources)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--upstream', required=True, type=Path)
    parser.add_argument('--base-data', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    print(f'Wrote {package(args.upstream, args.base_data, args.output)} native Cove resources')


if __name__ == '__main__':
    main()
