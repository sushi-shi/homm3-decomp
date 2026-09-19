#!/usr/bin/env python3
"""Canonical zone-size accessors and value ownership in canConnect.

Retail 0x532bd0 and authored source have all seven CFG blocks and sqrt/_ftol
calls in agreement. At +0x4f, otherSize and combinedSize exchange ECX/EBX.
The canonical getSize accessor already serves the position filter. Test each
receiver's source call together with scalar ownership and minimum selection,
without inventing a helper or changing its declaration/definition. Earlier
raw-field binding families did not settle the register assignment. No DC
counterpart exists for this Complete-only predicate.
"""
import argparse
import itertools
import json
from pathlib import Path
from homm3.core.common import HOMM3_DIR
from homm3.vc6 import source_families
from homm3.vc6.test_rmg_families import generator


def variants(original):
    for access, binding, minimum in itertools.product(range(4), range(5), range(3)):
        body = original
        if access & 1:
            body = body.replace('other->m_slot->m_size', 'other->getSize()')
        if access & 2:
            body = body.replace('m_slot->m_size', 'getSize()') if access & 1 else body.replace('int thisSize = m_slot->m_size;', 'int thisSize = getSize();')
        if binding in (1, 3):
            body = body.replace('int otherSize =', 'const int& otherSize =')
        if binding in (2, 3):
            body = body.replace('int thisSize =', 'const int& thisSize =')
        if binding == 4:
            body = body.replace('int otherSize =', 'const int otherSize =').replace('int thisSize =', 'const int thisSize =')
        old = '        int minimumSize = thisSize;\n        if (otherSize < minimumSize)\n            minimumSize = otherSize;'
        assert body.count(old) == 1
        if minimum == 1:
            body = body.replace(old, '        int minimumSize = otherSize;\n        if (thisSize < minimumSize)\n            minimumSize = thisSize;')
        elif minimum == 2:
            body = body.replace(old, '        int minimumSize = otherSize < thisSize ? otherSize : thisSize;')
        yield dict(name=f'access_{access}+binding_{binding}+minimum_{minimum}', replace=body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    original = generator('generate-rmg-connect-owners-family.py').definition((HOMM3_DIR/'src/rmg.cpp').read_text())
    options = list(variants(original))
    assert len(options) == len({x['replace'] for x in options}) == 60
    assert options[0]['replace'] == original
    payload = dict(schema=1, units=['rmg'], evidence=__doc__, axes=[dict(
        name='connection_size_accessors', source='src/rmg.cpp', find=original, options=options)])
    args.output.write_text(json.dumps(payload, indent=2)+'\n')
    source_families.load_manifest(args.output,HOMM3_DIR)
    print('60 accessor/binding/minimum states')


if __name__ == '__main__':
    main()
