"""Probe Bink pump guards while retaining canonical playback operations.

Retail 0x44daa0 retains serviceSounds, expands closeBink, and places the
not-ready return before the final-playback arm. Dreamcast's PC-port stubs
prove static BinkManager declarations, not these retail guard statements.
Compare equivalent pointer selection, readiness and paused scopes, and the
frame-end arm orientation. Keep the now-exact PlayBink body unchanged.
Do not snapshot mutable track globals across SDK/sound callbacks, add
helpers, or pin inlining.
"""
import argparse
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/binkmanager.cpp'
PUMP = 'void BinkManager::nextBinkFrame()'


def body_at(source, signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]


def pump_variants(body):
    select = '''    Bink* video = s_playingBink.m_bink;
    if (!video)
        video = s_playingBink.m_bink2;
'''
    begin = '    if (video && s_playingBinkActive && !_BinkWait(video)) {\n'
    tail = '    }\n\n    s_needsUpdate = 0;\n}'
    if body.count(select) != 1 or body.count(begin) != 1 or not body.endswith(tail):
        raise ValueError('Review the pump guard baseline')
    payload = body.split(begin, 1)[1][:-len(tail)]
    frame = '        if (video->m_frameNum == video->m_frames) {\n'
    start = payload.index(frame)
    stop = payload.index('        if (s_updateScreen)', start)
    old_frame = payload[start:stop]
    closing = '''        } else {
            _BinkNextFrame(video);
        }
'''
    if not old_frame.endswith(closing):
        raise ValueError('Review final-frame operation order')
    chain_arm = old_frame[len(frame):-len(closing)]
    flipped = ('''        if (video->m_frameNum != video->m_frames) {
            _BinkNextFrame(video);
        } else {
''' + chain_arm + '        }\n')
    for selection, readiness, paused, orientation in itertools.product(range(2), range(3), range(2), range(2)):
        core = payload.replace(old_frame, flipped) if orientation else payload
        if paused:
            prefix = '''        s_needsUpdate = 1;
        if (s_playingBink.m_paused)
            return;

'''
            if not core.startswith(prefix) or not core.endswith('        return;\n'):
                raise ValueError('Review paused-frame effects')
            work = core[len(prefix):-len('        return;\n')]
            core = ('        s_needsUpdate = 1;\n        if (!s_playingBink.m_paused) {\n'
                    + ''.join('    ' + line if line.strip() else line for line in work.splitlines(True))
                    + '        }\n        return;\n')
        selected = select if not selection else '    Bink* video = s_playingBink.m_bink ? s_playingBink.m_bink : s_playingBink.m_bink2;\n'
        if readiness == 0:
            guards = begin + core + tail
        elif readiness == 1:
            guards = ('    if (video) {\n        if (s_playingBinkActive) {\n'
                + '            if (!_BinkWait(video)) {\n'
                + ''.join('        ' + line if line.strip() else line for line in core.splitlines(True))
                + '            }\n        }\n' + tail)
        else:
            guards = ('''    if (!video || !s_playingBinkActive || _BinkWait(video)) {
        s_needsUpdate = 0;
        return;
    }
''' + ''.join(line[4:] if line.startswith('    ') else line for line in core.splitlines(True)) + '}')
        yield (f'select-{selection}-ready-{readiness}-pause-{paused}-frame-{orientation}',
               PUMP + '\n{\n' + selected + guards)


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    pump = body_at(source, PUMP)
    pumps = list(pump_variants(pump))
    assert pumps[0][1] == pump
    options = [{'name': 'unchanged'}]
    for name, candidate in pumps:
        if candidate != pump:
            options.append({'name': name, 'replace': candidate})
    assert len(options) == 24
    return {'schema': 1, 'source': SOURCE, 'units': ['binkmanager'], 'evidence': __doc__,
            'axes': [{'name': 'bink-pump', 'find': pump, 'options': options}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
