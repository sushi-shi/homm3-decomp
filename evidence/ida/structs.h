/* IDB structs view (members in declared order; python-idb exposes no offsets). HD-pressing reference. */

struct _SCOPETABLE_ENTRY {
    ?                        EnclosingLevel;
    ?                        FilterFunc;
    ?                        HandlerFunc;
};

struct CPPEH_RECORD {
    #D                       old_esp;
    ?                        exc_ptr;
    ?                        registration;
};

struct _EH3_EXCEPTION_REGISTRATION {
    ?                        Next;
    PVOID                    ExceptionHandler;
    #E                       ScopeTable;
    #D                       TryLevel;
};

struct tagPOINT {
    #K                       x;
    #K                       y;
};

struct tagRECT {
    #K                       left;
    #K                       top;
    #K                       right;
    #K                       bottom;
};

struct WNDCLASSA {
    #O                       style;
    #P                       lpfnWndProc;
    ?                        cbClsExtra;
    ?                        cbWndExtra;
    #X                       hInstance;
    #Z                       hIcon;
    #\                       hCursor;
    #]                       hbrBackground;
    #_                       lpszMenuName;
    #_                       lpszClassName;
};

struct tagMSG {
    #Q                       hwnd;
    #O                       message;
    #S                       wParam;
    #U                       lParam;
    #D                       time;
    #b                       pt;
};

struct POINT {
    #K                       x;
    #K                       y;
};

struct RECT {
    #K                       left;
    #K                       top;
    #K                       right;
    #K                       bottom;
};

struct WSAData {
    #e                       wVersion;
    #e                       wHighVersion;
    ?                        szDescription;
    ?                        szSystemStatus;
    ?                        iMaxSockets;
    ?                        iMaxUdpDg;
    ?                        lpVendorInfo;
};

struct sockaddr {
    #g                       sa_family;
    ?                        sa_data;
};

struct _FILETIME {
    #D                       dwLowDateTime;
    #D                       dwHighDateTime;
};

struct _SYSTEMTIME {
    #e                       wYear;
    #e                       wMonth;
    #e                       wDayOfWeek;
    #e                       wDay;
    #e                       wHour;
    #e                       wMinute;
    #e                       wSecond;
    #e                       wMilliseconds;
};

struct tagPAINTSTRUCT {
    #l                       hdc;
    #n                       fErase;
    #c                       rcPaint;
    #n                       fRestore;
    #n                       fIncUpdate;
    ?                        rgbReserved;
};

struct in_addr {
    #q                       S_un;
};

struct in_addr::$D689D43D03D53F61DA021DB261182132 {
    #r                       S_un_b;
    #t                       S_un_w;
    #u                       S_addr;
};

struct in_addr::$D689D43D03D53F61DA021DB261182132::$F085A1F6735C7CEA9C650424FAF692B1 {
    #s                       s_b1;
    #s                       s_b2;
    #s                       s_b3;
    #s                       s_b4;
};

struct in_addr::$D689D43D03D53F61DA021DB261182132::$B9D7529FFD1842B2B059BD2E926FB2C5 {
    #h                       s_w1;
    #h                       s_w2;
};

struct _OFSTRUCT {
    #o                       cBytes;
    #o                       fFixedDisk;
    #e                       nErrCode;
    #e                       Reserved1;
    #e                       Reserved2;
    ?                        szPathName;
};

struct _RTL_CRITICAL_SECTION {
    #x                       DebugInfo;
    #K                       LockCount;
    #K                       RecursionCount;
    #|                       OwningThread;
    #|                       LockSemaphore;
    #}                       SpinCount;
};

struct _WIN32_FIND_DATAA {
    #D                       dwFileAttributes;
    #                       ftCreationTime;
    #                       ftLastAccessTime;
    #                       ftLastWriteTime;
    #D                       nFileSizeHigh;
    #D                       nFileSizeLow;
    #D                       dwReserved0;
    #D                       dwReserved1;
    ?                        cFileName;
    ?                        cAlternateFileName;
};

struct FILETIME {
    #D                       dwLowDateTime;
    #D                       dwHighDateTime;
};

struct _TIME_ZONE_INFORMATION {
    #K                       Bias;
    ?                        StandardName;
    ?                        StandardDate;
    #K                       StandardBias;
    ?                        DaylightName;
    ?                        DaylightDate;
    #K                       DaylightBias;
};

struct SYSTEMTIME {
    #e                       wYear;
    #e                       wMonth;
    #e                       wDayOfWeek;
    #e                       wDay;
    #e                       wHour;
    #e                       wMinute;
    #e                       wSecond;
    #e                       wMilliseconds;
};

struct _SECURITY_ATTRIBUTES {
    #D                       nLength;
    ?                        lpSecurityDescriptor;
    #n                       bInheritHandle;
};

struct _STARTUPINFOA {
    #D                       cb;
    ?                        lpReserved;
    ?                        lpDesktop;
    ?                        lpTitle;
    #D                       dwX;
    #D                       dwY;
    #D                       dwXSize;
    #D                       dwYSize;
    #D                       dwXCountChars;
    #D                       dwYCountChars;
    #D                       dwFillAttribute;
    #D                       dwFlags;
    #e                       wShowWindow;
    #e                       cbReserved2;
    ?                        lpReserved2;
    #|                       hStdInput;
    #|                       hStdOutput;
    #|                       hStdError;
};

struct _cpinfo {
    #O                       MaxCharSize;
    ?                        DefaultChar;
    ?                        LeadByte;
};

struct _OSVERSIONINFOA {
    #D                       dwOSVersionInfoSize;
    #D                       dwMajorVersion;
    #D                       dwMinorVersion;
    #D                       dwBuildNumber;
    #D                       dwPlatformId;
    ?                        szCSDVersion;
};

struct FuncInfoV1 {
    ?                        magicNumber;
    ?                        maxState;
    ?                        pUnwindMap;
    ?                        nTryBlocks;
    ?                        pTryBlockMap;
    ?                        nIPMapEntries;
    ?                        pIPtoStateMap;
};

struct UnwindMapEntry {
    ?                        toState;
    ?                        action;
};

struct TryBlockMapEntry {
    ?                        tryLow;
    ?                        tryHigh;
    ?                        catchHigh;
    ?                        nCatches;
    ?                        pHandlerArray;
};

struct HandlerType {
    ?                        adjectives;
    ?                        pType;
    ?                        dispCatchObj;
    ?                        addressOfHandler;
};

struct TypeDescriptor {
    ?                        hash;
    ?                        spare;
    ?                        name;
};

struct FILE {
    ?                        _ptr;
    ?                        _cnt;
    ?                        _base;
    ?                        _flag;
    ?                        _file;
    ?                        _charbuf;
    ?                        _bufsiz;
    ?                        _tmpfname;
};

struct IID {
    ?                        Data1;
    ?                        Data2;
    ?                        Data3;
    ?                        Data4;
};

struct HWND__ {
    ?                        unused;
};

struct TCreatureTypeTraits {
    ?                        townType;
    ?                        level;
    ?                        cSamplePrefix;
    ?                        m_sprite_name;
    ?                        attributes;
    ?                        m_name;
    ?                        m_plural_name;
    ?                        special_ability;
    ?                        cost;
    ?                        baseFightValue;
    ?                        AI_value;
    ?                        growthRate;
    ?                        horde_growth_rate;
    ?                        hitPoints;
    ?                        speed;
    ?                        attackSkill;
    ?                        defenseSkill;
    ?                        damageLowBound;
    ?                        damageHighBound;
    ?                        numShots;
    ?                        hasSpell;
    ?                        wanderingLow;
    ?                        wanderingHigh;
};

struct SMonFrameInfo {
    ?                        iMissileOffset;
    ?                        fArrowAngle;
    ?                        iExtraNumTroopsXOffset;
    ?                        iAttackFrames;
    ?                        iFidgetFrequency;
    ?                        iWalkCycleTime;
    ?                        iAttackStartCycleTime;
    ?                        iFlightPixelSpan;
};

struct std::deque_SpellID_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        map;
    ?                        mapsize;
    ?                        size;
};

struct std::deque_SpellID_::iterator {
    ?                        first;
    ?                        last;
    ?                        next;
    ?                        map;
};

