// event_record.cpp - E:\gamedcs\event_record.cpp (compiland event_record.obj)
#include <math.h>
#include <va.h>
#include "event_record.h"
#include "game.h"
#include "abstractfile.h"
#include "cursor.h"
#include "inputmgr.h"
#include "misc.h"
#include "textresource.h"
#include "kbwin.h"
#include "message.h"
#include "prefs.h"
#include "advmgr.h"
#include "kb.h"
#include "includes.h"

// Dreamcast CodeView attests this inline wrapper (Hero.h:196) and game.cpp
// carries the same local definition. It is what makes VC6 zero-extend the
// byte boat id into the long parameter, which is exactly the
// `xor eax,eax / mov al,[boat+0x19]` pair hide_boat::undo emits.
// The one save-file revision inside the [0x12,0x1e] window that omits the
// boat record's occupancy/hero-ID tail; type_record_hide_boat::load has to
// step over it, and the cleanliness floor wants the domain named.
const int g_saveVersionBoatFieldsAbsent = 0x1c;

// E:\gamedcs\event_record.cpp:36. NO RETAIL BODY of its own - every
// construction site expands it - but the expansions prove the whole body:
// the vptr store followed by `mov dl,byte ptr [gNetLocalGamePos] /
// mov [this+4],dl`, i.e. the acting seat truncated into the signed byte.
// type_record_shroud::create (0x49bc30) is the clearest witness.
EventRecord::EventRecord()
{
    m_playerId = g_netLocalGamePos;
}

VA_COMPGEN(0x0049a5b0, 0x23, SCALAR_DELETING_DTOR, EventRecord)

VA(0x0049a5e0, 0x1D)  // dc 0x8c678
unsigned char EventRecord::load(AbstractFile* infile, int version)
{
    return infile->read(&m_playerId, 1) == 1;
}

VA(0x0049a600, 0x1D)  // dc 0x8c698
unsigned char EventRecord::save(AbstractFile* outfile)
{
    return outfile->write(&m_playerId, 1) == 1;
}

