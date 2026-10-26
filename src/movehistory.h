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

using CheckCaptureContinuationHistory = std::array<std::array<ContinuationHistory, 2>, 2>;

using ButterflyHistory = std::array<std::array<std::array<std::array<MoveScore, NB_SQUARE*NB_SQUARE>, 2>, 2>, NB_SIDE>;

constexpr int CONT_HIST_PLIES = 3;
constexpr int CONT_HIST_MAX_PLY = 4;
constexpr int QSEARCH_CONT_HIST_PLIES = 1;
constexpr int CONT_CORR_PLIES = 10;
constexpr int CONT_CORR_MAX_PLY = 11;
constexpr int NODE_SENTINELS = std::max(CONT_HIST_MAX_PLY, CONT_CORR_MAX_PLY);

constexpr int PAWN_HIST_SIZE = 4096;

using PawnHistory = std::array<std::array<std::array<int16_t, NB_SQUARE>, NB_PIECE>, PAWN_HIST_SIZE>;

constexpr int CORR_HIST_SIZE = 16384;
constexpr MoveScore CORR_HIST_GRAIN = 256;
constexpr MoveScore CORR_HIST_LIMIT = 32 * CORR_HIST_GRAIN;

using NonPawnCorrHist = std::array<std::array<std::array<MoveScore, CORR_HIST_SIZE>, NB_SIDE>, NB_SIDE>;
using MinorCorrHist = std::array<std::array<MoveScore, CORR_HIST_SIZE>, NB_SIDE>;
using MajorCorrHist = std::array<std::array<MoveScore, CORR_HIST_SIZE>, NB_SIDE>;

class MoveHistory {
public:
    MoveHistory(): counterMoves{}, killerMoves{}, history(std::make_unique<ButterflyHistory>()), captureHistory{}, corrHist{},
        nonPawnCorrHist(std::make_unique<NonPawnCorrHist>()),
        minorCorrHist(std::make_unique<MinorCorrHist>()),
        majorCorrHist(std::make_unique<MajorCorrHist>()),
        contCorrHist(std::make_unique<ContinuationHistory>()),
        continuationHistory(std::make_unique<CheckCaptureContinuationHistory>()),
        pawnHistory(std::make_unique<PawnHistory>()) { }

    inline void clear() {
        std::memset(counterMoves, 0, sizeof(counterMoves));
        std::memset(killerMoves, 0, sizeof(killerMoves));
        std::memset(history.get(), 0, sizeof(ButterflyHistory));
        std::memset(captureHistory, 0, sizeof(captureHistory));
        std::memset(corrHist, 0, sizeof(corrHist));
        std::memset(nonPawnCorrHist.get(), 0, sizeof(NonPawnCorrHist));
        std::memset(minorCorrHist.get(), 0, sizeof(MinorCorrHist));
        std::memset(majorCorrHist.get(), 0, sizeof(MajorCorrHist));
        std::memset(contCorrHist.get(), 0, sizeof(ContinuationHistory));
        std::memset(continuationHistory.get(), 0, sizeof(CheckCaptureContinuationHistory));
        std::memset(pawnHistory.get(), 0, sizeof(PawnHistory));
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

    template<Side Me, int NbContHist = CONT_HIST_PLIES>
    inline MoveScore getHistory(const Position& pos, Move m, const PieceToHistory* const* contHist) const {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);

        MoveScore score = 2 * getButterfly<Me>(pos, m);
        for (int i = 0; i < NbContHist; i++)
            score += (*contHist[i])[pc][to];

        return score;
    }

    inline MoveScore getPawnHistory(const Position& pos, Move m) const {
        return (*pawnHistory)[pos.pawnHash() & (PAWN_HIST_SIZE - 1)][pos.getPieceAt(moveFrom(m))][moveTo(m)];
    }

    inline MoveScore getCaptureHistory(const Position& pos, Move m) const {
        return captureHistory[pos.getPieceAt(moveFrom(m))][moveTo(m)][capturedType(pos, m)];
    }

    inline PieceToHistory* getContHistEntry(bool inCheck, bool capture, Piece pc, Square to) {
        return &(*continuationHistory)[inCheck][capture][pc][to];
    }

    inline PieceToHistory* getContHistEntry(const Position& pos, Move m) {
        return getContHistEntry(pos.inCheck(), pos.isTactical(m), pos.getPieceAt(moveFrom(m)), moveTo(m));
    }

    // Never indexed by a real move because NO_PIECE.
    inline PieceToHistory* getDefaultContHist() {
        return getContHistEntry(false, false, NO_PIECE, SQ_FIRST);
    }

    inline PieceToHistory* getContCorrEntry(const Position& pos, Move m) {
        return &(*contCorrHist)[pos.getPieceAt(moveFrom(m))][moveTo(m)];
    }

    // Never indexed by a real move because NO_PIECE.
    inline PieceToHistory* getDefaultContCorr() {
        return &(*contCorrHist)[NO_PIECE][SQ_FIRST];
    }

