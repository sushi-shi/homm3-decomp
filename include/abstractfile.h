// abstractfile.h - the common three-slot game stream interface.
#ifndef HOMM3_ABSTRACTFILE_H
#define HOMM3_ABSTRACTFILE_H

// Retail's vtable at 0x63dac0 names slot 0 as the deleting destructor;
// virtual calls use slot 1 to read and slot 2 to write (this in ECX,
// (buffer, size) on the stack). Keep that three-slot ABI canonical for every
// consumer rather than substituting an opaque pure-virtual placeholder.
class TAbstractFile {
public:
    // Retail expands this body in TGzFile::~TGzFile (0x4d6d60) and
    // t_memory_file::~t_memory_file (0x512b00). Their constructor cleanups
    // also emit it out of line, identical in bytes AND vftable relocation
    // to the implicit stream/resource-adapter destructors folded at
    // 0x487e00. A retained copy therefore does not imply an ordinary
    // declaration. Defining it only in customcampaign.cpp is the negative
    // control: the two derived destructors call it and fall to 80/87.69%.
    virtual ~TAbstractFile() {}
    virtual int read(void* data, int size) = 0;
    virtual int write(const void* data, int size) = 0;
};

// TODO: sweep the codebase for hand-rolled scalar read/write patterns and
// route them through these helpers. The shape to look for is a local staging
// scalar whose address is handed straight to read()/write() - e.g.
// `charBuffer = m_owner; outfile->write(&charBuffer, sizeof(charBuffer));` or
// the same with a braced scope per field. hero::save has 49 scalar helper
// calls and eleven direct buffer writes. The helper lifetimes reproduce
// retail's 0x8 frame; the earlier function-scope staging buffers used 0x1c.
// This is an inferred interface, not a recovered template declaration.
// town::save, SavedGameHeader::save, and the NewSMapHeader and mapcell
// readers still stage by hand, and several of them sit below 100%
// on frame-size residuals. Check each against its own retail bytes before
// converting - a staging local that is genuinely two distinct variables
// (town::load and town::save each need a SECOND char local for the position
// and dock fields) must stay two.

// Complete native serialization: the output-reference overload
// reads one native scalar into caller-owned storage and preserves the actual
// int byte count, including short reads and errors. The existing value reader
// owns its local and intentionally discards that count. This inferred API
// adds no virtual slot or conversion and claims no Dreamcast declaration.
// Its writing pair, absent until now. The value is taken BY VALUE on purpose:
// the parameter is the stack temp whose address Write() receives, so the
// PARAMETER's type - not the member's - fixes the width, which is what makes a
// record's write widths independent of its member widths. Inlined at every
// call, the instantiations reuse frame slots in hero::save. Dreamcast's
// per-width locals and nested scopes support short staging lifetimes, but
// do not distinguish a template from other original source spellings.
template <class T>
int writeValue(TAbstractFile* outfile, T value)
{
    return outfile->write(&value, sizeof(value));
}

// A native range preserves the actual byte count and the caller's guards.
// Deducing Count retains each serialized count's signed source type: char for
// black markets and short for vectors. This interface is inferred; it adds no
// stream virtual or special case.
template <class T, class Count>
int readValues(TAbstractFile* infile, T* values, Count count)
{
    return infile->read(values, count * sizeof(T));
}

template <class T>
int readValue(TAbstractFile* infile, T& value)
{
    return infile->read(&value, sizeof(value));
}

template <class T>
T readValue(TAbstractFile* infile)
{
    T value;
    readValue(infile, value);
    return value;
}

#endif  /* HOMM3_ABSTRACTFILE_H */
