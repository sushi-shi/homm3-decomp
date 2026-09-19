#!/usr/bin/env python3
"""Comparison-only spelling aliases for the three admitted byte flag slots.

Preserve raw compiled objects. Only full createTreasureObject symbol spellings
(and their compiler-generated owner suffixes) may be aliased, with a bijection
and unchanged section/relocation/symbol metadata asserted. No code bytes change.
Bool-vs-byte caller conversions remain visible and are scored normally.
"""
import hashlib
import itertools
import json
import sys
from pathlib import Path
from homm3.build.canonicalize_data_symbols import CoffObject, _rewrite_names
from homm3.vc6 import source_families

PREFIX='?createTreasureObject@type_random_map_generator@@QAEPAVtype_object@@PAUTRmgZone@@HHPAH'
SUFFIX='UTRmgMapPosition@@@Z'
CANONICAL=PREFIX+'EEE'+SUFFIX
def flag_mangling(types):
    # VC6 bool is back-reference eligible. Zone* and value* occupy slots 0/1;
    # the first bool occupies slot 2, unlike primitive unsigned char.
    seen_bool = False
    encoded = []
    for typ in types:
        encoded.append('2' if typ == '_N' and seen_bool else typ)
        seen_bool |= typ == '_N'
    return ''.join(encoded)


ALIASES={PREFIX+flag_mangling(types)+SUFFIX:CANONICAL
         for types in itertools.product(('E','_N'),repeat=3)}


def nominal_object(payload):
    before=CoffObject(payload); renames={};destinations={};rows=[]
    for symbol in before.symbols.values():
        name=symbol.name
        for variant,target in ALIASES.items():
            if variant in name:
                name=name.replace(variant,target)
                break
        if 'createTreasureObject@' in name and CANONICAL not in name:
            raise ValueError('unexpected treasure creation symbol: '+name)
        if name in destinations and destinations[name]!=symbol.name:
            raise ValueError('alias collision: '+name)
        destinations[name]=symbol.name
        if name!=symbol.name:
            renames[symbol.index]=name
            rows.append(dict(index=symbol.index,source=symbol.name,target=name))
    result=_rewrite_names(before,renames) if renames else payload
    after=CoffObject(result)
    assert before.sections==after.sections and before.relocations==after.relocations
    assert before.symbols.keys()==after.symbols.keys()
    for section in before.sections:
        assert before.section_bytes(section)==after.section_bytes(section)
    for index,a in before.symbols.items():
        b=after.symbols[index]
        assert (a.index,a.offset,a.value,a.section,a.typ,a.storage_class,a.aux_count)==(b.index,b.offset,b.value,b.section,b.typ,b.storage_class,b.aux_count)
        assert b.name==renames.get(index,a.name)
        start=a.offset+18;end=start+18*a.aux_count
        assert before.data[start:end]==after.data[start:end]
    return result,rows


def main():
    payload=json.loads(Path(sys.argv[1]).read_text())
    assert payload['diagnostic_runner_sha256']==hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    original=source_families.compile_candidate
    def compile_alias(candidate_root,unit,output):
        raw=original(candidate_root,unit,output)
        encoded,rows=nominal_object(raw.read_bytes())
        view=raw.with_name('candidate.nominal.obj');view.write_bytes(encoded)
        (Path(output)/'nominal-aliases.json').write_text(json.dumps(dict(aliases=rows,section_bytes_unchanged=True,relocations_unchanged=True,symbol_metadata_unchanged=True),indent=2)+'\n')
        return view
    source_families.compile_candidate=compile_alias
    return source_families.main()


if __name__=='__main__':
    raise SystemExit(main())
