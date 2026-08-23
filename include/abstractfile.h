// abstractfile.h - the common three-slot game stream interface.
#ifndef HOMM3_ABSTRACTFILE_H
#define HOMM3_ABSTRACTFILE_H

// Retail virtual-calls slot 1 to read and slot 2 to write (this in ECX,
// (buffer, size) on the stack). Most consumers only need an opaque slot-zero
// view; owners whose compiler-generated derived destructors are under study
// opt into the real virtual-destructor declaration before including this file.
class TAbstractFile {
public:
#ifdef HOMM3_TABSTRACTFILE_VIRTUAL_DTOR_VIEW
    virtual ~TAbstractFile() {}
#else
    virtual void _vslot0() = 0;
#endif
    virtual int Read(void* data, int size) = 0;
    virtual int Write(const void* data, int size) = 0;
};

#endif  /* HOMM3_ABSTRACTFILE_H */