// E:\gamedcs\event_record.cpp:65. Ordinary static helper, expanded
// into the four replay bodies and playRecordedEvents. The char parameter
// narrows the saved seat at the latter call; each replay passes m_playerId.
// Preserve these DC-proven source calls instead of copying the helper body.
static void setPlayer(char newPlayer)
{
    if (g_netLocalGamePos != newPlayer) {
        g_advManager->deactivateCurrTown(0);
        g_advManager->deactivateCurrHero(0);
    }
    g_netLocalGamePos = newPlayer;
    g_currentPlayer = &g_game->m_players[newPlayer];
    g_unnamed69ccc4 = 1 << newPlayer;
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:81
DC_ONLY(0x8c708, 0x4)
void EventRecord::replay()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:88
DC_ONLY(0x8c70c, 0x4)
void EventRecord::undo()
{
    // @stub
}

#endif  // @carcass

// E:\gamedcs\event_record.cpp:96
// NO RETAIL BODY: VC6 expands this constructor at record_move and
// record_teleport. Dreamcast gives seven ordered source rows and proves that
// line 100 obtains source through type_obscuring_object::get_location; retail
// corroborates the same packed x/y/z loads at both inline sites.
inline RecordMoveHero::RecordMoveHero(Hero* currentHero,
                                                    char direction,
                                                    type_point destination)
{
    m_currentHero = currentHero;
    m_restoreFlag = currentHero->m_facing;
    m_direction = direction;
    m_source = currentHero->getLocation();
    m_destination = destination;
}

#if 0  // @carcass

// E:\gamedcs\event_record.cpp:108
DC_ONLY(0x8c7c0, 0x26)
EventRecord* RecordMoveHero::create()
{
    // @stub
}

#endif  // @carcass

// Slot 0 of TEN derived vtables at once: 0x63de8c 0x63dea4 0x63debc 0x63ded4
// 0x63deec 0x63df04 0x63df1c 0x63df34 0x63df4c and 0x63df64 all name this
// address. Every one of those classes has a trivial implicit destructor that
// collapses to the base vptr store at 0x49abe0, so the ten wrappers were
// byte-identical and /OPT:ICF folded them onto one body; the claim names the
// first of the ten in link order.
VA_COMPGEN(0x0049a620, 0x21, SCALAR_DELETING_DTOR, RecordMoveHero)

VA(0x0049a650, 0x27)  // dc 0x8c7c0
EventRecord* RecordMoveHero::create()
{
    return new RecordMoveHero();
}

VA(0x0049a680, 0x6)  // dc 0x8c7e8
EventRecordType RecordMoveHero::getType() const
{
    return RECORD_MOVE_HERO;
}

VA(0x0049a690, 0xB1)  // dc 0x8c7ec
unsigned char RecordMoveHero::load(AbstractFile* infile, int version)
{
    if (infile->read(&m_playerId, 1) != 1)
        return 0;
    int heroId;
    if (infile->read(&heroId, sizeof(heroId)) != sizeof(heroId))
        return 0;
    m_currentHero = (heroId == -1) ? NULL : &g_game->m_heroes[heroId];
    if (infile->read(&m_direction, 1) != 1)
        return 0;
    if (infile->read(&m_source, sizeof(m_source)) != sizeof(m_source))
        return 0;
    unsigned char ok = infile->read(&m_destination, sizeof(m_destination)) == sizeof(m_destination);
    return ok;
}

VA(0x0049a750, 0x63)  // dc 0x8c8bc
unsigned char RecordMoveHero::save(AbstractFile* outfile)
{
    int heroId = m_currentHero->m_id;
    outfile->write(&m_playerId, 1);
    outfile->write(&heroId, sizeof(heroId));
    outfile->write(&m_direction, 1);
    outfile->write(&m_source, sizeof(m_source));
    unsigned char ok = outfile->write(&m_destination, sizeof(m_destination)) == sizeof(m_destination);
    return ok;
}

VA(0x0049a7c0, 0x144)  // dc 0x8c91c
void RecordMoveHero::replay(unsigned char draw)
{
    setPlayer(m_playerId);

    if (g_currentPlayer->m_currHeroId != m_currentHero->m_id
        || !g_advManager->m_curHeroMobile) {
        g_advManager->setHeroContext(m_currentHero->m_id, 1, 0, draw);
    }

    m_currentHero->m_facing = m_direction;
    g_completeDrawEnabled = draw && g_advManager->getMoveShowIt(m_currentHero, m_direction);
    if (g_completeDrawEnabled)
        g_advManager->m_drawCursor = 1;

    if (g_advManager->m_cursorDirection != m_direction)
        g_advManager->turnTo(m_direction);
    g_advManager->animateMove(m_currentHero, m_direction,
                               m_destination.m_x - m_source.m_x,
                               m_destination.m_y - m_source.m_y);
}

// E:\gamedcs\event_record.cpp:186

// Slot 5 of type_record_move_hero's retail vtable (0x63de8c), shared with
// type_record_teleport. The hero's `valid` byte is sampled BEFORE
// restore_cell clears it, which is what the leading `mov bl,[hero+6]` proves.
VA(0x0049a910, 0x65)  // anchor-vtable, dc 0x8c9ec
void RecordMoveHero::undo()
{
    unsigned char wasOnMap = m_currentHero->isOnMap();
    m_currentHero->restoreCell();
    m_currentHero->m_facing = m_restoreFlag;
    m_currentHero->m_x = m_source.m_x;
    m_currentHero->m_y = m_source.m_y;
    m_currentHero->m_z = m_source.m_z;
    if (wasOnMap)
        m_currentHero->obscureCell();
}

// E:\gamedcs\event_record.cpp:204
// NO RETAIL BODY: the complete construction is expanded into record_teleport.
// Dreamcast line 204 proves this remains a derived-to-base delegation rather
// than a flattened duplicate of type_record_move_hero's assignments.
inline RecordTeleport::RecordTeleport(Hero* currentHero,
                                                  type_point destination)
    : RecordMoveHero(currentHero, currentHero->m_facing, destination)
{
}

#if 0  // @carcass

// E:\gamedcs\event_record.cpp:211
DC_ONLY(0x8cac4, 0x26)
EventRecord* RecordTeleport::create()
{
    // @stub
}

#endif  // @carcass

VA(0x0049a980, 0x27)  // dc 0x8cac4
EventRecord* RecordTeleport::create()
{
    return new RecordTeleport();
}

VA(0x0049a9b0, 0x6)  // dc 0x8caec
EventRecordType RecordTeleport::getType() const
{
    return RECORD_TELEPORT;
}

VA(0x0049a9c0, 0x7B)  // dc 0x8caf0
void RecordTeleport::replay(unsigned char draw)
{
    setPlayer(m_playerId);

    g_advManager->teleportTo(m_currentHero, m_destination, 0, 0, draw, 1);
}
// E:\gamedcs\event_record.cpp:237
// NO RETAIL BODY: expanded into record_claim_mine. record_claim_town instead
// invokes the distinct default constructor at dc:0x8eda0. Dreamcast preserves
// this definition site and the id/new-owner/mine-owner statement order.
inline RecordClaimMine::RecordClaimMine(long id,
                                                      char newOwner)
{
    m_id = id;
    m_newOwner = newOwner;
    m_oldOwner = g_game->m_mines[id].m_playerOwner;
}

#if 0  // @carcass

// E:\gamedcs\event_record.cpp:247
DC_ONLY(0x8cb88, 0x26)
EventRecord* RecordClaimMine::create()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:255
DC_ONLY(0x8cbb0, 0x4)
EventRecordType RecordClaimMine::getType()
{
    // @stub
}

#endif  // @carcass

VA(0x0049aa40, 0x27)  // dc 0x8cb88
EventRecord* RecordClaimMine::create()
{
    return new RecordClaimMine();
}

VA(0x0049aa70, 0x71)  // dc 0x8cbb4
unsigned char RecordClaimMine::load(AbstractFile* infile, int version)
{
    if (infile->read(&m_playerId, 1) != 1)
        return 0;
    if (infile->read(&m_id, sizeof(m_id)) != sizeof(m_id))
        return 0;
    if (infile->read(&m_oldOwner, 1) != 1)
        return 0;
    unsigned char ok = infile->read(&m_newOwner, 1) == 1;
    return ok;
}

VA(0x0049aaf0, 0x4A)  // dc 0x8cc1c
unsigned char RecordClaimMine::save(AbstractFile* outfile)
{
    outfile->write(&m_playerId, 1);
    outfile->write(&m_id, sizeof(m_id));
    outfile->write(&m_oldOwner, 1);
    unsigned char ok = outfile->write(&m_newOwner, 1) == 1;
    return ok;
}

VA(0x0049ab40, 0x74)  // dc 0x8cc6c
void RecordClaimMine::replay(unsigned char draw)
{
    g_game->claimMine(m_id, m_newOwner, const_recorded_action);
    if (draw) {
        Mine& claimed = g_game->m_mines[m_id];
        if (getMapExtra(claimed.m_mapX, claimed.m_mapY, claimed.m_mapZ)
            & g_mapVisibilityBit) {
            g_advManager->completeDraw(0);
            g_advManager->updateScreen(0, 0);
        }
    }
}

VA(0x0049abc0, 0x19)  // dc 0x8ccd8
void RecordClaimMine::undo()
{
    g_game->m_mines[m_id].m_playerOwner = m_oldOwner;
}

VA(0x0049abe0, 0x7)  // dc 0x8c658
EventRecord::~EventRecord()
{
}
// E:\gamedcs\event_record.cpp:321
// Dreamcast resolves the base boundary specifically to the default header
// constructor at dc:0x8eda0, not the parameterized constructor at dc:0x8cb2c.
// The derived body then assigns the three claim fields, with old_owner coming
// from gpGame->towns. Retail corroborates that final assignment sequence and
// elides the intermediate claim_mine vptr store.
inline RecordClaimTown::RecordClaimTown(long id,
                                                      char newOwner)
    : RecordClaimMine()
{
    m_id = id;
    m_newOwner = newOwner;
    m_oldOwner = g_game->m_towns[id].m_owner;
}

#if 0  // @carcass

// E:\gamedcs\event_record.cpp:331
DC_ONLY(0x8cd5c, 0x26)
EventRecord* RecordClaimTown::create()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:339
// type_record_claim_town::get_type has no retail body of its own: slot 1 of
// its vtable (0x63ded4) is 0x16ebc0, outside this compiland's span, where
// /OPT:ICF folded the `mov eax,4 / ret` onto an identical body elsewhere.
DC_ONLY(0x8cd84, 0x54)
EventRecordType RecordClaimTown::getType()
{
    // @stub
}

#endif  // @carcass

VA(0x0049abf0, 0x27)  // dc 0x8cd5c
EventRecord* RecordClaimTown::create()
{
    return new RecordClaimTown();
}

VA(0x0049ac20, 0x7E)  // dc 0x8cdd8
void RecordClaimTown::replay(unsigned char draw)
{
    g_game->m_towns[m_id].m_owner = m_newOwner;
    if (draw) {
        Town& claimed = g_game->m_towns[m_id];
        if (getMapExtra(claimed.m_mapX, claimed.m_mapY, claimed.m_mapZ)
            & g_mapVisibilityBit) {
            g_advManager->completeDraw(0);
            g_advManager->updateScreen(0, 0);
        }
    }
}

VA(0x0049aca0, 0x1D)  // dc 0x8ce48
void RecordClaimTown::undo()
{
    g_game->m_towns[m_id].m_owner = m_oldOwner;
}
// E:\gamedcs\event_record.cpp:376
// Dreamcast's older record stores only the boat pointer here. Complete adds
// the replay state at +0xc/+0x10; record_hide_boat's retail `ret 0xc` and the
// two independent snapshot loads corroborate the revised constructor inputs.
inline RecordHideBoat::RecordHideBoat(Boat* currentBoat,
                                                    unsigned char occupied,
                                                    int occupyingHero)
{
    m_currentBoat = currentBoat;
    m_occupied = occupied;
    m_previousOccupied = currentBoat->m_occupied;
    m_occupyingHero = occupyingHero;
    m_previousOccupyingHero = currentBoat->m_occupyingHero;
}

#if 0  // @carcass

// E:\gamedcs\event_record.cpp:384
DC_ONLY(0x8ceb0, 0x26)
EventRecord* RecordHideBoat::create()
{
    // @stub
}

#endif  // @carcass

VA(0x0049acc0, 0x27)  // dc 0x8ceb0
EventRecord* RecordHideBoat::create()
{
    return new RecordHideBoat();
}

VA(0x0049acf0, 0x6)  // dc 0x8ced8
EventRecordType RecordHideBoat::getType() const
{
    return RECORD_HIDE_BOAT;
}

VA(0x0049ad00, 0xE7)  // dc 0x8cedc
unsigned char RecordHideBoat::load(AbstractFile* infile, int version)
{
    if (infile->read(&m_playerId, 1) != 1)
        return 0;
    signed char boatId;
    if (infile->read(&boatId, 1) != 1)
        return 0;
    if (version >= 0x12 && version != g_saveVersionBoatFieldsAbsent
        && (version <= 0x1e || version >= 0x23)) {
        {
            unsigned char flag;
            infile->read(&flag, 1);
            m_previousOccupied = flag != 0;
            infile->read(&flag, 1);
            m_occupied = flag != 0;
        }
        {
            // These serialized values are hero IDs.
            short heroId;
            infile->read(&heroId, sizeof(heroId));
            m_previousOccupyingHero = heroId;
            infile->read(&heroId, sizeof(heroId));
            m_occupyingHero = heroId;
        }
    } else {
        m_previousOccupied = 0;
        m_occupied = 1;
        m_previousOccupyingHero = -1;
        m_occupyingHero = -1;
    }
    m_currentBoat = &g_game->m_boats[boatId];
    return 1;
}

VA(0x0049adf0, 0x8A)  // dc 0x8cf2c
unsigned char RecordHideBoat::save(AbstractFile* outfile)
{
    outfile->write(&m_playerId, 1);
    if (outfile->write(&m_currentBoat->m_id, 1) != 1)
        return 0;
    {
        unsigned char b = m_previousOccupied;
        outfile->write(&b, 1);
    }
    {
        unsigned char b = m_occupied;
        outfile->write(&b, 1);
    }
    {
        short s = m_previousOccupyingHero;
        outfile->write(&s, sizeof(s));
    }
    {
        short s = m_occupyingHero;
        outfile->write(&s, sizeof(s));
    }
    return 1;
}

VA(0x0049ae80, 0x44)  // dc 0x8cf64
void RecordHideBoat::replay(unsigned char draw)
{
    m_currentBoat->m_occupied = m_occupied;
    m_currentBoat->m_occupyingHero = m_occupyingHero;
    m_currentBoat->restoreCell();
    if (draw) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
}

VA(0x0049aed0, 0x23)  // dc 0x8cf94
void RecordHideBoat::undo()
{
    m_currentBoat->m_occupied = m_previousOccupied;
    m_currentBoat->m_occupyingHero = m_previousOccupyingHero;
    m_currentBoat->obscureCell();
}
// E:\gamedcs\event_record.cpp:449
// DC line 450 calls type_obscuring_object::get_location. Retail inlines that
// helper into the packed x/y/z loads, so keep the source boundary even though
// spelling the three fields directly produces the same candidate bytes.
inline RecordShowBoat::RecordShowBoat(Boat* currentBoat,
                                                    type_point location)
    : RecordHideBoat(currentBoat, 0,
                            currentBoat->m_occupyingHero)
{
    m_previousLocation = currentBoat->getLocation();
    m_location = location;
}

#if 0  // @carcass

// E:\gamedcs\event_record.cpp:458
DC_ONLY(0x8d044, 0x26)
EventRecord* RecordShowBoat::create()
{
    // @stub
}

#endif  // @carcass

VA(0x0049af00, 0x27)  // dc 0x8d044
EventRecord* RecordShowBoat::create()
{
    return new RecordShowBoat();
}

VA(0x0049af30, 0x6)  // dc 0x8d06c
EventRecordType RecordShowBoat::getType() const
{
    return RECORD_SHOW_BOAT;
}

VA(0x0049af40, 0x51)  // dc 0x8d070
unsigned char RecordShowBoat::load(AbstractFile* infile, int version)
{
    if (!RecordHideBoat::load(infile, version))
        return 0;
    if (infile->read(&m_location, sizeof(m_location)) != sizeof(m_location))
        return 0;
    unsigned char ok = infile->read(&m_previousLocation, sizeof(m_previousLocation))
                       == sizeof(m_previousLocation);
    return ok;
}

VA(0x0049afa0, 0x9F)  // dc 0x8d110
unsigned char RecordShowBoat::save(AbstractFile* outfile)
{
    RecordHideBoat::save(outfile);
    outfile->write(&m_location, sizeof(m_location));
    unsigned char ok = outfile->write(&m_previousLocation, sizeof(m_previousLocation))
                       == sizeof(m_previousLocation);
    return ok;
}

VA(0x0049b040, 0xB5)  // dc 0x8d14c
void RecordShowBoat::replay(unsigned char draw)
{
    m_currentBoat->m_occupied = m_occupied;
    m_currentBoat->m_x = m_location.m_x;
    m_currentBoat->m_y = m_location.m_y;
    m_currentBoat->m_z = m_location.m_z;
    m_currentBoat->obscureCell();
    if (draw && (getMapExtra(m_location.m_x, m_location.m_y, m_location.m_z)
                 & g_mapVisibilityBit)) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
}

VA(0x0049b100, 0x4E)  // dc 0x8d1d8
void RecordShowBoat::undo()
{
    m_currentBoat->m_occupied = m_previousOccupied;
    m_currentBoat->restoreCell();
    m_currentBoat->m_x = m_previousLocation.m_x;
    m_currentBoat->m_y = m_previousLocation.m_y;
    m_currentBoat->m_z = m_previousLocation.m_z;
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:533
DC_ONLY(0x8d220, 0x70)
void RecordErase::RecordErase(type_point _location, long _object_id, unsigned long _extra_info, long _object_index)
{
    // @stub
}

// E:\gamedcs\event_record.cpp:544
DC_ONLY(0x8d290, 0x26)
EventRecord* RecordErase::create()
{
    // @stub
}

#endif  // @carcass

inline RecordErase::RecordErase(type_point location,
                                            long objectId,
                                            unsigned long extraInfo,
                                            long objectIndex)
{
    m_location = location;
    m_objectId = objectId;
    m_extraInfo = extraInfo;
    m_objectIndex = objectIndex;
}

// E:\gamedcs\event_record.cpp:544
VA(0x0049b150, 0x27)  // dc 0x8d290
EventRecord* RecordErase::create()
{
    return new RecordErase();
}

VA(0x0049b180, 0x6)  // dc 0x8d2b8
EventRecordType RecordErase::getType() const
{
    return RECORD_ERASE;
}

VA(0x0049b190, 0x8B)  // dc 0x8d2bc
unsigned char RecordErase::load(AbstractFile* infile, int version)
{
    if (infile->read(&m_playerId, 1) != 1)
        return 0;
    if (infile->read(&m_location, sizeof(m_location)) != sizeof(m_location))
        return 0;
    if (infile->read(&m_objectId, sizeof(m_objectId)) != sizeof(m_objectId))
        return 0;
    if (infile->read(&m_extraInfo, sizeof(m_extraInfo)) != sizeof(m_extraInfo))
        return 0;
    unsigned char ok = infile->read(&m_objectIndex, sizeof(m_objectIndex)) == sizeof(m_objectIndex);
    return ok;
}

VA(0x0049b220, 0x57)  // dc 0x8d338
unsigned char RecordErase::save(AbstractFile* outfile)
{
    outfile->write(&m_playerId, 1);
    outfile->write(&m_location, sizeof(m_location));
    outfile->write(&m_objectId, sizeof(m_objectId));
    outfile->write(&m_extraInfo, sizeof(m_extraInfo));
    unsigned char ok = outfile->write(&m_objectIndex, sizeof(m_objectIndex)) == sizeof(m_objectIndex);
    return ok;
}

VA(0x0049b280, 0xEA)  // dc 0x8d3c8
void RecordErase::replay(unsigned char draw)
{
    NewmapCell* cell = g_game->m_worldMap.cell(m_location);
    g_advManager->mobilizeCurrHero(1, 0, draw);
    g_advManager->eraseObj(cell, m_location, 0);
    if (draw && (getMapExtra(m_location.m_x, m_location.m_y, m_location.m_z)
                 & g_mapVisibilityBit)) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
    g_advManager->demobilizeCurrHero(0, draw);
}

VA(0x0049b370, 0x83)  // dc 0x8d46c
void RecordErase::undo()
{
    g_game->m_worldMap.placeObject(m_objectId, 0);
    NewmapCell* cell = g_game->m_worldMap.cell(m_location);
    cell->m_extraInfo = m_extraInfo;
    cell->m_objectIndex = m_objectIndex;
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:628
DC_ONLY(0x8d4b0, 0x50)
void RecordHideHero::RecordHideHero(Hero* _hero, char _owner)
{
    // @stub
}

// E:\gamedcs\event_record.cpp:638
DC_ONLY(0x8d500, 0x26)
EventRecord* RecordHideHero::create()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:646
DC_ONLY(0x8d528, 0x4)
EventRecordType RecordHideHero::getType()
{
    // @stub
}

#endif  // @carcass

inline RecordHideHero::RecordHideHero(Hero* who, char newOwner,
                                                    unsigned char townGarrison)
{
    // DC preserves this helper boundary; the two retail inline expansions
    // prove the snapshot is written before the requested replacement owner.
    m_currentHero = who;
    m_prevOwner = who->m_owner;
    m_newOwner = newOwner;
    m_townGarrison = townGarrison;
}

// E:\gamedcs\event_record.cpp:638
VA(0x0049b400, 0x27)  // dc 0x8d500
EventRecord* RecordHideHero::create()
{
    return new RecordHideHero();
}

VA(0x0049b430, 0xC8)  // dc 0x8d52c
unsigned char RecordHideHero::load(AbstractFile* infile, int version)
{
    if (infile->read(&m_playerId, 1) != 1)
        return 0;
    int heroId;
    if (infile->read(&heroId, sizeof(heroId)) != sizeof(heroId))
        return 0;
    m_currentHero = (heroId == -1) ? NULL : &g_game->m_heroes[heroId];
    if (infile->read(&m_newOwner, 1) != 1)
        return 0;
    if (infile->read(&m_prevOwner, 1) != 1)
        return 0;
    if (m_prevOwner < 0) {
        m_townGarrison = 0;
    } else {
        m_townGarrison = (static_cast<unsigned>(m_prevOwner) >> 6) & 1;
        m_prevOwner &= 0x3f;
    }
    return 1;
}

VA(0x0049b500, 0x61)  // dc 0x8d5a0
unsigned char RecordHideHero::save(AbstractFile* outfile)
{
    outfile->write(&m_playerId, 1);
    outfile->write(&m_currentHero->m_id, sizeof(m_currentHero->m_id));
    outfile->write(&m_newOwner, 1);
    unsigned char packed = m_prevOwner;
    if (m_townGarrison)
        packed |= 0x40;
    unsigned char ok = outfile->write(&packed, 1) == 1;
    return ok;
}

VA(0x0049b570, 0x102)  // dc 0x8d5f0
void RecordHideHero::replay(unsigned char draw)
{
    setPlayer(m_playerId);

    if (!m_townGarrison) {
        if (g_netLocalGamePos == m_prevOwner) {
            if (g_currentPlayer->m_currHeroId != m_currentHero->m_id
                || !g_advManager->m_curHeroMobile) {
                g_advManager->setHeroContext(m_currentHero->m_id, 1, 0, draw);
            }
        }
        m_currentHero->restoreCell();
    }
    m_currentHero->restoreCell();

    m_currentHero->m_owner = m_newOwner;
    if (g_netLocalGamePos == m_prevOwner && !m_townGarrison) {
        g_advManager->m_drawCursor = 0;
        g_advManager->m_curHeroMobile = 0;
    }
    if (draw) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
}

VA(0x0049b680, 0x1F)  // dc 0x8d688
void RecordHideHero::undo()
{
    m_currentHero->m_owner = m_prevOwner;
    if (!m_townGarrison)
        m_currentHero->obscureCell();
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:725
DC_ONLY(0x8d708, 0xB6)
void RecordShowHero::RecordShowHero(Hero* _hero, char _owner, type_point _location, unsigned char _is_boat)
{
    // @stub
}

// E:\gamedcs\event_record.cpp:736
DC_ONLY(0x8d7c0, 0x26)
EventRecord* RecordShowHero::create()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:744
DC_ONLY(0x8d7e8, 0x4)
EventRecordType RecordShowHero::getType()
{
    // @stub
}

#endif  // @carcass

inline RecordShowHero::RecordShowHero(Hero* who, char newOwner,
                                                    type_point location,
                                                    unsigned char onBoat)
    : RecordHideHero(who, newOwner, 0)
{
    m_previousBoat = (who->m_flags >> 18) & 1;
    m_onBoat = onBoat;
    m_previousLocation = type_point(who->m_x, who->m_y, who->m_z);
    m_location = location;
}

// E:\gamedcs\event_record.cpp:736
VA(0x0049b6a0, 0x27)  // dc 0x8d7c0
EventRecord* RecordShowHero::create()
{
    return new RecordShowHero();
}

VA(0x0049b6d0, 0x85)  // dc 0x8d7ec
unsigned char RecordShowHero::load(AbstractFile* infile, int version)
{
    if (!RecordHideHero::load(infile, version))
        return 0;
    if (infile->read(&m_location, sizeof(m_location)) != sizeof(m_location))
        return 0;
    if (infile->read(&m_previousLocation, sizeof(m_previousLocation)) != sizeof(m_previousLocation))
        return 0;
    if (infile->read(&m_onBoat, 1) != 1)
        return 0;
    unsigned char ok = infile->read(&m_previousBoat, 1) == 1;
    return ok;
}

VA(0x0049b760, 0x95)  // dc 0x8d860
unsigned char RecordShowHero::save(AbstractFile* outfile)
{
    RecordHideHero::save(outfile);
    outfile->write(&m_location, sizeof(m_location));
    outfile->write(&m_previousLocation, sizeof(m_previousLocation));
    outfile->write(&m_onBoat, 1);
    unsigned char ok = outfile->write(&m_previousBoat, 1) == 1;
    return ok;
}
VA(0x0049b800, 0x15E)  // dc 0x8d8b4
void RecordShowHero::replay(unsigned char draw)
{
    setPlayer(m_playerId);

    m_currentHero->m_x = m_location.m_x;
    m_currentHero->m_y = m_location.m_y;
    m_currentHero->m_z = m_location.m_z;
    m_currentHero->obscureCell();
    m_currentHero->m_owner = m_newOwner;
    if (m_onBoat)
        m_currentHero->m_flags |= 0x40000;
    else
        m_currentHero->m_flags &= ~0x40000;

    if (draw && (getMapExtra(m_location.m_x, m_location.m_y, m_location.m_z)
                 & g_mapVisibilityBit)) {
        if (g_currentPlayer->m_currHeroId != m_currentHero->m_id
            || !g_advManager->m_curHeroMobile) {
            g_advManager->setHeroContext(m_currentHero->m_id, 1, 0, draw);
        }
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
}

VA(0x0049b960, 0x9E)  // dc 0x8d9f4
void RecordShowHero::undo()
{
    m_currentHero->restoreCell();
    if (g_netLocalGamePos == m_newOwner) {
        g_advManager->m_drawCursor = 0;
        g_advManager->m_curHeroMobile = 0;
    }
    m_currentHero->m_owner = m_prevOwner;
    m_currentHero->m_x = m_previousLocation.m_x;
    m_currentHero->m_y = m_previousLocation.m_y;
    m_currentHero->m_z = m_previousLocation.m_z;
    if (m_previousBoat)
        m_currentHero->m_flags |= 0x40000;
    else
        m_currentHero->m_flags &= ~0x40000;
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:842
DC_ONLY(0x8da80, 0x40)
void RecordPlayerDeath::RecordPlayerDeath(char _player_id)
{
    // @stub
}

// E:\gamedcs\event_record.cpp:850
DC_ONLY(0x8dac0, 0x26)
EventRecord* RecordPlayerDeath::create()
{
    // @stub
}

#endif  // @carcass

VA(0x0049ba00, 0x27)  // dc 0x8dac0
EventRecord* RecordPlayerDeath::create()
{
    return new RecordPlayerDeath();
}

VA(0x0049ba30, 0x6)  // dc 0x8dae8
EventRecordType RecordPlayerDeath::getType() const
{
    return RECORD_PLAYER_DEATH;
}

VA(0x0049ba40, 0x3D)  // dc 0x8daec
unsigned char RecordPlayerDeath::load(AbstractFile* infile, int version)
{
    if (infile->read(&m_playerId, 1) != 1)
        return 0;
    unsigned char ok = infile->read(&m_extra, 1) == 1;
    return ok;
}

VA(0x0049ba80, 0x30)  // dc 0x8db2c
unsigned char RecordPlayerDeath::save(AbstractFile* outfile)
{
    outfile->write(&m_playerId, 1);
    unsigned char ok = outfile->write(&m_extra, 1) == 1;
    return ok;
}

VA(0x0049bab0, 0x11A)  // dc 0x8db94
void RecordPlayerDeath::replay(unsigned char draw)
{
    if (draw) {
        std::string text;
        text = formatString(g_generalText->getText(6),
                             g_game->getPlayerName(m_extra));
        normalDialog(text.c_str(), 1, -1, -1, 10, m_extra, -1, -1, -1, 5000,
                     -1, 0);
    }
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:905
DC_ONLY(0x8dc20, 0x4)
void RecordPlayerDeath::undo()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:912. NO RETAIL BODY: create expands it.
DC_ONLY(0x8dc24, 0x8C)
void RecordShroud::RecordShroud()
{
    // @stub
}

#endif  // @carcass

VA_COMPGEN(0x0049bbd0, 0x21, SCALAR_DELETING_DTOR, RecordShroud)

// The implicit destructor the wrapper above calls: the change vector's
// _Tidy inlined (`operator delete(_First)` then the three-pointer clear)
// followed by the base's vptr store.
VA_COMPGEN(0x0049bc00, 0x2C, IMPLICIT_DTOR, RecordShroud)

VA(0x0049bc30, 0x42)  // dc 0x8dcb0
EventRecord* RecordShroud::create()
{
    return new RecordShroud();
}

VA(0x0049bc80, 0x6)  // dc 0x8dcd4
EventRecordType RecordShroud::getType() const
{
    return RECORD_SHROUD;
}
VA(0x0049bc90, 0x151)  // dc 0x8dcd8
unsigned char RecordShroud::load(AbstractFile* infile, int version)
{
    if (infile->read(&m_playerId, 1) != 1)
        return 0;

    short count;
    if (infile->read(&count, sizeof(count)) != sizeof(count))
        return 0;

    ShroudChange change;
    m_changes.clear();
    m_changes.reserve(count);

    while (count--) {
        if (infile->read(&change, sizeof(change)) != sizeof(change))
            return 0;
        m_changes.push_back(change);
    }
    return 1;
}

VA(0x0049bdf0, 0x66)  // dc 0x8dd88
unsigned char RecordShroud::save(AbstractFile* outfile)
{
    outfile->write(&m_playerId, 1);
    short count = m_changes.size();
    outfile->write(&count, sizeof(count));
    for (int i = 0; i < count; ++i)
        outfile->write(&m_changes[i], sizeof(ShroudChange));
    return 1;
}
// E:\gamedcs\event_record.cpp:978
// NO RETAIL BODY: the carve leaves no row between save (0x49bdf0) and
// replay (0x49be60), and the vtable accounts for both, so Complete
// expanded this DC-named member into its two callers rather than
// emitting it.  It is defined here at its DC source position so /Ob2 can
// make that same decision - a defined-but-unclaimed symbol adds no
// objdiff row of its own.
void RecordShroud::addChange(int x, int y, int z,
                                    short oldValue, short newValue)
{
    ShroudChange change;
    change.m_x = x;
    change.m_y = y;
    change.m_z = z;
    change.m_oldValue = oldValue;
    change.m_newValue = newValue;
    m_changes.push_back(change);
}

VA(0x0049be60, 0xBA)  // dc 0x8de70
void RecordShroud::replay(unsigned char draw)
{
    unsigned char changed = 0;
    int i = m_changes.size();
    while (i--) {
        ShroudChange change = m_changes[i];
        if ((g_mapVisibilityBit & change.m_newValue)
            != (g_mapVisibilityBit & change.m_oldValue)) {
            changed = 1;
        }
        *getMapExtraPtr(change.m_x, change.m_y, change.m_z) = change.m_newValue;
    }
    if (draw && changed) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
}

VA(0x0049bf20, 0x6E)  // dc 0x8df60
void RecordShroud::undo()
{
    int i = m_changes.size();
    while (i--) {
        ShroudChange change = m_changes[i];
        *getMapExtraPtr(change.m_x, change.m_y, change.m_z) = change.m_oldValue;
    }
}

VA(0x0049bf90, 0x1F1)  // dc 0x8dfe0
void Game::recordClaimMine(long id, long newOwner)
{
    Mine& currentMine = m_mines[id];
    type_point location(currentMine.m_mapX, currentMine.m_mapY,
                        currentMine.m_mapZ);
    CMCClaimMine msg(id, newOwner);
    sendMapChange(&msg);
    m_eventRecords.push_back(new RecordClaimMine(id, newOwner));
}

VA(0x0049c190, 0x1FE)  // dc 0x8e058
void Game::recordClaimTown(long id, long newOwner)
{
    getTown(id);
    CMCClaimTown msg(id, newOwner);
    sendMapChange(&msg);
    m_eventRecords.push_back(new RecordClaimTown(id, newOwner));
}
// E:\gamedcs\event_record.cpp:1061
VA(0x0049c390, 0x1C2)  // anchor-vtable (constructs 0x63df1c), dc 0x8e0b8
void Game::recordEraseObject(NewmapCell* cell, type_point point)
{
    m_eventRecords.push_back(new RecordErase(point,
                                                 cell->m_objectTypeIndex,
                                                 cell->m_extraInfo,
                                                 cell->m_objectIndex));
}

// E:\gamedcs\event_record.cpp:1071
// Retail takes THREE arguments (`ret 0xc`), not the Dreamcast's one: the
// replay state goes in the record's +0xc/+0x10 pair while the constructor
// snapshots the boat's current state into +0xd/+0x14 for undo.
VA(0x0049c560, 0x1B8)  // anchor-vtable (constructs 0x63deec), dc 0x8e108
void Game::recordHideBoat(Boat* currentBoat, unsigned char occupied,
                            int occupyingHero)
{
    m_eventRecords.push_back(new RecordHideBoat(currentBoat, occupied,
                                                     occupyingHero));
}

VA(0x0049c720, 0x1DD)  // dc 0x8e148
void Game::recordHideHero(Hero* who, char newOwner,
                            unsigned char townGarrison)
{
    m_eventRecords.push_back(new RecordHideHero(who, newOwner,
                                                     townGarrison));
}

VA(0x0049c900, 0x217)  // dc 0x8e18c
void Game::recordShowBoat(Boat* currentBoat, type_point point)
{
    m_eventRecords.push_back(new RecordShowBoat(currentBoat, point));
}

VA(0x0049cb20, 0x226)  // dc 0x8e1d0
void Game::recordShowHero(Hero* who, signed char player, type_point point,
                            unsigned char reset)
{
    m_eventRecords.push_back(new RecordShowHero(who, player, point,
                                                     reset));
}

VA(0x0049cd50, 0x1FA)  // dc 0x8e270
void Game::recordMove(Hero* who, int direction, type_point destination)
{
    m_eventRecords.push_back(new RecordMoveHero(who, direction,
                                                     destination));
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:1115
// NO RETAIL BODY. The nine recorders between shroud::undo and SetVisibility
// pair one-for-one onto the DC roster's other nine by the derived vftable
// each stores, and no row is left for this one: the carve's 0x49cf50 stores
// type_record_teleport's 0x63dea4, not type_record_player_death's 0x63df64.
// Whatever the PC revision does on player death, it does not go through an
// out-of-line recorder here.
DC_ONLY(0x8e2bc, 0x3C)
// Before normalization (function): game::record_player_death.
void Game::recordPlayerDeath(char player_id)
{
    // @stub
}

#endif  // @carcass

VA(0x0049cf50, 0x20B)  // dc 0x8e2f8
void Game::recordTeleport(Hero* who, type_point destination)
{
    m_eventRecords.push_back(new RecordTeleport(who, destination));
}

// E:\gamedcs\event_record.cpp:1136
// Residual (88.1751% / 87.9349%): the register-homing family.  Retail gives
// `this` EDI and the inlined GetTeamMask scan ESI; our CL swaps them, and
// every later row follows.  Frames differ by one dword (0x30 against
// retail's 0x38).  MEASURED AND REJECTED 2026-09-06: the Dreamcast's own
// `rect` local (tagRECT at sp+0x44, the four clamp results as one object)
// does NOT survive into Complete - retail's four results sit at [ebp-0x38],
// [ebp-0x30], [ebp-0x2c] and a parameter home, which no 16-byte contiguous
// struct can produce - and spelling it costs 0.04 on both twins (85.6037 /
// 84.7442).  Spelling the queue guard as the DC's `get_change_count()`
// accessor instead of `changes.size()` is byte-flat.
// The positive visibility sweep. The radius test is a REAL sqrt against
// `range + 0.5` (the double at .rdata 0x63ac70), the clamps are the
// reference-returning min/max templates above - which is what puts their
// by-value temporaries in the dead parameter homes - and every cell whose
// mask actually changes is journalled into a shroud record. The record is
// queued only for a local, non-empty, non-replay sweep; otherwise it is
// deleted through the vtable.
VA(0x0049d160, 0x268)  // anchor-global (0x63df7c + GetMapExtraPtr), dc 0x8e33c
void Game::setVisibility(int startX, int startY, int z, int whichPlayer,
                         int range, unsigned char remoteMove)
{
    if (whichPlayer < 0 || whichPlayer >= 8)
        return;

    unsigned short visMask = getTeamMask(whichPlayer);
    double limit = range + 0.5;
    RecordShroud* record = new RecordShroud();

    int x0 = max(startX - range, 0);
    int x1 = cppMin(startX + range + 1, g_mapWidth);
    int y0 = max(startY - range, 0);
    int y1 = cppMin(startY + range + 1, g_mapHeight);

    for (int y = y0; y < y1; ++y) {
        int dy = startY - y;
        for (int x = x0; x < x1; ++x) {
            int dx = startX - x;
            if (sqrt(static_cast<double>(dx * dx + dy * dy)) <= limit) {
                unsigned short* extra = getMapExtraPtr(x, y, z);
                unsigned short oldValue = *extra;
                unsigned short newValue = oldValue | visMask;
                if (oldValue != newValue)
                    record->addChange(x, y, z, oldValue, newValue);
                *extra = newValue;
            }
        }
    }

    if (!remoteMove && record->m_changes.size() != 0 && !g_completeDrawMessageBypass)
    {
        // Retail CALLS insert(iterator, n, const T&) here where the smaller
        // game::record_* bodies expand it, so the site is pinned - with
        // end() hoisted OUT of the pinned statement, because retail keeps
        // that one inline (`mov eax,[ecx+8]`).
        // The record list NAMED AS A REFERENCE: 85.6452 -> 86.7235.  The same
        // change on `changes` in this body is flat, and on the sibling
        // ResetVisibility 0x49d3d0 it does not beat MAX.
        std::vector<EventRecord*>& rEventRecords = m_eventRecords;
        EventRecord** at = rEventRecords.end();
#pragma inline_depth(0)
        rEventRecords.insert(at, 1, record);
#pragma inline_depth()
    } else {
        delete record;
    }
}

// E:\gamedcs\event_record.cpp:1189
// SetVisibility's negative twin, and it is NOT a complement: the surviving
// mask is `GetTeamMask(whichPlayer) | 0x100` ANDed into every cell, so a
// named player keeps its own team's bit and everyone else loses theirs -
// Cover of Darkness's semantics exactly - while -1 clears all eight. It
// also has no replay guard on the queue, only the empty-record one.
VA(0x0049d3d0, 0x260)  // anchor-global (0x63df7c + GetMapExtraPtr), dc 0x8e54c
void Game::resetVisibility(int startX, int startY, int z, int whichPlayer,
                           int range)
{
    unsigned short keepMask = 0x100;
    if (whichPlayer != -1)
        keepMask = getTeamMask(whichPlayer) | 0x100;

    double limit = range + 0.5;
    RecordShroud* record = new RecordShroud();

    int x0 = max(startX - range, 0);
    int x1 = cppMin(startX + range + 1, g_mapWidth);
    int y0 = max(startY - range, 0);
    int y1 = cppMin(startY + range + 1, g_mapHeight);

    for (int y = y0; y < y1; ++y) {
        int dy = startY - y;
        for (int x = x0; x < x1; ++x) {
            int dx = startX - x;
            if (sqrt(static_cast<double>(dx * dx + dy * dy)) <= limit) {
                unsigned short* oldValue = getMapExtraPtr(x, y, z);
                unsigned short newValue = *oldValue & keepMask;
                if (*oldValue != newValue)
                    record->addChange(x, y, z, *oldValue, newValue);
                *oldValue = newValue;
            }
        }
    }

    // The EMPTY arm is the one retail lays out inline (`jne` forward to the
    // queue), so the test is spelled == 0, not != 0.
    if (record->m_changes.size() == 0) {
        delete record;
    } else {
        // Retail CALLS insert(iterator, n, const T&) here where the smaller
        // game::record_* bodies expand it, so the site is pinned - with
        // end() hoisted OUT of the pinned statement, because retail keeps
        // that one inline (`mov eax,[ecx+8]`).
        EventRecord** at = m_eventRecords.end();
#pragma inline_depth(0)
        m_eventRecords.insert(at, 1, record);
#pragma inline_depth()
    }
}
VA(0x0049d630, 0x8C)  // dc 0x8e730
void Game::clearEventRecords()
{
    int i = m_eventRecords.size();
    while (i-- != 0)
        delete m_eventRecords[i];
    m_eventRecords.clear();
}

VA(0x0049d6c0, 0xD3)  // dc 0x8e77c
void Game::clearEventRecords(char playerId)
{
    int i = 0;
    while (i < m_eventRecords.size() && m_eventRecords[i]->m_playerId != playerId)
        ++i;
    if (i == m_eventRecords.size())
        return;
    while (i < m_eventRecords.size() && m_eventRecords[i]->m_playerId == playerId)
        ++i;
    for (int j = 0; j < i; ++j)
        delete m_eventRecords[j];
    m_eventRecords.erase(m_eventRecords.begin(), m_eventRecords.begin() + i);
}

VA(0x0049d7a0, 0x2C1)  // dc 0x8e830
void Game::playRecordedEvents()
{
    int savedPlayer = g_netLocalGamePos;
    PlayerData* actingPlayer = g_currentPlayer;

    Town* currTown;
    Hero* currHero = g_game->getCurrHero();

    g_completeDrawMessageBypass = 1;

    if (actingPlayer->m_currTownId >= 0)
        currTown = g_game->getTown(actingPlayer->m_currTownId);
    else
        currTown = 0;

    if (currHero == 0 && currTown == 0) {
        if (actingPlayer->m_numHeroes > 0) {
            currHero = g_game->getHero(actingPlayer->m_heroes[0]);
        } else {
            currTown = g_game->getTown(actingPlayer->m_townIds[0]);
        }
    }

    long size = m_eventRecords.size();
    while (size--)
        m_eventRecords[size]->undo();

    g_advManager->completeDraw(0);
    g_advManager->updateScreen(0, 0);

    size = m_eventRecords.size();
    unsigned char interrupted = 0;
    Message msg;
    int savedWalkSpeed = g_unnamed698758.m_computerWalkSpeed;
    unsigned char savedSuppress = g_unnamed698790 != 0;
    if (g_unnamed698758.m_computerWalkSpeed > 4)
        g_unnamed698758.m_computerWalkSpeed = 4;
    g_unnamed698790 = 0;

    for (int j = 0; j < size; ++j) {
        unsigned char draw = !interrupted
            && m_eventRecords[j]->m_playerId != savedPlayer;
        m_eventRecords[j]->replay(draw);

        msg = g_inputManager->getEvent();
        if (msg.m_id != MESSAGE_NONE) {
            if (msg.m_id == MESSAGE_KEY_DOWN
                || msg.m_id == MESSAGE_LEFT_BUTTON_DOWN
                || msg.m_id == MESSAGE_RIGHT_BUTTON_DOWN
                || msg.m_id == MESSAGE_WIDGET)
                interrupted = 1;
            else
                process1WindowsMessage();
        }
    }

    g_completeDrawMessageBypass = 0;
    g_completeDrawEnabled = 1;
    setPlayer(savedPlayer);

    if (currHero != 0)
        g_advManager->setHeroContext(currHero->m_id, 0, 0, 1);
    if (currTown != 0)
        g_advManager->setTownContext(currTown->m_id, 0, 1);

    g_unnamed698758.m_computerWalkSpeed = savedWalkSpeed;
    g_unnamed698790 = savedSuppress;
    g_advManager->completeDraw(0);
    g_advManager->updateScreen(0, 0);
}
#if 0  // @carcass

// E:\gamedcs\event_record.cpp:1367
DC_ONLY(0x8ea88, 0x46)
unsigned char Game::replayAvailable()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:1380
DC_ONLY(0x8ead0, 0xF4)
unsigned char Game::loadRecordedEvents(void* infile)
{
    // @stub
}

// E:\gamedcs\event_record.h:64
DC_ONLY(0x8ec5c, 0x4)
// Before normalization (function): type_event_record::get_player_id.
char EventRecord::getPlayerId()
{
    // @stub
}

// E:\gamedcs\event_record.h:85
DC_ONLY(0x8ec60, 0x6C)
void RecordMoveHero::RecordMoveHero()
{
    // @stub
}

// E:\gamedcs\event_record.h:85
DC_ONLY(0x8eccc, 0x34)
void* RecordMoveHero::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:85
DC_ONLY(0x8ed00, 0x18)
void RecordMoveHero::~RecordMoveHero()
{
    // @stub
}

// E:\gamedcs\event_record.h:108
DC_ONLY(0x8ed18, 0x3C)
void RecordTeleport::RecordTeleport()
{
    // @stub
}

// E:\gamedcs\event_record.h:108
DC_ONLY(0x8ed54, 0x34)
void* RecordTeleport::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:108
DC_ONLY(0x8ed88, 0x18)
void RecordTeleport::~RecordTeleport()
{
    // @stub
}

// E:\gamedcs\event_record.h:128
DC_ONLY(0x8eda0, 0x3C)
void RecordClaimMine::RecordClaimMine()
{
    // @stub
}

// E:\gamedcs\event_record.h:128
DC_ONLY(0x8eddc, 0x34)
void* RecordClaimMine::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:128
DC_ONLY(0x8ee10, 0x18)
void RecordClaimMine::~RecordClaimMine()
{
    // @stub
}

// E:\gamedcs\event_record.h:149
DC_ONLY(0x8ee28, 0x3C)
void RecordClaimTown::RecordClaimTown()
{
    // @stub
}

// E:\gamedcs\event_record.h:149
DC_ONLY(0x8ee64, 0x34)
void* RecordClaimTown::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:149
DC_ONLY(0x8ee98, 0x18)
void RecordClaimTown::~RecordClaimTown()
{
    // @stub
}

// E:\gamedcs\event_record.h:169
DC_ONLY(0x8eeb0, 0x3C)
void RecordHideBoat::RecordHideBoat()
{
    // @stub
}

// E:\gamedcs\event_record.h:169
DC_ONLY(0x8eeec, 0x34)
void* RecordHideBoat::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:169
DC_ONLY(0x8ef20, 0x18)
void RecordHideBoat::~RecordHideBoat()
{
    // @stub
}

// E:\gamedcs\event_record.h:190
DC_ONLY(0x8ef38, 0x6C)
void RecordShowBoat::RecordShowBoat()
{
    // @stub
}

// E:\gamedcs\event_record.h:190
DC_ONLY(0x8efa4, 0x34)
void* RecordShowBoat::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:190
DC_ONLY(0x8efd8, 0x18)
void RecordShowBoat::~RecordShowBoat()
{
    // @stub
}

// E:\gamedcs\event_record.h:214
DC_ONLY(0x8eff0, 0x54)
void RecordErase::RecordErase()
{
    // @stub
}

// E:\gamedcs\event_record.h:214
DC_ONLY(0x8f044, 0x34)
void* RecordErase::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:214
DC_ONLY(0x8f078, 0x18)
void RecordErase::~RecordErase()
{
    // @stub
}

// E:\gamedcs\event_record.h:238
DC_ONLY(0x8f090, 0x3C)
void RecordHideHero::RecordHideHero()
{
    // @stub
}

// E:\gamedcs\event_record.h:238
DC_ONLY(0x8f0cc, 0x34)
void* RecordHideHero::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:238
DC_ONLY(0x8f100, 0x18)
void RecordHideHero::~RecordHideHero()
{
    // @stub
}

// E:\gamedcs\event_record.h:262
DC_ONLY(0x8f118, 0x6C)
void RecordShowHero::RecordShowHero()
{
    // @stub
}

// E:\gamedcs\event_record.h:262
DC_ONLY(0x8f184, 0x34)
void* RecordShowHero::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:262
DC_ONLY(0x8f1b8, 0x18)
void RecordShowHero::~RecordShowHero()
{
    // @stub
}

// E:\gamedcs\event_record.h:286
DC_ONLY(0x8f1d0, 0x3C)
void RecordPlayerDeath::RecordPlayerDeath()
{
    // @stub
}

// E:\gamedcs\event_record.h:286
DC_ONLY(0x8f20c, 0x34)
void* RecordPlayerDeath::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.h:286
DC_ONLY(0x8f240, 0x18)
void RecordPlayerDeath::~RecordPlayerDeath()
{
    // @stub
}

// E:\gamedcs\event_record.h:319
DC_ONLY(0x8f258, 0x18)
long RecordShroud::getChangeCount()
{
    // @stub
}

// E:\gamedcs\game.h:877
DC_ONLY(0x8f270, 0x58)
unsigned char Game::getTeamMask(int playerNum)
{
    // @stub
}

// E:\gamedcs\netmsg.h:577
DC_ONLY(0x8f2c8, 0x34)
void CMCClaimMine::CMCClaimMine(signed char mineId, int playerPos)
{
    // @stub
}

// E:\gamedcs\netmsg.h:591
DC_ONLY(0x8f2fc, 0x34)
void CMCClaimTown::CMCClaimTown(signed char townId, int playerPos)
{
    // @stub
}

// E:\gamedcs\event_record.cpp:38
DC_ONLY(0x8f330, 0x34)
void* EventRecord::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.cpp:913
DC_ONLY(0x8f364, 0x34)
void* RecordShroud::`scalar deleting destructor'(unsigned __flags)
{
    // @stub
}

// E:\gamedcs\event_record.cpp:913
DC_ONLY(0x8f398, 0x28)
void RecordShroud::~RecordShroud()
{
    // @stub
}

// E:\gamedcs\event_record.cpp:953
DC_ONLY(0x8f3c0, 0x28)
void RecordShroud::ShroudChange::ShroudChange()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x8f3e8, 0xC)
unsigned std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x8f3f4, 0x20)
RecordShroud::ShroudChange* std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:218
DC_ONLY(0x8f414, 0x1C)
void std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >(const std::allocator<RecordShroud::ShroudChange>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:288
DC_ONLY(0x8f430, 0x28)
void std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::~vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x8f458, 0x3C)
void std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::push_back(const RecordShroud::ShroudChange* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x8f494, 0x38)
void std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::clear()
{
    // @stub
}

// ..\stlport\stl_alloc.h:527
DC_ONLY(0x8f4cc, 0x4)
void std::allocator<RecordShroud::ShroudChange>::allocator<RecordShroud::ShroudChange>()
{
    // @stub
}

// ..\stlport\stl_alloc.h:537
DC_ONLY(0x8f4d0, 0x4)
void std::allocator<RecordShroud::ShroudChange>::~allocator<RecordShroud::ShroudChange>()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x8f4d4, 0x4)
EventRecord** std::vector<EventRecord *,std::allocator<EventRecord *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:195
DC_ONLY(0x8f4d8, 0xC)
unsigned std::vector<EventRecord *,std::allocator<EventRecord *> >::size()
{
    // @stub
}

// ..\stlport\stl_vector.h:203
DC_ONLY(0x8f4e4, 0x20)
EventRecord** std::vector<EventRecord *,std::allocator<EventRecord *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:204
DC_ONLY(0x8f504, 0x20)
EventRecord** std::vector<EventRecord *,std::allocator<EventRecord *> >::operator[](unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:368
DC_ONLY(0x8f524, 0x3C)
void std::vector<EventRecord *,std::allocator<EventRecord *> >::push_back(EventRecord** __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x8f560, 0x3C)
EventRecord** std::vector<EventRecord *,std::allocator<EventRecord *> >::erase(EventRecord** __first, EventRecord** __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:506
DC_ONLY(0x8f59c, 0x38)
void std::vector<EventRecord *,std::allocator<EventRecord *> >::clear()
{
    // @stub
}

// ..\stlport\stl_vector.h:179
DC_ONLY(0x8f5d4, 0x4)
RecordShroud::ShroudChange* std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x8f5d8, 0x4)
RecordShroud::ShroudChange* std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::end()
{
    // @stub
}

// ..\stlport\stl_vector.h:490
DC_ONLY(0x8f5dc, 0x3C)
RecordShroud::ShroudChange* std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::erase(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last)
{
    // @stub
}

// ..\stlport\stl_vector.h:89
DC_ONLY(0x8f618, 0x2C)
void std::_Vector_base<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::_Vector_base<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >(const std::allocator<RecordShroud::ShroudChange>* __a)
{
    // @stub
}

// ..\stlport\stl_vector.h:101
DC_ONLY(0x8f644, 0x30)
void std::_Vector_base<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::~_Vector_base<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >()
{
    // @stub
}

// ..\stlport\stl_vector.h:180
DC_ONLY(0x8f674, 0x4)
EventRecord** std::vector<EventRecord *,std::allocator<EventRecord *> >::begin()
{
    // @stub
}

// ..\stlport\stl_vector.h:181
DC_ONLY(0x8f678, 0x4)
EventRecord** std::vector<EventRecord *,std::allocator<EventRecord *> >::end()
{
    // @stub
}

// ..\stlport\stl_string.h:181
DC_ONLY(0x8f67c, 0x18)
void std::_STL_alloc_proxy<RecordShroud::type_shroud_chan()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1004
DC_ONLY(0x8f694, 0xC)
void std::_STL_alloc_proxy<RecordShroud::type_shroud_cha(const std::allocator<RecordShroud::ShroudChange>* __a, RecordShroud::ShroudChange** __p)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x8f6a0, 0x2C)
void std::_STL_alloc_proxy<RecordShroud::ShroudChange *,RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::deallocate(RecordShroud::ShroudChange* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x8f6cc, 0x1C)
void std::allocator<RecordShroud::ShroudChange>::deallocate(RecordShroud::ShroudChange* __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x8f6e8, 0xD0)
void std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::_M_insert_overflow(RecordShroud::ShroudChange* __position, const RecordShroud::ShroudChange* __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:68
DC_ONLY(0x8f7b8, 0x98)
void std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::reserve(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.c:248
DC_ONLY(0x8f850, 0xCC)
void std::vector<EventRecord *,std::allocator<EventRecord *> >::_M_insert_overflow(EventRecord** __position, EventRecord** __x, unsigned __fill_len)
{
    // @stub
}

// ..\stlport\stl_vector.c:68
DC_ONLY(0x8f91c, 0x94)
void std::vector<EventRecord *,std::allocator<EventRecord *> >::reserve(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x8f9b0, 0x30)
void std::destroy(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x8f9e0, 0x3C)
void std::construct(RecordShroud::ShroudChange* __p, const RecordShroud::ShroudChange* __value)
{
    // @stub
}

// ..\stlport\stl_construct.h:85
DC_ONLY(0x8fa1c, 0x28)
void std::construct(EventRecord** __p, EventRecord** __value)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x8fa44, 0x50)
EventRecord** std::copy(EventRecord** __first, EventRecord** __last, EventRecord** __result)
{
    // @stub
}

// ..\stlport\stl_construct.h:128
DC_ONLY(0x8fa94, 0x30)
void std::destroy(EventRecord** __first, EventRecord** __last)
{
    // @stub
}

// ..\stlport\stl_algobase.h:322
DC_ONLY(0x8fac4, 0x50)
RecordShroud::ShroudChange* std::copy(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last, RecordShroud::ShroudChange* __result)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x8fb14, 0x4)
std::allocator<RecordShroud::ShroudChange>* std::__stl_alloc_rebind(std::allocator<RecordShroud::ShroudChange>* __a, const RecordShroud::ShroudChange* __formal)
{
    // @stub
}

// ..\stlport\stl_vector.h:199
DC_ONLY(0x8fb18, 0xC)
unsigned std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::capacity()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x8fb24, 0x28)
RecordShroud::ShroudChange* std::_STL_alloc_proxy<RecordShroud::ShroudChange *,RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_vector.h:199
DC_ONLY(0x8fb4c, 0xC)
unsigned std::vector<EventRecord *,std::allocator<EventRecord *> >::capacity()
{
    // @stub
}

// ..\stlport\stl_alloc.h:1022
DC_ONLY(0x8fb58, 0x28)
EventRecord** std::_STL_alloc_proxy<EventRecord * *,EventRecord *,std::allocator<EventRecord *> >::allocate(unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:1025
DC_ONLY(0x8fb80, 0x2C)
void std::_STL_alloc_proxy<EventRecord * *,EventRecord *,std::allocator<EventRecord *> >::deallocate(EventRecord** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x8fbac, 0x28)
RecordShroud::ShroudChange* std::allocator<RecordShroud::ShroudChange>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:547
DC_ONLY(0x8fbd4, 0x24)
EventRecord** std::allocator<EventRecord *>::allocate(unsigned __n, const void* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:552
DC_ONLY(0x8fbf8, 0x1C)
void std::allocator<EventRecord *>::deallocate(EventRecord** __p, unsigned __n)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x8fc14, 0x38)
RecordShroud::ShroudChange* std::uninitialized_copy(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last, RecordShroud::ShroudChange* __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x8fc4c, 0x38)
RecordShroud::ShroudChange* std::uninitialized_fill_n(RecordShroud::ShroudChange* __first, unsigned __n, const RecordShroud::ShroudChange* __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:514
DC_ONLY(0x8fc84, 0x38)
std::vector<RecordShroud::ShroudChange,std::allocator<RecordShroud::ShroudChange> >::_M_allocate_and_copy(unsigned __n, RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:97
DC_ONLY(0x8fcbc, 0x38)
EventRecord** std::uninitialized_copy(EventRecord** __first, EventRecord** __last, EventRecord** __result)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:263
DC_ONLY(0x8fcf4, 0x38)
EventRecord** std::uninitialized_fill_n(EventRecord** __first, unsigned __n, EventRecord** __x)
{
    // @stub
}

// ..\stlport\stl_vector.h:514
DC_ONLY(0x8fd2c, 0x38)
std::vector<EventRecord *,std::allocator<EventRecord *> >::_M_allocate_and_copy(unsigned __n, EventRecord** __first, EventRecord** __last)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x8fd64, 0x4)
RecordShroud::ShroudChange* std::value_type(const RecordShroud::ShroudChange* __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x8fd68, 0x1C)
void std::__destroy(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last, RecordShroud::ShroudChange* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x8fd84, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, EventRecord** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x8fd90, 0x4)
int* std::distance_type(EventRecord** __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x8fd94, 0x1E)
EventRecord** std::__copy(EventRecord** __first, EventRecord** __last, EventRecord** __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:262
DC_ONLY(0x8fdb4, 0x4)
EventRecord** std::value_type(EventRecord** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:121
DC_ONLY(0x8fdb8, 0x1C)
void std::__destroy(EventRecord** __first, EventRecord** __last, EventRecord** __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:243
DC_ONLY(0x8fdd4, 0xC)
std::random_access_iterator_tag std::iterator_category(__$ReturnUdt, const RecordShroud::ShroudChange* __formal)
{
    // @stub
}

// ..\stlport\stl_iterator_base.h:291
DC_ONLY(0x8fde0, 0x4)
int* std::distance_type(const RecordShroud::ShroudChange* __formal)
{
    // @stub
}

// ..\stlport\stl_algobase.h:209
DC_ONLY(0x8fde4, 0x42)
RecordShroud::ShroudChange* std::__copy(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last, RecordShroud::ShroudChange* __result, std::random_access_iterator_tag __formal, int* __formal)
{
    // @stub
}

// ..\stlport\stl_alloc.h:968
DC_ONLY(0x8fe28, 0x4)
std::allocator<EventRecord* std::__stl_alloc_rebind(std::allocator<EventRecord* __a, EventRecord** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x8fe2c, 0x1C)
RecordShroud::ShroudChange* std::__uninitialized_copy(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last, RecordShroud::ShroudChange* __result, RecordShroud::ShroudChange* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x8fe48, 0x1C)
RecordShroud::ShroudChange* std::__uninitialized_fill_n(RecordShroud::ShroudChange* __first, unsigned __n, const RecordShroud::ShroudChange* __x, RecordShroud::ShroudChange* __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:88
DC_ONLY(0x8fe64, 0x1C)
EventRecord** std::__uninitialized_copy(EventRecord** __first, EventRecord** __last, EventRecord** __result, EventRecord** __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:255
DC_ONLY(0x8fe80, 0x1C)
EventRecord** std::__uninitialized_fill_n(EventRecord** __first, unsigned __n, EventRecord** __x, EventRecord** __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x8fe9c, 0x30)
void std::__destroy_aux(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:110
DC_ONLY(0x8fecc, 0x30)
void std::__destroy_aux(EventRecord** __first, EventRecord** __last, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x8fefc, 0x3C)
RecordShroud::ShroudChange* std::__uninitialized_copy_aux(RecordShroud::ShroudChange* __first, RecordShroud::ShroudChange* __last, RecordShroud::ShroudChange* __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x8ff38, 0x3C)
RecordShroud::ShroudChange* std::__uninitialized_fill_n_aux(RecordShroud::ShroudChange* __first, unsigned __n, const RecordShroud::ShroudChange* __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:70
DC_ONLY(0x8ff74, 0x3C)
EventRecord** std::__uninitialized_copy_aux(EventRecord** __first, EventRecord** __last, EventRecord** __result, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_uninitialized.h:239
DC_ONLY(0x8ffb0, 0x3C)
EventRecord** std::__uninitialized_fill_n_aux(EventRecord** __first, unsigned __n, EventRecord** __x, __false_type __formal)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x8ffec, 0x1C)
void std::destroy(RecordShroud::ShroudChange* __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:59
DC_ONLY(0x90008, 0x1C)
void std::destroy(EventRecord** __pointer)
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x90024, 0x4)
void std::__destroy_aux()
{
    // @stub
}

// ..\stlport\stl_construct.h:53
DC_ONLY(0x90028, 0x4)
void std::__destroy_aux()
{
    // @stub
}

#endif  // @carcass

VA(0x0049da70, 0x41)
unsigned char Game::replayAvailable() const
{
    for (unsigned i = 0; i < m_eventRecords.size(); ++i) {
        if (m_eventRecords[i]->m_playerId != g_netLocalGamePos)
            return 1;
    }
    return 0;
}

// The record factory table, retail .data 0x6776b0. Eleven slots indexed by
// the type byte load_recorded_events reads off the stream, in
// type_event_record_type order, plus the unused zero slot; the retail
// dispatch is `call dword ptr [ecx*4 + 0x6776b0]` after a 1..11 range check.
DATA(0x006776b0)
EventRecord* (*g_recordCreators[12])() = {
    0,
    RecordMoveHero::create,
    RecordTeleport::create,
    RecordClaimMine::create,
    RecordClaimTown::create,
    RecordHideBoat::create,
    RecordShowBoat::create,
    RecordErase::create,
    RecordHideHero::create,
    RecordShowHero::create,
    RecordPlayerDeath::create,
    RecordShroud::create
};

VA(0x0049dac0, 0x19C)  // dc 0x8ead0
unsigned char Game::loadRecordedEvents(AbstractFile* infile, int version)
{
    long count;
    if (infile->read(&count, sizeof(count)) != sizeof(count))
        return 0;

    clearEventRecords();
    m_eventRecords.reserve(count);

    char type;
    EventRecord* record;
    while (count--) {
        if (infile->read(&type, 1) != 1)
            return 0;
        if (type <= 0 || type > RECORD_SHROUD)
            return 0;
        record = (*g_recordCreators[type])();
        if (!record->load(infile, version))
            return 0;
        m_eventRecords.push_back(record);
    }
    return 1;
}

VA(0x0049dc60, 0x8C)  // dc 0x8ebc4
unsigned char Game::saveRecordedEvents(AbstractFile* outfile)
{
    long count = m_eventRecords.size();
    outfile->write(&count, 4);
    for (int i = 0; i < count; ++i) {
        char type = m_eventRecords[i]->getType();
        outfile->write(&type, 1);
        if (!m_eventRecords[i]->save(outfile))
            return 0;
    }
    return 1;
}
