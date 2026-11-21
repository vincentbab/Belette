
#include <cstdint>
#include "zobrist.h"
#include "utils.h"


namespace Belette {

namespace Zobrist {
    Bitboard keys[NB_PIECE][NB_SQUARE];
    Bitboard enpassantKeys[NB_FILE+1];
    Bitboard castlingKeys[NB_CASTLING_RIGHT];
    Bitboard sideToMoveKey;

    void init() {
        FastRandGenerator rng;

        for (int i=0; i<NB_PIECE; i++) {
            for (int j=0; j<NB_SQUARE; j++) {
                keys[i][j] = rng.rand();
            }
        }

        for (int i=0; i<NB_CASTLING_RIGHT; i++) {
            castlingKeys[i] = rng.rand();
        }

        for (int i=0; i<NB_FILE; i++) {
            enpassantKeys[i] = rng.rand();
        }
        enpassantKeys[NB_FILE] = 0; // used to avoid branching in doMove()

        sideToMoveKey = rng.rand();
    }
}

} /* namespace Belette */
