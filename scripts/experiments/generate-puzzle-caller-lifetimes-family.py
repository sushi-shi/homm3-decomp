"""Flat-visible ownership and origin construction under the recovered helpers.

DC caller line 624 attributes the threshold test and mark_AI_puzzle call
together; lines 615..617 leave room for declarations, without proving their
text. Test the compound guard and real array/origin lifetimes; no dummy mass.
"""
import json
from pathlib import Path
import sys

source = Path('src/puzzlewindow.cpp').read_text()
start = source.index('type_point aiAttemptPuzzleGuess(long player)')
original = source[start:source.index('\n}',start)+2]
declaration = '        unsigned char visible[17 * 19];\n'
threshold = 'g_puzzleGuessThreshold[g_game->m_setup.m_difficulty] <= uncovered'
outer = '    if (' + threshold + ') {\n'
inner = '        if (markAIPuzzle(player, visible)) {\n'
assert outer+declaration+inner in original


def closing(text,start):
    depth,end=1,start+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return end


scope = original.replace(declaration,'').replace('    type_point result;\n',
    '    type_point result;\n    unsigned char visible[17 * 19];\n')
begin=original.index(outer)
inside=original.index(inner,begin)+len(inner)
inside_end=closing(original,original.index('{',original.index(inner,begin)))
end=closing(original,original.index('{',begin))
contents='\n'.join(line[4:] if line.startswith('    ') else line for line in original[inside:inside_end-1].splitlines())
compound=original[:begin]+'    unsigned char visible[17 * 19];\n    if ('+threshold+'\n        && markAIPuzzle(player, visible)) {\n'+contents+'}\n'+original[end:]
early_compound=compound.replace('    unsigned char visible[17 * 19];\n','').replace(
    '    type_point result;\n','    type_point result;\n    unsigned char visible[17 * 19];\n')
init='type_point origin = g_game->getPuzzleOrigin();'
options=[{'name':'canonical-helper-control'}]
for label,body in [('nested-local-visible',original),('nested-function-visible',scope),
                   ('compound-late-visible',compound),('compound-early-visible',early_compound)]:
    for form in ['copy-init','direct-init','assign']:
        if label=='nested-local-visible' and form=='copy-init':continue
        if form=='direct-init':
            variant=body.replace(init,'type_point origin(g_game->getPuzzleOrigin());')
        elif form=='assign':
            indentation='            ' if label.startswith('nested') else '        '
            variant=body.replace(init,'type_point origin;\n'+indentation+'origin = g_game->getPuzzleOrigin();')
        else:variant=body
        options.append({'name':label+'-'+form,'replace':variant})
Path(sys.argv[1]).write_text(json.dumps({'schema':1,'source':'src/puzzlewindow.cpp',
    'units':['puzzlewindow'],'axes':[{'name':'caller-owner-lifetimes','find':original,'options':options}]},indent=2)+'\n')
print('Wrote twelve canonical-helper caller lifetimes')
