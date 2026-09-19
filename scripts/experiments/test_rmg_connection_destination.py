#!/usr/bin/env python3
"""Apply the opaque-mutation count oracle to actual new accessor bodies."""
from homm3.vc6.test_rmg_families import generator
base=generator('test_rmg_connection_count_owner.py')
model=generator('generate-rmg-connection-destination-family.py')
base.PRE=base.PRE.replace('struct TRmgTownSlot {','struct TRmgTownSlot { int getZoneIndex() const;')
base.PRE=base.PRE.replace('struct TRmgZoneConnection {','struct TRmgZoneConnection { TRmgTownSlot* getDestination() const; int getDestinationZoneIndex() const;')
base.PRE+='\n'+model.SLOT+'\n'+model.DEST+'\n'+model.INDEX
base.main()
