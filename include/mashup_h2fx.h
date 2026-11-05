#ifndef MASHUP_H2FX_H
#define MASHUP_H2FX_H

// HoMM2's procedural combat spell visuals (its SPELLS.cpp DoBolt and
// Armageddon), ported onto the HoMM3 battlefield for the mashup (not part
// of the retail game). Both draw over whatever is on the combat screen when
// called, present each frame themselves, and return with the battlefield
// redrawn clean. They do nothing in quick combat.

class combatManager;

// mashupH2Bolt's chainLink for the Lightning Bolt spell (HoMM2 spell 2),
// which HoMM2 draws with different parameters from any Chain Lightning
// strike: a forking bolt with the screen flash held until the frame timer.
enum {
    MASHUP_H2_LIGHTNING_BOLT = -1
};

// Draws HoMM2's lightning bolt from (startX, startY) to (endX, endY), in
// HoMM3 screen coordinates, with HoMM2's timing, then redraws the
// battlefield clean.
//
// chainLink selects HoMM2's call:
//   MASHUP_H2_LIGHTNING_BOLT  Lightning Bolt. The bolt stays up until the
//       combat frame timer (g_timers[0]) set by the caller's last
//       drawFrame runs out, as HoMM2's DrawFrame(waitForTimer) does, so
//       call it right after the cast animation's last frame.
//   0, 1, 2, ...  Chain Lightning strike n: 0 is the hero's hand to the
//       first target, n the jump from target n-1 to target n (HoMM2 strikes
//       up to four stacks). Each strike stays up for HoMM2's 100 ms.
void mashupH2Bolt(combatManager* cm, int startX, int startY, int endX, int endY,
                  int chainLink);

// HoMM2's Armageddon: the battlefield flashes white, shakes while it turns
// red, then the colours return to normal over the clean battlefield.
void mashupH2Armageddon(combatManager* cm);

#endif  // MASHUP_H2FX_H
