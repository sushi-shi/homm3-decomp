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
class TTimedEvent {
public:
    std::basic_string<char, std::char_traits<char>, std::allocator<char> > Message;
    int ResQty[7];
    unsigned char PlayerFlags;
    unsigned char ApplyToHuman;
    unsigned char ApplyToComputer;
    char pad_2f;
    unsigned short FirstTime;
    unsigned short Interval;

    int Save(TAbstractFile* outfile);
    int Load(TAbstractFile* infile, int saveVersion);
};
SIZE(TTimedEvent, 0x34);

// DC CodeView names the derived town-event payload. Retail's four-byte-wider
// base shifts the three fields to +0x34/+0x38/+0x40; saveTownEventList proves
// those offsets, the seven-word generator band, and the 0x50-byte stride.
class TTownEvent : public TTimedEvent {
public:
    signed char TownNum;
    char pad_35[3];
    __int64 BuildBuildings;
    unsigned short generatorBonuses[7];
    char pad_4e[2];
};
SIZE(TTownEvent, 0x50);

#endif
