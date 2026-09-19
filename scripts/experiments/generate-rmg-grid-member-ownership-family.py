#!/usr/bin/env python3
"""Four meaningful member-definition ownership forms of the generic point.

RMG has no Dreamcast counterpart proving in-class/inline placement. Keep the
retail two-reference constructor ABI and every canonical accessor declaration,
expression and call; test in-class bodies versus ordinary out-of-class template
definitions in the same header. This probes deferred template member emission,
not arbitrary definition-order permutations or changed coordinate expressions.
"""
import argparse
import json
import os
from pathlib import Path

from homm3.vc6.source_families import load_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    root = Path(os.environ['HOMM3_DIR'])
    header = (root / 'include/rmg.h').read_text()
    start = header.index('template<class Coordinate> struct TRmgCoordinatePoint {')
    end = header.index('\n};', start) + 3
    original = header[start:end]
    constructor = (
        '    // VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const unsigned int&, const unsigned int&)\n'
        '    VA(0x005B76B0, 0x18)\n'
        '    TRmgCoordinatePoint(const Coordinate& newX, const Coordinate& newY)\n'
        '        : m_x(newX), m_y(newY) {}')
    members = [
        ('Coordinate getX() const', 'return m_x;'),
        ('Coordinate getY() const', 'return m_y;'),
        ('void setX(Coordinate newX)', 'm_x = newX;'),
        ('void setY(Coordinate newY)', 'm_y = newY;'),
    ]
    options = []
    for accessor_out, constructor_out in ((False, False), (True, False),
                                           (False, True), (True, True)):
        replacement = original
        definitions = []
        if constructor_out:
            assert replacement.count(constructor) == 1
            replacement = replacement.replace(constructor,
                '    TRmgCoordinatePoint(const Coordinate& newX, const Coordinate& newY);')
            definitions.append(
                'template<class Coordinate>\n'
                '// VA instance: TRmgCoordinatePoint<unsigned int>::TRmgCoordinatePoint(const unsigned int&, const unsigned int&)\n'
                'VA(0x005B76B0, 0x18)\n'
                'TRmgCoordinatePoint<Coordinate>::TRmgCoordinatePoint(\n'
                '    const Coordinate& newX, const Coordinate& newY)\n'
                '    : m_x(newX), m_y(newY) {}')
        if accessor_out:
            for signature, expression in members:
                before = '    ' + signature + ' { ' + expression + ' }'
                assert replacement.count(before) == 1
                replacement = replacement.replace(before, '    ' + signature + ';')
                result, member = signature.split(' ', 1)
                definitions.append('template<class Coordinate>\n' + result +
                    ' TRmgCoordinatePoint<Coordinate>::' + member +
                    '\n{ ' + expression + ' }')
        if definitions:
            replacement += '\n\n' + '\n\n'.join(definitions)
        options.append(dict(name=('accessors-out' if accessor_out else 'accessors-in') +
                            '+' + ('constructor-out' if constructor_out else 'constructor-in'),
                            replace=replacement))
    manifest = dict(schema=1,
        units=['rmg', 'rmg_support', 'rmg_terrain', 'scenarioinfo',
               'singleselectionpopups', 'singleselectionwindow', 'tiles'],
        evidence=__doc__,
        axes=[dict(name='coordinate_member_ownership', source='include/rmg.h',
                   find=original, options=options)])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, indent=2) + '\n')
    load_manifest(args.output, root)
    print('4 finite member-ownership forms ->', args.output)


if __name__ == '__main__':
    main()
