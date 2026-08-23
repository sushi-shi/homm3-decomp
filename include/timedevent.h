// TTimedEvent - recurring adventure-map event payload.
#ifndef HOMM3_TIMEDEVENT_H
#define HOMM3_TIMEDEVENT_H

#include <string>
#include <va.h>

class TAbstractFile;

// Dreamcast CodeView supplies the shared member names. Retail widens the
// leading STL string to 16 bytes and adds ApplyToHuman before the DC-attested
// ApplyToComputer byte; TTimedEvent::Save/Load prove both bytes and every
// remaining offset, while saveTimedEventList closes the 0x34-byte stride.
// The alignment byte before FirstTime is deliberately implicit: naming it
// makes VC6's generated copies treat retail padding as a real member.
class TTimedEvent {
public:
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > Message;
    int ResQty[7];
    unsigned char PlayerFlags;
    unsigned char ApplyToHuman;
    unsigned char ApplyToComputer;
    unsigned short FirstTime;
    unsigned short Interval;

    // `ret 8`: the save version is a second argument, gating the
    // apply-to-human flag at 28 exactly as LoadGarrisonPool does.
    int Read(TAbstractFile* infile, int saveVersion);
    int Save(TAbstractFile* outfile);
    int Load(TAbstractFile* infile, int saveVersion);
};
SIZE(TTimedEvent, 0x34);

// DC CodeView names the derived town-event payload. Retail's four-byte-wider
// base shifts the three fields to +0x34/+0x38/+0x40; saveTownEventList proves
// those offsets, the seven-word generator band, and the 0x50-byte stride.
// Alignment before BuildBuildings and the tail rounding are likewise left
// implicit so generated copies do not copy padding bytes.
class TTownEvent : public TTimedEvent {
public:
    signed char TownNum;
    __int64 BuildBuildings;
    unsigned short generatorBonuses[7];

    // MapCell.h:400 in the DC roster - a header-inline default constructor.
    // loadTownEventList's resize temp proves its whole body: after the base
    // string is tidied it zeroes the eight bytes of BuildBuildings and
    // nothing else, TownNum and the generator band staying uninitialized.
    TTownEvent() { BuildBuildings = 0; }
};
SIZE(TTownEvent, 0x50);

#endif
