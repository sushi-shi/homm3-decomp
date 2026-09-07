#ifndef HOMM3_CHEATCODE_H
#define HOMM3_CHEATCODE_H

#include <string.h>
#include <va.h>

// Dreamcast Game.h proves the complete 200-byte class and its single char
// array. Retail's adventure/combat cheat handlers inline the constructor and
// compare members while sharing the out-of-line encoder at 0x402a30.
class TCheatCode {
public:
    TCheatCode() { m_code[0] = 0; }
    TCheatCode(const char* value) { encode(value); }

    bool compare(const char* value) const
    {
        return _strcmpi(m_code, value) == 0;
    }

    // Before normalization (function): TCheatCode::GetCode.
    const char* getCode() const { return m_code; }

private:
    void encode(const char* value);

    // Before normalization: a.
    static const char* s_a;
    // Before normalization: b.
    static const char* s_b;
    // Before normalization: code.
    char m_code[200];
};
SIZE(TCheatCode, 200);

#endif  /* HOMM3_CHEATCODE_H */
