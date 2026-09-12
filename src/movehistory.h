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
using HistoryScore = int16_t;

using PartialMoveList = fixed_vector<Move, 32, uint8_t>;

// Indexed by [piece][to] of the current move
using PieceToHistory = std::array<std::array<HistoryScore, NB_SQUARE>, NB_PIECE>;

// Indexed by [piece][to] of a previous move
using ContinuationHistory = std::array<std::array<PieceToHistory, NB_SQUARE>, NB_PIECE>;

constexpr int CONT_HIST_PLIES = 2;
constexpr int QSEARCH_CONT_HIST_PLIES = 1;

constexpr MoveScore MAIN_HIST_LIMIT = 8192;
constexpr MoveScore CAPTURE_HIST_LIMIT = 8192;
constexpr MoveScore CONT_HIST_LIMIT = 8192;

constexpr int CORR_HIST_SIZE = 16384;
constexpr MoveScore CORR_HIST_GRAIN = 256;
constexpr MoveScore CORR_HIST_LIMIT = 32 * CORR_HIST_GRAIN;

using NonPawnCorrHist = std::array<std::array<std::array<HistoryScore, CORR_HIST_SIZE>, NB_SIDE>, NB_SIDE>;
using MinorCorrHist = std::array<std::array<HistoryScore, CORR_HIST_SIZE>, NB_SIDE>;

class MoveHistory {
public:
    MoveHistory(): counterMoves{}, killerMoves{}, history{}, captureHistory{}, corrHist{},
        nonPawnCorrHist(std::make_unique<NonPawnCorrHist>()),
        minorCorrHist(std::make_unique<MinorCorrHist>()),
        contCorrHist(std::make_unique<ContinuationHistory>()),
        continuationHistory(std::make_unique<ContinuationHistory>()) { }

    inline void clear() {
        std::memset(counterMoves, 0, sizeof(counterMoves));
        std::memset(killerMoves, 0, sizeof(killerMoves));
        std::memset(history, 0, sizeof(history));
        std::memset(captureHistory, 0, sizeof(captureHistory));
        std::memset(corrHist, 0, sizeof(corrHist));
        std::memset(nonPawnCorrHist.get(), 0, sizeof(NonPawnCorrHist));
        std::memset(minorCorrHist.get(), 0, sizeof(MinorCorrHist));
        std::memset(contCorrHist.get(), 0, sizeof(ContinuationHistory));
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
    inline MoveScore getQuietOrderingHistory(const Position& pos, Move m, const PieceToHistory* const* contHist) const {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);

        MoveScore score = 2 * history[Me][moveFromTo(m)];
        for (int i = 0; i < CONT_HIST_PLIES; i++)
            score += (*contHist[i])[pc][to];

        return score;
    }

    template<Side Me, int NbContHist>
    inline MoveScore getEvasionQuietOrderingHistory(const Position& pos, Move m, const PieceToHistory* const* contHist) const {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);

        MoveScore score = 2 * history[Me][moveFromTo(m)];
        for (int i = 0; i < NbContHist; i++)
            score += (*contHist[i])[pc][to];

