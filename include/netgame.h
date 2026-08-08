// netgame.h - the multiplayer-protocol selector.
//
// `eNetGameType` is a Dreamcast LF_ENUM (evidence/dreamcast/enums.csv),
// and the DC roster also names the global that carries it:
// ?iMPNetProtocol@@3W4eNetGameType@@A, dc 0xcc2c. On retail that global
// is .bss 0x6989f0, byte-identified in game.obj - game::GetLocalPlayer,
// game::GetLocalPlayerGamePos and game::IsMultiplayer all branch on
// `== 3`, and 3 is MP_HOTSEAT, which is exactly the mode where "the
// local player" is whoever's turn it is rather than a fixed slot.
//
// Its real declaring header is a remote/net one (the DC attributes
// InitRemote(eNetGameType, const char*) to remote.obj); that TU is not
// located, so the enum gets its own domain header here rather than
// riding into anyone's include closure uninvited.
#ifndef HOMM3_NETGAME_H
#define HOMM3_NETGAME_H

enum eNetGameType {
    MP_SINGLE = 0,
    MP_IPX = 1,
    MP_TCP = 2,
    MP_HOTSEAT = 3,
    MP_SERIAL = 4,
    MP_MODEM = 5
};

// .bss 0x6989f0. Defined by the TU that owns the network setup (not
// located) - extern only, no DATA claim, the bitNumber pattern.
extern eNetGameType iMPNetProtocol;

#endif /* HOMM3_NETGAME_H */
