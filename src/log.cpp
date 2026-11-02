#include "va.h"
#include "log.h"

// Complete keeps this release hook out of line: EarlySetup passes the
// current-directory string and calls the shared empty body. Mac retains it
// outside kb's code region, between the gzfile initializer and a separate
// initialization block. The logging filename is inferred. Dreamcast
// kb.cpp:648 calls the older CLogFile::InitLogFile() instead. The map
// editor passes its established directory to the same release hook from
// InitInstance; its body is also empty.
MAC_ADDRESS(0x221ee0, 0x4)
VA_AT(h3maped, 0x004e3ca0, 0x1)
void initLogFile(const char* path)
{
}
