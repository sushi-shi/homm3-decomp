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

// Unchecked native scalar reads. SavedGameHeader::load (0x4bc750) expands
// this operation six times: value lifetimes and cleanup edges match retail
// through one ordinary value reader. Its name and free-function binding are
// inferred; no standalone retail address or Dreamcast declaration is claimed.
template <class T>
T readValue(TAbstractFile* infile)
{
    T value;
    infile->read(&value, sizeof(value));
    return value;
}

#endif  /* HOMM3_ABSTRACTFILE_H */
