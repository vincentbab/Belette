#ifndef MOVEHISTORY_H_INCLUDED
#define MOVEHISTORY_H_INCLUDED

#include <cstdint>
#include "chess.h"
#include "position.h"
#include "fixed_vector.h"

namespace Belette {

using MoveScore = int32_t;

using PartialMoveList = fixed_vector<Move, 32, uint8_t>;

class MoveHistory {
public:
    MoveHistory(): counterMoves{}, killerMoves{}, history{} { }

    inline void clearKillers(int ply) {
        assert(ply >= 0 && ply < MAX_PLY + 1);
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

    template<Side Me>
    inline MoveScore getHistory(Move m) const {
        return history[Me][moveFromTo(m)];
    }

    template<Side Me>
    inline void update(const Position& pos, Move bestMove, int ply, int depth, const PartialMoveList& quietMoves) {
        if (!pos.isTactical(bestMove)) {
            updateKiller(bestMove, ply);
            updateCounter(pos, bestMove);

            MoveScore bonus = historyBonus(depth);
            updateHistoryEntry(history[Me][moveFromTo(bestMove)], bonus);

            for (auto m : quietMoves) {
                updateHistoryEntry(history[Me][moveFromTo(m)], -bonus);
            }
        }
    }
private:
    Move counterMoves[NB_PIECE][NB_SQUARE];
    Move killerMoves[MAX_PLY+1][2];
    MoveScore history[NB_SIDE][NB_SQUARE*NB_SQUARE];

    inline MoveScore historyBonus(int depth) {
        return std::min(1536, 8*depth*depth);
    }

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

    inline void updateHistoryEntry(MoveScore &entry, MoveScore bonus) {
        entry += bonus - entry * std::abs(bonus) / 8192;
    }
};

} /* namespace Belette */

#endif /* MOVEHISTORY_H_INCLUDED */
