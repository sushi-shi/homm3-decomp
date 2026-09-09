// Native semantics only, not an x86 ABI fixture. The test supplies the actual
// enum, record, game array, state declarations, entry and dispatcher arm.
// @RECORD@
struct hero {};
struct NewmapCell { unsigned m_extraInfo; };
struct game {
    // @MEMBER@
    std::vector<TBlackMarket> m_blackMarkets;
};
static game* g_game;
// @GLOBALS@
static TArtifact* expectedArtifacts;
static hero* expectedHero;
static TBlackMarket* expectedRecord;
static int modalCalls, aiCalls, changedSlot;
static bool valid;

void doMarket() {
    ++modalCalls;
    if (g_marketArtifacts != expectedArtifacts || g_marketHero != expectedHero
        || g_marketCount != 5 || g_marketWindow != 2 || g_marketSource != 2) {
        valid = false;
        return; // Do not dereference deliberately bad negative-control pointers.
    }
    g_marketArtifacts[changedSlot] = ARTIFACT_NONE;
}

void aiVisitBlackMarket(hero* currentHero, TBlackMarket* record) {
    ++aiCalls;
    if (currentHero != expectedHero || record != expectedRecord)
        valid = false;
}

// @ENTRY@

void dispatch(hero* currentHero, NewmapCell* cell, bool humanPlayer) {
    enum { BLACK_MARKET };
    switch (BLACK_MARKET) {
        // @ARM@
    }
}

bool check() {
    const TArtifact seed[7] = { ARTIFACT_HOLY_GRAIL, ARTIFACT_SPELL_SCROLL,
        ARTIFACT_CATAPULT, ARTIFACT_BALLISTA, ARTIFACT_AMMO_CART,
        ARTIFACT_FIRST_AID_TENT, ARTIFACT_LEGS_OF_LEGION };
    // Four separately owned map records and the game's seven-slot array.
    // Exercise every slot, both entry paths, null/non-null hero forwarding,
    // and the AI arm which must receive the whole record without opening UI.
    for (int owner = 0; owner < 5; ++owner)
    for (changedSlot = 0; changedSlot < 7; ++changedSlot)
    for (int actor = 0; actor < 2; ++actor)
    for (int human = 0; human < 2; ++human) {
        game state;
        state.m_blackMarkets.resize(4);
        for (int slot = 0; slot < 7; ++slot) {
            state.m_marketArtifacts[slot] = seed[slot];
            for (int row = 0; row < 4; ++row)
                state.m_blackMarkets[row].m_artifacts[slot] = seed[(slot + row + 1) % 7];
        }
        hero player;
        g_game = &state;
        expectedHero = actor ? &player : 0;
        expectedRecord = owner < 4 ? &state.m_blackMarkets[owner] : 0;
        expectedArtifacts = expectedRecord ? expectedRecord->m_artifacts : state.m_marketArtifacts;
        g_marketArtifacts = 0;
        g_marketHero = 0;
        g_marketCount = g_marketWindow = g_marketSource = -77;
        modalCalls = aiCalls = 0;
        valid = true;
        const bool opensMarket = owner == 4 || human;
        if (owner == 4)
            doBlackMarket(expectedHero, state.m_marketArtifacts);
        else {
            NewmapCell cell;
            cell.m_extraInfo = owner;
            dispatch(expectedHero, &cell, human != 0);
        }
        if (!valid || modalCalls != int(opensMarket) || aiCalls != int(!opensMarket))
            return false;
        if (!opensMarket && (g_marketArtifacts || g_marketHero
            || g_marketCount != -77 || g_marketWindow != -77 || g_marketSource != -77))
            return false;
        for (int slot = 0; slot < 7; ++slot) {
            TArtifact wanted = owner == 4 && slot == changedSlot ? ARTIFACT_NONE : seed[slot];
            if (state.m_marketArtifacts[slot] != wanted)
                return false;
            for (int row = 0; row < 4; ++row) {
                wanted = opensMarket && row == owner && slot == changedSlot
                    ? ARTIFACT_NONE : seed[(slot + row + 1) % 7];
                if (state.m_blackMarkets[row].m_artifacts[slot] != wanted)
                    return false;
            }
        }
    }
    return true;
}
