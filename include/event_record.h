// event_record.h - prototypes of event_record.cpp (compiland event_record.obj)
#ifndef HOMM3_EVENT_RECORD_H
#define HOMM3_EVENT_RECORD_H

#include <vector>

// type_point is a value member of several record subclasses below; struct.h is
// already in this TU's include closure (game.h pulls it in), so parsing it here
// adds no declarators. hero/boat/NewmapCell appear only as pointer members.
#include "struct.h"

class TAbstractFile;
class hero;
class boat;
class NewmapCell;

// The world extents both visibility sweeps clamp against. DECLARATIONS
// ONLY - game.h owns the DATA claims on 0x6783c8 / 0x6783cc, and a
// second claim on one RVA is a fatal duplicate at delink time. Declared
// here rather than by including game.h, whose closure event_record.obj
// does not otherwise need.
extern int g_mapWidth;
extern int g_mapHeight;

// Record discriminant returned by get_type(); values byte-proven from the
// retail get_type bodies (mov eax,N / ret) reached through each class vtable.
enum type_event_record_type {
    RECORD_MOVE_HERO    = 1,
    RECORD_TELEPORT     = 2,
    RECORD_CLAIM_MINE   = 3,
    RECORD_CLAIM_TOWN   = 4,
    RECORD_HIDE_BOAT    = 5,
    RECORD_SHOW_BOAT    = 6,
    RECORD_ERASE        = 7,
    RECORD_HIDE_HERO    = 8,
    RECORD_SHOW_HERO    = 9,
    RECORD_PLAYER_DEATH = 10,
    RECORD_SHROUD       = 11,
};

// Polymorphic base of the recorded adventure actions. Retail's
// replay_available reads the signed owner byte immediately after the vptr;
// Dreamcast supplies the class and `player_id` identity at the same offset.
// Virtual layout (byte-proven from the type_record_* vtables): slot 0 is the
// scalar deleting destructor, slot 1 get_type, then load/save/replay/undo.
// get_type is PURE: slot 1 of the base vtable (0x63de74) is 0x617d9a, which
// is the CRT's __purecall stub (`push 0x19 / call __amsg_exit`) and nothing
// else. Slots 4 and 5 are the empty bodies at 0x485d80 (`ret 4`) and
// 0x5bc690 (`ret`), both /OPT:ICF folds shared with unrelated compilands, so
// neither replay nor undo has a body this TU can own.
class type_event_record {
public:
    type_event_record();
    virtual ~type_event_record();
    virtual type_event_record_type getType() const = 0;
    virtual unsigned char load(TAbstractFile* infile, int version);
    virtual unsigned char save(TAbstractFile* outfile);
    virtual void replay(unsigned char draw);
    virtual void undo();
    signed char m_playerId;  // +0x04
};

// A recorded hero step. load/save (0x49a690/0x49a750) serialize player_id,
// the hero's own id (hero+0x1a, re-resolved to &gpGame->heroes[id] on load),
// direction(+0x11), source(+0xc) and destination(+0x12); the +0x10 byte holds
// the pre-move hero attribute (hero+0x47) and is captured at record time, not
// serialized. undo (0x49a910) restores source, +0x10 and re-obscures the cell.
class type_record_move_hero : public type_event_record {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;
    // Retail's record_move (0x49cd50) expands this: the hero, its CURRENT
    // facing byte snapshotted into restore_flag, the step direction, the
    // hero's own map point into source and the caller's into destination.
    type_record_move_hero(hero* who, char direction, type_point destination);
    type_record_move_hero() {}

    hero* m_currentHero;          // +0x08
    type_point m_source;           // +0x0c - hero position before the move
    signed char m_restoreFlag;    // +0x10 - hero+0x47 snapshot (not serialized)
    signed char m_direction;       // +0x11
    type_point m_destination;      // +0x12
};

