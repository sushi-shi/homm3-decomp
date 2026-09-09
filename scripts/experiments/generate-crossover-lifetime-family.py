"""Probe real local lifetimes in retail-only campaign crossover pruning.

Retail 0x489e20 retains ScenarioStruct::markCrossoverHeroes, vector appends,
std::sort workers and vector assignment. The source keeps all those canonical
calls but VC6 currently chooses different nested expansions. Compare naming
an already used hero reference/pointer, the repeated scenario vector, and
sort endpoints; compare front() with the public begin() dereference. No
phase helper is invented and no algorithm operation, virtual call, repeated
inflated-size read, canonical max call, or artifact accessor is removed.
"""
import argparse
import itertools
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = 'src/customcampaign.cpp'
SIGNATURE = 'void SCampaign::pruneCrossoverHeroes(void* campaignHeader)'


def original_body():
    source = (ROOT / SOURCE).read_text()
    start = source.index(SIGNATURE + '\n{')
    return source[start:source.index('\n}', start) + 2]


def variants(body):
    old_hero = '''        for (unsigned int which = pooled.size(); which--;) {
            if (wanted[pooled[which].m_id]) {
                kept.push_back(pooled[which]);'''
    if body.count(old_hero) != 1 or body.count('std::sort(') != 2:
        raise ValueError('Review the crossover caller before refining its lifetimes')
    results = []
    for hero_binding, scenarios, sorting, front in itertools.product(range(3), range(2), range(3), range(2)):
        candidate = body
        if hero_binding:
            declaration = ('const hero& candidate = pooled[which];' if hero_binding == 1
                           else 'const hero* candidate = &pooled[which];')
            value = 'candidate' if hero_binding == 1 else '*candidate'
            field = 'candidate.m_id' if hero_binding == 1 else 'candidate->m_id'
            replacement = ('        for (unsigned int which = pooled.size(); which--;) {\n'
                           '            ' + declaration + '\n'
                           '            if (wanted[' + field + ']) {\n'
                           '                kept.push_back(' + value + ');')
            candidate = candidate.replace(old_hero, replacement)
        if scenarios:
            candidate = candidate.replace('header->m_scenarios', 'scenarios')
            marker = '    unsigned char wanted[game::HERO_COUNT];'
            declaration = ('    const std::vector<TCampaignBrief::ScenarioStruct*>& scenarios\n'
                           '        = header->m_scenarios;\n\n')
            candidate = candidate.replace(marker, declaration + marker, 1)
        if sorting:
            for vector in ('pooled', 'kept'):
                old = '        std::sort(' + vector + '.begin(), ' + vector + '.end(), CrossoverHeroStronger());'
                if candidate.count(old) != 1:
                    raise ValueError('Review sort boundary ' + vector)
                if sorting == 1:
                    new = ('        {\n            hero* first = ' + vector + '.begin();\n'
                           '            hero* last = ' + vector + '.end();\n'
                           '            std::sort(first, last, CrossoverHeroStronger());\n        }')
                else:
                    prefix = ('        hero* first = ' if vector == 'pooled' else '        first = ')
                    last = ('        hero* last = ' if vector == 'pooled' else '        last = ')
                    new = (prefix + vector + '.begin();\n' + last + vector + '.end();\n'
                           '        std::sort(first, last, CrossoverHeroStronger());')
                candidate = candidate.replace(old, new)
        if front:
            if candidate.count('kept.push_back(pooled.front());') != 1:
                raise ValueError('Review guarded strongest-hero access')
            candidate = candidate.replace('kept.push_back(pooled.front());', 'kept.push_back(*pooled.begin());')
        results.append((f'hero-{hero_binding}-scenarios-{scenarios}-sort-{sorting}-front-{front}', candidate))
    return results


def make_manifest():
    body = original_body()
    choices = variants(body)
    assert choices[0][1] == body
    return {'schema': 1, 'source': SOURCE, 'units': ['customcampaign'], 'evidence': __doc__,
            'axes': [{'name': 'crossover-local-lifetimes', 'find': body, 'options': [
                {'name': 'unchanged'}] + [{'name': name, 'replace': candidate}
                                        for name, candidate in choices[1:]]}]}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(make_manifest(), indent=2) + '\n')
