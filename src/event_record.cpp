#include "va.h"
#include "includes.h"

#include <math.h>

#include "event_record.h"

#include "abstractfile.h"
#include "advmgr.h"
#include "cursor.h"
#include "game.h"
#include "inputmgr.h"
#include "kb.h"
#include "kbwin.h"
#include "message.h"
#include "misc.h"
#include "prefs.h"
#include "textresource.h"

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
// type_record_shroud::create (0x49bc30) is the clearest Windows witness.
// Mac retains this source constructor at 0:0xbef5c and 17 derived
// construction sites call it, including ten event factory methods.
DC_ADDRESS(0x08c624, 0x34)
MAC_ADDRESS(0x0bef5c, 0x1c)
type_event_record::type_event_record()
{
    m_playerId = g_netLocalGamePos;
}

VA_COMPGEN(0x0049a5b0, 0x23, SCALAR_DELETING_DTOR, type_event_record)

// Seven derived Mac load/save prefixes reproduce these complete base bodies:
// this+4, one byte, and read/write slot 0xc/0x10. Recover the base calls.
// Loads retain the base failure check; saves discard its result.
VA(0x0049a5e0, 0x1D)
DC_ADDRESS(0x08c678, 0x1e)
MAC_ADDRESS(0x0befc0, 0x48)
bool type_event_record::load(TAbstractFile* infile, int version)
{
    return infile->read(&m_playerId, 1) == 1;
}

VA(0x0049a600, 0x1D)
DC_ADDRESS(0x08c698, 0x1e)
MAC_ADDRESS(0x0bf008, 0x48)
bool type_event_record::save(TAbstractFile* outfile)
{
    return outfile->write(&m_playerId, 1) == 1;
}

// E:\gamedcs\event_record.cpp:65. Ordinary static helper, expanded
// into the four replay bodies and playRecordedEvents. The char parameter
// narrows the saved seat at the latter call; each replay passes m_playerId.
// Preserve these DC-proven source calls instead of copying the helper body.
DC_ADDRESS(0x08c6b8, 0x50)
MAC_ADDRESS(0x0bf050, 0xa8)
static void setPlayer(char newPlayer)
{
    if (g_netLocalGamePos != newPlayer) {
        g_advManager->deactivateCurrTown(0);
        g_advManager->deactivateCurrHero(0);
    }
    g_netLocalGamePos = newPlayer;
    g_currentPlayer = &g_game->m_players[newPlayer];
    g_curPlayerBit = 1 << newPlayer;
}

// E:\gamedcs\event_record.cpp:81
// Retail base vtable slot4 folds to the empty ret4 body at0x485d80.
DC_ADDRESS(0x08c708, 0x4)
MAC_ADDRESS(0x0bf0f8, 0x4)
void type_event_record::replay(bool draw)
{
}

// E:\gamedcs\event_record.cpp:88
// Retail base vtable slot5 folds to the empty ret body at0x5bc690.
DC_ADDRESS(0x08c70c, 0x4)
MAC_ADDRESS(0x0bf0fc, 0x4)
void type_event_record::undo()
{
}

// E:\gamedcs\event_record.cpp:96
// NO RETAIL BODY: VC6 auto-expands this ordinary constructor at record_move and
// record_teleport and leaves an unreferenced COMDAT body. Mac retains it at
// code0+0xbf100, between base undo and move-hero create. Dreamcast gives seven
// ordered source rows and proves that line 100 obtains source through
// type_obscuring_object::get_location; retail corroborates the packed x/y/z
// loads at both expansion sites.
DC_ADDRESS(0x08c710, 0xae)
MAC_ADDRESS(0x0bf100, 0xa8)
type_record_move_hero::type_record_move_hero(hero* currentHero,
                                             char direction,
                                             type_point destination)
{
    m_currentHero = currentHero;
    m_restoreFlag = currentHero->m_facing;
    m_direction = direction;
    m_source = currentHero->getLocation();
    m_destination = destination;
}

// Slot 0 of TEN derived vtables at once: 0x63de8c 0x63dea4 0x63debc 0x63ded4
// 0x63deec 0x63df04 0x63df1c 0x63df34 0x63df4c and 0x63df64 all name this
// address. Every one of those classes has a trivial implicit destructor that
// collapses to the base vptr store at 0x49abe0, so the ten wrappers were
// byte-identical and /OPT:ICF folded them onto one body; the claim names the
// first of the ten in link order.
VA_COMPGEN(0x0049a620, 0x21, SCALAR_DELETING_DTOR, type_record_move_hero)

VA(0x0049a650, 0x27)
DC_ADDRESS(0x08c7c0, 0x26)
MAC_ADDRESS(0x0bf1a8, 0x48)
type_event_record* type_record_move_hero::create()
{
    return new type_record_move_hero();
}

VA(0x0049a680, 0x6)
DC_ADDRESS(0x08c7e8, 0x4)
MAC_ADDRESS(0x0bf1f0, 0x8)
type_event_record_type type_record_move_hero::getType() const
{
    return RECORD_MOVE_HERO;
}

VA(0x0049a690, 0xB1)
DC_ADDRESS(0x08c7ec, 0xce)
MAC_ADDRESS(0x0bf1f8, 0x148)
bool type_record_move_hero::load(TAbstractFile* infile, int version)
{
    if (!type_event_record::load(infile, version))
        return 0;
    int heroId;
    if (infile->read(&heroId, sizeof(heroId)) != sizeof(heroId))
        return 0;
    m_currentHero = g_game->getHero(heroId);
    if (infile->read(&m_direction, 1) != 1)
        return 0;
    if (infile->read(&m_source, sizeof(m_source)) != sizeof(m_source))
        return 0;
    bool ok = infile->read(&m_destination, sizeof(m_destination)) == sizeof(m_destination);
    return ok;
}

VA(0x0049a750, 0x63)
DC_ADDRESS(0x08c8bc, 0x60)
MAC_ADDRESS(0x0bf340, 0xe4)
bool type_record_move_hero::save(TAbstractFile* outfile)
{
    int heroId = m_currentHero->m_id;
    type_event_record::save(outfile);
    outfile->write(&heroId, sizeof(heroId));
    outfile->write(&m_direction, 1);
    outfile->write(&m_source, sizeof(m_source));
    bool ok = outfile->write(&m_destination, sizeof(m_destination)) == sizeof(m_destination);
    return ok;
}