// Teleport reuses move_hero's whole serializer and its undo: slots 2, 3 and
// 5 of its vtable (0x63dea4) are literally move_hero's addresses. Only
// get_type and replay differ.
class type_record_teleport : public type_record_move_hero {
public:
    // record_teleport (0x49cf50) reads hero+0x47 TWICE - once at the call
    // site for this argument and once inside the base body for
    // restore_flag - which is what proves the facing is forwarded here.
    type_record_teleport(hero* who, type_point destination);
    type_record_teleport() {}

    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
};

// Retail serializes these four fields in address order except that the two
// owner bytes are written old-then-new. replay reads new_owner at +0x0c;
// undo reads old_owner at +0x0d and restores mines[id].playerOwner.
class type_record_claim_mine : public type_event_record {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;

    type_record_claim_mine(long id, char newOwner);
    type_record_claim_mine() {}

    int m_id;                 // +0x08
    signed char m_newOwner;  // +0x0c
    signed char m_oldOwner;  // +0x0d
};

// The town variant has the same four-field tail. replay writes new_owner to
// towns[id].owner; undo restores old_owner from the following byte.
class type_record_claim_town : public type_record_claim_mine {
public:
    type_record_claim_town(long id, char newOwner);
    type_record_claim_town() {}

    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;
};

// A recorded boat-hide. load/save (0x49ad00/0x49adf0) serialize the boat (by its
// byte id at boat+0x19, re-resolved via &gpGame->boats[]); two flag bytes(+0xc,
// +0xd) and two hero IDs (+0x10/+0x14, int in memory, 16-bit on disk) are only
// present in save versions [0x12,0x1e] except 0x1c, or >= 0x23; older saves
// default them to {1,0,-1,-1}.
class type_record_hide_boat : public type_event_record {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;
    // The +0xc/+0x10 pair is the state replay installs and comes from the
    // caller; the +0xd/+0x14 pair is the boat's CURRENT state, snapshotted
    // here for undo.
    type_record_hide_boat(boat* currentBoat, unsigned char occupied,
                          int occupyingHero);
    type_record_hide_boat() {}

    boat* m_currentBoat;      // +0x08
    // Replay 0x49ae80 installs the caller's new occupancy/hero pair;
    // the constructor supplies these role-derived names (PC-only fields).
    unsigned char m_occupied;  // +0x0c - replay occupancy (old-save default 1)
    // Undo 0x49aed0 restores the snapshot taken from the original boat.
    unsigned char m_previousOccupied;  // +0x0d - undo occupancy (old-save default 0)
    int m_occupyingHero;            // +0x10 - replay hero ID (old-save default -1)
    int m_previousOccupyingHero;    // +0x14 - undo hero ID (old-save default -1)
};

// show_boat extends hide_boat with two trailing type_points (+0x18/+0x1c).
// load calls hide_boat::load then reads them; save inlines hide_boat::save
// then writes them. The two are four bytes apiece and serialized whole, which
// is why they read as dwords in load/save - but replay unpacks +0x18 into the
// boat's x/y/z and undo unpacks +0x1c, both with the 10/10/4 bitfield shifts,
// and that is what types them.
class type_record_show_boat : public type_record_hide_boat {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;
    type_record_show_boat(boat* currentBoat, type_point location);
    type_record_show_boat() {}

    type_point m_location;           // +0x18 - replay destination
    type_point m_previousLocation;  // +0x1c - restored by undo
};

// A recorded object erasure. load/save (0x49b190/0x49b220) serialize four
// dwords after player_id, in declaration order: the map location, the erased
// object's id, its extra-info word and its object-list index.
class type_record_erase : public type_event_record {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;
    type_record_erase(type_point location, long objectId,
                      unsigned long extraInfo, long objectIndex);
    type_record_erase() {}

    type_point m_location;         // +0x08
    int m_objectId;               // +0x0c
    unsigned int m_extraInfo;     // +0x10
    int m_objectIndex;            // +0x14
};

