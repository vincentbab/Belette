#ifndef MOVEHISTORY_H_INCLUDED
#define MOVEHISTORY_H_INCLUDED

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include "chess.h"
#include "position.h"
#include "fixed_vector.h"

namespace Belette {

using MoveScore = int32_t;

using PartialMoveList = fixed_vector<Move, 32, uint8_t>;

// Indexed by [piece][to] of the current move
using PieceToHistory = std::array<std::array<MoveScore, NB_SQUARE>, NB_PIECE>;

// Indexed by [piece][to] of a previous move
using ContinuationHistory = std::array<std::array<PieceToHistory, NB_SQUARE>, NB_PIECE>;

constexpr int CONT_HIST_PLIES = 2;

constexpr int CORR_HIST_SIZE = 16384;
constexpr MoveScore CORR_HIST_GRAIN = 256;
constexpr MoveScore CORR_HIST_LIMIT = 32 * CORR_HIST_GRAIN;

class MoveHistory {
public:
    MoveHistory(): counterMoves{}, killerMoves{}, history{}, corrHist{},
        continuationHistory(std::make_unique<ContinuationHistory>()) { }

    inline void clear() {
        std::memset(counterMoves, 0, sizeof(counterMoves));
        std::memset(killerMoves, 0, sizeof(killerMoves));
        std::memset(history, 0, sizeof(history));
        std::memset(corrHist, 0, sizeof(corrHist));
        std::memset(continuationHistory.get(), 0, sizeof(ContinuationHistory));
    }

    inline void clearAllKillers() {
        std::memset(killerMoves, 0, sizeof(killerMoves));
    }

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
    inline MoveScore getHistory(const Position& pos, Move m, const PieceToHistory* const* contHist) const {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);

        MoveScore score = 2 * history[Me][moveFromTo(m)];
        for (int i = 0; i < CONT_HIST_PLIES; i++)
            score += (*contHist[i])[pc][to];

        return score;
    }

    inline PieceToHistory* getContHistEntry(Piece pc, Square to) {
        return &(*continuationHistory)[pc][to];
    }

    inline PieceToHistory* getContHistEntry(const Position& pos, Move m) {
        return getContHistEntry(pos.getPieceAt(moveFrom(m)), moveTo(m));
    }

    // Never indexed by a real move because NO_PIECE.
    inline PieceToHistory* getDefaultContHist() {
        return getContHistEntry(NO_PIECE, SQ_FIRST);
    }

    template<Side Me>
    inline Score correctEval(const Position& pos, Score eval) const {
        MoveScore correction = corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)];
        return std::clamp<Score>(eval + correction / CORR_HIST_GRAIN, -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
    }

    template<Side Me>
    inline void updateCorrection(const Position& pos, Score bestScore, Score staticEval, int depth) {
        MoveScore& entry = corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)];
        MoveScore diff = (bestScore - staticEval) * CORR_HIST_GRAIN;
        MoveScore weight = std::min(depth + 1, 16);

        entry = (entry * (256 - weight) + diff * weight) / 256;
        entry = std::clamp(entry, -CORR_HIST_LIMIT, CORR_HIST_LIMIT);
    }

    template<Side Me>
    inline void update(const Position& pos, Move bestMove, int ply, int depth, const PartialMoveList& quietMoves,
                       PieceToHistory* const* contHist) {
        if (!pos.isTactical(bestMove)) {
            updateKiller(bestMove, ply);
            updateCounter(pos, bestMove);

            MoveScore bonus = historyBonus(depth);
            updateQuiet<Me>(pos, bestMove, bonus, contHist);

            for (auto m : quietMoves) {
                updateQuiet<Me>(pos, m, -bonus, contHist);
            }
        }
    }
private:
    Move counterMoves[NB_PIECE][NB_SQUARE];
    Move killerMoves[MAX_PLY+1][2];
    MoveScore history[NB_SIDE][NB_SQUARE*NB_SQUARE];
    MoveScore corrHist[NB_SIDE][CORR_HIST_SIZE];
    std::unique_ptr<ContinuationHistory> continuationHistory;

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

    template<Side Me>
    inline void updateQuiet(const Position& pos, Move m, MoveScore bonus, PieceToHistory* const* contHist) {
        updateHistoryEntry(history[Me][moveFromTo(m)], bonus);

        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);
        for (int i = 0; i < CONT_HIST_PLIES; i++)
            if (contHist[i] != getDefaultContHist())
                updateHistoryEntry((*contHist[i])[pc][to], bonus);
    }
};

} /* namespace Belette */

#endif /* MOVEHISTORY_H_INCLUDED */
