#ifndef HOMM3_CHEATCODE_H
#define HOMM3_CHEATCODE_H

#include <string.h>
#include <va.h>

// Dreamcast Game.h proves the complete 200-byte class and its single char
// array. Retail's adventure/combat cheat handlers inline the constructor and
// compare members while sharing the out-of-line encoder at 0x402a30.
class TCheatCode {
public:
    TCheatCode() { code[0] = 0; }
    TCheatCode(const char* value) { encode(value); }

    bool compare(const char* value) const
    {
        return _strcmpi(code, value) == 0;
    }

    const char* GetCode() const { return code; }

private:
    void encode(const char* value);

    static const char* a;
    static const char* b;
    char code[200];
};
SIZE(TCheatCode, 200);

#endif  /* HOMM3_CHEATCODE_H */