// Dreamcast names the complete tail; retail independently proves each
// offset through load/save/replay/undo. The final byte records whether the
// hero was a town garrison, in which case undo must not put it on the map.
class type_record_hide_hero : public type_event_record {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;

    type_record_hide_hero(hero* who, char newOwner,
                          unsigned char townGarrison);
    type_record_hide_hero() {}

    hero* m_currentHero;          // +0x08
    signed char m_newOwner;       // +0x0c
    signed char m_prevOwner;      // +0x0d
    unsigned char m_townGarrison; // +0x0e
};

// show_hero extends hide_hero with the replay and undo map locations followed
// by the corresponding aboard-boat flags. Retail replay reads the first pair;
// undo reads the second pair.
class type_record_show_hero : public type_record_hide_hero {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;
    type_record_show_hero(hero* who, char newOwner, type_point location,
                          unsigned char onBoat);
    type_record_show_hero() {}

    type_point m_location;          // +0x10 - replay destination
    type_point m_previousLocation; // +0x14 - restored by undo
    unsigned char m_onBoat;        // +0x18 - replay state
    unsigned char m_previousBoat;  // +0x19 - restored by undo
};

class type_record_player_death : public type_event_record {
public:
    static type_event_record* create();
    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    type_record_player_death() {}

    // Retail replay sign-extends this serialized byte for both the player-name
    // lookup and the dialog payload; the role is still unknown, but its
    // signedness is byte-proven.
    signed char m_extra;  // +0x08 - second serialized byte (role TBD)
};

class type_record_shroud : public type_event_record {
public:
    struct type_shroud_change : public type_point {
        unsigned short m_oldValue;
        unsigned short m_newValue;
    };

    static type_event_record* create();

    virtual type_event_record_type getType() const OVERRIDE;
    virtual unsigned char load(TAbstractFile* infile, int version) OVERRIDE;
    virtual unsigned char save(TAbstractFile* outfile) OVERRIDE;
    virtual void replay(unsigned char draw) OVERRIDE;
    virtual void undo() OVERRIDE;

    // E:\gamedcs\event_record.cpp:978 (dc 0x8ddec) and event_record.h:319
    // (dc 0x8f258). Neither survives as a retail body - the carve leaves no
    // row between save (0x49bdf0) and replay (0x49be60) - so Complete
    // expanded both into game::SetVisibility / game::ResetVisibility.
    void addChange(int x, int y, int z, short oldValue, short newValue);
    long getChangeCount();

    std::vector<type_shroud_change> m_changes;  // +0x08 (allocator at +0x08)
};

// --- globals ---
// CODEVIEW(E:\gamedcs\event_record.cpp:65, dc 0x8c6b8) void set_player(char new_player);

// --- game ---
// CODEVIEW(E:\gamedcs\event_record.cpp:1061, dc 0x8e0b8) void game::record_erase_object(NewmapCell* cell, type_point point);
// CODEVIEW(E:\gamedcs\event_record.cpp:1071, dc 0x8e108) void game::record_hide_boat(boat* current_boat);
// CODEVIEW(E:\gamedcs\event_record.cpp:1115, dc 0x8e2bc) void game::record_player_death(char player_id);
// CODEVIEW(E:\gamedcs\event_record.cpp:1136, dc 0x8e33c) void game::SetVisibility(const int startX, const int startY, const int z, const int whichPlayer, int range, unsigned char remote_move);
// CODEVIEW(E:\gamedcs\event_record.cpp:1189, dc 0x8e54c) void game::ResetVisibility(int start_x, int start_y, int z, int whichPlayer, int range);
// CODEVIEW(E:\gamedcs\event_record.cpp:1367, dc 0x8ea88) unsigned char game::replay_available();

