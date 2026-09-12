"""Restore the four ordinary map readers proved by the Dreamcast caller.

DC readObject calls readBoatData, readShipyardData, readHolyGrailData and
readShrineData. Complete expands these operations in its matching arms;
absence of standalone retail slots does not erase the source boundary.
The original source order, signatures (with the PC TAbstractFile stream),
locals, reads and return checks come from each DC dossier and block listing.
Retail owns field layout, character conversions and the deferred shipyard
scan. The caller discards each status and keeps its shared return of one.
This finite control compares the flattened checkpoint with all four real
helpers restored together; it adds no inline declaration or diagnostic pin.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/mapcell.cpp'
SIGNATURE = 'int NewfullMap::readObject('
CARCASS_START = '#if 0  // @carcass -- located/reconstruction-pending bodies\n\n'
CARCASS_END = '\n#endif  // @carcass'

HELPERS = {
    'boat': '''// Original: NewfullMap::readBoatData, mapcell.cpp:1095, dc 0xed984.
// DC proves the ordinary helper, boatType/x/y locals and call order.
// Retail readObject's BOAT arm expands this body and discards status.
int NewfullMap::readBoatData(TAbstractFile* infile, CObject* boatObject)
{
    signed char boatType = static_cast<signed char>(
        m_objectTypes[boatObject->m_typeIndex].m_extra);
    int x;
    int y;
    boatObject->findTrigger(x, y);
    boatObject->m_extraInfo = g_game->createBoat(
        x, y, boatObject->m_z, -1, 1, boatType);
    return 0;
}
''',
    'grail': '''// Original: NewfullMap::readHolyGrailData, mapcell.cpp:1199, dc 0xedd14.
// Before normalization (locals): char_buffer. DC proves count, padding[3],
// both read checks and -1/0 status; readObject discards the return, so retail
// eliminates the final padding-read comparison from its inline expansion.
int NewfullMap::readHolyGrailData(TAbstractFile* infile, CObject* grailObject)
{
    char charBuffer;
    int count;
    count = infile->read(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;
    g_game->m_ultimateArtifactX = grailObject->m_x;
    g_game->m_ultimateArtifactY = grailObject->m_y;
    g_game->m_ultimateArtifactZ = grailObject->m_z;
    g_game->m_ultimateRadius = charBuffer;

    char padding[3];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
        return -1;
    return 0;
}

// Original: NewfullMap::readShrineData, mapcell.cpp:1224, dc 0xedde8.
// Before normalization (locals): char_buffer. The ordinary helper keeps its
// count local and both read checks. Its discarded final status leaves only
// the second virtual read in readObject's retail expansion.
int NewfullMap::readShrineData(TAbstractFile* infile, CObject* shrineObject)
{
    char charBuffer;
    int count;
    count = infile->read(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;
    shrineObject->m_shrineInfo.m_spell = charBuffer;

    char padding[3];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
        return -1;
    return 0;
}
''',
    'shipyard': '''// Original: NewfullMap::readShipyardData, mapcell.cpp:2383, dc 0xefe28.
// Before normalization (locals): char_buffer. DC proves the ordinary member,
// count/padding locals and two guarded reads. Complete defers the later DC
// trigger/terrain scan to loadShipyards; its readObject arm only initializes
// the two boat coordinates after the reads, then discards the status.
int NewfullMap::readShipyardData(TAbstractFile* infile, CObject* shipyardObject)
{
    char charBuffer;
    int count;
    count = infile->read(&charBuffer, sizeof(charBuffer));
    if (count < sizeof(charBuffer))
        return -1;
    shipyardObject->m_shipyardInfo.m_owner = charBuffer;

    char padding[3];
    count = infile->read(padding, sizeof(padding));
    if (count < sizeof(padding))
        return -1;
    shipyardObject->m_shipyardInfo.m_boatX = 0xff;
    shipyardObject->m_shipyardInfo.m_boatY = 0xff;
    return 0;
}
''',
}


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    edits = []
    for name, function in [('boat', 'readBoatData'), ('grail', 'readHolyGrailData'),
                           ('shipyard', 'readShipyardData')]:
        position = source.index('int NewfullMap::' + function + '(void* infile')
        start = source.rfind(CARCASS_START, 0, position)
        end = source.index(CARCASS_END, position) + len(CARCASS_END)
        if start < 0:
            raise ValueError('Review ordinary helper boundary: ' + function)
        edits.append({'source': SOURCE, 'find': source[start:end],
                      'replace': HELPERS[name].rstrip()})

    start = source.index(SIGNATURE)
    end = source.index('\n}', start) + 2
    body = source[start:end]
    for arm, next_arm, function in [
        ('BOAT', 'RANDOM_TOWN', 'readBoatData'),
        ('SHIPYARD', 'RANDOM_RESOURCE', 'readShipyardData'),
        ('HOLY_GRAIL', 'BLACK_BOX', 'readHolyGrailData'),
        ('SHRINE1', 'OCEAN_BOTTLE', 'readShrineData'),
    ]:
        begin = body.index('    case ' + arm + ':')
        finish = body.index('    case ' + next_arm + ':', begin)
        labels = ('    case SHRINE1:\n    case SHRINE2:\n    case SHRINE3:\n'
                  if arm == 'SHRINE1' else '    case ' + arm + ':\n')
        edits.append({'source': SOURCE, 'find': body[begin:finish],
                      'replace': labels + '        ' + function + '(infile, tempObject);\n        break;\n\n'})
    anchor = '    int readGeneratorData(TAbstractFile* infile, CObject* object);'
    declarations = '''    // Ordinary mapcell.cpp readers expanded in Complete's readObject.
    // DC source lines 1095, 1199, 1224 and 2383; pointer object parameters.
    int readBoatData(TAbstractFile* infile, CObject* boatObject);
    int readHolyGrailData(TAbstractFile* infile, CObject* grailObject);
    int readShrineData(TAbstractFile* infile, CObject* shrineObject);
    int readShipyardData(TAbstractFile* infile, CObject* shipyardObject);
'''
    edits.append({'source': 'include/mapcell.h', 'find': anchor,
                  'replace': declarations + anchor})
    units = []
    for block in subprocess.check_output(['ninja', '-t', 'deps'], cwd=ROOT, text=True).split('\n\n'):
        if str(ROOT / 'include/mapcell.h') in block:
            match = re.match(r'build/objdiff/base/(.+)\.obj:', block)
            if match:
                units.append(match[1])
    if not {'mapcell', 'game', 'advmgr'}.issubset(units):
        raise ValueError('Refresh dependency records with the full build')
    main, *extra = edits
    return {'schema': 1, 'source': SOURCE, 'units': sorted(set(units)), 'evidence': __doc__,
            'axes': [{'name': 'ordinary-map-readers', 'find': main['find'], 'options': [
                {'name': 'unchanged'}, {'name': 'source-helpers', 'replace': main['replace'],
                                        'extra_edits': extra}]}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
