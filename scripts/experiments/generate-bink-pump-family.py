"""Probe Bink pump guards while retaining canonical playback operations.

Retail 0x44daa0 retains serviceSounds, expands closeBink, and places the
not-ready return before the final-playback arm. Dreamcast's PC-port stubs
prove static BinkManager declarations, not these retail guard statements.
Compare equivalent pointer selection, readiness and paused scopes, and the
frame-end arm orientation. PlayBink's duplicated teardown is independently
replaced with its existing canonical closeBink call. Do not snapshot mutable
track globals across SDK/sound callbacks, add helpers, or pin inlining.
"""
import argparse
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/binkmanager.cpp'
PUMP = 'void BinkManager::nextBinkFrame()'
PLAY = 'int BinkManager::playBink(int id, int x, int y, int w, int h)'
CLOSE = '''        if (g_binkVideo) {
            _BinkPause(g_binkVideo, 1);
            _BinkClose(g_binkVideo);
        }
        if (g_binkVideo2) {
            _BinkPause(g_binkVideo2, 1);
            _BinkClose(g_binkVideo2);
        }
        g_binkVideo2 = 0;
        g_binkVideo = 0;
        g_binkPaused = 0;
        g_binkFrameReady = 0;
        g_binkDirty = 0;'''


def body_at(source, signature):
    start = source.index(signature + '\n{')
    return source[start:source.index('\n}', start) + 2]


def pump_variants(body):
    select = '''    Bink* video = g_binkVideo;
    if (!video)
        video = g_binkVideo2;
'''
    begin = '    if (video && g_binkFrameReady && !_BinkWait(video)) {\n'
    tail = '    }\n\n    g_binkDirty = 0;\n}'
    if body.count(select) != 1 or body.count(begin) != 1 or not body.endswith(tail):
        raise ValueError('Review the pump guard baseline')
    payload = body.split(begin, 1)[1][:-len(tail)]
    frame = '        if (video->m_frameNum == video->m_frames) {\n'
    start = payload.index(frame)
    stop = payload.index('        if (g_binkUseDirtyRects)', start)
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
            prefix = '''        g_binkDirty = 1;
        if (g_binkPaused)
            return;

'''
            if not core.startswith(prefix) or not core.endswith('        return;\n'):
                raise ValueError('Review paused-frame effects')
            work = core[len(prefix):-len('        return;\n')]
            core = ('        g_binkDirty = 1;\n        if (!g_binkPaused) {\n'
                    + ''.join('    ' + line if line.strip() else line for line in work.splitlines(True))
                    + '        }\n        return;\n')
        selected = select if not selection else '    Bink* video = g_binkVideo ? g_binkVideo : g_binkVideo2;\n'
        if readiness == 0:
            guards = begin + core + tail
        elif readiness == 1:
            guards = ('    if (video) {\n        if (g_binkFrameReady) {\n'
                + '            if (!_BinkWait(video)) {\n'
                + ''.join('        ' + line if line.strip() else line for line in core.splitlines(True))
                + '            }\n        }\n' + tail)
        else:
            guards = ('''    if (!video || !g_binkFrameReady || _BinkWait(video)) {
        g_binkDirty = 0;
        return;
    }
''' + ''.join(line[4:] if line.startswith('    ') else line for line in core.splitlines(True)) + '}')
        yield (f'select-{selection}-ready-{readiness}-pause-{paused}-frame-{orientation}',
               PUMP + '\n{\n' + selected + guards)


def make_manifest():
    source = (ROOT / SOURCE).read_text()
    pump, play = body_at(source, PUMP), body_at(source, PLAY)
    close_call = '        BinkManager::closeBink();'
    if play.count(CLOSE) == 1:
        original_play = play
    elif play.count(close_call) == 1:
        original_play = play.replace(close_call, CLOSE)
    else:
        raise ValueError('Review the canonical playback cleanup control')
    play_choices = [original_play, original_play.replace(CLOSE, close_call)]
    if play not in play_choices:
        raise ValueError('Playback body is outside the reviewed cleanup family')
    pumps = list(pump_variants(pump))
    assert pumps[0][1] == pump
    options = [{'name': 'unchanged'}]
    for (name, candidate), close in itertools.product(pumps, range(2)):
        candidate_play = play_choices[close]
        if candidate == pump and candidate_play == play:
            continue
        option = {'name': name + f'-close-{close}', 'replace': candidate}
        if candidate_play != play:
            option['extra_edits'] = [{'source': SOURCE, 'find': play,
                'replace': candidate_play}]
        options.append(option)
    assert len(options) == 48
    return {'schema': 1, 'source': SOURCE, 'units': ['binkmanager'], 'evidence': __doc__,
            'axes': [{'name': 'bink-pump-and-cleanup', 'find': pump, 'options': options}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
