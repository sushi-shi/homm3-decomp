// abstractfile.h - the common three-slot game stream interface.
#ifndef HOMM3_ABSTRACTFILE_H
#define HOMM3_ABSTRACTFILE_H

// Retail's vtable at 0x63dac0 names slot 0 as the deleting destructor;
// virtual calls use slot 1 to read and slot 2 to write (this in ECX,
// (buffer, size) on the stack). Keep that three-slot ABI canonical for every
// consumer rather than substituting an opaque pure-virtual placeholder.
class TAbstractFile {
public:
    virtual ~TAbstractFile() {}
    virtual int Read(void* data, int size) = 0;
    virtual int Write(const void* data, int size) = 0;
};

#endif  /* HOMM3_ABSTRACTFILE_H */