VA(0x0049a7c0, 0x144)
DC_ADDRESS(0x08c91c, 0xce)
MAC_ADDRESS(0x0bf424, 0x15c)
void type_record_move_hero::replay(bool draw)
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
VA(0x0049a910, 0x65)
DC_ADDRESS(0x08c9ec, 0x66)
MAC_ADDRESS(0x0bf580, 0x9c)  // anchor-vtable
void type_record_move_hero::undo()
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
// NO RETAIL BODY: VC6 auto-expands this ordinary constructor into record_teleport;
// Mac retains it at code0+0xbf61c after move-hero undo.
// Dreamcast line 204 proves this remains a derived-to-base delegation rather
// than a flattened duplicate of type_record_move_hero's assignments.
DC_ADDRESS(0x08ca54, 0x70)
MAC_ADDRESS(0x0bf61c, 0x44)
type_record_teleport::type_record_teleport(hero* currentHero,
                                           type_point destination)
    : type_record_move_hero(currentHero, currentHero->m_facing, destination)
{
}

VA(0x0049a980, 0x27)
DC_ADDRESS(0x08cac4, 0x26)
MAC_ADDRESS(0x0bf6c0, 0x50)
type_event_record* type_record_teleport::create()
{
    return new type_record_teleport();
}

VA(0x0049a9b0, 0x6)
DC_ADDRESS(0x08caec, 0x4)
MAC_ADDRESS(0x0bf710, 0x8)
type_event_record_type type_record_teleport::getType() const
{
    return RECORD_TELEPORT;
}

VA(0x0049a9c0, 0x7B)
DC_ADDRESS(0x08caf0, 0x3c)
MAC_ADDRESS(0x0bf718, 0x60)
void type_record_teleport::replay(bool draw)
{
    setPlayer(m_playerId);

    g_advManager->teleportTo(m_currentHero, m_destination, 0, 0, draw, 1);
}

// E:\gamedcs\event_record.cpp:237
// NO RETAIL BODY: VC6 auto-expands this ordinary constructor into
// record_claim_mine. Mac retains it at code0+0xbf778 after teleport replay.
// record_claim_town instead invokes the distinct default constructor at
// dc:0x8eda0. Dreamcast preserves this definition site and the field order.
DC_ADDRESS(0x08cb2c, 0x5c)
MAC_ADDRESS(0x0bf778, 0x78)
type_record_claim_mine::type_record_claim_mine(long id,
                                               char newOwner)
{
    m_id = id;
    m_newOwner = newOwner;
    m_oldOwner = g_game->getMine(id)->m_playerOwner;
}

// E:\gamedcs\event_record.cpp:255
// Retail derived vtable slot1 folds to 0x56e3d0: mov eax,3; ret.
DC_ADDRESS(0x08cbb0, 0x4)
MAC_ADDRESS(0x0bf838, 0x8)
type_event_record_type type_record_claim_mine::getType() const
{
    return RECORD_CLAIM_MINE;
}

VA(0x0049aa40, 0x27)
DC_ADDRESS(0x08cb88, 0x26)
MAC_ADDRESS(0x0bf7f0, 0x48)
type_event_record* type_record_claim_mine::create()
{
    return new type_record_claim_mine();
}

VA(0x0049aa70, 0x71)
DC_ADDRESS(0x08cbb4, 0x66)
MAC_ADDRESS(0x0bf840, 0xec)
bool type_record_claim_mine::load(TAbstractFile* infile, int version)
{
    if (!type_event_record::load(infile, version))
        return 0;
    if (infile->read(&m_id, sizeof(m_id)) != sizeof(m_id))
        return 0;
    if (infile->read(&m_oldOwner, 1) != 1)
        return 0;
    bool ok = infile->read(&m_newOwner, 1) == 1;
    return ok;
}

VA(0x0049aaf0, 0x4A)
DC_ADDRESS(0x08cc1c, 0x4e)
MAC_ADDRESS(0x0bf92c, 0xbc)
bool type_record_claim_mine::save(TAbstractFile* outfile)
{
    type_event_record::save(outfile);
    outfile->write(&m_id, sizeof(m_id));
    outfile->write(&m_oldOwner, 1);
    bool ok = outfile->write(&m_newOwner, 1) == 1;
    return ok;
}

VA(0x0049ab40, 0x74)
DC_ADDRESS(0x08cc6c, 0x6c)
MAC_ADDRESS(0x0bf9e8, 0xc4)
void type_record_claim_mine::replay(bool draw)
{
    g_game->claimMine(m_id, m_newOwner, const_recorded_action);
    if (draw) {
        mine& claimed = *g_game->getMine(m_id);
        if (getMapExtra(claimed.m_mapX, claimed.m_mapY, claimed.m_mapZ)
            & g_mapVisibilityBit) {
            g_advManager->completeDraw(0);
            g_advManager->updateScreen(0, 0);
        }
    }
}

VA(0x0049abc0, 0x19)
DC_ADDRESS(0x08ccd8, 0x24)
MAC_ADDRESS(0x0bfaac, 0x24)
void type_record_claim_mine::undo()
{
    g_game->getMine(m_id)->m_playerOwner = m_oldOwner;
}

VA(0x0049abe0, 0x7)
DC_ADDRESS(0x08c658, 0x20)
MAC_ADDRESS(0x0bef78, 0x48)
type_event_record::~type_event_record()
{
}

// E:\gamedcs\event_record.cpp:321
// Dreamcast resolves the base boundary specifically to the default header
// constructor at dc:0x8eda0, not the parameterized constructor at dc:0x8cb2c.
// The derived body then assigns the three claim fields, with old_owner coming
// from gpGame->towns. Mac retains it at code0+0xbfad0 after claim-mine undo.
// Retail corroborates that final assignment sequence and elides the
// intermediate claim_mine vptr store.
DC_ADDRESS(0x08ccfc, 0x60)
MAC_ADDRESS(0x0bfad0, 0x84)
type_record_claim_town::type_record_claim_town(long id,
                                               char newOwner)
    : type_record_claim_mine()
{
    m_id = id;
    m_newOwner = newOwner;
    m_oldOwner = g_game->m_towns[id].m_owner;
}