struct std::vector {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct resource {
    ?                        vftable;
    ?                        Name;
    ?                        resType;
    ?                        ReferenceCount;
};

struct sample {
    ?                        baseclass_0;
    ?                        memSample;
};

struct CSprite {
    ?                        baseclass_0;
    ?                        s;
    ?                        p16;
    ?                        p24;
    ?                        numSequences;
    ?                        validSeqMask;
    ?                        Width;
    ?                        Height;
};

struct searchArray {
    ?                        maxQueueCount;
    ?                        pay_transition_costs;
    ?                        this_turns_movement;
    ?                        land_movement;
    ?                        sea_movement;
    ?                        can_summon_boat;
    ?                        can_cast_teleport;
    ?                        can_cast_flight;
    ?                        can_cast_water_walk;
    ?                        water_walk_level;
    ?                        flight_level;
    ?                        limit_reached;
    ?                        cellData;
    #L                       valid_rectangle;
    ?                        queue;
    ?                        result;
    ?                        visited_points;
    ?                        bIsMoatSlowed;
    ?                        danger_zones;
};

struct type_point {
    ?                        X;
    ?                        YZ;
};

struct pathCell {
    ?                        baseclass_0;
    ?                        _bf_4;
    ?                        _bf_5;
    ?                        _bf_6;
    ?                        _bf_7;
    ?                        last_point;
    ?                        monster;
    ?                        barrier_value;
    ?                        danger_value;
    ?                        cost;
    ?                        adjusted_cost;
    ?                        move_left;
};

struct hero {
    ?                        baseclass_0;
    ?                        mana;
    ?                        id;
    ?                        order;
    ?                        playerOwner;
    ?                        name;
    ?                        hero_class;
    ?                        portrait;
    ?                        targetX;
    ?                        targetY;
    ?                        targetZ;
    ?                        last_magic_school_level;
    ?                        target_distance;
    ?                        target_is_critical;
    ?                        patrolX;
    ?                        patrolY;
    ?                        patrolRadius;
    ?                        facing;
    ?                        formation;
    ?                        maxMobility;
    ?                        currMobility;
    ?                        experience;
    ?                        Level;
    ?                        LearningStoneFlags;
    ?                        DefenseTowerFlags;
    ?                        GardenOfRevelationFlags;
    ?                        MercCampFlags;
    ?                        StarAxisFlags;
    ?                        TreeOfKnowledgeFlags;
    ?                        LibraryFlags;
    ?                        ArenaFlags;
    ?                        MagicSchoolFlags;
    ?                        WarSchoolFlags;
    ?                        UniversityFlags;
    ?                        Shrine1Flags;
    ?                        Shrine2Flags;
    ?                        Shrine3Flags;
    ?                        iLevelSeed;
    ?                        lastWisdom;
    ?                        heroArmy;
    ?                        SSLevel;
    ?                        SSOrder;
    ?                        numSSs;
    ?                        flags;
    ?                        turnExperienceToRVRatio;
    ?                        dWalkSpellsCast;
    ?                        disguiseLevel;
    ?                        flightLevel;
    ?                        waterWalkPower;
    ?                        moraleBonus;
    ?                        luckBonus;
    ?                        IsSleeping;
    ?                        bounty;
    ?                        TownSpecialGrantedMask;
    ?                        visionsPower;
    ?                        equipped;
    ?                        blockedSlots;
    ?                        backpack;
    ?                        backpack_count;
    ?                        sex;
    ?                        bio_customized;
    ?                        bio;
    ?                        in_spellbook;
    ?                        available_spells;
    ?                        stats;
    ?                        aggression;
    ?                        value_of_power;
    ?                        value_of_duration;
    ?                        value_of_knowledge;
    ?                        value_of_spring;
    ?                        value_of_well;
};

struct type_obscuring_object {
    ?                        mapX;
    ?                        mapY;
    ?                        mapZ;
    ?                        valid;
    ?                        obscured_location;
    byte                     aligned1;
    ?                        type;
    ?                        was_trigger;
    ?                        aligned2;
    ?                        extra_info;
};

struct armyGroup {
    ?                        type;
    ?                        amount;
};

struct std::bitset_48_ {
    ?                        bitset_array;
};

struct type_artifact {
    ?                        type;
    ?                        spell;
};

struct std::string {
    ?                        allocator;
    ?                        c_str;
    ?                        length;
    ?                        capacity;
};

struct TArtifactTraits {
    ?                        m_name;
    ?                        m_cost;
    ?                        m_allowableSlotMask;
    ?                        m_class;
    ?                        m_description;
    ?                        m_comboType;
    ?                        m_targetCombo;
    ?                        m_disabled;
    ?                        m_givesSpells;
};

struct TNormalDialogInfo {
    ?                        dialog_text;
    ?                        x;
    ?                        y;
    ?                        width;
    ?                        height;
    ?                        text_widget_x;
    ?                        text_widget_y;
    ?                        text_widget_width;
    ?                        text_widget_height;
    ?                        text_expansion;
    ?                        icons;
    ?                        iMBType;
    ?                        iSpecial;
    ?                        timeout;
};

struct baseManager {
    ?                        vftable;
    ?                        nextManager;
    ?                        prevManager;
    ?                        id;
    ?                        priority;
    ?                        cMgrName;
    ?                        status;
};

struct hexcell {
    ?                        refX;
    ?                        refY;
    ?                        hexULX;
    ?                        hexULY;
    ?                        hexBRX;
    ?                        hexBRY;
    ?                        fullHexBRY;
    ?                        attributes;
    ?                        obstacleIndex;
    ?                        armyGrp;
    ?                        armyIndex;
    ?                        partOfDouble;
    ?                        iBodiesInHex;
    ?                        deadArmyGrp;
    ?                        deadArmyIndex;
    ?                        deadPartOfDouble;
    ?                        bValidMove;
    ?                        front_move;
    ?                        mouse_shaded;
    ?                        background_offset;
    ?                        obstacleLimitData;
    ?                        cloudLimitData;
};

struct SLimitData {
    ?                        MinX;
    ?                        MinY;
    ?                        MaxX;
    ?                        MaxY;
};

struct TPalette24 {
    ?                        baseclass_0;
    ?                        Palette;  /* 384 / 8 players = 48 colors */
};

struct TPalette16 {
    ?                        baseclass_0;
    ?                        Palette;
};

struct heroWindow {
    ?                        vftable;
    ?                        priority;
    ?                        nextWindow;
    ?                        prevWindow;
    ?                        type;
    ?                        status;
    ?                        x;
    ?                        y;
    ?                        width;
    ?                        height;
    ?                        headWidget;
    ?                        tailWidget;
    ?                        Widgets;
    ?                        focusId;
    ?                        background;
    ?                        sleepCount;
};

struct TSubWindow {
    ?                        vftable;
    ?                        X;
    ?                        Y;
    ?                        Width;
    ?                        Height;
    ?                        Widgets;
    ?                        ParentWindow;
    ?                        FirstWidgetID;
    ?                        LastWidgetID;
    ?                        Background;
};

struct widget {
    ?                        vftable;
    ?                        parentWindow;
    ?                        prevWidget;
    ?                        nextWidget;
    ?                        id;
    ?                        priority;
    ?                        style;
    ?                        status;
    ?                        x;
    ?                        y;
    ?                        width;
    ?                        height;
    ?                        RollOver;
    ?                        RightClick;
    ?                        freeText;
    ?                        sleepCount;
};

struct TAdventureMapWindow {
    ?                        baseclass_0;
    ?                        RadarWidget;
    ?                        MapWidget;
    ?                        ChatTextWidget;
    ?                        chatEdit;
    ?                        ResourceDisplay;
    ?                        topHero;
    ?                        topTown;
    ?                        RolloverWidget;
    ?                        animate_in_background;
    ?                        hero_borders;
    ?                        hero_highlight_borders;
    ?                        bottom_view;
    ?                        immersion_ptr;
};

struct TTextResource {
    ?                        baseclass_0;
    ?                        Text;
    ?                        Data;
};

struct textWidget {
    ?                        baseclass_0;
    ?                        Text;
    ?                        Font;
    ?                        Color;
    ?                        BackColor;
    ?                        Justify;
};

struct advManager {
    ?                        baseclass_0;
    ?                        pNetMsgHandler;
    ?                        DebugShowFPS;
    ?                        DebugViewAll;
    ?                        advCommand;
    ?                        advWindow;
    ?                        pRouteArray;
    ?                        bShowRoute;
    ?                        seedingValid;
    ?                        fullySeeded;
    ?                        lastTerrain;
    ?                        map;
    ?                        groundTileset;
    ?                        riverTileset;
    ?                        roadTileset;
    ?                        borderTileset;
    ?                        arrowTileset;
    ?                        gemIcons;
    ?                        starTileset;
    ?                        radarIcons;
    ?                        cloudIcons;
    ?                        CachedGraphics;
    ?                        monAttackSprites;
    ?                        map_origin;
    ?                        last_map_hover;
    ?                        lastHoverX;
    ?                        lastHoverY;
    ?                        scrollX;
    ?                        scrollY;
    ?                        animFrame;
    ?                        animCtr;
    ?                        animCtrPaused;
    ?                        flagFrame;
    ?                        cursorIcons;
    ?                        boatIcons;
    ?                        boatFrothIcons;
    ?                        flagIcons;
    ?                        boatFlagIcons;
    ?                        heroVisible;
    ?                        heroType;
    ?                        heroDirection;
    ?                        heroBaseFrame;
    ?                        heroSequence;
    ?                        heroFrameCount;
    ?                        heroTurning;
    ?                        heroDrawn;
    ?                        bCurHeroMobile;
    ?                        iShowMode;
    ?                        bForceCompleteDraw;
    ?                        monAttackObjIndex;
    ?                        monAttackSpriteIndex;
    ?                        monAttackFlip;
    ?                        touchedSounds;
    ?                        soundArray;
    ?                        loopedSample;
    ?                        heroSamples;
    ?                        bHeroLogoShowing;
    ?                        bHeroMoving;
    ?                        CurrentBottomView;
    ?                        BottomViewOverride;
    ?                        BottomViewOverrideEndTime;
    ?                        BottomViewResource;
    ?                        BottomViewResourceQty;
    ?                        BottomViewText;
};

struct soundNode {
    ?                        soundID;
    ?                        priority;
};

struct executive {
    ?                        headManager;
    ?                        tailManager;
    ?                        currentManager;
    ?                        dialogReturn;
};

struct type_AI_player {
    ?                        team;
    ?                        magus_hut_value;
    ?                        reserved_funds;
    ?                        resource_supply;
    ?                        resource_demand;
    ?                        resource_value;
};

struct CNetMsgHandler {
    ?                        vftable;
    ?                        m_inPopup;
    ?                        m_pAbortPopupMsg;
};

struct CNetMsg {
    ?                        m_from;
    ?                        m_dpidFrom;
    ?                        m_subType;
    ?                        m_size;
    ?                        m_UncompressedSize;
};

struct playerData {
    ?                        color;
    ?                        numHeroes;
    ?                        align1;
    ?                        currHero;
    ?                        heroes;
    ?                        recruits;
    ?                        startingNumHeroes;
    ?                        align2;
    ?                        personality;
    ?                        extraPuzzlePieces;
    ?                        puzzle_guess;
    ?                        iDeathCountDown;
    ?                        numTowns;
    ?                        currTown;
    ?                        towns;
    ?                        placement_help_enabled;
    ?                        align3;
    ?                        shipyards;
    ?                        resources;
    ?                        MysticalGardenFlags;
    ?                        MagicSpringFlags;
    ?                        DeadGuyFlags;
    ?                        LeanToFlags;
    ?                        dpid;
    ?                        cName;
    ?                        isLocal;
    ?                        isHuman;
    ?                        align4;
    ?                        quickCombat;
    ?                        builtArtifacts;
    ?                        ai;
};

struct AI {
    ?                        turnExpectedResource;
    ?                        turnProductionResource;
    ?                        pad_38;
    ?                        resource_value;
    ?                        average_resource_value;
    ?                        turnValueOfAvgArtifact;
};

struct std::bitset_70_ {
    ?                        bitset_array;
};

struct town {
    ?                        id;
    ?                        playerOwner;
    ?                        builtThisTurn;
    ?                        threatening_heroes;
    ?                        townType;
    ?                        mapX;
    ?                        mapY;
    ?                        mapZ;
    ?                        boatX;
    ?                        boatY;
    ?                        garrisonHero;
    ?                        occupyingHero;
    ?                        mageLevel;
    ?                        population;
    ?                        bIsGrouped;
    ?                        ManaVortexFull;
    ?                        pond_amount;
    ?                        pond_resource;
    ?                        summoningType;
    ?                        summoningPopulation;
    ?                        townSpells;
    ?                        maxTownSpellAvailable;
    ?                        cName;
    ?                        SpellDisabledMask;
    ?                        town_army;
    ?                        generator_bonus;
    ?                        populationMask;
    ?                        full_building_mask;
    ?                        legal_buildings;
};

struct CSpriteFrame {
    ?                        baseclass_0;
    ?                        DataSize;
    ?                        ImageSize;
    ?                        EncodingMethod;
    ?                        Width;
    ?                        Height;
    ?                        CroppedWidth;
    ?                        CroppedHeight;
    ?                        CroppedX;
    ?                        CroppedY;
    ?                        Pitch;
    ?                        map;
};

struct VictoryConditionStruct {
    ?                        Type;
    ?                        AllowNormalVictory;
    ?                        AppliesToComputer;
    ?                        ArtifactNum;
    ?                        CreatureType;
    ?                        NumCreatures;
    ?                        ResourceType;
    ?                        NumResources;
    ?                        TownX;
    ?                        TownY;
    ?                        TownZ;
    ?                        HallLevel;
    ?                        CastleLevel;
    ?                        HeroX;
    ?                        HeroY;
    ?                        HeroZ;
    ?                        HeroID;
    ?                        MonsterX;
    ?                        MonsterY;
    ?                        MonsterZ;
    ?                        time_to_survive;
    ?                        GameWon;
    ?                        playerWinner;
};

struct LossConditionStruct {
    ?                        Type;
    ?                        TownX;
    ?                        TownY;
    ?                        TownZ;
    ?                        HeroX;
    ?                        HeroY;
    ?                        HeroZ;
    ?                        HeroID;
    ?                        NumDays;
    ?                        GameLost;
    ?                        playerLoser;
};

struct type_AI_combat_data {
    ?                        creatures;
    ?                        magic_terrain;
    ?                        mana;
    ?                        can_cast_spells;
    ?                        total_combat_value;
    ?                        tactics_advantage;
    ?                        current_hero;
    ?                        current_army;
    ?                        enemy_hero;
    ?                        wall_archery_penalty;
    ?                        wall_speed_limit;
};

struct std::vector_type_monster_data_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct type_monster_data {
    ?                        index;
    ?                        type;
    ?                        number;
    ?                        original_number;
    ?                        speed;
    ?                        melee_modifier;
    ?                        final_melee_modifier;
    ?                        ranged_modifier;
    ?                        combat_value_per_hit;
    ?                        category;
    ?                        value;
    ?                        total_value;
};

struct type_AI_combat_parameters {
    ?                        lowest_attack;
    ?                        lowest_defense;
    ?                        kills_only;
    ?                        simulated;
    ?                        friendly_combat_value;
    ?                        enemy_combat_value;
    ?                        awake_friendly_value;
    ?                        awake_enemy_value;
    ?                        rounds_left;
    ?                        our_group;
    ?                        enemy_group;
};

struct type_AI_spellcaster {
    ?                        vftable;
    ?                        current_hero;
    ?                        enemy_hero;
    ?                        our_group;
    ?                        enemy_group;
    ?                        enemy_can_attack;
    ?                        can_be_attacked;
    ?                        win_likely;
    ?                        is_creature_spell;
    ?                        estimate;
    ?                        enemy_caster;
    ?                        owns_enemy_caster;
    ?                        melee_enemies;
    ?                        ranged_enemies;
    ?                        worst_enemies;
};

struct type_AI_enemy_data {
    ?                        enemy;
    ?                        damage;
    ?                        count;
    ?                        total_damage;
};

struct game {
    ?                        newGameWin;
    ?                        spellAllocInfo;
    ?                        spellDisabledInfo;
    LPCRITICAL_SECTION       bink_critical_section;
    ?                        townExtraPool;
    ?                        heroExtraPool;
    ?                        difficultyRating;
    ?                        sCampaign;
    ?                        bNewCampaignStarted;
    ?                        cGameFilename;
    ?                        numPlayers;
    ?                        numDeadPlayers;
    ?                        playerDead;
    ?                        day;
    ?                        week;
    ?                        month;
    ?                        cUniqueSystemID;
    ?                        marketArtifacts;
    ?                        BlackMarkets;
    ?                        ultimateX;
    ?                        ultimateY;
    ?                        ultimateZ;
    ?                        ultimateRadius;
    ?                        ultimateValid;
    ?                        align8;
    ?                        iGameType;
    ?                        bIsCheater;
    ?                        is_tutorial;
    ?                        align9;
    ?                        sSetup;
    ?                        sMapHeader;
    ?                        worldMap;
    ?                        align10;
    ?                        player;
    ?                        townPool;
    ?                        heroPool;
    ?                        heroAllocInfo;
    ?                        heroAvailable;
    ?                        artifactAllocInfo;
    ?                        reservedArtifactInfo;
    ?                        InfoFlags;
    ?                        GuardFlags;
    ?                        cartographerMask;
    ?                        cartographerFlags;
    ?                        aligned11;
    ?                        signPool;
    ?                        minePool;
    ?                        generatorPool;
    ?                        garrisonPool;
    ?                        boatPool;
    ?                        university_pool;
    ?                        creature_banks;
    ?                        numObelisks;
    ?                        obeliskPool;
    ?                        cCurRumour;
    ?                        aligned12;
    ?                        rumourAllocInfo;
    ?                        aligned13;
    ?                        MapRumours;
    ?                        ss_disabled;
    ?                        armyWindow;
    ?                        aligned14;
    ?                        two_way_liths;
    ?                        lith_exits;
    ?                        whirlpools;
    ?                        underground_gates;
    ?                        underground_gate_exits;
    ?                        recorded_events;
    ?                        quest_monsters;
    ?                        aligned15;
};

struct HeroExtra {
    ?                        Owner;
    ?                        pad_2;
    ?                        id;
    ?                        objRef;
    ?                        bCustomName;
    ?                        Name;
    ?                        customExperience;
    ?                        pad_1B;
    ?                        Experience;
    ?                        bCustomPortraitNumber;
    ?                        PortraitNumber;
    ?                        bCustomSecondarySkills;
    ?                        pad_23;
    ?                        NumSecondarySkills;
    ?                        secondarySkill;
    ?                        secondarySkillLevel;
    ?                        bCustomArmies;
    ?                        pad_39;
    ?                        armies;
    ?                        numTroops;
    ?                        GroupFormation;
    ?                        bCustomArtifacts;
    ?                        artifacts;
    ?                        backpack;
    ?                        numInBackpack;
    ?                        location;
    ?                        PatrolRadius;
    ?                        bCustomBiography;
    ?                        pad_307;
    ?                        sBiography;
    ?                        sex;
    ?                        bCustomSpells;
    ?                        pad_31D;
    ?                        customSpells;
    ?                        bCustomPrimarySkills;
    ?                        primarySkills;
    ?                        aligned5;
};

struct SCampaign {
    ?                        bIsCheater;
    ?                        bSecretActive;
    ?                        iCurMap;
    ?                        align1;
    ?                        iCurrentCampaign;
    ?                        NumMapRegions;
    ?                        iCrossoverArrayIndex;
    ?                        briefing_choice;
    ?                        CampaignFilename;
    ?                        bCampaignCompleted;
    ?                        align2;
    ?                        carryover_pool;
    ?                        carryover_artifact;
    ?                        scenarios;
    ?                        assigned_carryover;
};

struct SGameSetupOptions {
    ?                        color;
    ?                        handicap;
    ?                        alignment;
    ?                        playerPos;
    ?                        difficulty;
    ?                        cFilename;
    ?                        cPath;
    ?                        canFlipFromToComputer;
    ?                        curSelectedPlayer;
    ?                        bThisFileInitialized;
    ?                        initializationNumHumans;
    ?                        turnDuration;
    ?                        startingHero;
    ?                        startingBonus;
};

struct NewSMapHeader {
    ?                        baseclass_0;
    ?                        heroPlayerSetups;
    ?                        mapName;
    ?                        mapDescription;
    ?                        availableHeroes;
};

struct CMapHeaderData {
    ?                        iVersion;
    ?                        IsPlayable;
    ?                        iDifficulty;
    ?                        numPlayers;
    ?                        minNumHumanPlayers;
    ?                        maxNumHumanPlayers;
    ?                        lastTownNameAssigned;
    ?                        mapHasNotBeenSaved;
    ?                        max_hero_level;
    ?                        numTeams;
    ?                        teamInfo;
    ?                        Size;
    ?                        HasTwoLayers;
    ?                        placeholders;
    ?                        victory_condition;
    ?                        loss_condition;
    ?                        PlayerSlotAttributes;
};

struct CMapHeaderData::TPlayerSlotAttributes {
    ?                        CanBeHuman;
    ?                        CanBeComputer;
    ?                        AIStrategy;
    ?                        legal_alignments;
    ?                        HasRandomAlignment;
    ?                        GenerateHero;
    ?                        has_main_town;
    ?                        main_town_type;
    ?                        CastleLoc;
    ?                        hasRandomHero;
    ?                        nonRandomHeroId;
    ?                        nonRandomHeroCustomPortrait;
    ?                        nonRandomHeroCustomName;
    ?                        default_placeholders;
    ?                        player_heroes;
};

struct std::map_int_HeroPlayerInfo_ {
    ?                        allocator;
    ?                        key_compare;
    ?                        _Head;
    ?                        _Multi;
    size_t                   _Size;
};

struct std::bitset_156_ {
    ?                        bitset_array;
};

struct NewfullMap {
    ?                        ObjectTypes;
    ?                        Objects;
    ?                        Sprites;
    ?                        CustomTreasureList;
    ?                        CustomMonsterList;
    ?                        BlackBoxList;
    ?                        SeerHutList;
    ?                        QuestGuardList;
    ?                        TimedEventList;
    ?                        TownEventList;
    ?                        PlaceHolderList;
    ?                        QuestList;
    ?                        RandomDwellingList;
    ?                        cellData;
    ?                        Size;
    ?                        HasTwoLevels;
    ?                        ObjectTypeTables;
};

struct type_town_threat_checker {
    ?                        vftable;
    ?                        current_player_id;
};

struct iconWidget {
    ?                        baseclass_0;
    ?                        Sprite;
    ?                        Frame;
    ?                        seqId;
    ?                        IsFlipped;
    ?                        PostPostWalkSequence;
    ?                        BackColor;
};

struct TTownScreenWindow {
    ?                        baseclass_0;
    ?                        topTown;
    ?                        zBuffer;
    ?                        growth_bonus_icon;
    ?                        growth_bonus_text;
    ?                        bonus_creatures;
};

struct TResourceDisplay {
    ?                        baseclass_0;
    ?                        IsSmall;
    ?                        ResourceWidgets;
    ?                        ResourceIconWidgets;
    ?                        BackgroundWidget;
    ?                        DayWidget;
};

struct townManager {
    ?                        baseclass_0;
    ?                        currTown;
    ?                        panorama;
    ?                        MonPix;
    ?                        objects;
    ?                        numObjects;
    ?                        loadedTownType;
    ?                        unused;
    ?                        townWindow;
    ?                        garrisonStrip;
    ?                        heroStrip;
    ?                        currStrip;
    ?                        currIndex;
    ?                        srcStrip;
    ?                        srcIndex;
    ?                        destStrip;
    ?                        destIndex;
    ?                        townBank;
    ?                        townPopupBank;
    ?                        townText;
    ?                        lastHover;
    ?                        lastQualifier;
    ?                        command;
    ?                        canBuyMask;
    ?                        canBuildMask;
    ?                        pNetMsgHandler;
    ?                        pNetMsgHandlerSave;
    ?                        objToBuild;
    ?                        dialogWindow;
    ?                        multiWin;
    ?                        divideStatus;
    ?                        recruitSelected;
    ?                        currentDwellingIDOff;
    ?                        align;
};

struct message {
    ?                        command;
    ?                        subType;
    ?                        itemId;
    ?                        qualifier;
    ?                        mouseX;
    ?                        mouseY;
    ?                        extra;
    ?                        window;
};

struct NewmapCell {
    ?                        baseclass_0;
    ?                        GroundSet;
    ?                        GroundIndex;
    ?                        RiverSet;
    ?                        RiverIndex;
    ?                        RoadSet;
    ?                        RoadIndex;
    ?                        align1;
    ?                        _bf_c;
    ?                        ObjectCellList;
    ?                        type;
    ?                        objectIndex;
    ?                        object_type_index;
};

struct ExtraInfoUnion {
    ?                        anonymous_0;
};

struct ExtraInfoUnion::$78D74CA2AC055E83C754EB0E8B29499D {
    ?                        extraInfo;
    ?                        Artifact;
    ?                        BlackMarket;
    ?                        Boat;
    ?                        campfire;
    ?                        corpse;
    ?                        creatureBank;
    ?                        event;
    ?                        flotsam;
    ?                        fountainFortune;
    ?                        garrison;
    ?                        generator;
    ?                        hero;
    ?                        leanTo;
    ?                        learningStone;
    ?                        lighthouse;
    ?                        magicShrine;
    ?                        magicSpring;
    ?                        mine;
    ?                        monolith;
    ?                        wanderingCreature;
    ?                        mysticGarden;
    ?                        obelisk;
    ?                        oceanBottle;
    ?                        pandorasBox;
    ?                        prison;
    ?                        pyramid;
    ?                        questGuard;
    ?                        refugeeCamp;
    ?                        resource;
    ?                        scholar;
    ?                        spellScroll;
    ?                        seaChest;
    ?                        seerHut;
    ?                        shipwreckSurvivor;
    ?                        shipyard;
    ?                        signPost;
    ?                        town;
    ?                        treasureChest;
    ?                        treeKnowledge;
    ?                        university;
    ?                        wagon;
    ?                        warriorsTomb;
    ?                        watermill;
    ?                        windmill;
    ?                        witchHut;
};

struct mapCellArtifact {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellDefaultObject {
    ?                        id;
};

struct mapCellCampfire {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellCorpse {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellCreatureBank {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellEvent {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellFlotsam {
    ?                        type;
};

struct mapCellFountainFortune {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellLeanTo {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellMagicShrine {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellMagicSpring {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellMonster {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellMysticGarden {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellPandorasBox {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellPyramid {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellRefugeeCamp {
    ?                        amount;
};

struct mapCellResource {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellScholar {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellScroll {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellSeaChest {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellShipwreckSurvivor {
    ?                        artifact;
};

struct mapCellShipyard {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellTreasureChest {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellTreeOfKnowledge {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellUniversity {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellWagon {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellWarriorsTomb {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellWaterMill {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellWindMill {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct mapCellWitchHut {
    ?                        _bf_0;
    ?                        _bf_1;
    ?                        _bf_2;
    ?                        _bf_3;
};

struct heroWindowManager {
    ?                        baseclass_0;
    ?                        dialogReturn;
    ?                        lastHover;
    ?                        screenBitmap;
    ?                        colorCyclingOn;
    ?                        isWaitingForFadeIn;
    ?                        bmpFizzleSource;
    ?                        activeWindow;
    ?                        lastActive;
    ?                        headWindow;
    ?                        tailWindow;
};

struct HeroDestination {
    ?                        value;
    ?                        move_cost;
    ?                        is_nearby;
    ?                        is_critical;
};

struct type_horde_effect {
    ?                        creature;
    ?                        bonus;
    ?                        dwelling;
};

struct swapManager {
    ?                        baseclass_0;
    ?                        parent;
    ?                        border;
    ?                        heroes;
    ?                        heroDonor;
    ?                        heroReciever;
    ?                        sourceSlot;
    ?                        destSlot;
    ?                        slotClicked;
    byte                     align1;
    ?                        samePlayer;
    ?                        align2;
    ?                        msgHandler;
    ?                        swapManagerMsgHandler;
};

struct CAdvPopup {
    ?                        baseclass_0;
    ?                        exitId;
    ?                        exitCodeSubtype;
    ?                        exitCommand;
    ?                        netHandlerInPopup;
};

struct TAdventureOptionsWindow {
    ?                        baseclass_0;
};

struct THeroSpecificAbility {
    ?                        type;
    ?                        info;
    ?                        creatureAttackBonus;
    ?                        creatureDefenseBonus;
    ?                        creatureDamageBonus;
    ?                        creatureGrade;
    ?                        unknown;
    ?                        nameShort;
    ?                        name;
    ?                        description;
};

struct Bitmap16Bit {
    ?                        baseclass_0;
    ?                        DataSize;
    ?                        ImageSize;
    ?                        Width;
    ?                        Height;
    ?                        Pitch;
    ?                        map;
    ?                        keepData;
};

struct mouseManager {
    ?                        baseclass_0;
    ?                        bNoChangePointer;
    #L                       LastDraw;
    ?                        Set;
    ?                        Frame;
    ?                        Sprite;
    ?                        ImageX;
    ?                        ImageY;
    ?                        DisableCount;
    ?                        SystemPointerIsOn;
    ?                        iHideCount;
    #b                       cursorPos;
    ?                        Busy;
    ?                        CriticalSection;
};

struct CRITICAL_SECTION {
    #x                       DebugInfo;
    #K                       LockCount;
    #K                       RecursionCount;
    #|                       OwningThread;
    #|                       LockSemaphore;
    #}                       SpinCount;
};

struct Bitmap816 {
    ?                        baseclass_0;
    ?                        DataSize;
    ?                        ImageSize;
    ?                        Width;
    ?                        Height;
    ?                        Pitch;
    ?                        map;
    ?                        Palette;
    ?                        Palette24;
};

struct File::vftable_t {
    ?                        Close;
    ?                        Open;
    ?                        IsOpen;
    ?                        Read;
    ?                        Write;
    ?                        Seek;
    ?                        SeekBegin;
    ?                        SeekEnd;
    ?                        SeekCur;
    ?                        GetPosition;
    ?                        GetLength;
};

struct LODFile {
    ?                        fileptr;
    ?                        LODFileName;
    ?                        opened;
    ?                        dataBuffer;
    ?                        dataBufferSize;
    ?                        dataItemIndex;
    ?                        dataPos;
    ?                        matchindex;
    ?                        header;
    ?                        numEntries;
    ?                        subindex;
};

struct LODHeader {
    ?                        LOD_ID;
    ?                        version;
    ?                        numEntries;
    ?                        reserved;
};

struct LODEntry {
    ?                        name;
    ?                        offset;
    ?                        size;
    ?                        attrib;
    ?                        csize;
};

struct combatManager::adjacency_array {
    ?                        adjacent;
};

struct combatManager::TArcher {
    ?                        Type;
    ?                        Sprite;
    ?                        Missile;
    ?                        X;
    ?                        Y;
    ?                        Facing;
    ?                        Sequence;
    ?                        Frame;
    ?                        Amount;
};

struct SBolt {
    ?                        sourceX;
    ?                        sourceY;
    ?                        destX;
    ?                        destY;
    ?                        splitFrequency;
    ?                        startThickness;
    ?                        color;
    ?                        dword1C;
    ?                        dword20;
    ?                        float24;
    ?                        float28;
    ?                        source_X;
    ?                        source_Y;
    ?                        out_of_range;
    ?                        dword34;
    ?                        float3C;
    ?                        dword40;
    ?                        someBool;
    ?                        dword44;
    ?                        gap48;
    ?                        dword54;
    ?                        thickness;
    ?                        endThickness;
    ?                        length;
    ?                        angleDistortMin;
    ?                        dword68;
    ?                        dword6C;
    ?                        dword70;
    ?                        dword74;
};

struct TSpellTraits {
    ?                        m_karma;
    ?                        m_sample;
    ?                        m_effect;
    ?                        m_flags;
    ?                        m_name;
    ?                        m_abbreviated_name;
    ?                        m_level;
    ?                        m_school;
    ?                        m_manaCost;
    ?                        m_power_factor;
    ?                        m_mastery_bonus;
    ?                        m_townGetsItChance;
    ?                        m_AI_value;
    ?                        m_description;
};

struct combatManager::TWallTarget {
    ?                        target_hex;
    ?                        blocked_row;
    ?                        hit_x;
    ?                        hit_y;
    ?                        wall;
};

struct soundManager {
    ?                        baseclass_0;
    ?                        mssHandle;
    ?                        driver;
    ?                        samples;
    ?                        samples_array;
    ?                        sampleNum;
    ?                        currentTerrainMusic;
    ?                        playSounds;
    ?                        bChangeSounds;
    ?                        MP3Playing;
    ?                        section_sound_call;
    ?                        section_MP3_change;
    ?                        section_MP3_name_change;
};

struct SAMPLE2 {
    ?                        resSample;
    #|                       playSample;
};

struct configStruct {
    ?                        walkSpeed;
    ?                        musicVolume;
    ?                        soundVolume;
    ?                        lastMusicVolume;
    ?                        lastSoundVolume;
    ?                        AutoSave;
    ?                        ShowRoute;
    ?                        MoveReminder;
    ?                        QuickCombat;
    ?                        VideoSubtitles;
    ?                        TownOutlines;
    ?                        AnimateSpellBook;
    ?                        WindowScrollSpeed;
    ?                        BlackoutComputer;
    ?                        AutoCreatures;
    ?                        AutoSpells;
    ?                        AutoCatapult;
    ?                        AutoBallista;
    ?                        AutoFirstAidTent;
    ?                        PreferBink;
    ?                        MainGameShowMenu;
    ?                        ScreenX;
    ?                        ScreenY;
    ?                        FullScreen;
    ?                        bCombatShowEntireGrid;
    ?                        bCombatShowMouseHex;
    ?                        iCombatGridLevel;
    ?                        iCombatViewArmy;
    ?                        padding;
    ?                        bDontTryRedbook;
    ?                        bFirstInstall;
    ?                        cUniqueSystemID;
    ?                        iCombatSpeed;
    ?                        unknown;
    ?                        cCurRemoteReceive;
    ?                        cRemoteReceiveDiff;
    ?                        cCurRemoteSend;
    ?                        cNetName;
};

struct CLogFile {
    ?                        m_logFileName;
};

struct CTurnDuration {
    ?                        m_lastWarned;
    ?                        m_turnStartTime;
    ?                        m_currDuration;
    ?                        m_nextWarning;
    ?                        m_pauseTime;
};

struct inputManager {
    ?                        baseclass_0;
    ?                        iBuffer;
    ?                        iHead;
    ?                        iTail;
    ?                        bufferBusy;
    ?                        mouseInstalled;
    ?                        scanCodeTable;
    ?                        keyboardInstalled;
    ?                        keyboardFilter;
    ?                        keyCodeType;
    ?                        extendFlag;
    ?                        currWidgetID;
    ?                        possibleWidgetID;
};

struct TPickANumber {
    ?                        Low;
    ?                        Numbersleft;
    ?                        Available;
};

struct std::vector_bool_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::bitset_28_ {
    ?                        _bits;
};

struct TViewArmyWindow {
    ?                        baseclass_0;
    ?                        ArmyType;
    ?                        ArmySize;
    ?                        morale;
    ?                        morale_help;
    ?                        luck;
    ?                        luck_help;
    ?                        Upgrade;
    ?                        ShowingUpgradeButton;
    ?                        ShowingDismissButton;
    ?                        ShowingOkButton;
    ?                        Influence;
    ?                        Duration;
    ?                        RolloverWidget;
    ?                        SpriteWidget;
};

struct highScoreManager {
    ?                        baseclass_0;
    ?                        scenarioRecords;
    ?                        campaignRecords;
    ?                        showSingleScenarios;
};

struct highScoreManager::HighScoreRec {
    ?                        playerName;
    ?                        scenarioName;
    ?                        totalScore;
    ?                        totalTime;
    ?                        basicScore;
    ?                        bIsCheater;
};

struct type_enchant_data {
    ?                        spell;
    ?                        mastery;
    ?                        power;
    ?                        duration;
    ?                        check_resistance;
};

struct type_AI_attack_hex_chooser {
    ?                        attacker;
    ?                        speed;
    ?                        enemy;
    ?                        search_array;
    ?                        enemy_attacks;
    ?                        retaliation_strength;
    ?                        our_strength;
    ?                        best_value;
    ?                        best_hex;
    ?                        best_attack_time;
    ?                        estimate;
};

struct generator {
    ?                        genClass;
    ?                        genType;
    ?                        type;
    ?                        population;
    ?                        guards;
    ?                        mapX;
    ?                        mapY;
    ?                        mapZ;
    ?                        playerOwner;
    ?                        town_id;
};

struct TTimedEvent {
    ?                        Message;
    ?                        ResQty;
    ?                        PlayerFlags;
    ?                        ApplyToPlayer;
    ?                        ApplyToComputer;
    ?                        FirstTime;
    ?                        Interval;
};

struct CObjectType {
    ?                        ImageName;
    ?                        Width;
    ?                        Height;
    ?                        PlacementMask;
    ?                        PassableMask;
    ?                        ShadowMask;
    ?                        TriggerMask;
    ?                        TerrainMask;
    ?                        Type;
    ?                        Subtype;
    ?                        IsUnderlay;
    byte                     align1;
    ?                        objectTypeIndex;
};

struct CObject {
    ?                        baseclass_0;
    ?                        x;
    ?                        y;
    ?                        z;
    ?                        TypeID;
    ?                        frameOffset;
};

struct NewmapCell::TObjectCell {
    ?                        ObjectIndex;
    ?                        _bf_2;
    ?                        Height;
};

struct TreasureData {
    ?                        Message;
    ?                        HasCustomGuardians;
    ?                        Guardians;
};

struct type_event_record {
    ?                        vftable;
    ?                        player_id;
};

struct BlackBoxData {
    ?                        baseclass_0;
    ?                        HasCustomTreasure;
    ?                        ExperienceBonus;
    ?                        ManaBonus;
    ?                        MoraleBonus;
    ?                        LuckBonus;
    ?                        ResQty;
    ?                        PrimarySkillBonus;
    ?                        SecondarySkills;
    ?                        Artifacts;
    ?                        Spells;
    ?                        Creatures;
};

struct TQuestLogWindow {
    ?                        baseclass_0;
    ?                        unknown;
    ?                        seerHutLogList;
};

struct slider {
    ?                        baseclass_0;
    ?                        sliderSprite;
    ?                        sliderBitmap;
    ?                        oldState;
    ?                        currentState;
    ?                        knobPos;
    ?                        knobRange;
    ?                        numStates;
    ?                        length;
    ?                        pageSize;
    ?                        knob_start;
    ?                        clickX;
    ?                        clickY;
    ?                        hotKeys;
    ?                        scrolling;
    ?                        lastFocus;
    ?                        gap_5F;
    ?                        sliderFunction;
};

struct CCombatInitMsg {
    ?                        baseclass_0;
    ?                        m_point;
    ?                        m_leftHero;
    ?                        m_rightTown;
    ?                        m_rightHero;
    ?                        gap_1F;
    ?                        m_seed;
    ?                        m_winner;
    ?                        m_retreatWin;
    ?                        m_combatSurrender;
    ?                        gap_2A;
    ?                        m_leftOwner;
    ?                        m_leftGold;
    ?                        m_rightOwner;
    ?                        m_rightGold;
    ?                        m_leftArmyGroup;
    ?                        m_rightArmyGroup;
    ?                        gap_AC;
    ?                        m_town;
    ?                        m_leftHeroData;
    ?                        m_rightHeroData;
};

struct recruitUnit {
    ?                        baseclass_0;
    ?                        CurrentSpriteFrame;
    ?                        type;
    ?                        view_only;
    ?                        monsterType;
    ?                        numAvail;
    ?                        selectedPosition;
    ?                        MonType1;
    ?                        MonType2;
    ?                        MonType3;
    ?                        MonType4;
    ?                        available;
    ?                        thisHero;
    ?                        availSource;
    ?                        goldPerTroop;
    ?                        altResource;
    ?                        resourcesPerTroop;
    ?                        bInTownMainScreen;
    ?                        errorWin;
    ?                        currArmyGroup;
    ?                        bCurrArmyGroupIsTownGarrison;
    ?                        addIndex;
    ?                        updateNeeded;
    ?                        errorExit;
    ?                        maxAvail;
    ?                        totalGold;
    ?                        totalResources;
    ?                        numberToBuy;
};

struct TSpreadsheetResource {
    ?                        baseclass_0;
    ?                        SpreadSheet;
    ?                        Data;
    ?                        DataSize;
};

struct type_AI_creature_swapper {
    ?                        army;
    ?                        adjacent_army;
    ?                        has_angelic_alliance;
    ?                        morale;
    ?                        alignment_count;
    ?                        alignments;
    ?                        army_value_increase;
    ?                        improvement;
};

struct type_AI_creature_purchaser {
    ?                        baseclass_0;
    ?                        player_id;
    ?                        funds;
    ?                        subtract_cost_mode;
    ?                        creatures;
};

struct DPLCONNECTION {
    ?                        dwSize;
    ?                        dwFlags;
    ?                        lpSessionDesc;
    ?                        lpPlayerName;
    ?                        guidSP;
    ?                        lpAddress;
    ?                        dwAddressSize;
};

struct GUID {
    ?                        Data1;
    ?                        Data2;
    ?                        Data3;
    ?                        Data4;
};

struct DPSESSIONDESC2 {
    ?                        dwSize;
    ?                        dwFlags;
    ?                        guidInstance;
    ?                        guidApplication;
    ?                        dwMaxPlayers;
    ?                        dwCurrentPlayers;
    ?                        lpszSessionNameA;
    ?                        lpszPasswordA;
    ?                        dwReserved1;
    ?                        dwReserved2;
    ?                        dwUser1;
    ?                        dwUser2;
    ?                        dwUser3;
    ?                        dwUser4;
};

struct DPNAME {
    ?                        dwSize;
    ?                        dwFlags;
    ?                        lpszShortNameA;
    ?                        lpszLongNameA;
};

struct CDPlay {
    ?                        vftable;
    ?                        m_caps;
    ?                        m_lpDP;
    ?                        m_guid;
    ?                        m_hRes;
    ?                        m_pSessionArray;
    ?                        m_pConnectionArray;
    ?                        m_pGroupArray;
    ?                        m_pPlayerArray;
    ?                        m_connected;
    ?                        m_inSession;
    ?                        m_isHost;
};

struct DPCAPS {
    ?                        dwSize;
    ?                        dwFlags;
    ?                        dwMaxBufferSize;
    ?                        dwMaxQueueSize;
    ?                        dwMaxPlayers;
    ?                        dwHundredBaud;
    ?                        dwLatency;
    ?                        dwMaxLocalPlayers;
    ?                        dwHeaderLength;
    ?                        dwTimeout;
};

struct CDPlayHeroes {
    ?                        baseclass_0;
    ?                        dpMsg;
    ?                        msgQueue;
    ?                        sLocalIPAddress;
    ?                        confirmId;
    ?                        currMessageId;
    ?                        m_pNetMsgHandler;
};

struct CDPlayLobby {
    ?                        baseclass_0;
    ?                        m_lpLobby;
    ?                        m_pAddressArray;
};

struct CDPlayMsg {
    ?                        pData;
    ?                        dataSize;
};

struct CRect {
    #L                       baseclass_0;
};

struct CNetPlayerInfo {
    ?                        dpid;
    ?                        cName;
    ?                        version;
};

struct button {
    ?                        baseclass_0;
    ?                        buttonIcon;
    ?                        normalFrame;
    ?                        selectedFrame;
    ?                        disabled_frame;
    ?                        highlightedFrame;
    ?                        _end;
    ?                        hotKeyCodes;
    ?                        Text;
};

struct CSequence {
    ?                        numFrames;
    ?                        allocatedFrames;
    ?                        f;
};

struct CDiffMaker {
    ?                        m_oldData;
    ?                        m_newData;
    ?                        m_oldSize;
    ?                        m_newSize;
};

struct TDialogBox {
    ?                        baseclass_0;
    ?                        beginID;
    ?                        endID;
};

struct boat {
    ?                        baseclass_0;
    ?                        allocated;
    ?                        id;
    ?                        type;
    ?                        facing;
    ?                        playerOwner;
    ?                        occupying_hero;
    ?                        occupied;
};

struct type_spellvalue {
    ?                        our_hero;
    ?                        stack_value;
    ?                        power;
    ?                        duration;
    ?                        mana;
    ?                        list;
};

struct _DIG_DRIVER {
    ?                        tag;
    ?                        backgroundtimer;
    ?                        quiet;
    ?                        n_active_samples;
    ?                        master_volume;
    ?                        DMA_rate;
    ?                        hw_format;
    ?                        hw_mode_flags;
    ?                        channels_per_sample;
    ?                        bytes_per_channel;
    ?                        channels_per_buffer;
    ?                        samples_per_buffer;
    ?                        playing;
    #|                       samples;
    ?                        n_samples;
    ?                        build_size;
    ?                        build_buffer;
    ?                        system_data;
    ?                        buffer_size;
    ?                        hWaveOut;
    ?                        reset_works;
    ?                        request_reset;
    ?                        first;
    ?                        n_buffers;
    ?                        return_list;
    ?                        return_head;
    ?                        return_tail;
    ?                        deviceid;
    ?                        wformat;
    ?                        guid;
    ?                        pDS;
    ?                        ds_priority;
    ?                        emulated_ds;
    ?                        lppdsb;
    #Q                       dsHwnd;
    ?                        lpbufflist;
    ?                        samp_list;
    ?                        sec_format;
    ?                        max_buffs;
    ?                        released;
    ?                        foreground_timer;
    ?                        next;
    ?                        callingCT;
    ?                        callingDS;
    ?                        DS_initialized;
    ?                        DS_sec_buff;
    ?                        DS_out_buff;
    ?                        DS_buffer_size;
    ?                        DS_frag_cnt;
    ?                        DS_frag_size;
    ?                        DS_last_frag;
    ?                        DS_last_write;
    ?                        DS_last_timer;
    ?                        DS_skip_time;
    ?                        DS_use_default_format;
    ?                        master_wet;
    ?                        master_dry;
    ?                        use_MMX;
    ?                        us_count;
    ?                        ms_count;
    ?                        last_ms_polled;
    ?                        last_percent;
    ?                        pipeline;
    ?                        ri;
    ?                        reverb_build_buffer;
    ?                        reverb_build_size;
    ?                        reverb_buffer_size;
    ?                        reverb_on;
    ?                        reverb_off_time;
    ?                        reverb_duration;
    ?                        reverb_time;
    ?                        reverb_damping;
    ?                        reverb_predelay;
    ?                        reverb_into;
    ?                        reverb_outof;
    ?                        no_wom_done;
    ?                        wom_done_buffers;
};

struct PCMWAVEFORMAT {
    ?                        wF;
    #e                       wBitsPerSample;
};

struct DPINFO {
    ?                        off;
};

struct REVERB_INFO {
    ?                        u;
    ?                        c;
};

struct REVERB_UPDATED_INFO {
    ?                        address0;
    ?                        address1;
    ?                        address2;
    ?                        address3;
    ?                        address4;
    ?                        address5;
    ?                        X0;
    ?                        X1;
    ?                        Y0;
    ?                        Y1;
};

struct REVERB_CONSTANT_INFO {
    ?                        start0;
    ?                        start1;
    ?                        start2;
    ?                        start3;
    ?                        start4;
    ?                        start5;
    ?                        end0;
    ?                        end1;
    ?                        end2;
    ?                        end3;
    ?                        end4;
    ?                        end5;
    ?                        C0;
    ?                        C1;
    ?                        C2;
    ?                        C3;
    ?                        C4;
    ?                        C5;
    ?                        A;
    ?                        B0;
    ?                        B1;
};

struct waveformat_tag {
    #e                       wFormatTag;
    #e                       nChannels;
    #D                       nSamplesPerSec;
    #D                       nAvgBytesPerSec;
    #e                       nBlockAlign;
};

struct WAVEFORMAT {
    #e                       wFormatTag;
    #e                       nChannels;
    #D                       nSamplesPerSec;
    #D                       nAvgBytesPerSec;
    #e                       nBlockAlign;
};

struct THillFortWindow {
    ?                        baseclass_0;
};

struct type_creature_bank {
    ?                        guards;
    ?                        resources;
    ?                        reward_creature;
    ?                        reward_creatures;
    ?                        artifacts;
};

struct resource::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        Dispose;
    ?                        GetSize;
};

struct CAdvMgrNetMsgHandler {
    ?                        baseclass_0;
};

struct bitmapBorder {
    ?                        baseclass_0;
    ?                        borderBitmap16;
};

struct border {
    ?                        baseclass_0;
};

struct TViewWorldWindow {
    ?                        baseclass_0;
    ?                        viewable_width;
    ?                        viewable_height;
    ?                        RolloverWidget;
    ?                        origin;
};

struct type_func_button {
    ?                        baseclass_0;
    ?                        handler;
};

struct TCampaignMenu {
    ?                        baseclass_0;
};

struct TCampaignWindow {
    ?                        baseclass_0;
};

struct CChatManager {
    ?                        msgArray;
    ?                        currMsg;
    ?                        msgCount;
    ?                        widgetText;
    ?                        pauseTime;
    ?                        changed;
    ?                        lastWidget;
    ?                        maxLines;
    ?                        position;
    ?                        chatKilled;
    ?                        channel;
    ?                        isSysMsg;
    ?                        chatSample;
    ?                        playerDropSample;
    ?                        sysMsgSample;
    ?                        turnDurSample;
    ?                        playerEnterSample;
};

struct CChatManager::CChatStr {
    ?                        sText;
    ?                        killTime;
    ?                        isSystem;
};

struct TCombatWindow {
    ?                        baseclass_0;
    ?                        unknown40;
};

struct type_record_shroud {
    ?                        baseclass_0;
    ?                        changes;
};

struct CGameTransferDlg {
    ?                        baseclass_0;
    ?                        smack;
    ?                        m_sending;
};

struct CTextDialog {
    ?                        baseclass_0;
    ?                        pTextWidget;
};

struct CGameTransferSmack {
    ?                        m_x;
    ?                        m_y;
    ?                        m_lastFrame;
    ?                        m_started;
    ?                        m_sending;
    ?                        m_drawText;
    ?                        m_saveScreen;
};

struct TSpellEffectTraits {
    ?                        m_spriteName;
    ?                        m_name;
    ?                        m_flags;
};

struct EXCEPTION_RECORD {
    #D                       ExceptionCode;
    #D                       ExceptionFlags;
    ?                        ExceptionRecord;
    ?                        ExceptionAddress;
    #D                       NumberParameters;
    ?                        ExceptionInformation;
};

struct std::vector_combatManager::TObstacle_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_pathCell_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_pathCell_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_army_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_resource_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct TWidgetVector {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_type_creature_value_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_type_creature_source_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_type_creature_bank_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct font::TFontSpec {
    ?                        first;
    ?                        last;
    ?                        depth;
    ?                        xspace;
    ?                        yspace;
    ?                        height;
    ?                        baseyoffset;
    ?                        pad;
    ?                        numpal;
    ?                        pal;
    ?                        abc;
    ?                        Offset;
};

struct font::TFontSpec::myABC {
    ?                        abcA;
    ?                        abcB;
    ?                        abcC;
};

struct TAbstractFile::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        read;
    ?                        write;
};

struct TGzFile {
    ?                        baseclass_0;
    ?                        file;
};

struct std::vector_generator_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TBlackMarket_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct type_record_claim_mine {
    ?                        baseclass_0;
    ?                        id;
    ?                        new_owner;
    ?                        prev_owner;
};

struct CMapChange {
    ?                        baseclass_0;
};

struct widget::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        Open;
    ?                        Main;
    ?                        zBufferDraw;
    ?                        Draw;
    ?                        GetRealHeight;
    ?                        GetRealWidth;
    ?                        process_hover;
    ?                        Dim;
    ?                        enable;
    ?                        OnSetFocus;
    ?                        OnKillFocus;
    ?                        sleep;
};

struct TDrawParts {
    ?                        IsValid;
    ?                        X;
    ?                        Y;
    ?                        Id;
};

struct combatManager::TObstacle {
    ?                        sprite;
    ?                        info;
    ?                        grid_index;
    ?                        owner;
    ?                        is_visible;
    ?                        damage;
    ?                        duration;
    ?                        dispel_effect;
};

struct garrison {
    ?                        playerOwner;
    ?                        garrisonArmy;
    ?                        armyRemovable;
    ?                        mapX;
    ?                        mapY;
    ?                        mapZ;
};

struct townObject {
    ?                        numFrames;
    ?                        currFrame;
    ?                        x;
    ?                        y;
    ?                        h;
    ?                        w;
    ?                        visible;
    ?                        objId;
    ?                        objIcon;
    ?                        objOutline;
    ?                        objHotspot;
    ?                        objBorder;
};

struct type_spell_choice {
    ?                        baseclass_0;
    ?                        target_hex;
    ?                        second_target_hex;
    ?                        value;
    ?                        cast_now;
};

struct type_record_show_hero {
    ?                        baseclass_0;
    ?                        new_location;
    ?                        prev_location;
    ?                        is_in_boat;
    ?                        was_in_boat;
};

struct TSingleSelectionWindow {
    ?                        baseclass_0;
    ?                        clickTime;
    ?                        loadGameMode;
    ?                        saveGameMode;
    ?                        showRandomMaps;
    ?                        gap_67;
    ?                        textIndex;
    ?                        VersionIcon;
    ?                        VictoryIcon;
    ?                        LossIcon;
    ?                        TownPix;
    ?                        Resource;
    ?                        heroSpecificAbility;
    ?                        GoldBox;
    ?                        Flags;
    ?                        Panels;
    ?                        HeroPix;
    ?                        randomTownQuestion;
    ?                        randomHeroQuestion;
    ?                        randomTown;
    ?                        randomHero;
    ?                        noDice;
    ?                        noHero;
    ?                        sortDescending;
    ?                        gap_36D;
    ?                        currentIndex;
    ?                        currentMap;
    ?                        durationIndex;
    ?                        inAdvancedOptions;
    ?                        inScenarioOptions;
    ?                        inRandomMapOptions;
    ?                        randomMapGeneration;
    ?                        saveGameEdit;
    ?                        gap_384;
    ?                        pNewPlayerUpdateMan;
    ?                        pendingRandomMap;
    ?                        mapsList;
    ?                        randomMapsList;
    ?                        currentMapsList;
    ?                        selectedMap;
    ?                        netPlayerHandler;
    ?                        receivedMaps;
    ?                        gap_1835;
    ?                        chatSlider;
    ?                        fileSlider;
    ?                        durationSlider;
    ?                        gap_1844;
    ?                        chatWidget;
    ?                        nameList1;
    ?                        nameList2;
    ?                        mapChanged;
    ?                        readingMaps;
    ?                        gap_1856;
    ?                        chatEdit;
    ?                        sortWhich;
    ?                        filterSize;
    ?                        scenarioOptionsStarted;
    ?                        chatShowing;
    ?                        gap_1866;
    ?                        chatToggle;
    ?                        receivingMaps;
    ?                        gap_186D;
    ?                        flagBack;
    ?                        cGameVersion;
    ?                        netMsgHandler;
    ?                        gameVersion;
    ?                        gap_189C;
    ?                        randomMapSize;
    ?                        randomMapTwoLevels;
    ?                        randomMapHumans;
    ?                        randomMapTeams;
    ?                        randomMapComputers;
    ?                        randomMapComputerTeams;
    ?                        randomMapWater;
    ?                        randomMapMonsterStrength;
    ?                        randomMapPlayerButtons;
    ?                        randomMapComputerButtons;
    ?                        randomMapTeamButtons;
    ?                        randomMapComputerTeamButtons;
    ?                        randomMapWaterButtons;
    ?                        randomMapMonsterStrengthButtons;
    ?                        mapDescScroller;
};

struct heroWindow::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        Open;
    ?                        Close;
    ?                        handle_message;
    ?                        handle_widget_hover;
    ?                        DrawWindow;
    ?                        DoModal;
    ?                        AddWidgetsToMessageStream;
    ?                        sleep;
};

struct type_sacrifice_window {
    ?                        baseclass_0;
    ?                        current_hero;
    ?                        holding_artifact;
    ?                        sacrificing_artifacts;
    ?                        can_sacrifice_artifacts;
    ?                        can_sacrifice_creatures;
    ?                        total_experience;
    ?                        experience_widget;
    ?                        experience_total_widget;
    ?                        current_artifact_value;
    ?                        creature_name_widget;
    ?                        rollover_widget;
    ?                        current_artifact_widget;
    ?                        creature_slider;
    ?                        left_backpack_button;
    ?                        right_backpack_button;
    ?                        empty_backpack_button;
    ?                        sacrifice_button;
    ?                        all_artifacts_button;
    ?                        creatures_button;
    ?                        max_creatures_button;
    ?                        all_creatures_button;
    ?                        artifacts_button;
    ?                        artifact_offerings;
    ?                        artifact_value_widgets;
    ?                        artifact_offering_widgets;
    ?                        slot_back_widgets;
    ?                        slot_widgets;
    ?                        backpack_widgets;
    ?                        creature_offerings;
    ?                        current_creature;
    ?                        artifact_widgets;
    ?                        creature_widgets;
    ?                        artifact_mode;
};

struct TownExtra {
    ?                        objRef;
    ?                        playerOwner;
    ?                        bCustomBuildings;
    ?                        BuildingBuiltMask;
    ?                        BuildingDisabledMask;
    ?                        HasFort;
    ?                        bCustomArmies;
    ?                        townArmy;
    ?                        bCustomName;
    ?                        cName;
    ?                        townType;
    ?                        bIsGrouped;
    ?                        align1;
    ?                        SpellDisabledMask;
    ?                        SpellMask;
};

struct TTownEvent {
    ?                        baseclass_0;
    ?                        TownNum;
    ?                        BuildBuildings;
    ?                        generatorBonuses;
};

struct bitmapBorder16 {
    ?                        baseclass_0;
    ?                        borderBitmap16;
};

struct TSubWindow::vftable_t {
    ?                        scalar_deleting_destructor;
};

struct Bitmap24Bit {
    ?                        baseclass_0;
    ?                        DataSize;
    ?                        ImageSize;
    ?                        Width;
    ?                        Height;
    ?                        data;
};

struct IDirectDraw {
    ?                        lpVtbl;
};

struct DDSURFACEDESC {
    #D                       dwSize;
    #D                       dwFlags;
    #D                       dwHeight;
    #D                       dwWidth;
    ?                        anonymous_0;
    #D                       dwBackBufferCount;
    ?                        anonymous_1;
    #D                       dwAlphaBitDepth;
    #D                       dwReserved;
    ?                        lpSurface;
    ?                        ddckCKDestOverlay;
    ?                        ddckCKDestBlt;
    ?                        ddckCKSrcOverlay;
    ?                        ddckCKSrcBlt;
    DDPIXELFORMAT            ddpfPixelFormat;
    ?                        ddsCaps;
};

struct DDSURFACEDESC::$F9D0D49E746EA05C6F8F62A8D439C7A9 {
    #K                       lPitch;
    #D                       dwLinearSize;
};

struct DDSURFACEDESC::$732C1078520B5FCBD2DC52BA2F31A7C8 {
    #D                       dwMipMapCount;
    #D                       dwZBufferBitDepth;
    #D                       dwRefreshRate;
};

struct DDCOLORKEY {
    #D                       dwColorSpaceLowValue;
    #D                       dwColorSpaceHighValue;
};

struct DDPIXELFORMAT {
    #D                       dwSize;
    #D                       dwFlags;
    #D                       dwFourCC;
    ?                        anonymous_0;
    ?                        anonymous_1;
    ?                        anonymous_2;
    ?                        anonymous_3;
    ?                        anonymous_4;
};

struct _DDPIXELFORMAT::$F1D3FB4D78950D0942225445130999CB {
    #D                       dwRGBBitCount;
    #D                       dwYUVBitCount;
    #D                       dwZBufferBitDepth;
    #D                       dwAlphaBitDepth;
    #D                       dwLuminanceBitCount;
    #D                       dwBumpBitCount;
    #D                       dwPrivateFormatBitCount;
};

struct _DDPIXELFORMAT::$6A86D2BA2D533C5D3D5AB1F1491969D5 {
    #D                       dwRBitMask;
    #D                       dwYBitMask;
    #D                       dwStencilBitDepth;
    #D                       dwLuminanceBitMask;
    #D                       dwBumpDuBitMask;
    #D                       dwOperations;
};

struct _DDPIXELFORMAT::$95F56DB01BB1548DF390D9ACB4F5DA09 {
    #D                       dwGBitMask;
    #D                       dwUBitMask;
    #D                       dwZBitMask;
    #D                       dwBumpDvBitMask;
    ?                        MultiSampleCaps;
};

struct _DDPIXELFORMAT::$95F56DB01BB1548DF390D9ACB4F5DA09::$A78036EB239B85FA27F661E6E98FFEA9 {
    #e                       wFlipMSTypes;
    #e                       wBltMSTypes;
};

struct _DDPIXELFORMAT::$4C86B66084EB9B6F3AE81991D3FADB38 {
    #D                       dwBBitMask;
    #D                       dwVBitMask;
    #D                       dwStencilBitMask;
    #D                       dwBumpLuminanceBitMask;
};

struct _DDPIXELFORMAT::$23DF69239FC04D9BE22118E1AD8451FB {
    #D                       dwRGBAlphaBitMask;
    #D                       dwYUVAlphaBitMask;
    #D                       dwLuminanceAlphaBitMask;
    #D                       dwRGBZBitMask;
    #D                       dwYUVZBitMask;
};

struct DDSCAPS {
    #D                       dwCaps;
};

struct IDirectDrawSurface4 {
    ?                        vftable;
    #D                       dwSize;
    #D                       dwFlags;
    #D                       dwHeight;
    #D                       dwWidth;
    #K                       lPitch;
    #D                       dwBackBufferCount;
    #D                       dwAlphaBitDepth;
    #D                       dwReserved;
    ?                        lpSurface;
    ?                        ddckCKDestOverlay;
    ?                        ddckCKDestBlt;
    ?                        ddckCKSrcOverlay;
    ?                        ddckCKSrcBlt;
    ?                        ddpfPixelFormat;
    #D                       dwTextureStage;
};

struct IDirectDraw::IDirectDrawVtbl {
    ?                        QueryInterface;
    ?                        AddRef;
    ?                        Release;
    ?                        AddAttachedSurface;
    ?                        AddOverlayDirtyRect;
    ?                        Blt;
    ?                        BltBatch;
    ?                        BltFast;
    ?                        DeleteAttachedSurface;
    ?                        EnumAttachedSurfaces;
    ?                        EnumOverlayZOrders;
    ?                        Flip;
    ?                        GetAttachedSurface;
    ?                        GetBltStatus;
    ?                        GetCaps;
    ?                        GetClipper;
    ?                        GetColorKey;
    ?                        GetDC;
    ?                        GetFlipStatus;
    ?                        GetOverlayPosition;
    ?                        GetPalette;
    ?                        GetPixelFormat;
    ?                        GetSurfaceDesc;
    ?                        Initialize;
    ?                        IsLost;
    ?                        Lock;
    ?                        ReleaseDC;
    ?                        Restore;
    ?                        SetClipper;
    ?                        SetColorKey;
    ?                        SetOverlayPosition;
    ?                        SetPalette;
    ?                        Unlock;
    ?                        UpdateOverlay;
    ?                        UpdateOverlayDisplay;
    ?                        UpdateOverlayZOrder;
    ?                        GetDDInterface;
    ?                        PageLock;
    ?                        PageUnlock;
    ?                        SetSurfaceDesc;
    ?                        SetPrivateData;
    ?                        GetPrivateData;
    ?                        FreePrivateData;
    ?                        GetUniquenessValue;
    ?                        ChangeUniquenessValue;
};

struct CAutoArray {
    ?                        vftable;
    #D                       step;
    ?                        pArray;
    #D                       allocSize;
    #D                       size;
};

struct CAutoArray::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        Add;
    ?                        Get;
    ?                        Put;
    ?                        Delete;
    ?                        Insert;
    ?                        GetCount;
};

struct TSpellbookWindow {
    ?                        baseclass_0;
    ?                        AllowedContext;
    ?                        Hero;
    ?                        EnemyGroup;
    ?                        PlainsType;
    ?                        School;
    ?                        ContextMask;
    ?                        Page;
    ?                        SpellMap;
    ?                        SpellLevelWidgets;
    ?                        SpellIconWidgets;
    ?                        SpellNameWidgets;
    ?                        HeadingWidget;
    ?                        NextPageWidget;
    ?                        PreviousPageWidget;
    ?                        SchoolTabsWidget;
    ?                        RolloverWidget;
};

struct type_cell_adjuster {
    ?                        obscuring_hero;
    ?                        obscuring_boat;
    ?                        mobile_hero;
};

struct TTavernWindow {
    ?                        baseclass_0;
};

struct TQuestGuard {
    ?                        quest;
    byte                     setup;
};

struct std::vector_type_event_record_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct type_event_record::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        get_type;
    ?                        load;
    ?                        save;
    ?                        replay;
    ?                        undo;
};

struct type_record_teleport {
    ?                        baseclass_0;
};

struct type_record_move_hero {
    ?                        baseclass_0;
    ?                        current_hero;
    ?                        start;
    ?                        facing_start;
    ?                        facing_end;
    ?                        destination;
};

struct type_record_hide_hero {
    ?                        baseclass_0;
    ?                        current_hero;
    ?                        new_owner;
    ?                        prev_owner;
    ?                        town_garrison;
};

struct std::vector_type_shroud_change_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct mine {
    ?                        playerOwner;
    ?                        type;
    ?                        is_abandoned;
    ?                        guards;
    ?                        mapX;
    ?                        mapY;
    ?                        mapZ;
};

struct std::vector_Sign_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_mine_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_town_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct type_record_hide_boat {
    ?                        baseclass_0;
    ?                        current_boat;
    ?                        is_occupied;
    ?                        was_occupied;
    ?                        new_hero_id;
    ?                        prev_hero_id;
};

struct type_record_show_boat {
    ?                        baseclass_0;
    ?                        new_location;
    ?                        prev_location;
};

struct type_record_erase {
    ?                        baseclass_0;
    ?                        location;
    ?                        object_id;
    ?                        object_extra_info;
    ?                        object_index;
};

struct type_record_player_death {
    ?                        baseclass_0;
    ?                        died_player;
};

struct type_record_shroud::type_shroud_change {
    ?                        baseclass_0;
    ?                        old_value;
    ?                        new_value;
};

struct DDSCAPS2 {
    #D                       dwCaps;
    #D                       dwCaps2;
    #D                       dwCaps3;
    ?                        anonymous_0;
};

struct _DDSCAPS2::$19AC68468C4510B3DC631A4E89752068 {
    #D                       dwCaps4;
    #D                       dwVolumeDepth;
};

struct _DDBLTFX {
    #D                       dwSize;
    #D                       dwDDFX;
    #D                       dwROP;
    #D                       dwDDROP;
    #D                       dwRotationAngle;
    #D                       dwZBufferOpCode;
    #D                       dwZBufferLow;
    #D                       dwZBufferHigh;
    #D                       dwZBufferBaseDest;
    #D                       dwZDestConstBitDepth;
    #D                       dwZDestConst;
    #D                       dwZSrcConstBitDepth;
    #D                       dwZSrcConst;
    #D                       dwAlphaEdgeBlendBitDepth;
    #D                       dwAlphaEdgeBlend;
    #D                       dwReserved;
    #D                       dwAlphaDestConstBitDepth;
    #D                       dwAlphaDestConst;
    #D                       dwAlphaSrcConstBitDepth;
    #D                       dwAlphaSrcConst;
    #D                       dwFillColor;
    ?                        ddckDestColorkey;
    ?                        ddckSrcColorkey;
};

struct combatManager::SElevationOverlay {
    ?                        terrainMask;
    ?                        specialTerrainMask;
    ?                        x;
    ?                        y;
    ?                        blockedSquares;
    ?                        FileName;
};

struct Sign {
    ?                        hasText;
    ?                        signText;
};

struct textEntryWidget {
    ?                        baseclass_0;
    ?                        textBack;
    ?                        saveBack;
    ?                        cursorIndex;
    ?                        bufferSize;
    ?                        textWidth;
    ?                        textHeight;
    ?                        textX;
    ?                        textY;
    ?                        textLines;
    ?                        attributes;
    ?                        type;
    ?                        displayOffset;
    ?                        cursorFlashOn;
    ?                        focus;
    ?                        autoDraw;
};

struct THeroClassTraits {
    ?                        m_townType;
    ?                        m_name;
    ?                        m_aggression;
    ?                        m_initialPrimarySkill;
    ?                        m_gainPrimarySkillChance;
    ?                        m_gainPrimarySkillChance10P;
    ?                        m_gainSecondarySkillChance;
    ?                        m_foundInTownType;
};

struct type_ballistics_traits {
    ?                        chance_to_hit_main_building;
    ?                        chance_to_hit_tower;
    ?                        chance_to_hit_drawbridge;
    ?                        chance_to_hit_wall;
    ?                        shots;
    ?                        chance_to_inflict_damage;
};

struct THeroTraits {
    ?                        m_sex;
    ?                        m_race;
    ?                        m_class;
    ?                        m_1stSkill;
    ?                        m_1stSkillLevel;
    ?                        m_2ndSkill;
    ?                        m_2ndSkillLevel;
    ?                        m_startsWithSpellbook;
    ?                        m_startingSpell;
    ?                        m_1stStack;
    ?                        m_2ndStack;
    ?                        m_3rdStack;
    ?                        m_small_portrait_name;
    ?                        m_large_portrait_name;
    ?                        m_allowedInRoE;
    ?                        m_allowedInABSoD;
    ?                        m_isCampaignHero;
    ?                        attributes;
    ?                        m_name;
    ?                        m_1stStackLow;
    ?                        m_1stStackHigh;
    ?                        m_2ndStackLow;
    ?                        m_2ndStackHigh;
    ?                        m_3rdStackLow;
    ?                        m_3rdStackHigh;
};

struct CDiffFile {
    ?                        saveGameSize;
};

struct RGB8 {
    ?                        Red;
    ?                        Green;
    ?                        Blue;
};

struct TRGBA {
    ?                        Red;
    ?                        Green;
    ?                        Blue;
    ?                        Alpha;
};

struct std::vector_CObjectType_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::bitset_10_ {
    ?                        bits;
};

struct TSeerHut {
    ?                        baseclass_0;
    ?                        reward;
    ?                        NameIndex;
    byte                     unknown;
};

struct combatManager::TObstacleInfo {
    ?                        backgroundMask;
    ?                        height;
    ?                        width;
    ?                        numSquares;
    ?                        underlay;
    ?                        sOffsets;
    ?                        FileName;
};

struct std::bitset_12_ {
    ?                        bits;
};

struct std::bitset_32_ {
    ?                        bits;
};

struct type_artifact_offering {
    ?                        baseclass_0;
    ?                        source;
    ?                        value;
};

struct TSSkillTraits {
    ?                        name;
    ?                        desc;
};

struct MonsterData {
    ?                        Message;
    ?                        ResQty;
    ?                        Artifact;
};

struct CampaignIconPreview {
    ?                        index;
    ?                        x;
    ?                        y;
    ?                        width;
    ?                        height;
    ?                        unknown1;
    ?                        name;
    ?                        widgetIndex;
    ?                        bink;
};

struct baseManager::vftable_t {
    ?                        Open;
    ?                        Close;
    ?                        Main;
};

struct GameSelectionHeadersStruct {
    ?                        baseclass_0;
    ?                        setup;
    ?                        unknown_1;
    ?                        hero_status;
    ?                        file_name;
    ?                        map_desc;
    ?                        unknown_2;
    #                       timestamp;
    ?                        header;
};

struct SavedGameHeader {
    ?                        id;
    ?                        version;
    ?                        game_version;
    ?                        map_header;
    ?                        map_setup;
    ?                        campaign_game;
    ?                        align;
    ?                        campaign;
    ?                        file_name;
    ?                        difficultyRating;
    ?                        numDeadPlayers;
    ?                        dead_player;
    ?                        human_player;
    ?                        current_player;
};

struct CampaignRegionData {
    ?                        unknown;
    ?                        background;
    ?                        amount;
    ?                        regions;
};

struct CampaignRegionBaseData {
    ?                        unknown0;
    ?                        x;
    ?                        y;
    ?                        image;
};

struct std::pair_const_char_int_ {
    ?                        first;
    ?                        second;
};

struct BINKSUMMARY {
    ?                        Width;
    ?                        Height;
    ?                        TotalTime;
    ?                        FileFrameRate;
    ?                        FileFrameRateDiv;
    ?                        FrameRate;
    ?                        FrameRateDiv;
    ?                        TotalOpenTime;
    ?                        TotalFrames;
    ?                        TotalPlayedFrames;
    ?                        SkippedFrames;
    ?                        SoundSkips;
    ?                        TotalBlitTime;
    ?                        TotalReadTime;
    ?                        TotalDecompTime;
    ?                        TotalBackReadTime;
    ?                        TotalReadSpeed;
    ?                        SlowestFrameTime;
    ?                        Slowest2FrameTime;
    ?                        SlowestFrameNum;
    ?                        Slowest2FrameNum;
    ?                        AverageDataRate;
    ?                        AverageFrameSize;
    ?                        HighestMemAmount;
    ?                        TotalIOMemory;
    ?                        HighestIOUsed;
    ?                        Highest1SecRate;
    ?                        Highest1SecFrame;
};

struct std::vector_type_artifact_offering_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_vector_char_ptr__ptr__ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_char_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct THelpText {
    ?                        Rollover;
    ?                        RightClick;
};

struct std::vector_widget_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct type_quest {
    ?                        vftable;
    ?                        seer_hut;
    ?                        proposal_text;
    ?                        progress_text;
    ?                        completion_text;
    ?                        text_variant;
    ?                        limit;
};

struct TBlackMarket {
    ?                        artifacts;
};

struct TGzInflateBuf {
    ?                        baseclass_0;
    ?                        m_source;
    ?                        m_zstream;
    ?                        m_input_buffer;
    ?                        m_output_buffer;
    ?                        m_crc;
    ?                        m_is_compressed;
    ?                        m_end_of_file;
    ?                        m_stream_is_open;
    ?                        m_open;
};

struct std::streambuf {
    ?                        vftable;
    ?                        _Gbeg;
    ?                        _Pbeg;
    ?                        _IGbeg;
    ?                        _IPbeg;
    ?                        _Gnext;
    ?                        _Pnext;
    ?                        _IGnext;
    ?                        _IPnext;
    ?                        _Gcnt;
    ?                        _Pcnt;
    ?                        _IGcnt;
    ?                        _IPcnt;
    ?                        _Loc;
};

struct z_stream {
    ?                        next_in;
    ?                        avail_in;
    ?                        total_in;
    ?                        next_out;
    ?                        avail_out;
    ?                        total_out;
    ?                        msg;
    ?                        state;
    ?                        zalloc;
    ?                        zfree;
    ?                        opaque;
    ?                        data_type;
    ?                        adler;
    ?                        reserved;
};

struct type_text_scroller {
    ?                        baseclass_0;
    ?                        font_filename;
    ?                        text_lines;
    ?                        line_images;
    ?                        text_slider;
    ?                        background;
};

struct CTextEntrySave {
    ?                        baseclass_0;
    ?                        saved;
};

struct std::bitset_8_ {
    ?                        bits;
};

struct combatManager::SCmbtHero {
    ?                        SpriteName;
    ?                        castX;
    ?                        castY;
    ?                        castFrame;
};

struct CAdventurMapChatEdit {
    ?                        baseclass_0;
};

struct CGameChatEdit {
    ?                        baseclass_0;
    ?                        activated;
};

struct tilePoint {
    ?                        x;
    ?                        y;
    ?                        align;
};

struct CImmProject {
    ?                        m_hProj;
    #D                       m_dwProjectFileType;
    ?                        m_pCreatedEffects;
    ?                        m_pDevice;
    LPDIRECTINPUT            m_piDI7;
    LPDIRECTINPUTDEVICE2     m_piDIDevice7;
    ?                        m_szProjectFileName;
    ?                        m_nCreatedEffects;
    ?                        m_pNext;
};

struct CImmEffect {
    ?                        vftable;
    ?                        m_Effect;
    ?                        m_dwaAxes;
    ?                        m_laDirections;
    ?                        m_Envelope;
    ?                        m_guidEffect;
    #n                       m_bIsPlaying;
    #D                       m_dwDeviceType;
    ?                        m_piImmDevice;
    ?                        m_piImmEffect;
    #D                       m_cAxes;
    #D                       m_dwNoDownload;
    #D                       m_dwIterations;
    ?                        m_lpszName;
    #n                       m_bIsInsideEffect;
    ?                        m_pOutsideEffect;
};

struct FEELIT_EFFECT {
    #D                       dwSize;
    ?                        guidEffect;
    #D                       dwFlags;
    #D                       dwDuration;
    #D                       dwSamplePeriod;
    #D                       dwGain;
    #D                       dwTriggerButton;
    #D                       dwTriggerRepeatInterval;
    #D                       cAxes;
    ?                        rgdwAxes;
    ?                        rglDirection;
    ?                        lpEnvelope;
    #D                       cbTypeSpecificParams;
    ?                        lpvTypeSpecificParams;
    #D                       dwStartDelay;
};

struct FEELIT_ENVELOPE {
    #D                       dwSize;
    #D                       dwAttackLevel;
    #D                       dwAttackTime;
    #D                       dwFadeLevel;
    #D                       dwFadeTime;
};

struct std::vector_uint_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct _GUID {
    ?                        Data1;
    ?                        Data2;
    ?                        Data3;
    ?                        Data4;
};

struct std::vector_NewmapCell__TObjectCell_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct TAbstractFile {
    ?                        vftable;
};

struct std::map_ResourceManager::TCacheMapKey_resource_ptr__ {
    ?                        allocator;
    ?                        key_compare;
    ?                        _Head;
    ?                        _Multi;
    size_t                   _Size;
};

struct std::map_ResourceManager::TCacheMapKey_resource_ptr__::_Node {
    ?                        _Left;
    ?                        _Parent;
    ?                        _Right;
    ?                        _Value;
    ?                        _Color;
};

struct std::pair_ResourceManager::TCacheMapKey_resource_ptr__ {
    ?                        first;
    ?                        second;
};

struct ResourceManager::TCacheMapKey {
    ?                        name;
    ?                        b;
};

struct std::_Lockit {
    ?                        unk;
};

struct std::pair_const_char_ptr_LODFile_ {
    ?                        first;
    ?                        second;
};

struct TSeerReward {
    ?                        rewardType;
    ?                        reward;
};

struct type_experience_quest {
    ?                        baseclass_0;
    ?                        level;
};

struct type_quest::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        ai_value;
    ?                        can_complete;
    ?                        complete;
    ?                        do_progress_dialog;
    ?                        do_proposal_dialog;
    ?                        get_quest_text;
    ?                        get_help_text;
    ?                        get_type;
    ?                        hero_defeated;
    ?                        monster_defeated;
    ?                        read;
    ?                        load;
    ?                        write;
    ?                        init;
};

struct TStreamBufFile {
    ?                        baseclass_0;
    ?                        stream;
};

struct CChatEdit {
    ?                        baseclass_0;
};

struct MemorySampleStructure {
    HANDLE                   memHSample;
    ?                        data;
    ?                        size;
    ?                        memCindex;
    ?                        memVolume;
    ?                        memLooping;
};

struct t_complex_net_message {
    ?                        vftable;
    ?                        netmsg;
};

struct CPlayerDropMsg {
    ?                        baseclass_0;
    ?                        m_dpid;
};

struct IUnknown_vtbl {
    ?                        QueryInterface;
    ?                        AddRef;
    ?                        Release;
};

struct IDirectPlayLobby2 {
    ?                        baseclass_0;
};

struct IDirectPlayLobby {
    ?                        vftable;
};

struct IUnknown {
    ?                        vftable;
};

struct CDPlayLobby::vftable_t {
    ?                        baseclass_0;
    ?                        RegisterApp;
    ?                        EnumLobbyConnections;
    ?                        SetGroupConnectionSettings;
    ?                        GetGroupConnectionSettings;
    ?                        EnumGroupsInGroup;
    ?                        EnumGroupPlayers;
    ?                        EnumGroupPlayersRemote;
    ?                        EnumAddress;
    ?                        GetIPAddress;
    ?                        HandleSystemLobbyMsg;
    ?                        AddAddressEnum;
};

struct IDirectPlay3_vtbl {
    ?                        QueryInterface;
    ?                        AddRef;
    ?                        Release;
    ?                        AddPlayerToGroup;
    ?                        Close;
    ?                        CreateGroup;
    ?                        CreatePlayer;
    ?                        DeletePlayerFromGroup;
    ?                        DestroyGroup;
    ?                        DestroyPlayer;
    ?                        EnumGroupPlayers;
    ?                        EnumGroups;
    ?                        EnumPlayers;
    ?                        EnumSessions;
    ?                        GetCaps;
    ?                        GetGroupData;
    ?                        GetGroupName;
    ?                        GetMessageCount;
    ?                        GetPlayerAddress;
    ?                        GetPlayerCaps;
    ?                        GetPlayerData;
    ?                        GetPlayerName;
    ?                        GetSessionDesc;
    ?                        Initialize;
    ?                        Open;
    ?                        Receive;
    ?                        Send;
    ?                        SetGroupData;
    ?                        SetGroupName;
    ?                        SetPlayerData;
    ?                        SetPlayerName;
    ?                        SetSessionDesc;
    ?                        AddGroupToGroup;
    ?                        CreateGroupInGroup;
    ?                        DeleteGroupFromGroup;
    ?                        EnumConnections;
    ?                        EnumGroupsInGroup;
    ?                        GetGroupConnectionSettings;
    ?                        InitializeConnection;
    ?                        SecureOpen;
    ?                        SendChatMessage;
    ?                        SetGroupConnectionSettings;
    ?                        StartSession;
    ?                        GetGroupFlags;
    ?                        GetGroupParent;
    ?                        GetPlayerAccount;
    ?                        GetPlayerFlags;
};

struct CAutoArray_CDPlayAddressElement_ {
    ?                        vftable;
    #D                       step;
    ?                        pArray;
    #D                       allocSize;
    #D                       size;
};

struct CDPlayConnection {
    ?                        guidSP;
    ?                        pConnection;
    ?                        sName;
    ?                        size;
};

struct CSessionLostMsg {
    ?                        baseclass_0;
};

struct CSetAsHostMsg {
    ?                        baseclass_0;
};

struct CDPlaySession {
    ?                        dwFlags;
    ?                        guidInstance;
    ?                        guidApp;
    ?                        maxPlayers;
    ?                        playerCount;
    ?                        sessionName;
    ?                        password;
    ?                        dwUser1;
    ?                        dwUser2;
    ?                        dwUser3;
    ?                        dwUser4;
};

struct DPAPPLICATIONDESC {
    #D                       dwSize;
    #D                       dwFlags;
    ?                        lpszApplicationNameA;
    ?                        guidApplication;
    ?                        lpszFilenameA;
    ?                        lpszCommandLineA;
    ?                        lpszPathA;
    ?                        lpszCurrentDirectoryA;
    ?                        lpszDescriptionA;
};

struct IDirectPlay4 {
    ?                        baseclass_0;
};

struct IDirectPlay3 {
    ?                        baseclass_0;
};

struct IDirectPlay2 {
    ?                        baseclass_0;
};

struct CAutoArray_CDPlayConnection_ {
    ?                        vftable;
    #D                       step;
    ?                        pArray;
    #D                       allocSize;
    #D                       size;
};

struct IDirectPlayLobby2_vtbl {
    ?                        QueryInterface;
    ?                        AddRef;
    ?                        Release;
    ?                        Connect;
    ?                        CreateAddress;
    ?                        EnumAddress;
    ?                        EnumAddressTypes;
    ?                        EnumLocalApplications;
    ?                        GetConnectionSettings;
    ?                        ReceiveLobbyMessage;
    ?                        RunApplication;
    ?                        SendLobbyMessage;
    ?                        SetConnectionSettings;
    ?                        SetLobbyMessageEvent;
    ?                        CreateCompoundAddress;
};

struct IDirectPlayLobby3 {
    ?                        baseclass_0;
};

struct DPCOMPOUNDADDRESSELEMENT {
    ?                        guidDataType;
    #D                       dwDataSize;
    ?                        lpData;
};

struct CPingResponseMsg {
    ?                        baseclass_0;
    ?                        m_pingTime;
};

struct CAnimatedDlg {
    ?                        baseclass_0;
    ?                        m_lastTick;
    ?                        m_spriteX;
    ?                        m_spriteY;
    ?                        m_spriteFrame;
    ?                        m_seq;
    ?                        m_sSprite;
    ?                        m_palUpdated;
    ?                        m_pSprite;
};

struct type_artifact_effect {
    ?                        vftable;
};

struct IDirectDraw4 {
    ?                        lpVtbl;
};

struct IDirectDraw4Vtbl {
    ?                        QueryInterface;
    ?                        AddRef;
    ?                        Release;
    ?                        Compact;
    ?                        CreateClipper;
    ?                        CreatePalette;
    ?                        CreateSurface;
    ?                        DuplicateSurface;
    ?                        EnumDisplayModes;
    ?                        EnumSurfaces;
    ?                        FlipToGDISurface;
    ?                        GetCaps;
    ?                        GetDisplayMode;
    ?                        GetFourCCCodes;
    ?                        GetGDISurface;
    ?                        GetMonitorFrequency;
    ?                        GetScanLine;
    ?                        GetVerticalBlankStatus;
    ?                        Initialize;
    ?                        RestoreDisplayMode;
    ?                        SetCooperativeLevel;
    ?                        SetDisplayMode;
    ?                        WaitForVerticalBlank;
    ?                        GetAvailableVidMem;
    ?                        GetSurfaceFromDC;
    ?                        RestoreAllSurfaces;
    ?                        TestCooperativeLevel;
    ?                        GetDeviceIdentifier;
};

struct PcxData {
    ?                        PCXvers;
    ?                        width;
    ?                        length;
    ?                        BPPixel;
    ?                        Nplanes;
    ?                        BytesPerLine;
    ?                        PalInt;
    ?                        vbitcount;
};

struct imgdes {
    ?                        ibuff;
    ?                        stx;
    ?                        sty;
    ?                        endx;
    ?                        endy;
    ?                        buffwidth;
    ?                        palette;
    ?                        colors;
    ?                        imgtype;
    ?                        bmh;
    ?                        hBitmap;
};

struct RGBQUAD {
    ?                        rgbBlue;
    ?                        rgbGreen;
    ?                        rgbRed;
    ?                        rgbReserved;
};

struct CGiftMsg {
    ?                        baseclass_0;
    ?                        m_niceGuy;
    ?                        m_resource;
    ?                        m_qty;
};

struct font {
    ?                        baseclass_0;
    ?                        fr;
    ?                        p16;
    ?                        Data;
    ?                        DataSize;
};

struct std::vector_string_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct TCombatResultsWindow {
    ?                        baseclass_0;
};

struct TMainMenu {
    ?                        baseclass_0;
    ?                        bShowCDMessage;
    ?                        RolloverWidget;
};

struct std::streambuf::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        overflow;
    ?                        pbackfail;
    ?                        showmanyc;
    ?                        underflow;
    ?                        uflow;
    ?                        xsgetn;
    ?                        xsputn;
    ?                        seekoff;
    ?                        seekpos;
    ?                        setbuf;
    ?                        sync;
    ?                        imbue;
};

struct std::filebuf {
    ?                        baseclass_0;
    ?                        _Pcvt;
    ?                        _State0;
    ?                        _State;
    ?                        _Str;
    ?                        _Closef;
    ?                        _Loc;
    ?                        _File;
};

struct TCampaignBrief::CampaignHeaderStruct {
    ?                        file_error;
    ?                        file_name;
    ?                        campaign_version;
    ?                        region_map;
    ?                        campaign_name;
    ?                        campaign_desc;
    ?                        scenarios;
    ?                        data;
    ?                        stream;
    ?                        variable_difficulty;
    ?                        campaign_music;
};

struct std::vector_TCampaignBrief::ScenarioStruct_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct TCampaignBrief::MapTextStruct {
    ?                        video;
    ?                        audio;
    ?                        subtitles;
};

struct TCampaignBrief::ScenarioStruct {
    ?                        name;
    ?                        offset;
    ?                        inflated_size;
    ?                        prerequisites;
    ?                        region_desc;
    ?                        region_color;
    ?                        difficulty;
    ?                        prologue;
    ?                        epilogue;
    ?                        retain_xp;
    ?                        retain_pskills;
    ?                        retain_sskills;
    ?                        retain_spellbook;
    ?                        retain_artifacts;
    ?                        placeholder_status;
    ?                        hero_placeholders;
    ?                        crossover_creatures;
    ?                        crossover_artifacts;
    ?                        options;
};

struct std::vector_int_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::bitset_145_ {
    ?                        bits;
};

struct std::bitset_144_ {
    ?                        bitset_array;
};

struct BINKRECT {
    ?                        Left;
    ?                        Top;
    ?                        Width;
    ?                        Height;
};

struct TObjectType {
    ?                        _imageNum;
    ?                        _passableMask;
    ?                        _triggerMask;
    ?                        _terrainMask;
    ?                        _terrainRecommendedMask;
    ?                        _type;
    ?                        _subtype;
    ?                        _slotCategory;
    ?                        _isUnderlay;
    ?                        _hasTrigger;
    ?                        _triggerCell;
    ?                        _imageInfo;
};

struct TPoint {
    ?                        x;
    ?                        y;
};

struct TObjectType::_TImageInfo {
    ?                        objSize;
    ?                        drawMask;
    ?                        shadowMask;
};

struct FuncInfo {
    ?                        magicNumber;
    ?                        maxState;
    ?                        pUnwindMap;
    ?                        nTryBlocks;
    ?                        pTryBlockMap;
    ?                        nIPMapEntries;
    ?                        pIPtoStateMap;
    ?                        pESTypeList;
    ?                        EHFlags;
};

struct EHExceptionRecord {
    ?                        ExceptionCode;
    ?                        ExceptionFlags;
    ?                        ExceptionRecord;
    ?                        ExceptionAddress;
    ?                        NumberParameters;
    ?                        params;
};

struct EHExceptionRecord::EHParameters {
    ?                        magicNumber;
    ?                        pExceptionObject;
    ?                        pThrowInfo;
};

struct ThrowInfo {
    ?                        attributes;
    ?                        pmfnUnwind;
    ?                        pForwardCompat;
    ?                        pCatchableTypeArray;
};

struct std::length_error {
    ?                        baseclass_0;
};

struct std::logic_error {
    ?                        baseclass_0;
    ?                        _Str;
};

struct std::exception {
    ?                        vftable;
    ?                        _m_what;
    ?                        _m_doFree;
};

struct std::bad_alloc {
    ?                        baseclass_0;
};

struct _EXCEPTION_POINTERS {
    ?                        ExceptionRecord;
    ?                        ContextRecord;
};

struct TranslatorGuardRN {
    ?                        pNext;
    ?                        pFrameHandler;
    ?                        pFuncInfo;
    ?                        pRN;
    ?                        CatchDepth;
    ?                        pMarkerRN;
    ?                        jumpToNode;
    ?                        _ESP;
    ?                        _EBP;
    ?                        DidUnwind;
};

struct _TEB {
    ?                        NtTib;
    ?                        EnvironmentPointer;
    ?                        ClientId;
    ?                        ActiveRpcHandle;
    ?                        ThreadLocalStoragePointer;
    ?                        ProcessEnvironmentBlock;
    ?                        LastErrorValue;
    ?                        CountOfOwnedCriticalSections;
    ?                        CsrClientThread;
    ?                        Win32ThreadInfo;
    ?                        User32Reserved;
    ?                        UserReserved;
    ?                        WOW32Reserved;
    ?                        CurrentLocale;
    ?                        FpSoftwareStatusRegister;
    ?                        SystemReserved1;
    ?                        ExceptionCode;
    ?                        ActivationContextStackPointer;
    ?                        SpareBytes;
    ?                        TxFsContext;
    ?                        GdiTebBatch;
    ?                        RealClientId;
    ?                        GdiCachedProcessHandle;
    ?                        GdiClientPID;
    ?                        GdiClientTID;
    ?                        GdiThreadLocalInfo;
    ?                        Win32ClientInfo;
    ?                        glDispatchTable;
    ?                        glReserved1;
    ?                        glReserved2;
    ?                        glSectionInfo;
    ?                        glSection;
    ?                        glTable;
    ?                        glCurrentRC;
    ?                        glContext;
    ?                        LastStatusValue;
    ?                        StaticUnicodeString;
    ?                        StaticUnicodeBuffer;
    ?                        DeallocationStack;
    ?                        TlsSlots;
    ?                        TlsLinks;
    ?                        Vdm;
    ?                        ReservedForNtRpc;
    ?                        DbgSsReserved;
    ?                        HardErrorMode;
    ?                        Instrumentation;
    ?                        ActivityId;
    ?                        SubProcessTag;
    ?                        EtwLocalData;
    ?                        EtwTraceData;
    ?                        WinSockData;
    ?                        GdiBatchCount;
    ?                        anonymous_0;
    ?                        ReservedPad1;
    ?                        ReservedPad2;
    ?                        IdealProcessor;
    ?                        GuaranteedStackBytes;
    ?                        ReservedForPerf;
    ?                        ReservedForOle;
    ?                        WaitingOnLoaderLock;
    ?                        SavedPriorityState;
    ?                        SoftPatchPtr1;
    ?                        ThreadPoolData;
    ?                        TlsExpansionSlots;
    ?                        MuiGeneration;
    ?                        IsImpersonating;
    ?                        NlsCache;
    ?                        pShimData;
    ?                        HeapVirtualAffinity;
    ?                        CurrentTransactionHandle;
    ?                        ActiveFrame;
    ?                        FlsData;
    ?                        PreferredLanguages;
    ?                        UserPrefLanguages;
    ?                        MergedPrefLanguages;
    ?                        MuiImpersonation;
    ?                        anonymous_1;
    ?                        anonymous_2;
    ?                        TxnScopeEnterCallback;
    ?                        TxnScopeExitCallback;
    ?                        TxnScopeContext;
    ?                        LockCount;
    ?                        SpareUlong0;
    ?                        ResourceRetValue;
};

struct _NT_TIB {
    ?                        ExceptionList;
    ?                        StackBase;
    ?                        StackLimit;
    ?                        SubSystemTib;
    ?                        anonymous_0;
    ?                        ArbitraryUserPointer;
    ?                        Self;
};

struct _NT_TIB::$0349ADB4452EC09BEC08E2292695FBBA {
    ?                        FiberData;
    #D                       Version;
};

struct _CLIENT_ID {
    ?                        UniqueProcess;
    ?                        UniqueThread;
};

struct _GDI_TEB_BATCH {
    ?                        Offset;
    ?                        HDC;
    ?                        Buffer;
};

struct _UNICODE_STRING {
    ?                        Length;
    ?                        MaximumLength;
    ?                        Buffer;
};

struct _LIST_ENTRY {
    ?                        Flink;
    ?                        Blink;
};

struct _TEB::$A3D02A70492DFE9D91413B66511C1D96 {
    ?                        CurrentIdealProcessor;
    ?                        IdealProcessorValue;
    ?                        ReservedPad0;
};

struct _PROCESSOR_NUMBER {
    #e                       Group;
    #o                       Number;
    #o                       Reserved;
};

struct _TEB::$9CF806A5F7AA4F50D4778A9253E08EA3 {
    ?                        CrossTebFlags;
    ?                        anonymous_0;
};

struct _TEB::$9CF806A5F7AA4F50D4778A9253E08EA3::$88D35C6E749BA8930BA8A8A22D5F60D0 {
    ?                        SpareCrossTebBits;
};

struct _TEB::$368A8F43BCCCFC17E5DBBF98016F1166 {
    ?                        SameTebFlags;
    ?                        anonymous_0;
};

struct _TEB::$368A8F43BCCCFC17E5DBBF98016F1166::$DB2E6D00F02C708C0B2EF262B4D055F5 {
    ?                        _bf_0;
};

struct _s_CatchableType {
    ?                        properties;
    ?                        pType;
    ?                        thisDisplacement;
    ?                        sizeOrOffset;
    ?                        copyFunction;
};

struct PMD {
    ?                        mdisp;
    ?                        pdisp;
    ?                        vdisp;
};

struct TMultiPlayerWindow {
    ?                        baseclass_0;
    ?                        GameState;
    ?                        inSessionList;
    ?                        showSplash;
    ?                        currentGame;
    ?                        currentIndex;
    ?                        pSessions;
    ?                        sessTimer;
    ?                        sessionRefreshTimeout;
    ?                        localIPAddress;
    ?                        playerName;
    ?                        hostJoinScreen;
    ?                        splash;
    ?                        hotSeat;
    ?                        ipx;
    ?                        tcp;
    ?                        modem;
    ?                        direct;
    ?                        online;
    ?                        host;
    ?                        join;
    ?                        search;
    ?                        cancel;
    ?                        gameSlider;
    ?                        sessNameHeader;
    ?                        userNameHeader;
    ?                        RolloverWidget;
};

struct CHeroWindowEx {
    ?                        baseclass_0;
    ?                        m_lastIMHoverID;
};

struct _tiddata {
    ?                        _tid;
    ?                        _thandle;
    ?                        _terrno;
    ?                        _tdoserrno;
    ?                        _fpds;
    ?                        _holdrand;
    ?                        _token;
    ?                        _wtoken;
    ?                        _mtoken;
    ?                        _errmsg;
    ?                        _namebuf0;
    ?                        _wnamebuf0;
    ?                        _namebuf1;
    ?                        _wnamebuf1;
    ?                        _asctimebuf;
    ?                        _wasctimebuf;
    ?                        _gmtimebuf;
    ?                        _cvtbuf;
    ?                        _initaddr;
    ?                        _initarg;
    ?                        _pxcptacttab;
    ?                        _tpxcptinfoptrs;
    ?                        _tfpecode;
    ?                        _NLG_dwCode;
    ?                        _terminate;
    ?                        _unexpected;
    ?                        _translator;
    ?                        _curexception;
    ?                        _curcontext;
};

struct errentry {
    ?                        oscode;
    ?                        errnocode;
};

struct _XCPT_ACTION {
    ?                        XcptNum;
    ?                        SigNum;
    ?                        XcptAction;
};

struct type_creature_bank_traits {
    ?                        name;
    ?                        levels;
};

struct type_creature_bank_level {
    ?                        guards;
    ?                        resources;
    ?                        creature_type;
    ?                        creature_amount;
    ?                        chance;
    ?                        upg_chance;
    ?                        treasure_artifacts;
    ?                        minor_artifacts;
    ?                        major_artifacts;
    ?                        relic_artifacts;
};

struct CImmMouse {
    ?                        baseclass_0;
    ?                        m_piApi;
    ?                        m_piDevice;
};

struct CImmDevice {
    ?                        vftable;
    #n                       m_bInitialized;
    #D                       m_dwDeviceType;
    ?                        m_guidDevice;
    #n                       m_bGuidValid;
    #D                       m_dwProductType;
};

struct std::vector_vector__hero__ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_vector__type_artifact__ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_CampaignScenarioInfo_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct SpriteDefHeader {
    ?                        type;
    ?                        width;
    ?                        height;
    ?                        numseqs;
    ?                        pal;
};

struct SpriteDataHeader {
    ?                        seqnumber;
    ?                        numframes;
    ?                        fname;
    ?                        frameOffsets;
};

struct DPCHAT {
    #D                       dwSize;
    #D                       dwFlags;
    ?                        lpszMessageA;
};

struct _DDSURFACEDESC {
    #D                       dwSize;
    #D                       dwFlags;
    #D                       dwHeight;
    #D                       dwWidth;
    ?                        anonymous_0;
    #D                       dwBackBufferCount;
    ?                        anonymous_1;
    #D                       dwAlphaBitDepth;
    #D                       dwReserved;
    ?                        lpSurface;
    ?                        ddckCKDestOverlay;
    ?                        ddckCKDestBlt;
    ?                        ddckCKSrcOverlay;
    ?                        ddckCKSrcBlt;
    ?                        ddpfPixelFormat;
    ?                        ddsCaps;
};

struct $F9D0D49E746EA05C6F8F62A8D439C7A9 {
    #K                       lPitch;
    #D                       dwLinearSize;
};

struct $732C1078520B5FCBD2DC52BA2F31A7C8 {
    #D                       dwMipMapCount;
    #D                       dwZBufferBitDepth;
    #D                       dwRefreshRate;
};

struct IDirectDrawClipper {
    ?                        lpVtbl;
};

struct IDirectDrawSurfaceVtbl {
    ?                        QueryInterface;
    ?                        AddRef;
    ?                        Release;
    ?                        AddAttachedSurface;
    ?                        AddOverlayDirtyRect;
    ?                        Blt;
    ?                        BltBatch;
    ?                        BltFast;
    ?                        DeleteAttachedSurface;
    ?                        EnumAttachedSurfaces;
    ?                        EnumOverlayZOrders;
    ?                        Flip;
    ?                        GetAttachedSurface;
    ?                        GetBltStatus;
    ?                        GetCaps;
    ?                        GetClipper;
    ?                        GetColorKey;
    ?                        GetDC;
    ?                        GetFlipStatus;
    ?                        GetOverlayPosition;
    ?                        GetPalette;
    ?                        GetPixelFormat;
    ?                        GetSurfaceDesc;
    ?                        Initialize;
    ?                        IsLost;
    ?                        Lock;
    ?                        ReleaseDC;
    ?                        Restore;
    ?                        SetClipper;
    ?                        SetColorKey;
    ?                        SetOverlayPosition;
    ?                        SetPalette;
    ?                        Unlock;
    ?                        UpdateOverlay;
    ?                        UpdateOverlayDisplay;
    ?                        UpdateOverlayZOrder;
};

struct IDirectDrawSurface {
    ?                        lpVtbl;
};

struct TCSLock {
    LPCRITICAL_SECTION       m_lpCriticalSection;
};

struct CHSInputDlg {
    ?                        baseclass_0;
    ?                        field1;
    ?                        header1;
    ?                        rollover;
};

struct CPlayerWonMsg {
    ?                        baseclass_0;
    ?                        m_winner;
    ?                        m_victoryConditionStruct;
};

struct CWaitForRemoteBattleDlg {
    ?                        baseclass_0;
    ?                        unknown1;
    ?                        m_pNetMsgHandlerPause;
    ?                        unknown2;
};

struct CNetMsgHandler::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        CheckHandleNet;
    ?                        GetAbortPopupMsg;
    ?                        HandleNetMsg;
    ?                        HandleGiftMsg;
};

struct CNetMsgHandlerPause {
    ?                        baseclass_0;
    ?                        m_pNetMsgHandlerSave;
};

struct heroWindow::vftable_union_t {
    ?                        heroWindow_vftable;
    ?                        TDialogBox_vftable;
    ?                        CTextDialog_vftable;
    ?                        CHeroWindowEx_vftable_t;
    ?                        CAdvPopup_vftable;
    ?                        CAnimatedDlg_vftable;
};

struct fpos_int_ {
    ?                        _Off;
    fpos_t                   _Fpos;
    ?                        _State;
};

struct std::ios {
    ?                        baseclass_0;
};

struct t_stdio_file_adapter {
    ?                        baseclass_0;
    ?                        file;
};

struct combatManager::TArcherTraits {
    ?                        CreatureType;
    ?                        MainBuildingX;
    ?                        MainBuildingY;
    ?                        LowerTowerX;
    ?                        LowerTowerY;
    ?                        UpperTowerX;
    ?                        UpperTowerY;
    ?                        MissileName;
};

struct int64_wrapper_t {
    ?                        value;
};

struct TBuyBuildWindow {
    ?                        baseclass_0;
    ?                        description;
    ?                        id;
};

struct std::basic_ios {
    ?                        baseclass_0;
    ?                        _Sb;
    ?                        _Tiestr;
    ?                        _Fillch;
};

struct std::ios_base {
    ?                        vftable;
    ?                        _State;
    ?                        _Except;
    ?                        _Fmtfl;
    ?                        _Prec;
    ?                        _Wide;
    ?                        _Arr;
    ?                        _Calls;
    ?                        _Loc;
    size_t                   _Stdstr;
};

struct std::ofstream {
    ?                        baseclass_0;
    ?                        _Fb;
};

struct std::ostream {
    ?                        baseclass_0;
    ?                        x_floatused;
};

struct std::strstreambuf {
    ?                        baseclass_0;
    ?                        _Pendsave;
    ?                        _Seekhigh;
    ?                        _Alsize;
    ?                        _Strmode;
    ?                        _Palloc;
    ?                        _Pfree;
};

struct std::ostrstream {
    ?                        baseclass_0;
};

struct t_lod_file_adapter {
    ?                        baseclass_0;
    ?                        lod_file;
};

struct type_AI_puzzle_tile {
    ?                        object_type_bf;
    ?                        object_cords_bf;
    ?                        terr_river_road_bf;
    ?                        diggable_has_grail_visible_bf;
};

struct t_complex_net_message::vftable_t {
    ?                        read;
    ?                        write;
};

struct CGameHeaderInfoMsg {
    ?                        baseclass_0;
    ?                        m_map_entry;
    ?                        m_headerInfo;
};

struct t_map_list_entry {
    ?                        random_map;
    ?                        index;
};

struct std::vector_GameSelectionHeadersStruct_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct CNetPlayerHandler {
    ?                        humanPlayers;
    ?                        computerPlayers;
    ?                        playerPos;
    ?                        playersCount;
    ?                        unused;
    ?                        assignedPos;
};

struct CNetPlayerHandlerPlayer {
    ?                        baseclass_0;
    ?                        heroIndex;
    ?                        townIndex;
    ?                        availableHeroesCount;
    ?                        availableHeroes;
    ?                        startBonusIndex;
    ?                        playerPos;
    ?                        color;
    ?                        handicap;
};

struct CSingleSelectionNetMsgHandler {
    ?                        baseclass_0;
    ?                        flushMessages;
};

struct BITMAPINFOHEADER {
    ?                        biSize;
    ?                        biWidth;
    ?                        biHeight;
    ?                        biPlanes;
    ?                        biBitCount;
    ?                        biCompression;
    ?                        biSizeImage;
    ?                        biXPelsPerMeter;
    ?                        biYPelsPerMeter;
    ?                        biClrUsed;
    ?                        biClrImportant;
};

struct type_artifact_quest {
    ?                        baseclass_0;
    ?                        artifacts;
};

struct std::deque_CNetMsg_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        map;
    ?                        mapsize;
    ?                        size;
};

struct std::deque_CNetMsg_ptr_::iterator {
    ?                        first;
    ?                        last;
    ?                        next;
    ?                        map;
};

struct BinkManager::BinkManagerStruct {
    ?                        bink;
    ?                        bink2;
    ?                        screen;
    ?                        pitch;
    ?                        height;
    ?                        x;
    ?                        y;
    ?                        w;
    ?                        h;
    ?                        id;
    ?                        loop;
    ?                        paused;
};

struct BINKIO {
    ?                        ReadHeader;
    ?                        ReadFrame;
    ?                        GetBufferSize;
    ?                        SetInfo;
    ?                        Idle;
    ?                        Close;
    ?                        bink;
    ?                        ReadError;
    ?                        DoingARead;
    ?                        BytesRead;
    ?                        Working;
    ?                        TotalTime;
    ?                        ForegroundTime;
    ?                        IdleTime;
    ?                        ThreadTime;
    ?                        BufSize;
    ?                        BufHighUsed;
    ?                        CurBufSize;
    ?                        CurBufUsed;
    ?                        iodata;
    ?                        suspend_callback;
    ?                        try_suspend_callback;
    ?                        resume_callback;
    ?                        idle_on_callback;
    ?                        callback_control;
};

struct BUNDLEPOINTERS {
    ?                        typeptr;
    ?                        type16ptr;
    ?                        colorptr;
    ?                        bits2ptr;
    ?                        motionXptr;
    ?                        motionYptr;
    ?                        dctptr;
    ?                        mdctptr;
    ?                        patptr;
};

struct TCampaignBrief {
    ?                        baseclass_0;
    ?                        zBuffer;
    ?                        oldVolume;
    ?                        scenarios;
    ?                        campaign;
    ?                        unknown;
    ?                        selected_scenario;
    ?                        start_bonus_borders;
    ?                        bitmap_bonus_images;
    ?                        sprite_bonus_images;
    ?                        difficulty_buttons;
    ?                        difficulty_decr_button;
    ?                        difficulty_incr_button;
    ?                        scroller;
};

struct CDPlay::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        Init;
    ?                        InitConnection;
    ?                        HostSession;
    ?                        JoinSession;
    ?                        StartSession;
    ?                        CloseSession;
    ?                        CreatePlayer;
    ?                        DestroyPlayer;
    ?                        CreateGroup;
    ?                        DestroyGroup;
    ?                        DeleteGroupFromGroup;
    ?                        SetGroupName;
    ?                        SetGroupData;
    ?                        SetPlayerName;
    ?                        SetPlayerData;
    ?                        GetGroupData;
    ?                        GetGroupName;
    ?                        GetPlayerData;
    ?                        GetPlayerName;
    ?                        CreateGroupInGroup;
    ?                        AddPlayerToGroup;
    ?                        DeletePlayerFromGroup;
    ?                        UpdateSessionDesc;
    ?                        GetCurrSession;
    ?                        EnumConnections;
    ?                        EnumSessions;
    ?                        EnumGroups;
    ?                        EnumPlayers;
    ?                        EnumGroupPlayers;
    ?                        SetGuid;
    ?                        GetGuid;
    ?                        Send;
    ?                        SendChat;
    ?                        Receive;
    ?                        GetErrorDesc;
    ?                        IsHost;
    ?                        FlushReceiveQueue;
    ?                        GetPlayerAddress;
    ?                        GetCaps;
    ?                        GetSendQueueSize;
    ?                        GetReceiveQueueSize;
    ?                        ReceiveMsg;
    ?                        ReceiveSystemMsg;
    ?                        SysMsgAddGroupToGroup;
    ?                        SysMsgAddPlayerToGroup;
    ?                        SysMsgChat;
    ?                        SysMsgDeleteGroupFromGroup;
    ?                        SysMsgDeletePlayerFromGroup;
    ?                        SysMsgSecureMessage;
    ?                        SysMsgSessionLost;
    ?                        SysMsgSetPlayerOrGroupData;
    ?                        SysMsgSetPlayerOrGroupName;
    ?                        SysMsgSetSessionDesc;
    ?                        SysMsgStartSession;
    ?                        SysMsgHost;
    ?                        SysMsgCreatePlayerOrGroup;
    ?                        SysMsgDestroyPlayerOrGroup;
    ?                        AddGroupEnum;
    ?                        AddPlayerEnum;
    ?                        AddSessionEnum;
    ?                        AddConnectionEnum;
};

struct CDPlay::vftable_union_t {
    ?                        CDPlay_vftable;
    ?                        CDPlayLobby_vftable;
    ?                        CDPlayHeroes_vftable;
};

struct CAutoArray_CDPlayPlayer_ {
    ?                        vftable;
    #D                       step;
    ?                        pArray;
    #D                       allocSize;
    #D                       size;
};

struct type_skeleton_window {
    ?                        baseclass_0;
    ?                        rollover_widget;
    ?                        sacrifice_button;
    ?                        all_creatures_button;
    ?                        selected_group;
    ?                        selected_index;
    ?                        selected_creatures;
    ?                        armies;
    ?                        army_widget;
    ?                        select_border;
    ?                        army_label;
    ?                        death_samples;
};

struct type_university_window {
    ?                        baseclass_0;
    ?                        current_hero;
    ?                        purchase_button;
    ?                        purchase_title_widget;
    ?                        purchase_text_widget;
    ?                        rollover_widget;
    ?                        skills;
    ?                        selected_skill;
    ?                        selection_widgets;
    ?                        purchase_widgets;
};

struct type_university_skill {
    ?                        unknown;
};

struct type_university {
    ?                        skills;
};

struct t_custom_campaign_window {
    ?                        baseclass_0;
    ?                        unk1;
};

struct _finddata_t {
    ?                        attrib;
    ?                        time_create;
    ?                        time_access;
    ?                        time_write;
    size_t                   size;
    ?                        name;
};

struct widget::vftable_union_t {
    ?                        widget_vftable;
    ?                        iconWidget_vftable;
    ?                        textWidget_vftable;
    ?                        textEntryWidget_vftable;
    ?                        border_vftable;
    ?                        bitmapBorder_vftable;
    ?                        coloredBorderFrame_vftable;
    ?                        button_vftable;
    ?                        textButton_vftable;
    ?                        type_func_button_vftable;
    ?                        CChatEdit_vftable;
    ?                        bitmapBackedTextWidget_vftable;
    ?                        CGameChatEdit_vftable;
    ?                        slider_vftable;
};

struct THeroScreenWindow {
    ?                        baseclass_0;
    ?                        heroLocatorIndex;
    ?                        heroLocatorWidget;
};

struct CombinationArtifact {
    ?                        type;
    ?                        requirements;
};

struct textButton {
    ?                        baseclass_0;
    ?                        Font;
    ?                        textColor;
    ?                        textImage;
};

struct textWidget::vftable_t {
    ?                        baseclass_0;
    ?                        SetText;
};

struct CChatEdit::vftable_t {
    ?                        baseclass_0;
    ?                        UpdateScreen;
    ?                        OnEnter;
    ?                        OnEscape;
    ?                        OnFunctionKey;
    ?                        IsOpen;
    ?                        SendChat;
};

struct textEntryWidget::vftable_t {
    ?                        baseclass_0;
    ?                        SetFocus;
    ?                        OnKeyPress;
    ?                        IgnoreKey;
    ?                        SetAutoDraw;
    ?                        SaveBackground;
};

struct TSubWindow::vftable_union_t {
    ?                        TSubWindow_vftable;
    ?                        type_bottom_view_window_vftable;
};

struct type_bottom_view_window::vftable_t {
    ?                        baseclass_0;
    ?                        animate;
};

struct type_dialog_icon {
    ?                        resource;
    ?                        qualifier;
    ?                        spriteName;
    ?                        text;
    ?                        spriteFrameIndex;
    #b                       spritePos;
    ?                        spriteHeight;
    ?                        spriteWidth;
    #b                       textPos;
    ?                        textHeight;
    ?                        textWidth;
};

struct CWaitForReadyPlayersDlg {
    ?                        baseclass_0;
    ?                        startTime;
    ?                        lastMsg;
    ?                        m_netMsgHandler;
    ?                        playerReady;
};

struct TTownMenu {
    ?                        baseclass_0;
};

struct CTownNetMsgHandler {
    ?                        baseclass_0;
};

struct type_artifact_effect::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        get_value;
};

struct std::vector_type_artifact_effect_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct type_resource_quest {
    ?                        baseclass_0;
    ?                        resources;
};

struct std::list_TPoint_::_Node {
    ?                        _Next;
    ?                        _Prev;
    ?                        _Value;
};

struct std::list_TPoint_::iterator {
    ?                        _Ptr;
};

struct std::list_TPoint_ {
    ?                        allocator;
    ?                        _Head;
    ?                        _Size;
};

struct std::map_int_HeroPlayerInfo_::_Node {
    ?                        _Left;
    ?                        _Parent;
    ?                        _Right;
    ?                        _Value;
    ?                        _Color;
};

struct std::pair_int_HeroPlayerInfo_ {
    ?                        first;
    ?                        second;
};

struct HeroPlayerInfo {
    ?                        baseclass_0;
    ?                        players;
};

struct HeroIdentity {
    ?                        portrait;
    ?                        name;
};

struct std::vector_garrison_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_boat_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_type_university_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_game__TRumour_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_QuestMonster_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_type_point_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TownExtra_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct CGameChatEdit::vftable_t {
    ?                        baseclass_0;
    ?                        SendChatCleanup;
    ?                        Activate;
};

struct TCheatCode {
    ?                        code;
};

struct TArtifactSlotTraits {
    ?                        name;
    ?                        type;
};

struct std::bitset_19_ {
    ?                        bits;
};

struct TAutoStrPtr {
    ?                        data;
};

struct std::bitset_9_ {
    ?                        bits;
};

struct type_creature_source {
    ?                        type;
    ?                        ptr;
    ?                        number;
    ?                        is_free;
};

struct std::vector_HeroDestination_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct TObjectTypeTable {
    ?                        objectTypes;
};

struct std::vector_TObjectType_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct TBottomViewMessage {
    ?                        baseclass_0;
};

struct type_bottom_view_window {
    ?                        baseclass_0;
};

struct CHourGlass {
    ?                        m_thread;
};

struct THeroSpecificAbilityUnion {
    ?                        creature;
    ?                        skill;
    ?                        resource;
    ?                        spell;
};

struct TCombatOptionsWindow {
    ?                        baseclass_0;
    ?                        bPrefsChanged;
    ?                        RolloverWidget;
};

struct t_start_building_bonus {
    ?                        baseclass_0;
    ?                        town;
    ?                        building;
};

struct t_start_bonus {
    ?                        vftable;
};

struct std::array_int_7_ {
    ?                        _data;
};

struct ExtraObjectProperties {
    ?                        impassable;
    ?                        omnidirectional;
    ?                        removable;
    ?                        name;
    ?                        type;
    ?                        decorative;
};

struct std::vector_CObject_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_CSprite_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TreasureData_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_MonsterData_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_BlackBoxData_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TSeerHut_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TQuestGuard_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TTimedEvent_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TTownEvent_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_HeroPlaceholder_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_type_quest_ptr_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct std::vector_TRandomDwelling_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct SeerHutText {
    ?                        quests;
    ?                        seerHut;
    ?                        questGuard;
};

struct QuestText {
    ?                        quest;
    ?                        progress;
    ?                        complete;
    ?                        rollover;
    ?                        log;
};

struct type_garrison_purchaser {
    ?                        baseclass_0;
};

struct slider::vftable_t {
    ?                        baseclass_0;
    ?                        SetResolution;
    ?                        SetState;
    ?                        UpdateResolution;
    ?                        Refresh;
};

struct type_text_slider {
    ?                        baseclass_0;
    ?                        scroller;
};

struct std::vector_CampaignScenarioPreview_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct CampaignScenarioPreview {
    ?                        baseclass_0;
    ?                        game_setup;
    ?                        available;
};

struct std::vector_hero_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct LODResourceFiles {
    ?                        spriteLOD;
    ?                        bitmapLOD;
    ?                        soundLOD;
};

struct LODFileDescriptor {
    ?                        index;
    ?                        data;
};

struct TRandomDwelling {
    ?                        townId;
    ?                        towns;
    ?                        playerOwner;
    ?                        minLVL;
    ?                        maxLVL;
    ?                        gap_9;
    ?                        object;
};

struct CScenarioInfoDlg {
    ?                        baseclass_0;
    ?                        VictoryIcon;
    ?                        LossIcon;
    ?                        TownPix;
    ?                        bonusSprite;
    ?                        Panels;
    ?                        Flags;
    ?                        heroSpecificAbility;
};

struct CAnimatedDlg::vftable_t {
    ?                        baseclass_0;
    ?                        CalcDimensions;
    ?                        Setup;
};

struct CTextDialog::vftable_t {
    ?                        baseclass_0;
    ?                        Setup;
    ?                        UpdateText;
    ?                        CalcDimensions;
};

struct TDialogBox::vftable_t {
    ?                        baseclass_0;
    ?                        Setup;
};

struct type_map_creation_bar {
    ?                        baseclass_0;
    ?                        widgets;
    ?                        parent;
    ?                        progress_sprite;
    ?                        num_progress_sprites;
};

struct type_progress_bar {
    ?                        vftable;
    ?                        maximum;
    ?                        value;
};

struct type_progress_bar::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        set_maximum;
    ?                        increment;
};

struct std::set_SpellID_::_Node {
    ?                        _Left;
    ?                        _Parent;
    ?                        _Right;
    ?                        _Value;
    ?                        _Color;
};

struct TSeerReward::SeerRewardUnion {
    ?                        ExperienceBonus;
    ?                        ManaBonus;
    ?                        MoraleBonus;
    ?                        LuckBonus;
    ?                        ResourceReward;
    ?                        PrimarySkillReward;
    ?                        SecondarySkillReward;
    ?                        ArtifactReward;
    ?                        SpellReward;
    ?                        CreatureReward;
};

struct TSeerResourceReward {
    ?                        resType;
    ?                        resQty;
};

struct TSeerPrimarySkillReward {
    ?                        skillType;
    ?                        bonus;
    ?                        gap5;
};

struct SecondarySkillData {
    ?                        type;
    ?                        level;
};

struct TCreatureStack {
    ?                        Creature;
    ?                        numTroops;
    ?                        gap6;
};

struct SoundHeaderDescriptor {
    ?                        sounds;
    ?                        numSound;
    ?                        fileHandle;
};

struct SoundHeaders {
    ?                        SoundHeader;
    ?                        SoundHeaderCD;
    ?                        SoundHeaderAB;
};

struct SoundHeaderStruct {
    ?                        filename;
    ?                        offset;
    ?                        size;
};

struct combatManager::TWallTraits {
    ?                        x;
    ?                        y;
    ?                        hex;
    ?                        filenames;
    ?                        name;
    ?                        hitpoints;
};

struct CTimer {
    ?                        startTime;
    ?                        stopTime;
    ?                        elapsedTime;
    ?                        _IsRunning;
    ?                        enabled;
};

struct TPickRandomTownName {
    ?                        baseclass_0;
};

struct CNewPlayerUpdateProc {
    ?                        vftable;
    ?                        dpid;
    ?                        numSent;
    ?                        map_list;
    ?                        sentTime;
    ?                        finished;
};

struct std::vector_t_map_list_entry_ {
    ?                        allocator;
    ?                        first;
    ?                        last;
    ?                        end;
};

struct CHotSeatDlg {
    ?                        baseclass_0;
    ?                        edit;
    ?                        m_rollover;
    ?                        gHotSeatHelp;
};

struct t_scenario_start_options::vftable_t {
    ?                        scalar_deleting_destructor;
    ?                        is_image_bitmap;
    ?                        get_options_amount;
    ?                        get_image_name;
    ?                        get_frame;
    ?                        get_linked_scenario;
    ?                        get_option_description;
    ?                        get_option_hero;
    ?                        get_option_player;
    ?                        read;
    ?                        to_scenario;
    ?                        from_header;
    ?                        is_transferable;
};

struct TCombatHeroSubWindow {
    ?                        baseclass_0;
    ?                        background;
    ?                        portrait;
    ?                        attackText;
    ?                        defenseText;
    ?                        powerText;
    ?                        knowledgeText;
    ?                        moraleIcon;
    ?                        luckIcon;
    ?                        manaText;
    ?                        shown;
};

struct _DDPIXELFORMAT {
    #D                       dwSize;
    #D                       dwFlags;
    #D                       dwFourCC;
    ?                        anonymous_0;
    ?                        anonymous_1;
    ?                        anonymous_2;
    ?                        anonymous_3;
    ?                        anonymous_4;
};

struct AnimHeaderStruct {
    ?                        filename;
    ?                        offset;
};

struct resource::vftable_union_t {
    ?                        resource_vftable;
    ?                        Bitmap816_vftable;
};

struct Bitmap816::vftable_t {
    ?                        baseclass_0;
    ?                        zBufferDraw;
};

struct townObjectProperties {
    ?                        frames;
    ?                        x;
    ?                        y;
};

struct CampaignScenarioInfo {
    ?                        completed;
    ?                        days;
    ?                        score;
    ?                        index;
    ?                        complete_order;
};

