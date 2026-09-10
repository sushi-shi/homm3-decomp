"""Recover the flat visibility allocation proved by DC AI_attempt_puzzle_guess.

DC 0x115f64 has unsigned char visible[156] for its 13x12 puzzle; Complete's
retail dimensions are 19x17. Passing visible[0] from a reconstructed 2D array
gives markPuzzle only the first row's array object. Preserve the proven flat
owner and compare direct indexing with a named read-only row binding.
"""
import json
from pathlib import Path
import sys

source = Path('src/puzzlewindow.cpp').read_text()
start = source.index('type_point aiAttemptPuzzleGuess(long player)')
original = source[start:source.index('\n}',start)+2]
flat = original.replace('unsigned char visible[17][19];','unsigned char visible[17 * 19];').replace(
    'bitmap->markPuzzle(visible[0],','bitmap->markPuzzle(visible,').replace('visible[row][col]','visible[row * 19 + col]')
row = flat.replace('            for (int row = 0; row < 17; ++row) {',
    '            for (int row = 0; row < 17; ++row) {\n'
    '                const unsigned char* visibleRow = visible + row * 19;').replace(
    'visible[row * 19 + col]', 'visibleRow[col]')
Path(sys.argv[1]).write_text(json.dumps({'schema':1,'source':'src/puzzlewindow.cpp',
    'units':['puzzlewindow'],'axes':[{'name':'visibility-owner','find':original,'options':[
        {'name':'first-row-control'}, {'name':'dc-flat-visibility','replace':flat},
        {'name':'dc-flat-visible-row','replace':row}]}]},indent=2)+'\n')