// E:\gamedcs\event_record.cpp:339
// type_record_claim_town::get_type has no retail body of its own: slot 1 of
// its vtable (0x63ded4) is 0x16ebc0, outside this compiland's span, where
// /OPT:ICF folded the `mov eax,4 / ret` onto an identical body elsewhere.
// Retail derived vtable slot1 folds to 0x56ebc0: mov eax,4; ret.
DC_ADDRESS(0x08cd84, 0x54)
MAC_ADDRESS(0x0bfc04, 0x8)
type_event_record_type type_record_claim_town::getType() const
{
    return RECORD_CLAIM_TOWN;
}

VA(0x0049abf0, 0x27)
DC_ADDRESS(0x08cd5c, 0x26)
MAC_ADDRESS(0x0bfbb4, 0x50)
type_event_record* type_record_claim_town::create()
{
    return new type_record_claim_town();
}

VA(0x0049ac20, 0x7E)
DC_ADDRESS(0x08cdd8, 0x70)
MAC_ADDRESS(0x0bfc0c, 0xac)
void type_record_claim_town::replay(bool draw)
{
    g_game->m_towns[m_id].m_owner = m_newOwner;
    if (draw) {
        town& claimed = g_game->m_towns[m_id];
        if (getMapExtra(claimed.m_mapX, claimed.m_mapY, claimed.m_mapZ)
            & g_mapVisibilityBit) {
            g_advManager->completeDraw(0);
            g_advManager->updateScreen(0, 0);
        }
    }
}

VA(0x0049aca0, 0x1D)
DC_ADDRESS(0x08ce48, 0x2a)
MAC_ADDRESS(0x0bfcb8, 0x28)
void type_record_claim_town::undo()
{
    g_game->m_towns[m_id].m_owner = m_oldOwner;
}

// E:\gamedcs\event_record.cpp:376
// Dreamcast's older record stores only the boat pointer here. Complete adds
// the replay state at +0xc/+0x10; record_hide_boat's retail `ret 0xc` and the
// two independent snapshot loads corroborate the revised constructor inputs.
// DC retains this constructor at 0x8ce74 and Mac recordHideBoat calls 0xbfce0;
// VC6 expands the ordinary same-TU helper in both Windows boat callers.
// Mac reloads m_currentBoat before each snapshot. Keeping those member reads
// also restores the retail vector expansion in recordHideBoat (88.9253 -> 100).
MAC_ADDRESS(0x0bfce0, 0x80)
type_record_hide_boat::type_record_hide_boat(boat* currentBoat,
                                                    unsigned char occupied,
                                                    int occupyingHero)
{
    m_currentBoat = currentBoat;
    m_occupied = occupied;
    m_previousOccupied = m_currentBoat->m_occupied;
    m_occupyingHero = occupyingHero;
    m_previousOccupyingHero = m_currentBoat->m_occupyingHero;
}

VA(0x0049acc0, 0x27)
DC_ADDRESS(0x08ceb0, 0x26)
MAC_ADDRESS(0x0bfd60, 0x48)
type_event_record* type_record_hide_boat::create()
{
    return new type_record_hide_boat();
}

VA(0x0049acf0, 0x6)
DC_ADDRESS(0x08ced8, 0x4)
MAC_ADDRESS(0x0bfda8, 0x8)
type_event_record_type type_record_hide_boat::getType() const
{
    return RECORD_HIDE_BOAT;
}

VA(0x0049ad00, 0xE7)
DC_ADDRESS(0x08cedc, 0x4e)
MAC_ADDRESS(0x0bfdb0, 0x1c0)
bool type_record_hide_boat::load(TAbstractFile* infile, int version)
{
    if (!type_event_record::load(infile, version))
        return 0;
    signed char boatId;
    if (infile->read(&boatId, 1) != 1)
        return 0;
    if (version >= 0x12 && version != g_saveVersionBoatFieldsAbsent
        && (version <= 0x1e || version >= 0x23)) {
        m_previousOccupied = readValue<char>(infile) != 0;
        m_occupied = readValue<char>(infile) != 0;
        // Mac stages each short separately, then uses lhbrx; the conversion
        // remains unresolved in the shared native scalar helper.
        m_previousOccupyingHero = readValue<short>(infile);
        m_occupyingHero = readValue<short>(infile);
    } else {
        m_previousOccupied = 0;
        m_occupied = 1;
        m_previousOccupyingHero = -1;
        m_occupyingHero = -1;
    }
    m_currentBoat = g_game->getBoat(boatId);
    return 1;
}

VA(0x0049adf0, 0x8A)
DC_ADDRESS(0x08cf2c, 0x36)
MAC_ADDRESS(0x0bff70, 0x11c)
bool type_record_hide_boat::save(TAbstractFile* outfile)
{
    type_event_record::save(outfile);
    if (outfile->write(&m_currentBoat->m_id, 1) != 1)
        return 0;
    writeValue<unsigned char>(outfile, m_previousOccupied);
    writeValue<unsigned char>(outfile, m_occupied);
    // Mac uses sthbrx in these scalar expansions; native helper byte order
    // remains an unresolved comparison difference.
    writeValue<short>(outfile, m_previousOccupyingHero);
    writeValue<short>(outfile, m_occupyingHero);
    return 1;
}

VA(0x0049ae80, 0x44)
DC_ADDRESS(0x08cf64, 0x2e)
MAC_ADDRESS(0x0c008c, 0x78)
void type_record_hide_boat::replay(bool draw)
{
    m_currentBoat->m_occupied = m_occupied;
    m_currentBoat->m_occupyingHero = m_occupyingHero;
    m_currentBoat->restoreCell();
    if (draw) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
}

VA(0x0049aed0, 0x23)
DC_ADDRESS(0x08cf94, 0x12)
MAC_ADDRESS(0x0c0104, 0x44)
void type_record_hide_boat::undo()
{
    m_currentBoat->m_occupied = m_previousOccupied;
    m_currentBoat->m_occupyingHero = m_previousOccupyingHero;
    m_currentBoat->obscureCell();
}