        return score;
    }

    inline MoveScore getCaptureHistory(const Position& pos, Move m) const {
        return captureHistory[pos.getPieceAt(moveFrom(m))][moveTo(m)][capturedType(pos, m)];
    }

    template<Side Me>
    inline MoveScore getStatScore(const Position& pos, Move m, bool isTactical, const PieceToHistory* const* contHist) const {
        if (isTactical)
            return getCaptureHistory(pos, m);

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

    inline PieceToHistory* getContCorrEntry(const Position& pos, Move m) {
        return &(*contCorrHist)[pos.getPieceAt(moveFrom(m))][moveTo(m)];
    }

    // Never indexed by a real move because NO_PIECE.
    inline PieceToHistory* getDefaultContCorr() {
        return &(*contCorrHist)[NO_PIECE][SQ_FIRST];
    }

    template<Side Me>
    inline Score correctEval(const Position& pos, Score eval, const PieceToHistory* contCorr) const {
        MoveScore correction = corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)]
                             + (*nonPawnCorrHist)[Me][WHITE][pos.nonPawnHash(WHITE) & (CORR_HIST_SIZE - 1)]
                             + (*nonPawnCorrHist)[Me][BLACK][pos.nonPawnHash(BLACK) & (CORR_HIST_SIZE - 1)]
                             + (*minorCorrHist)[Me][pos.minorHash() & (CORR_HIST_SIZE - 1)];

        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove))
            correction += (*contCorr)[pos.getPieceAt(moveTo(prevMove))][moveTo(prevMove)];

        return std::clamp<Score>(eval + correction / CORR_HIST_GRAIN, -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
    }

    template<Side Me>
    inline void updateCorrection(const Position& pos, Score bestScore, Score staticEval, int depth, PieceToHistory* contCorr) {
        MoveScore diff = (bestScore - staticEval) * CORR_HIST_GRAIN;
        MoveScore weight = std::min(depth + 1, 16);

        updateCorrEntry(corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)], diff, weight);
        updateCorrEntry((*nonPawnCorrHist)[Me][WHITE][pos.nonPawnHash(WHITE) & (CORR_HIST_SIZE - 1)], diff, weight);
        updateCorrEntry((*nonPawnCorrHist)[Me][BLACK][pos.nonPawnHash(BLACK) & (CORR_HIST_SIZE - 1)], diff, weight);
        updateCorrEntry((*minorCorrHist)[Me][pos.minorHash() & (CORR_HIST_SIZE - 1)], diff, weight);

        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove) && contCorr != getDefaultContCorr())
            updateCorrEntry((*contCorr)[pos.getPieceAt(moveTo(prevMove))][moveTo(prevMove)], diff, weight);
    }

    template<Side Me>
    inline void update(const Position& pos, Move bestMove, int ply, int depth, const PartialMoveList& quietMoves,
                       const PartialMoveList& captureMoves, PieceToHistory* const* contHist) {
        MoveScore bonus = historyBonus(depth);
        MoveScore malus = historyMalus(depth);

        if (!pos.isTactical(bestMove)) {
            updateKiller(bestMove, ply);
            updateCounter(pos, bestMove);

            updateMainHistory<Me>(bestMove, bonus);
            updateContinuationHistory(pos, bestMove, bonus, contHist);

            for (auto m : quietMoves) {
                updateMainHistory<Me>(m, -malus);
                updateContinuationHistory(pos, m, -malus, contHist);
            }
        } else {
            updateCaptureHistory(pos, bestMove, bonus);
        }

        for (auto m : captureMoves) {
            updateCaptureHistory(pos, m, -malus);
        }
    }
private:
    Move counterMoves[NB_PIECE][NB_SQUARE];
    Move killerMoves[MAX_PLY+1][2];
    HistoryScore history[NB_SIDE][NB_SQUARE*NB_SQUARE];
    HistoryScore captureHistory[NB_PIECE][NB_SQUARE][NB_PIECE_TYPE];
    HistoryScore corrHist[NB_SIDE][CORR_HIST_SIZE];
    std::unique_ptr<NonPawnCorrHist> nonPawnCorrHist;
    std::unique_ptr<MinorCorrHist> minorCorrHist;
    std::unique_ptr<ContinuationHistory> contCorrHist;
    std::unique_ptr<ContinuationHistory> continuationHistory;

    inline MoveScore historyBonus(int depth) {
        return std::min(1536, 8*depth*depth);
    }

    inline MoveScore historyMalus(int depth) {
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

    inline void updateHistoryEntry(HistoryScore &entry, MoveScore bonus, MoveScore limit) {
        assert(std::abs(bonus) <= limit && limit <= INT16_MAX);
        entry += bonus - entry * std::abs(bonus) / limit;
    }

    inline void updateCorrEntry(HistoryScore &entry, MoveScore diff, MoveScore weight) {
        MoveScore value = (entry * (256 - weight) + diff * weight) / 256;
        entry = std::clamp(value, -CORR_HIST_LIMIT, CORR_HIST_LIMIT);
    }

    inline PieceType capturedType(const Position& pos, Move m) const {
        return moveType(m) == EN_PASSANT ? PAWN : pieceType(pos.getPieceAt(moveTo(m)));
    }

    inline void updateCaptureHistory(const Position& pos, Move m, MoveScore bonus) {
        updateHistoryEntry(captureHistory[pos.getPieceAt(moveFrom(m))][moveTo(m)][capturedType(pos, m)], bonus, CAPTURE_HIST_LIMIT);
    }

    template<Side Me>
    inline void updateMainHistory(Move m, MoveScore bonus) {
        updateHistoryEntry(history[Me][moveFromTo(m)], bonus, MAIN_HIST_LIMIT);
    }

    inline void updateContinuationHistory(const Position& pos, Move m, MoveScore bonus, PieceToHistory* const* contHist) {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);
        for (int i = 0; i < CONT_HIST_PLIES; i++)
            if (contHist[i] != getDefaultContHist())
                updateHistoryEntry((*contHist[i])[pc][to], bonus, CONT_HIST_LIMIT);
    }
};

} /* namespace Belette */

#endif /* MOVEHISTORY_H_INCLUDED */
