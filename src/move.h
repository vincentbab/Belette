#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include "chess.h"
#include "position.h"
#include "fixed_vector.h"

namespace BabChess {

typedef fixed_vector<Move, MAX_MOVE, uint8_t> MoveList;

enum MoveGenType {
    QUIET_MOVES = 1,
    NON_QUIET_MOVES = 2,
    ALL_MOVES = QUIET_MOVES | NON_QUIET_MOVES,
};

template<Side Me, MoveGenType MGType = ALL_MOVES>
void generateLegalMoves(const Position &pos, MoveList &moves);

template<MoveGenType MGType = ALL_MOVES>
inline void generateLegalMoves(const Position &pos, MoveList &moves) {
    pos.getSideToMove() == WHITE ? generateLegalMoves<WHITE, MGType>(pos, moves) : generateLegalMoves<BLACK, MGType>(pos, moves);
    /*if (pos.getSideToMove() == WHITE) {
        generateLegalMoves<WHITE, NON_QUIET_MOVES>(pos, moves);
        generateLegalMoves<WHITE, QUIET_MOVES>(pos, moves);
    } else {
        generateLegalMoves<BLACK, NON_QUIET_MOVES>(pos, moves);
        generateLegalMoves<BLACK, QUIET_MOVES>(pos, moves);
    }*/
}

} /* namespace BabChess */

#endif /* MOVE_H_INCLUDED */