// E:\gamedcs\event_record.cpp:449
// DC line 450 calls type_obscuring_object::get_location. Retail inlines that
// helper into the packed x/y/z loads, so keep the source boundary even though
// spelling the three fields directly produces the same candidate bytes.
DC_ADDRESS(0x08cfa8, 0x9a)
MAC_ADDRESS(0x0c0148, 0x94)
inline type_record_show_boat::type_record_show_boat(boat* currentBoat,
                                                    type_point location)
    : type_record_hide_boat(currentBoat, 0,
                            currentBoat->m_occupyingHero)
{
    m_previousLocation = currentBoat->getLocation();
    m_location = location;
}

VA(0x0049af00, 0x27)
DC_ADDRESS(0x08d044, 0x26)
MAC_ADDRESS(0x0c023c, 0x50)
type_event_record* type_record_show_boat::create()
{
    return new type_record_show_boat();
}

VA(0x0049af30, 0x6)
DC_ADDRESS(0x08d06c, 0x4)
MAC_ADDRESS(0x0c028c, 0x8)
type_event_record_type type_record_show_boat::getType() const
{
    return RECORD_SHOW_BOAT;
}

VA(0x0049af40, 0x51)
DC_ADDRESS(0x08d070, 0xa0)
MAC_ADDRESS(0x0c0294, 0xa8)
bool type_record_show_boat::load(TAbstractFile* infile, int version)
{
    if (!type_record_hide_boat::load(infile, version))
        return 0;
    if (infile->read(&m_location, sizeof(m_location)) != sizeof(m_location))
        return 0;
    bool ok = infile->read(&m_previousLocation, sizeof(m_previousLocation))
                       == sizeof(m_previousLocation);
    return ok;
}

VA(0x0049afa0, 0x9F)
DC_ADDRESS(0x08d110, 0x3a)
MAC_ADDRESS(0x0c033c, 0x88)
bool type_record_show_boat::save(TAbstractFile* outfile)
{
    type_record_hide_boat::save(outfile);
    outfile->write(&m_location, sizeof(m_location));
    bool ok = outfile->write(&m_previousLocation, sizeof(m_previousLocation))
                       == sizeof(m_previousLocation);
    return ok;
}

VA(0x0049b040, 0xB5)
DC_ADDRESS(0x08d14c, 0x8a)
MAC_ADDRESS(0x0c03c4, 0xf8)
void type_record_show_boat::replay(bool draw)
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

VA(0x0049b100, 0x4E)
DC_ADDRESS(0x08d1d8, 0x46)
MAC_ADDRESS(0x0c04bc, 0x78)
void type_record_show_boat::undo()
{
    m_currentBoat->m_occupied = m_previousOccupied;
    m_currentBoat->restoreCell();
    m_currentBoat->m_x = m_previousLocation.m_x;
    m_currentBoat->m_y = m_previousLocation.m_y;
    m_currentBoat->m_z = m_previousLocation.m_z;
}

