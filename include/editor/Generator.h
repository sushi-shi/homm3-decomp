// Generator.h - the Armageddon's Blade random dwellings (Generator.cpp,
// name inferred: the unit that installs them).
//
// Each derives from TFlaggableObject first, then from its random parts;
// the parts carry vtables, so VC6 places them at the front and the
// flaggable part, whose vbptr the class shares, after them (RTTI:
// TFlaggableObject at +16 or +32).
//
// Each returns its flaggable part to the random parts' pure virtuals; the
// name tables are Generator.cpp's.
#ifndef HOMM3_EDITOR_GENERATOR_H
#define HOMM3_EDITOR_GENERATOR_H

#include "editor/ObjectSpecializations.h"

// A dwelling of a fixed level whose alignment is random.
class TRandomlyAlignedGenerator : public TFlaggableObject, public TAbstractRandomlyAlignedGenerator {
public:
    // One name per level (the palette's tooltip), formatted from the
    // editor's random dwelling string at start-up.
    struct TTypeTraits {
        const char* m_name;
    };

    enum { s_kNumTypes = TAbstractRandomlyLeveledGenerator::s_kNumLevels };

    static const TTypeTraits* s_akTypeTraits;

    static void initializeTypeTraitsTable();

    TRandomlyAlignedGenerator(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TRandomlyAlignedGenerator(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual std::string getTypeName() const { return s_akTypeTraits[getExtra()].m_name; }

    virtual TFlaggableObject* getPFlaggableObject();
    virtual const TFlaggableObject* getPFlaggableObject() const;
};

// A dwelling of a fixed alignment whose level is random.
class TRandomlyLeveledGenerator : public TFlaggableObject, public TAbstractRandomlyLeveledGenerator {
public:
    // One name per town type (the palette's tooltip), formatted from the
    // editor's random town dwelling string at start-up.
    struct TTypeTraits {
        const char* m_name;
    };

    static const TTypeTraits* s_akTypeTraits;

    static void initializeTypeTraitsTable();

    TRandomlyLeveledGenerator(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TRandomlyLeveledGenerator(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual std::string getTypeName() const { return s_akTypeTraits[getExtra()].m_name; }

    virtual TFlaggableObject* getPFlaggableObject();
    virtual const TFlaggableObject* getPFlaggableObject() const;
};

// A dwelling whose alignment and level are both random.
class TRandomGenerator : public TFlaggableObject, public TAbstractRandomlyAlignedGenerator,
                         public TAbstractRandomlyLeveledGenerator {
public:
    TRandomGenerator(const TObjectType& objType, TPlayer owner = ePlayerNone);
    TRandomGenerator(const TObjectType& objType, TRawIStream* pIStream, int version);

    virtual void write(TRawOStream* pOStream, int version) const;
    virtual bool isCustomized() const;

    virtual TFlaggableObject* getPFlaggableObject();
    virtual const TFlaggableObject* getPFlaggableObject() const;
};

#endif  /* HOMM3_EDITOR_GENERATOR_H */
