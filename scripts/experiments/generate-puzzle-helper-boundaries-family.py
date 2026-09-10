"""Restore the two ordinary DC puzzle helpers, preserving the flat owner.

DC 0x115838/0x115944 prove static helpers and the tile-array reference.
Complete changes 13x12 to 19x17; its retail setup-alignment load and virtual
bitmap disposal replace the older get_alignment/ResourceManager calls.
The recovered helpers do not inherit the flattened caller's inline pins.
"""
import json
from pathlib import Path
import sys
import textwrap

source = Path('src/puzzlewindow.cpp').read_text()
start = source.index('type_point aiAttemptPuzzleGuess(long player)')
caller = source[start:source.index('\n}', start)+2]
carcass_start = source.index('#if 0  // @carcass: remaining AI/puzzle helpers are not reconstructed yet')
carcass = source[carcass_start:source.index('#endif  // @carcass', carcass_start)+len('#endif  // @carcass')]
mark_start = caller.index('        long puzzle;')
mark_end = caller.index('            type_point origin =')
mark_region = caller[mark_start:mark_end]
loop_start = mark_region.index('            for (int i = 0; i < 48; ++i) {')
mark_loop = textwrap.dedent('\n'.join(line for line in mark_region[loop_start:].rstrip().splitlines()
    if not line.startswith('#pragma')))
mark_loop = mark_loop.replace('g_puzzlePiecesRemoved.test(i)', 'g_puzzlePiecesRemoved[i]')
mark_helper = '''// E:\\gamedcs\\puzzlewindow.cpp:403. Before normalization: mark_AI_puzzle.
// Complete reads setup alignment directly and disposes through the bitmap
// vtable; those retail operations override the older DC callees.
DC_ONLY(0x115838, 0x10A)
static unsigned char markAIPuzzle(long player, unsigned char* visible)
{
    long puzzle;
    if (player < 0 || (puzzle = g_game->m_setup.m_alignment[player]) == -1)
        puzzle = 0;
    if (g_game->m_ultimateArtifactX < 0 || !g_game->m_ultimateArtifactPresent)
        return 0;
    if (!g_game->setupPuzzlePieces(player, 0))
        return 0;
    memset(visible, 1, 17 * 19);
''' + textwrap.indent(mark_loop, '    ') + '\n    return 1;\n}\n'
create_start = caller.index('            type_point point;')
create_end = caller.index('            type_point guess =')
create_region = caller[create_start:create_end]
create_body = textwrap.dedent('\n'.join(line for line in create_region.splitlines() if not line.startswith('#pragma')))
create_body = create_body.replace('origin.m_x', 'puzzleX').replace('origin.m_y', 'puzzleY')
lookup = 'NewmapCell* mapCell = g_game->m_worldMap.cell(\n                point.m_x, point.m_y, point.m_z);'
assert lookup in create_body
create_body = create_body.replace(lookup, 'NewmapCell* mapCell = g_game->getCell(point);')
create_helper = '''// E:\\gamedcs\\puzzlewindow.cpp:445. Before normalization: create_AI_puzzle_map,
// puzzle_x, puzzle_y, puzzle_map. DC proves the array reference and point local.
// Complete's tile dimensions are 19x17, independently fixed by retail strides.
DC_ONLY(0x115944, 0x12C)
static void createAIPuzzleMap(long player, unsigned char* visible,
                            long puzzleX, long puzzleY,
                            type_AI_puzzle_tile (&puzzleMap)[19][17])
{
''' + textwrap.indent(create_body.rstrip(), '    ') + '\n}\n'
assert '                NewmapCell* mapCell = g_game->getCell(point);' in create_helper
direct_helper = create_helper.replace('                NewmapCell* mapCell = g_game->getCell(point);\n', '').replace('type_AI_puzzle_tile(mapCell, point)', 'type_AI_puzzle_tile(g_game->getCell(point), point)')

options = [{'name':'flattened-control'}]
for mark, create in [(True,0),(False,1),(False,2),(True,1),(True,2)]:
    body = caller
    if mark:
        body = body.replace(mark_region, '        unsigned char visible[17 * 19];\n        if (markAIPuzzle(player, visible)) {\n')
    if create:
        body = body.replace(create_region,
            '            createAIPuzzleMap(player, visible, origin.m_x, origin.m_y, puzzleMap);\n\n')
    helpers = mark_helper if mark else ''
    helpers += (create_helper if create == 1 else direct_helper) if create else ''
    options.append({'name':f'ordinary-mark{int(mark)}-create{create}', 'replace':body,
        'extra_edits':[{'source':'src/puzzlewindow.cpp','find':carcass,'replace':helpers}]})
Path(sys.argv[1]).write_text(json.dumps({'schema':1,'source':'src/puzzlewindow.cpp',
    'units':['puzzlewindow'],'axes':[{'name':'canonical-puzzle-helpers','find':caller,'options':options}]},indent=2)+'\n')
print('Wrote six original-helper boundary forms')
