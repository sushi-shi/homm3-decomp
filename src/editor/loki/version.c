/* version.c - the editor's build banner (Loki h3maped).
 *
 * The image links one more GCC 2.95.2 object after the 103 project objects
 * and before libglade: it has no .text and no .eh_frame CIE, only this
 * array in .data (32-byte aligned at 0x08420c20, 0x77 bytes) and its
 * .comment entry, and -export-dynamic lists `game_version` in .dynsym. The
 * text names the compile flags of Loki's build. The file name is not
 * proven.
 */
char game_version[] = "Heroes of Might and Magic III Map Editor Linux/GTK+ 1.0-i686\n"
                      "Built with flags: \n"
                      "  [ -DUNIX -DCHAR_BIT=8 -DTOOLBUILD ]\n";