// --- type_event_record ---
// CODEVIEW(E:\gamedcs\event_record.cpp:36, dc 0x8c624) void type_event_record::type_event_record();
// CODEVIEW(E:\gamedcs\event_record.cpp:81, dc 0x8c708) void type_event_record::replay();
// CODEVIEW(E:\gamedcs\event_record.cpp:88, dc 0x8c70c) void type_event_record::undo();
// CODEVIEW(E:\gamedcs\event_record.h:64, dc 0x8ec5c) char type_event_record::get_player_id();
// CODEVIEW(E:\gamedcs\event_record.cpp:38, dc 0x8f330) void* type_event_record::`scalar deleting destructor'(unsigned __flags);

// --- type_record_claim_mine ---
// CODEVIEW(E:\gamedcs\event_record.cpp:237, dc 0x8cb2c) void type_record_claim_mine::type_record_claim_mine(long _id, char _owner);
// CODEVIEW(E:\gamedcs\event_record.cpp:255, dc 0x8cbb0) type_event_record_type type_record_claim_mine::get_type();
// CODEVIEW(E:\gamedcs\event_record.h:128, dc 0x8eda0) void type_record_claim_mine::type_record_claim_mine();
// CODEVIEW(E:\gamedcs\event_record.h:128, dc 0x8eddc) void* type_record_claim_mine::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:128, dc 0x8ee10) void type_record_claim_mine::~type_record_claim_mine();

// --- type_record_claim_town ---
// CODEVIEW(E:\gamedcs\event_record.cpp:321, dc 0x8ccfc) void type_record_claim_town::type_record_claim_town(long _id, char _new_owner);
// CODEVIEW(E:\gamedcs\event_record.cpp:339, dc 0x8cd84) type_event_record_type type_record_claim_town::get_type();
// CODEVIEW(E:\gamedcs\event_record.h:149, dc 0x8ee28) void type_record_claim_town::type_record_claim_town();
// CODEVIEW(E:\gamedcs\event_record.h:149, dc 0x8ee64) void* type_record_claim_town::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:149, dc 0x8ee98) void type_record_claim_town::~type_record_claim_town();

// --- type_record_erase ---
// CODEVIEW(E:\gamedcs\event_record.cpp:533, dc 0x8d220) void type_record_erase::type_record_erase(type_point _location, long _object_id, unsigned long _extra_info, long _object_index);
// CODEVIEW(E:\gamedcs\event_record.h:214, dc 0x8eff0) void type_record_erase::type_record_erase();
// CODEVIEW(E:\gamedcs\event_record.h:214, dc 0x8f044) void* type_record_erase::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:214, dc 0x8f078) void type_record_erase::~type_record_erase();

// --- type_record_hide_boat ---
// CODEVIEW(E:\gamedcs\event_record.cpp:376, dc 0x8ce74) void type_record_hide_boat::type_record_hide_boat(boat* _current_boat);
// CODEVIEW(E:\gamedcs\event_record.h:169, dc 0x8eeb0) void type_record_hide_boat::type_record_hide_boat();
// CODEVIEW(E:\gamedcs\event_record.h:169, dc 0x8eeec) void* type_record_hide_boat::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:169, dc 0x8ef20) void type_record_hide_boat::~type_record_hide_boat();

// --- type_record_hide_hero ---
// CODEVIEW(E:\gamedcs\event_record.cpp:628, dc 0x8d4b0) void type_record_hide_hero::type_record_hide_hero(hero* _hero, char _owner);
// CODEVIEW(E:\gamedcs\event_record.cpp:646, dc 0x8d528) type_event_record_type type_record_hide_hero::get_type();
// CODEVIEW(E:\gamedcs\event_record.h:238, dc 0x8f090) void type_record_hide_hero::type_record_hide_hero();
// CODEVIEW(E:\gamedcs\event_record.h:238, dc 0x8f0cc) void* type_record_hide_hero::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:238, dc 0x8f100) void type_record_hide_hero::~type_record_hide_hero();