// DC cpp:533 defaults m_location before the line-534 assignment. Spelling
// that default as `: m_location()` or spelling the base initializer explicitly
// is byte-flat at recordEraseObject (88.7023%). The ordinary definition is
// also byte-flat; nested vector _Ufill still gets budget 55 against cost 56.
DC_ADDRESS(0x08d220, 0x70)
MAC_ADDRESS(0x0c0534, 0x74)
type_record_erase::type_record_erase(type_point location,
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
VA(0x0049b150, 0x27)
DC_ADDRESS(0x08d290, 0x26)
MAC_ADDRESS(0x0c05a8, 0x48)
type_event_record* type_record_erase::create()
{
    return new type_record_erase();
}

VA(0x0049b180, 0x6)
DC_ADDRESS(0x08d2b8, 0x4)
MAC_ADDRESS(0x0c05f0, 0x8)
type_event_record_type type_record_erase::getType() const
{
    return RECORD_ERASE;
}

VA(0x0049b190, 0x8B)
DC_ADDRESS(0x08d2bc, 0x7a)
MAC_ADDRESS(0x0c05f8, 0x118)
bool type_record_erase::load(TAbstractFile* infile, int version)
{
    if (!type_event_record::load(infile, version))
        return 0;
    if (infile->read(&m_location, sizeof(m_location)) != sizeof(m_location))
        return 0;
    if (infile->read(&m_objectId, sizeof(m_objectId)) != sizeof(m_objectId))
        return 0;
    if (infile->read(&m_extraInfo, sizeof(m_extraInfo)) != sizeof(m_extraInfo))
        return 0;
    bool ok = infile->read(&m_objectIndex, sizeof(m_objectIndex)) == sizeof(m_objectIndex);
    return ok;
}

VA(0x0049b220, 0x57)
DC_ADDRESS(0x08d338, 0x90)
MAC_ADDRESS(0x0c0710, 0xd8)
bool type_record_erase::save(TAbstractFile* outfile)
{
    type_event_record::save(outfile);
    outfile->write(&m_location, sizeof(m_location));
    outfile->write(&m_objectId, sizeof(m_objectId));
    outfile->write(&m_extraInfo, sizeof(m_extraInfo));
    bool ok = outfile->write(&m_objectIndex, sizeof(m_objectIndex)) == sizeof(m_objectIndex);
    return ok;
}

VA(0x0049b280, 0xEA)
DC_ADDRESS(0x08d3c8, 0xa4)
MAC_ADDRESS(0x0c07e8, 0x144)
void type_record_erase::replay(bool draw)
{
    NewmapCell* cell = g_game->getCell(m_location);
    g_advManager->mobilizeCurrHero(1, 0, draw);
    g_advManager->eraseObj(cell, m_location, 0);
    if (draw && (getMapExtra(m_location.m_x, m_location.m_y, m_location.m_z)
                 & g_mapVisibilityBit)) {
        g_advManager->completeDraw(0);
        g_advManager->updateScreen(0, 0);
    }
    g_advManager->demobilizeCurrHero(0, draw);
}

VA(0x0049b370, 0x83)
DC_ADDRESS(0x08d46c, 0x42)
MAC_ADDRESS(0x0c092c, 0xc0)
void type_record_erase::undo()
{
    g_game->m_worldMap.placeObject(m_objectId, 0);
    NewmapCell* cell = g_game->getCell(m_location);
    cell->m_extraInfo = m_extraInfo;
    cell->m_objectIndex = m_objectIndex;
}

// E:\gamedcs\event_record.cpp:646
// Retail derived vtable slot1 folds to 0x5721f0: mov eax,8; ret.
DC_ADDRESS(0x08d528, 0x4)
type_event_record_type type_record_hide_hero::getType() const
{
    return RECORD_HIDE_HERO;
}

MAC_ADDRESS(0x0c09ec, 0x70)
inline type_record_hide_hero::type_record_hide_hero(hero* who, char newOwner,
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
VA(0x0049b400, 0x27)
DC_ADDRESS(0x08d500, 0x26)
MAC_ADDRESS(0x0c0a5c, 0x48)
type_event_record* type_record_hide_hero::create()
{
    return new type_record_hide_hero();
}

VA(0x0049b430, 0xC8)
DC_ADDRESS(0x08d52c, 0x72)
MAC_ADDRESS(0x0c0aac, 0x150)
bool type_record_hide_hero::load(TAbstractFile* infile, int version)
{
    if (!type_event_record::load(infile, version))
        return 0;
    int heroId;
    if (infile->read(&heroId, sizeof(heroId)) != sizeof(heroId))
        return 0;
    m_currentHero = g_game->getHero(heroId);
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

VA(0x0049b500, 0x61)
DC_ADDRESS(0x08d5a0, 0x4e)
MAC_ADDRESS(0x0c0bfc, 0xdc)
bool type_record_hide_hero::save(TAbstractFile* outfile)
{
    type_event_record::save(outfile);
    outfile->write(&m_currentHero->m_id, sizeof(m_currentHero->m_id));
    outfile->write(&m_newOwner, 1);
    unsigned char packed = m_prevOwner;
    if (m_townGarrison)
        packed |= 0x40;
    bool ok = outfile->write(&packed, 1) == 1;
    return ok;
}

VA(0x0049b570, 0x102)
DC_ADDRESS(0x08d5f0, 0x96)
MAC_ADDRESS(0x0c0cd8, 0x130)
void type_record_hide_hero::replay(bool draw)
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

VA(0x0049b680, 0x1F)
DC_ADDRESS(0x08d688, 0x80)
MAC_ADDRESS(0x0c0e08, 0x44)
void type_record_hide_hero::undo()
{
    m_currentHero->m_owner = m_prevOwner;
    if (!m_townGarrison)
        m_currentHero->obscureCell();
}

DC_ADDRESS(0x08d708, 0xb6)
MAC_ADDRESS(0x0c0e4c, 0xb8)
inline type_record_show_hero::type_record_show_hero(hero* who, char newOwner,
                                                    type_point location,
                                                    unsigned char onBoat)
    : type_record_hide_hero(who, newOwner, 0)
{
    m_previousBoat = (who->m_flags >> 18) & 1;
    m_onBoat = onBoat;
    m_previousLocation = who->getLocation();
    m_location = location;
}

// E:\gamedcs\event_record.cpp:736
VA(0x0049b6a0, 0x27)
DC_ADDRESS(0x08d7c0, 0x26)
MAC_ADDRESS(0x0c0f64, 0x50)
type_event_record* type_record_show_hero::create()
{
    return new type_record_show_hero();
}

// E:\gamedcs\event_record.cpp:744
// Retail derived vtable slot1 folds to 0x572810: mov eax,9; ret.
DC_ADDRESS(0x08d7e8, 0x4)
type_event_record_type type_record_show_hero::getType() const
{
    return RECORD_SHOW_HERO;
}

VA(0x0049b6d0, 0x85)
DC_ADDRESS(0x08d7ec, 0x72)
MAC_ADDRESS(0x0c0fbc, 0x100)
bool type_record_show_hero::load(TAbstractFile* infile, int version)
{
    if (!type_record_hide_hero::load(infile, version))
        return 0;
    if (infile->read(&m_location, sizeof(m_location)) != sizeof(m_location))
        return 0;
    if (infile->read(&m_previousLocation, sizeof(m_previousLocation)) != sizeof(m_previousLocation))
        return 0;
    if (infile->read(&m_onBoat, 1) != 1)
        return 0;
    bool ok = infile->read(&m_previousBoat, 1) == 1;
    return ok;
}

VA(0x0049b760, 0x95)
DC_ADDRESS(0x08d860, 0x52)
MAC_ADDRESS(0x0c10bc, 0xc0)
bool type_record_show_hero::save(TAbstractFile* outfile)
{
    type_record_hide_hero::save(outfile);
    outfile->write(&m_location, sizeof(m_location));
    outfile->write(&m_previousLocation, sizeof(m_previousLocation));
    outfile->write(&m_onBoat, 1);
    bool ok = outfile->write(&m_previousBoat, 1) == 1;
    return ok;
}

VA(0x0049b800, 0x15E)
DC_ADDRESS(0x08d8b4, 0x140)
MAC_ADDRESS(0x0c117c, 0x178)
void type_record_show_hero::replay(bool draw)
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

VA(0x0049b960, 0x9E)
DC_ADDRESS(0x08d9f4, 0x8c)
MAC_ADDRESS(0x0c12f4, 0xe0)
void type_record_show_hero::undo()
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

// E:\gamedcs\event_record.cpp:842
// DC843 stores the requested player at+8, distinct from the base acting-seat byte at+4.
DC_ADDRESS(0x08da80, 0x40)
type_record_player_death::type_record_player_death(char playerId)
{
    m_extra = playerId;
}

VA(0x0049ba00, 0x27)
DC_ADDRESS(0x08dac0, 0x26)
MAC_ADDRESS(0x0c13d4, 0x48)
type_event_record* type_record_player_death::create()
{
    return new type_record_player_death();
}

VA(0x0049ba30, 0x6)
DC_ADDRESS(0x08dae8, 0x4)
MAC_ADDRESS(0x0c141c, 0x8)
type_event_record_type type_record_player_death::getType() const
{
    return RECORD_PLAYER_DEATH;
}

VA(0x0049ba40, 0x3D)
DC_ADDRESS(0x08daec, 0x3e)
MAC_ADDRESS(0x0c1424, 0x94)
bool type_record_player_death::load(TAbstractFile* infile, int version)
{
    if (!type_event_record::load(infile, version))
        return 0;
    bool ok = infile->read(&m_extra, 1) == 1;
    return ok;
}

VA(0x0049ba80, 0x30)
DC_ADDRESS(0x08db2c, 0x68)
MAC_ADDRESS(0x0c14b8, 0x84)
bool type_record_player_death::save(TAbstractFile* outfile)
{
    type_event_record::save(outfile);
    bool ok = outfile->write(&m_extra, 1) == 1;
    return ok;
}

VA(0x0049bab0, 0x11A)
DC_ADDRESS(0x08db94, 0x8a)
MAC_ADDRESS(0x0c153c, 0xf4)
void type_record_player_death::replay(bool draw)
{
    if (draw) {
        std::string text;
        text = formatString((*g_generalText)[GENERAL_TEXT_PLAYER_DEFEATED_FORMAT],
                             g_game->getPlayerName(m_extra));
        normalDialog(text.c_str(), 1, -1, -1, 10, m_extra, -1, -1, -1, 5000,
                     -1, 0);
    }
}

// E:\gamedcs\event_record.cpp:905
// The player-death undo slot shares the empty base undo body in retail.
DC_ADDRESS(0x08dc20, 0x4)
MAC_ADDRESS(0x0c1630, 0x4)
void type_record_player_death::undo()
{
}

// Mac default construction calls the base, then initializes the change vector.
MAC_COMPGEN_ADDRESS(0x0c1634, 0x5c, CLASS_CTOR, type_record_shroud)

VA_COMPGEN(0x0049bbd0, 0x21, SCALAR_DELETING_DTOR, type_record_shroud)

// DC0x8f398 has only vector/base cleanup, supplied by the implicit C++
// destructor; its exact emission is documented in config/source/dc_only.tsv.
// The implicit destructor the wrapper above calls: the change vector's
// _Tidy inlined (`operator delete(_First)` then the three-pointer clear)
// followed by the base's vptr store.
VA_COMPGEN(0x0049bc00, 0x2C, IMPLICIT_DTOR, type_record_shroud)
MAC_COMPGEN_ADDRESS(0x0c26d0, 0x7c, IMPLICIT_DTOR, type_record_shroud)

VA(0x0049bc30, 0x42)
DC_ADDRESS(0x08dcb0, 0x24)
MAC_ADDRESS(0x0c1698, 0x40)
type_event_record* type_record_shroud::create()
{
    return new type_record_shroud();
}

VA(0x0049bc80, 0x6)
DC_ADDRESS(0x08dcd4, 0x4)
MAC_ADDRESS(0x0c16d8, 0x8)
type_event_record_type type_record_shroud::getType() const
{
    return RECORD_SHROUD;
}

VA(0x0049bc90, 0x151)
DC_ADDRESS(0x08dcd8, 0xb0)
MAC_ADDRESS(0x0c16e0, 0xf4)
bool type_record_shroud::load(TAbstractFile* infile, int version)
{
    if (!type_event_record::load(infile, version))
        return 0;

    short count;
    if (infile->read(&count, sizeof(count)) != sizeof(count))
        return 0;

    type_shroud_change change;
    m_changes.clear();
    m_changes.reserve(count);

    while (count--) {
        if (infile->read(&change, sizeof(change)) != sizeof(change))
            return 0;
        m_changes.push_back(change);
    }
    return 1;
}

VA(0x0049bdf0, 0x66)
DC_ADDRESS(0x08dd88, 0x62)
MAC_ADDRESS(0x0c17d4, 0xcc)
bool type_record_shroud::save(TAbstractFile* outfile)
{
    type_event_record::save(outfile);
    short count = m_changes.size();
    outfile->write(&count, sizeof(count));
    for (int i = 0; i < count; ++i)
        outfile->write(&m_changes[i], sizeof(type_shroud_change));
    return 1;
}

// E:\gamedcs\event_record.cpp:978
// NO RETAIL BODY: the carve leaves no row between save (0x49bdf0) and
// replay (0x49be60), and the vtable accounts for both, so Complete
// expanded this DC-named member into its two callers rather than
// emitting it.  It is defined here at its DC source position so /Ob2 can
// make that same decision - a defined-but-unclaimed symbol adds no
// objdiff row of its own.
DC_ADDRESS(0x08ddec, 0x84)
MAC_ADDRESS(0x0c18a0, 0x60)
void type_record_shroud::addChange(int x, int y, int z,
                                    short oldValue, short newValue)
{
    type_shroud_change change;
    change.m_x = x;
    change.m_y = y;
    change.m_z = z;
    change.m_oldValue = oldValue;
    change.m_newValue = newValue;
    m_changes.push_back(change);
}

VA(0x0049be60, 0xBA)
DC_ADDRESS(0x08de70, 0xf0)
MAC_ADDRESS(0x0c1900, 0xe4)
void type_record_shroud::replay(bool draw)
{
    unsigned char changed = 0;
    int i = m_changes.size();
    while (i--) {
        type_shroud_change change = m_changes[i];
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

VA(0x0049bf20, 0x6E)
DC_ADDRESS(0x08df60, 0x7e)
MAC_ADDRESS(0x0c19e4, 0xa4)
void type_record_shroud::undo()
{
    int i = m_changes.size();
    while (i--) {
        type_shroud_change change = m_changes[i];
        *getMapExtraPtr(change.m_x, change.m_y, change.m_z) = change.m_oldValue;
    }
}

VA(0x0049bf90, 0x1F1)
DC_ADDRESS(0x08dfe0, 0x78)
MAC_ADDRESS(0x0c1a88, 0x124)
void game::recordClaimMine(long id, long newOwner)
{
    mine& currentMine = m_mines[id];
    type_point location(currentMine.m_mapX, currentMine.m_mapY,
                        currentMine.m_mapZ);
    CMCClaimMine msg(id, newOwner);
    sendMapChange(&msg);
    m_eventRecords.push_back(new type_record_claim_mine(id, newOwner));
}

VA(0x0049c190, 0x1FE)
DC_ADDRESS(0x08e058, 0x60)
MAC_ADDRESS(0x0c1bac, 0x124)
void game::recordClaimTown(long id, long newOwner)
{
    getTown(id);
    CMCClaimTown msg(id, newOwner);
    sendMapChange(&msg);
    m_eventRecords.push_back(new type_record_claim_town(id, newOwner));
}

// E:\gamedcs\event_record.cpp:1061
VA(0x0049c390, 0x1C2)
DC_ADDRESS(0x08e0b8, 0x4e)
MAC_ADDRESS(0x0c1cd0, 0xec)  // anchor-vtable (constructs 0x63df1c)
void game::recordEraseObject(NewmapCell* cell, type_point point)
{
    m_eventRecords.push_back(new type_record_erase(point,
                                                 cell->m_objectTypeIndex,
                                                 cell->m_extraInfo,
                                                 cell->m_objectIndex));
}

// E:\gamedcs\event_record.cpp:1071
// Retail takes THREE arguments (`ret 0xc`), not the Dreamcast's one: the
// replay state goes in the record's +0xc/+0x10 pair while the constructor
// snapshots the boat's current state into +0xd/+0x14 for undo.
VA(0x0049c560, 0x1B8)
DC_ADDRESS(0x08e108, 0x3e)
MAC_ADDRESS(0x0c1dbc, 0xdc)  // anchor-vtable (constructs 0x63deec)
void game::recordHideBoat(boat* currentBoat, unsigned char occupied,
                            int occupyingHero)
{
    m_eventRecords.push_back(new type_record_hide_boat(currentBoat, occupied,
                                                     occupyingHero));
}

VA(0x0049c720, 0x1DD)
DC_ADDRESS(0x08e148, 0x44)
MAC_ADDRESS(0x0c1e98, 0xdc)
void game::recordHideHero(hero* who, char newOwner,
                            unsigned char townGarrison)
{
    m_eventRecords.push_back(new type_record_hide_hero(who, newOwner,
                                                     townGarrison));
}

VA(0x0049c900, 0x217)
DC_ADDRESS(0x08e18c, 0x42)
MAC_ADDRESS(0x0c1f74, 0xe4)
void game::recordShowBoat(boat* currentBoat, type_point point)
{
    m_eventRecords.push_back(new type_record_show_boat(currentBoat, point));
}

VA(0x0049cb20, 0x226)
DC_ADDRESS(0x08e1d0, 0xa0)
MAC_ADDRESS(0x0c2058, 0xe4)
void game::recordShowHero(hero* who, signed char player, type_point point,
                            unsigned char reset)
{
    m_eventRecords.push_back(new type_record_show_hero(who, player, point,
                                                     reset));
}

VA(0x0049cd50, 0x1FA)
DC_ADDRESS(0x08e270, 0x4a)
MAC_ADDRESS(0x0c213c, 0xf4)
void game::recordMove(hero* who, int direction, type_point destination)
{
    m_eventRecords.push_back(new type_record_move_hero(who, direction,
                                                     destination));
}

// E:\gamedcs\event_record.cpp:1115
// NO RETAIL BODY. The nine recorders between shroud::undo and SetVisibility
// pair one-for-one onto the DC roster's other nine by the derived vftable
// each stores, and no row is left for this one: the carve's 0x49cf50 stores
// type_record_teleport's 0x63dea4, not type_record_player_death's 0x63df64.
// Whatever the PC revision does on player death, it does not go through an
// out-of-line recorder here.
// DC1116 constructs the typed record and calls push_back. No standalone retail address is claimed.
DC_ADDRESS(0x08e2bc, 0x3c)
void game::recordPlayerDeath(char playerId)
{
    m_eventRecords.push_back(new type_record_player_death(playerId));
}

VA(0x0049cf50, 0x20B)
DC_ADDRESS(0x08e2f8, 0x42)
MAC_ADDRESS(0x0c2230, 0xe4)
void game::recordTeleport(hero* who, type_point destination)
{
    m_eventRecords.push_back(new type_record_teleport(who, destination));
}

// E:\gamedcs\event_record.cpp:1136
// The DC RECT, distance and teamMask locals reproduce Complete exactly.
// Both dc 0x8e48c and retail +0x17f branch around addChange and the map-cell
// store, so unchanged cells are not rewritten. The header getChangeCount
// helper and vector push_back remain canonical source calls and expand to
// the retail sequences without inline steering.
VA(0x0049d160, 0x268)
DC_ADDRESS(0x08e33c, 0x210)
MAC_ADDRESS(0x0c2314, 0x3bc)  // anchor-global (0x63df7c + GetMapExtraPtr)
void game::setVisibility(const int startX, const int startY, const int z,
                         const int whichPlayer,
                         int range, bool remoteMove)
{
    if (whichPlayer < 0 || whichPlayer >= 8)
        return;

    unsigned short teamMask = getTeamMask(whichPlayer);
    double distance = range + 0.5;
    type_record_shroud* record = new type_record_shroud();

    RECT rect;
    rect.left = max(startX - range, 0);
    rect.right = min(startX + range + 1, g_mapWidth);
    rect.top = max(startY - range, 0);
    rect.bottom = min(startY + range + 1, g_mapHeight);

    for (int y = rect.top; y < rect.bottom; ++y) {
        for (int x = rect.left; x < rect.right; ++x) {
            if (sqrt(static_cast<double>(
                    (startX - x) * (startX - x)
                    + (startY - y) * (startY - y))) <= distance) {
                unsigned short* oldValue = getMapExtraPtr(x, y, z);
                unsigned short newValue = *oldValue | teamMask;
                if (*oldValue != newValue) {
                    record->addChange(x, y, z, *oldValue, newValue);
                    *oldValue = newValue;
                }
            }
        }
    }

    if (!remoteMove && record->getChangeCount() != 0 && !g_completeDrawMessageBypass)
    {
        // Dreamcast line 1178 identifies the canonical source call.
        m_eventRecords.push_back(record);
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
// DC line 1199's double distance is range+0.5, not the sqrt result; lines
// 1206..1209 fill tagRECT rect. Both dc 0x8e624 and retail +0x18e skip the
// write when unchanged, so line 1224's store belongs inside the inequality.
// Recovering that store reaches 97.8326%; canonical push_back (line 1232)
// removes the inline-depth pin and reaches 99.9070%; RECT ownership closes
// the frame and reaches 100%. Direct and named per-cell deltas reproduce it.
VA(0x0049d3d0, 0x260)
DC_ADDRESS(0x08e54c, 0x1e4)
MAC_ADDRESS(0x0c274c, 0x39c)  // anchor-global (0x63df7c + GetMapExtraPtr)
void game::resetVisibility(int startX, int startY, int z, int whichPlayer,
                           int range)
{
    unsigned short enemyMask = 0x100;
    if (whichPlayer != -1)
        enemyMask = getTeamMask(whichPlayer) | 0x100;

    double distance = range + 0.5;
    type_record_shroud* record = new type_record_shroud();

    RECT rect;
    rect.left = max(startX - range, 0);
    rect.right = min(startX + range + 1, g_mapWidth);
    rect.top = max(startY - range, 0);
    rect.bottom = min(startY + range + 1, g_mapHeight);

    for (int y = rect.top; y < rect.bottom; ++y) {
        for (int x = rect.left; x < rect.right; ++x) {
            if (sqrt(static_cast<double>(
                    (startX - x) * (startX - x)
                    + (startY - y) * (startY - y))) <= distance) {
                unsigned short* oldValue = getMapExtraPtr(x, y, z);
                unsigned short newValue = *oldValue & enemyMask;
                if (*oldValue != newValue) {
                    record->addChange(x, y, z, *oldValue, newValue);
                    *oldValue = newValue;
                }
            }
        }
    }

    // The EMPTY arm is the one retail lays out inline (`jne` forward to the
    // queue), so the test is spelled == 0, not != 0.
    if (record->getChangeCount() == 0) {
        delete record;
    } else {
        m_eventRecords.push_back(record);
    }
}

VA(0x0049d630, 0x8C)
DC_ADDRESS(0x08e730, 0x4a)
MAC_ADDRESS(0x0c2ae8, 0x90)
void game::clearEventRecords()
{
    int i = m_eventRecords.size();
    while (i-- != 0)
        delete m_eventRecords[i];
    m_eventRecords.clear();
}

VA(0x0049d6c0, 0xD3)
DC_ADDRESS(0x08e77c, 0xb2)
MAC_ADDRESS(0x0c2b78, 0x12c)
void game::clearEventRecords(char playerId)
{
    int i = 0;
    while (i < m_eventRecords.size() && m_eventRecords[i]->getPlayerId() != playerId)
        ++i;
    if (i == m_eventRecords.size())
        return;
    while (i < m_eventRecords.size() && m_eventRecords[i]->getPlayerId() == playerId)
        ++i;
    for (int j = 0; j < i; ++j)
        delete m_eventRecords[j];
    m_eventRecords.erase(m_eventRecords.begin(), m_eventRecords.begin() + i);
}

VA(0x0049d7a0, 0x2C1)
DC_ADDRESS(0x08e830, 0x258)
MAC_ADDRESS(0x0c2ca4, 0x2f4)
void game::playRecordedEvents()
{
    int savedPlayer = g_netLocalGamePos;
    playerData* actingPlayer = g_currentPlayer;

    town* currTown;
    hero* currHero = g_game->getCurrHero();

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
    message msg;
    int savedWalkSpeed = g_config.m_walkSpeed[0];
    unsigned char savedSuppress = g_config.m_blackoutComputer != 0;
    if (g_config.m_walkSpeed[0] > 4)
        g_config.m_walkSpeed[0] = 4;
    g_config.m_blackoutComputer = 0;

    for (int j = 0; j < size; ++j) {
        // Boolean carrier for the independently proven virtual replay(bool).
        bool draw = !interrupted
            && m_eventRecords[j]->getPlayerId() != savedPlayer;
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

    g_config.m_walkSpeed[0] = savedWalkSpeed;
    g_config.m_blackoutComputer = savedSuppress;
    g_advManager->completeDraw(0);
    g_advManager->updateScreen(0, 0);
}

VA(0x0049da70, 0x41)
DC_ADDRESS(0x08ea88, 0x46)
MAC_ADDRESS(0x0c2f98, 0x50)
bool game::replayAvailable() const
{
    for (unsigned i = 0; i < m_eventRecords.size(); ++i) {
        if (m_eventRecords[i]->getPlayerId() != g_netLocalGamePos)
            return 1;
    }
    return 0;
}

// The record factory table, retail .data 0x6776b0. Eleven slots indexed by
// the type byte load_recorded_events reads off the stream, in
// type_event_record_type order, plus the unused zero slot; the retail
// dispatch is `call dword ptr [ecx*4 + 0x6776b0]` after a 1..11 range check.
DATA(0x006776b0)
type_event_record* (*g_recordCreators[12])() = {
    0,
    type_record_move_hero::create,
    type_record_teleport::create,
    type_record_claim_mine::create,
    type_record_claim_town::create,
    type_record_hide_boat::create,
    type_record_show_boat::create,
    type_record_erase::create,
    type_record_hide_hero::create,
    type_record_show_hero::create,
    type_record_player_death::create,
    type_record_shroud::create
};

VA(0x0049dac0, 0x19C)
DC_ADDRESS(0x08ead0, 0xf4)
MAC_ADDRESS(0x0c2fe8, 0x19c)
bool game::loadRecordedEvents(TAbstractFile* infile, int version)
{
    long count;
    if (infile->read(&count, sizeof(count)) != sizeof(count))
        return 0;

    clearEventRecords();
    m_eventRecords.reserve(count);

    char type;
    type_event_record* record;
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

VA(0x0049dc60, 0x8C)
DC_ADDRESS(0x08ebc4, 0x98)
MAC_ADDRESS(0x0c3184, 0xf4)
bool game::saveRecordedEvents(TAbstractFile* outfile)
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
