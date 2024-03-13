#ifndef ZOBRIST_H_INCLUDED
#define ZOBRIST_H_INCLUDED

#include "chess.h"

namespace Belette {

namespace Zobrist {
    extern Bitboard keys[NB_PIECE][NB_SQUARE];
    extern Bitboard enpassantKeys[NB_FILE+1];
    extern Bitboard castlingKeys[NB_CASTLING_RIGHT];
    extern Bitboard sideToMoveKey;

    void init();
}

} /* namespace Belette */

#endif /* ZOBRIST_H_INCLUDED */