// --- type_record_move_hero ---
// CODEVIEW(E:\gamedcs\event_record.cpp:96, dc 0x8c710) void type_record_move_hero::type_record_move_hero(hero* _hero, char _direction, type_point _destination);
// CODEVIEW(E:\gamedcs\event_record.cpp:186, dc 0x8c9ec) void type_record_move_hero::undo();
// CODEVIEW(E:\gamedcs\event_record.h:85, dc 0x8ec60) void type_record_move_hero::type_record_move_hero();
// CODEVIEW(E:\gamedcs\event_record.h:85, dc 0x8eccc) void* type_record_move_hero::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:85, dc 0x8ed00) void type_record_move_hero::~type_record_move_hero();

// --- type_record_player_death ---
// CODEVIEW(E:\gamedcs\event_record.cpp:842, dc 0x8da80) void type_record_player_death::type_record_player_death(char _player_id);
// CODEVIEW(E:\gamedcs\event_record.cpp:905, dc 0x8dc20) void type_record_player_death::undo();
// CODEVIEW(E:\gamedcs\event_record.h:286, dc 0x8f1d0) void type_record_player_death::type_record_player_death();
// CODEVIEW(E:\gamedcs\event_record.h:286, dc 0x8f20c) void* type_record_player_death::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:286, dc 0x8f240) void type_record_player_death::~type_record_player_death();

// --- type_record_show_boat ---
// CODEVIEW(E:\gamedcs\event_record.cpp:449, dc 0x8cfa8) void type_record_show_boat::type_record_show_boat(boat* _current_boat, type_point _location);
// CODEVIEW(E:\gamedcs\event_record.h:190, dc 0x8ef38) void type_record_show_boat::type_record_show_boat();
// CODEVIEW(E:\gamedcs\event_record.h:190, dc 0x8efa4) void* type_record_show_boat::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:190, dc 0x8efd8) void type_record_show_boat::~type_record_show_boat();

// --- type_record_show_hero ---
// CODEVIEW(E:\gamedcs\event_record.cpp:725, dc 0x8d708) void type_record_show_hero::type_record_show_hero(hero* _hero, char _owner, type_point _location, unsigned char _is_boat);
// CODEVIEW(E:\gamedcs\event_record.cpp:744, dc 0x8d7e8) type_event_record_type type_record_show_hero::get_type();
// CODEVIEW(E:\gamedcs\event_record.h:262, dc 0x8f118) void type_record_show_hero::type_record_show_hero();
// CODEVIEW(E:\gamedcs\event_record.h:262, dc 0x8f184) void* type_record_show_hero::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:262, dc 0x8f1b8) void type_record_show_hero::~type_record_show_hero();

// --- type_record_shroud ---
// CODEVIEW(E:\gamedcs\event_record.cpp:912, dc 0x8dc24) void type_record_shroud::type_record_shroud();
// CODEVIEW(E:\gamedcs\event_record.cpp:978, dc 0x8ddec) void type_record_shroud::add_change(int x, int y, int z, short old_value, short new_value);
// CODEVIEW(E:\gamedcs\event_record.h:319, dc 0x8f258) long type_record_shroud::get_change_count();
// CODEVIEW(E:\gamedcs\event_record.cpp:913, dc 0x8f364) void* type_record_shroud::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.cpp:913, dc 0x8f398) void type_record_shroud::~type_record_shroud();
// CODEVIEW(E:\gamedcs\event_record.cpp:953, dc 0x8f3c0) void type_record_shroud::type_shroud_change::type_shroud_change();

// --- type_record_teleport ---
// CODEVIEW(E:\gamedcs\event_record.cpp:204, dc 0x8ca54) void type_record_teleport::type_record_teleport(hero* _hero, type_point _destination);
// CODEVIEW(E:\gamedcs\event_record.h:108, dc 0x8ed18) void type_record_teleport::type_record_teleport();
// CODEVIEW(E:\gamedcs\event_record.h:108, dc 0x8ed54) void* type_record_teleport::`scalar deleting destructor'(unsigned __flags);
// CODEVIEW(E:\gamedcs\event_record.h:108, dc 0x8ed88) void type_record_teleport::~type_record_teleport();

#endif  /* HOMM3_EVENT_RECORD_H */
