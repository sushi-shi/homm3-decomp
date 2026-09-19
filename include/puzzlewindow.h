// puzzlewindow.h - puzzlewindow.cpp (compiland puzzlewindow.obj)
#ifndef HOMM3_PUZZLEWINDOW_H
#define HOMM3_PUZZLEWINDOW_H

#include <bitset>
#include "advmgr_popup.h"
#include "struct.h"

struct type_point;

class Bitmap816;
class NewmapCell;
class TResourceDisplay;

// DC's 0x58-byte CAdvPopup grows to retail's proven 0x60-byte base. The
// remaining fields translate directly: one piece-count byte, the resource
// display pointer, 48 puzzle bitmaps, and the selected puzzle index.
class TPuzzleWindow : public CAdvPopup {
public:
    enum {
        ACCEPT_ID = 0x7802,
        DIALOG_CLOSE_KEY = 1,
        DIALOG_ACCEPT_KEY = 28,
        BACKGROUND_ID = 200,
        ROLLOVER_ID = 201,
        NWIDGETS = 5,
        // UpdatePuzzle's return domain: the count of pieces still down.
        // 48 = the full board (puzzlePieces' extent) - ViewPuzzle skips
        // the reveal fizzle when every piece is already removed.
        PUZZLE_PIECE_COUNT = 48
    };

    char m_numPieces;
    // The preceding byte field and following four-byte field establish
    // this alignment gap; the reference layout retains the same boundary.
    char m_paddingBeforePuzzleResourceBar[3];

    TPuzzleWindow(int puzzlenum);
    virtual ~TPuzzleWindow();
    int updatePuzzle(int full);
    virtual int windowHandler(message& msg);

private:
    TResourceDisplay* m_puzzleResourceBar;
    Bitmap816* m_puzzlePieces[48];
    int m_puzWhich;

    int convertID2HelpID(int id) const;
};
SIZE(TPuzzleWindow, 0x12c);

extern std::bitset<48> g_puzzlePiecesRemoved;
extern short g_puzzlePieceOrder[];
// Retail UpdatePuzzle (0x52c6c0) and AI_attempt_puzzle_guess (0x52c9b0)
// read signed words at 2 * (puzzle * 96 + piece). DC UpdatePuzzle also
// reads word coordinates (its scaling differs); these are short tables,
// not byte buffers requiring pointer reinterpretation.
extern short g_puzzlePieceX[];
extern short g_puzzlePieceY[];
extern const char* g_puzzleFilePrefixes[];
// 0x6822c8: five doubles - 1.1, 0.5, 0.25, 0.0, 0.0 - read from the
// image, indexed by SGameSetupOptions::difficulty and compared against
// the fraction of the puzzle the AI has uncovered. 1.1 on the easiest
// setting is unreachable, i.e. that AI never guesses. NAME PROVISIONAL:
// nothing attests it, the table sits immediately below this TU's string
// pool and only AI_attempt_puzzle_guess reads it.
extern double g_puzzleGuessThreshold[];
// --- globals ---
// Retail 0x52c9b0; CODEVIEW(E:\gamedcs\puzzlewindow.cpp:614, dc 0x115f64).
// The explicit Dreamcast return-buffer marker is represented by C++'s normal
// by-value return. Retail's call from playerData::guess_grail_location has the
// same hidden-result-pointer-in-ECX / player-in-EDX convention.
type_point aiAttemptPuzzleGuess(long player);

#endif  /* HOMM3_PUZZLEWINDOW_H */