    template<Side Me>
    inline Score correctEval(const Position& pos, Score eval, PieceToHistory* const* contCorr) const {
        MoveScore correction = corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)]
                             + (*nonPawnCorrHist)[Me][WHITE][pos.nonPawnHash(WHITE) & (CORR_HIST_SIZE - 1)]
                             + (*nonPawnCorrHist)[Me][BLACK][pos.nonPawnHash(BLACK) & (CORR_HIST_SIZE - 1)]
                             + (*minorCorrHist)[Me][pos.minorHash() & (CORR_HIST_SIZE - 1)]
                             + (*majorCorrHist)[Me][pos.majorHash() & (CORR_HIST_SIZE - 1)];

        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove)) {
            Piece pc = pos.getPieceAt(moveTo(prevMove));
            Square to = moveTo(prevMove);
            for (int i = 0; i < CONT_CORR_PLIES; i++)
                correction += (*contCorr[i])[pc][to];
        }

        return std::clamp<Score>(eval + correction / CORR_HIST_GRAIN, -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
    }

    template<Side Me>
    inline void updateCorrection(const Position& pos, Score bestScore, Score staticEval, int depth, PieceToHistory* const* contCorr) {
        MoveScore diff = (bestScore - staticEval) * CORR_HIST_GRAIN;
        MoveScore weight = std::min(depth + 1, 16);

        updateCorrEntry(corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)], diff, weight);
        updateCorrEntry((*nonPawnCorrHist)[Me][WHITE][pos.nonPawnHash(WHITE) & (CORR_HIST_SIZE - 1)], diff, weight);
        updateCorrEntry((*nonPawnCorrHist)[Me][BLACK][pos.nonPawnHash(BLACK) & (CORR_HIST_SIZE - 1)], diff, weight);
        updateCorrEntry((*minorCorrHist)[Me][pos.minorHash() & (CORR_HIST_SIZE - 1)], diff, weight);
        updateCorrEntry((*majorCorrHist)[Me][pos.majorHash() & (CORR_HIST_SIZE - 1)], diff, weight);

        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove)) {
            Piece pc = pos.getPieceAt(moveTo(prevMove));
            Square to = moveTo(prevMove);
            for (int i = 0; i < CONT_CORR_PLIES; i++)
                if (contCorr[i] != getDefaultContCorr())
                    updateCorrEntry((*contCorr[i])[pc][to], diff, weight);
        }
    }

    template<Side Me>
    inline void update(const Position& pos, Move bestMove, int ply, int depth, const PartialMoveList& quietMoves,
                       const PartialMoveList& captureMoves, PieceToHistory* const* contHist) {
        MoveScore bonus = historyBonus(depth);

        if (!pos.isTactical(bestMove)) {
            updateKiller(bestMove, ply);
            updateCounter(pos, bestMove);

            updateQuiet<Me>(pos, bestMove, bonus, contHist);

            for (auto m : quietMoves) {
                updateQuiet<Me>(pos, m, -bonus, contHist);
            }
        } else {
            updateCapture(pos, bestMove, bonus);
        }

        for (auto m : captureMoves) {
            updateCapture(pos, m, -bonus);
        }
    }
private:
    Move counterMoves[NB_PIECE][NB_SQUARE];
    Move killerMoves[MAX_PLY+1][2];
    std::unique_ptr<ButterflyHistory> history;
    MoveScore captureHistory[NB_PIECE][NB_SQUARE][NB_PIECE_TYPE];
    MoveScore corrHist[NB_SIDE][CORR_HIST_SIZE];
    std::unique_ptr<NonPawnCorrHist> nonPawnCorrHist;
    std::unique_ptr<MinorCorrHist> minorCorrHist;
    std::unique_ptr<MajorCorrHist> majorCorrHist;
    std::unique_ptr<ContinuationHistory> contCorrHist;
    std::unique_ptr<CheckCaptureContinuationHistory> continuationHistory;
    std::unique_ptr<PawnHistory> pawnHistory;

    template<Side Me>
    inline MoveScore& getButterfly(const Position& pos, Move m) const {
        Bitboard threats = pos.threatsFor(KING);
        return (*history)[Me][bool(threats & moveFrom(m))][bool(threats & moveTo(m))][moveFromTo(m)];
    }

    inline MoveScore historyBonus(int depth) {
        return std::min(1536, 128*depth - 64);
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

    template<typename T>
    inline void updateHistoryEntry(T &entry, MoveScore bonus) {
        entry += bonus - entry * std::abs(bonus) / 8192;
    }

    inline void updateCorrEntry(MoveScore &entry, MoveScore diff, MoveScore weight) {
        entry = (entry * (256 - weight) + diff * weight) / 256;
        entry = std::clamp(entry, -CORR_HIST_LIMIT, CORR_HIST_LIMIT);
    }

    inline PieceType capturedType(const Position& pos, Move m) const {
        return moveType(m) == EN_PASSANT ? PAWN : pieceType(pos.getPieceAt(moveTo(m)));
    }

    inline void updateCapture(const Position& pos, Move m, MoveScore bonus) {
        updateHistoryEntry(captureHistory[pos.getPieceAt(moveFrom(m))][moveTo(m)][capturedType(pos, m)], bonus);
    }

    template<Side Me>
    inline void updateQuiet(const Position& pos, Move m, MoveScore bonus, PieceToHistory* const* contHist) {
        updateHistoryEntry(getButterfly<Me>(pos, m), bonus);

        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);
        for (int i = 0; i < CONT_HIST_PLIES; i++)
            if (contHist[i] != getDefaultContHist())
                updateHistoryEntry((*contHist[i])[pc][to], bonus);

        updateHistoryEntry((*pawnHistory)[pos.pawnHash() & (PAWN_HIST_SIZE - 1)][pc][to], bonus);
    }
};

} /* namespace Belette */

#endif /* MOVEHISTORY_H_INCLUDED */
