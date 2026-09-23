// Shared constructor bodies included inside the canonical type_artifact
// declaration and the reviewed Mac declaration view. DC attribution: Hero.h.
    // DC Hero.h:211 stores the artifact argument at +0, then line 212 stores
    // the -1 sentinel at +4. Retail value_of_town preserves that order in
    // its register allocation even though the eventual by-value pushes are
    // ordered by record layout.
    // The generated offering constructor at dc 0x128714 calls this
    // constructor with -1. That proves the default argument: no separate
    // zero-argument type_artifact constructor exists in the DC class API.
    explicit type_artifact(TArtifact id = ARTIFACT_NONE)
    {
        m_artifactId = id;
        m_extra = -1;
    }
    // Dreamcast Hero.h:214-218. A spell scroll is represented by artifact
    // id 1 and its SpellID payload; this semantic constructor is distinct
    // from the generic TArtifact constructor above.
    explicit type_artifact(SpellID spell)
    {
        m_artifactId = ARTIFACT_SPELL_SCROLL;
        m_extra = spell;
    }
