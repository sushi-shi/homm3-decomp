"""Names consumed by the Gruntz data oracles from the canonical Model.

The existing HoMM3 PDB writer serializes these alongside its admitted full
function inventory. Segment numbers come from the pinned PE, not Gruntz's
section ordering or incremental-link thunk band.
"""
UNIT_CHANNELS = ('src', 'src_compgen', 'src_dyninit', 'functions_zlib')


def unit_names(model):
    return {b.rva: (b.name, b.unit, b.size) for b in model.functions
            if b.channel in UNIT_CHANNELS and b.name}
