// CATALOG: C5
// PHENOMENON: the construction phase is observable in the store schedule.
//   A member cleared in the MEMBER-INITIALIZER LIST is stored BEFORE the
//   class's own vptr install; the same assignment written in the ctor BODY
//   schedules AFTER the vptr store (soundManager::soundManager EXACT via
//   MP3Playing in the init list; inputManager's buffer clear before the
//   vptr proving nontrivial-member construction).
// FLAGS: /O2 /Ob2 /Oy- /Op /ML /Gr /GX /GR- /D_WINDOWS
// EXPECT-ASM(0managerA@@): mov\s+BYTE PTR \[eax\+4\], 0[\s\S]{0,120}\?\?_7managerA@@6B@
// EXPECT-ASM(0managerB@@): \?\?_7managerB@@6B@[\s\S]{0,120}mov\s+BYTE PTR \[eax\+4\], 0
struct managerA {
    virtual int Serve();
    unsigned char playing;
    managerA();
};
managerA::managerA() : playing(0) {}
struct managerB {
    virtual int Serve();
    unsigned char playing;
    managerB();
};
managerB::managerB() { playing = 0; }
