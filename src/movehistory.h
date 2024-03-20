#ifndef MOVEHISTORY_H_INCLUDED
#define MOVEHISTORY_H_INCLUDED

#include "chess.h"
#include "position.h"

namespace Belette {

class MoveHistory {
public:
    MoveHistory(): counterMoves{MOVE_NONE}, killerMoves{MOVE_NONE} { }

    inline void clearKillers(int ply) {
        assert(ply >= 0 && ply < MAX_PLY);
        killerMoves[ply][0] = killerMoves[ply][1] = MOVE_NONE;
    }

    template<int K> inline Move getKiller(int ply) const {
        assert(ply >= 0 && ply < MAX_PLY);
        static_assert(K == 0 || K == 1);
        return killerMoves[ply][K];
    }

    inline Move getCounter(const Position& pos) const {
        Move prevMove = pos.previousMove();
        if (!isValidMove(prevMove)) return MOVE_NONE;

        return counterMoves[pos.getPieceAt(moveTo(prevMove))][moveTo(prevMove)];
    }

    inline void update(const Position& pos, Move bestMove, int ply) {
        if (!pos.isTactical(bestMove)) {
            updateKiller(bestMove, ply);
            updateCounter(pos, bestMove);
        }
    }
private:
    Move counterMoves[NB_PIECE][NB_SQUARE];
    Move killerMoves[MAX_PLY][2];
    //Move history[NB_SIDE][NB_SQUARE*NB_SQUARE];

    inline void updateKiller(Move move, int ply) {
        assert(ply >= 0 && ply < MAX_PLY);

        if (killerMoves[ply][0] != move) {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = move;
        }
    }

    inline void updateCounter(const Position& pos, Move move) {
        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove))
            counterMoves[pos.getPieceAt(moveTo(prevMove))][moveTo(prevMove)] = move;
    }
};

} /* namespace Belette */

#endif /* MOVEHISTORY_H_INCLUDED */
