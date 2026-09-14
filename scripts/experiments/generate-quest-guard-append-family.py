"""Refine the real QUEST_GUARD append boundary after map-reader restoration.

DC readObject owns a function-scope int count and separate read/result-test
statements for its five leading fields. Restore that supported local, then
compare public vector append spelling and actual iterator/reference scopes
in Complete's QUEST_GUARD arm. Retail keeps the two-argument guard and data
inserts and guard size query at 0x503472/0x50349c/0x503479. Preserve the
ordinary constructor/read calls, record values, append-before-index order,
quest-null guard and opaque data registration. Existing pins elsewhere in
the caller are unchanged. No wrapper, dummy iterator or copied vector body
is introduced. Count-free choices isolate the old negative source control.
"""
import argparse
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/mapcell.cpp'
SIGNATURE = 'int NewfullMap::readObject('


def original_body():
    source = (ROOT / SOURCE).read_text()
    start = source.index(SIGNATURE)
    return source[start:source.index('\n}', start) + 2]


def guard_arm(body):
    start = body.index('    case QUEST_GUARD: {')
    return body[start:body.index('    case WITCH_HUT:', start)]


def variants(body):
    old = guard_arm(body)
    expected = '''    case QUEST_GUARD: {
        TQuestGuard tempGuard;
        tempGuard.read(infile);
        {
            std::vector<TQuestGuard>::iterator guardEnd
                = m_questGuardList.end();
            m_questGuardList.insert(guardEnd, tempGuard);
            tempObject->m_extraInfo = m_questGuardList.size() - 1;
        }
        if (tempGuard.m_quest) {
            CMapObjectData* questData = static_cast<CMapObjectData*>(
                static_cast<void*>(tempGuard.m_quest));
            std::vector<CMapObjectData*>::iterator dataEnd
                = m_mapObjectData.end();
            m_mapObjectData.insert(dataEnd, questData);
        }
        break;
    }

'''
    # Recognize a previously adopted member of this finite family by an
    # exact round trip. Never discard an unreviewed arm or count operation.
    control = body.replace(old, expected)
    prefix_end = control.index('    switch (')
    prefix = control[:prefix_end]
    if '    int count;' in prefix:
        for args, value, number in [('&value, sizeof(value)', 'value', 3),
                                    ('&typeIndex, sizeof(typeIndex)', 'typeIndex', 1),
                                    ('padding, sizeof(padding)', 'padding', 1)]:
            staged = ('    count = infile->read(' + args + ');\n'
                      '    if (count < sizeof(' + value + '))')
            if prefix.count(staged) != number:
                raise ValueError('Review the staged leading read: ' + args)
            prefix = prefix.replace(staged, '    if (infile->read(' + args
                                    + ') < sizeof(' + value + '))')
        prefix = prefix.replace('    int count;\n', '', 1)
        control = prefix + control[prefix_end:]
    result = []
    for count, binding, append, data_end in itertools.product(range(2), range(3), range(3), range(2)):
        candidate = control
        if count:
            if '    int count;' in candidate[:candidate.index('    switch (')]:
                raise ValueError('Review the existing count local')
            candidate = candidate.replace('    char value;', '    int count;\n    char value;', 1)
            prefix_end = candidate.index('    switch (')
            prefix = candidate[:prefix_end]
            for args, value, number in [('&value, sizeof(value)', 'value', 3),
                                        ('&typeIndex, sizeof(typeIndex)', 'typeIndex', 1),
                                        ('padding, sizeof(padding)', 'padding', 1)]:
                read = '    if (infile->read(' + args + ') < sizeof(' + value + '))'
                if prefix.count(read) != number:
                    raise ValueError('Review the leading field read: ' + args)
                prefix = prefix.replace(read, '    count = infile->read(' + args + ');\n'
                                        '    if (count < sizeof(' + value + '))')
            candidate = prefix + candidate[prefix_end:]
        arm = expected
        if append:
            named = '''            std::vector<TQuestGuard>::iterator guardEnd
                = m_questGuardList.end();
            m_questGuardList.insert(guardEnd, tempGuard);'''
            call = ('m_questGuardList.insert(m_questGuardList.end(), tempGuard);' if append == 1
                    else 'm_questGuardList.push_back(tempGuard);')
            arm = arm.replace(named, '            ' + call)
        if binding:
            arm = arm.replace('m_questGuardList.', 'guards.')
            declaration = 'std::vector<TQuestGuard>& guards = m_questGuardList;\n'
            if binding == 1:
                marker = '        tempGuard.read(infile);\n'
                arm = arm.replace(marker, marker + '        ' + declaration)
            else:
                arm = arm.replace('        {\n', '        {\n            ' + declaration, 1)
        if data_end:
            arm = arm.replace('''            std::vector<CMapObjectData*>::iterator dataEnd
                = m_mapObjectData.end();
            m_mapObjectData.insert(dataEnd, questData);''',
                              '            m_mapObjectData.insert(m_mapObjectData.end(), questData);')
        candidate = candidate.replace(expected, arm)
        result.append((f'count-{count}-binding-{binding}-append-{append}-data-end-{data_end}', candidate))
    if not any(candidate == body for _, candidate in result):
        raise ValueError('Review the changed quest-guard arm before searching')
    return result


def make_manifest():
    body = original_body()
    choices = variants(body)
    if any(name not in body for name in ('readBoatData(', 'readHolyGrailData(', 'readShrineData(', 'readShipyardData(')):
        raise ValueError('Restore the four ordinary readers before this local search')
    return {'schema': 1, 'source': SOURCE, 'units': ['mapcell'], 'evidence': __doc__,
            'axes': [{'name': 'quest-guard-append', 'find': body, 'options': [
                {'name': 'unchanged'}] + [{'name': name, 'replace': candidate}
                                        for name, candidate in choices if candidate != body]}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
