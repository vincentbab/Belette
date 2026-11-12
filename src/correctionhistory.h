#ifndef CORRECTIONHISTORY_H_INCLUDED
#define CORRECTIONHISTORY_H_INCLUDED

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cstring>
#include <memory>
#include "chess.h"
#include "position.h"
#include "movehistory.h"
#include "tune.h"

namespace Belette {

constexpr int CONT_CORR_PLIES = 10;
constexpr int CONT_CORR_MAX_PLY = 11;
constexpr int CONT_CORR2_PLIES = 9;
constexpr int CONT_CORR2_MAX_PLY = 11;

struct ContCorrContext {
    PieceToHistory* contCorr[CONT_CORR_PLIES];
    PieceToHistory* contCorr2[CONT_CORR2_PLIES];
    Piece prevPiece2;
    Square prevTo2;
};

constexpr size_t CORR_HIST_BASE_SIZE = 16384;
constexpr MoveScore CORR_HIST_GRAIN = 256;

// Correction histories shared by a group of search threads
// Hash indexed tables have CORR_HIST_BASE_SIZE * bit_ceil(nbThreads) entries per side
class SharedCorrectionHistory {
public:
    explicit SharedCorrectionHistory(int nbThreads)
    : size(CORR_HIST_BASE_SIZE * std::bit_ceil(unsigned(std::max(nbThreads, 1)))), mask(size - 1),
      pawnCorrHist(std::make_unique<MoveScore[]>(NB_SIDE * size)),
      nonPawnCorrHist(std::make_unique<MoveScore[]>(NB_SIDE * NB_SIDE * size)),
      minorCorrHist(std::make_unique<MoveScore[]>(NB_SIDE * size)),
      majorCorrHist(std::make_unique<MoveScore[]>(NB_SIDE * size)),
      contCorrHist(std::make_unique<ContinuationHistory>()),
      contCorr2Hist(std::make_unique<ContinuationHistory>()) { }

    inline void clear() {
        std::memset(pawnCorrHist.get(), 0, NB_SIDE * size * sizeof(MoveScore));
        std::memset(nonPawnCorrHist.get(), 0, NB_SIDE * NB_SIDE * size * sizeof(MoveScore));
        std::memset(minorCorrHist.get(), 0, NB_SIDE * size * sizeof(MoveScore));
        std::memset(majorCorrHist.get(), 0, NB_SIDE * size * sizeof(MoveScore));
        std::memset(contCorrHist.get(), 0, sizeof(ContinuationHistory));
        std::memset(contCorr2Hist.get(), 0, sizeof(ContinuationHistory));
    }

    inline PieceToHistory* getContCorrEntry(const Position& pos, Move m) {
        return &(*contCorrHist)[pos.getPieceAt(moveFrom(m))][moveTo(m)];
    }

    // Never indexed by a real move because NO_PIECE.
    inline PieceToHistory* getDefaultContCorr() {
        return &(*contCorrHist)[NO_PIECE][SQ_FIRST];
    }

    inline PieceToHistory* getContCorr2Entry(const Position& pos, Move m) {
        return &(*contCorr2Hist)[pos.getPieceAt(moveFrom(m))][moveTo(m)];
    }

    inline PieceToHistory* getDefaultContCorr2() {
        return &(*contCorr2Hist)[NO_PIECE][SQ_FIRST];
    }

    template<Side Me>
    inline void prefetchCorrection(const Position& pos) const {
        __builtin_prefetch(&pawnEntry<Me>(pos));
        __builtin_prefetch(&nonPawnEntry<Me>(pos, WHITE));
        __builtin_prefetch(&nonPawnEntry<Me>(pos, BLACK));
        __builtin_prefetch(&minorEntry<Me>(pos));
        __builtin_prefetch(&majorEntry<Me>(pos));
    }

    template<Side Me>
    inline Score correctEval(const Position& pos, Score eval, const ContCorrContext& ctx) const {
        MoveScore correction = CorrPawn * load(pawnEntry<Me>(pos))
                             + CorrNonPawn * (load(nonPawnEntry<Me>(pos, WHITE)) + load(nonPawnEntry<Me>(pos, BLACK)))
                             + CorrMinor * load(minorEntry<Me>(pos))
                             + CorrMajor * load(majorEntry<Me>(pos));

        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove)) {
            Piece pc = pos.getPieceAt(moveTo(prevMove));
            Square to = moveTo(prevMove);
            const int weights[CONT_CORR_PLIES] = {CorrContPly2, CorrContPly3, CorrContPly4, CorrContPly5,
                                                  CorrContPly6to8, CorrContPly6to8, CorrContPly6to8,
                                                  CorrContPly9to11, CorrContPly9to11, CorrContPly9to11};
            for (int i = 0; i < CONT_CORR_PLIES; i++)
                correction += weights[i] * load((*ctx.contCorr[i])[pc][to]);
        }

        if (ctx.prevPiece2 != NO_PIECE) {
            const int weights[CONT_CORR2_PLIES] = {CorrCont2Ply3, CorrCont2Ply4, CorrCont2Ply5,
                                                   CorrCont2Ply6to8, CorrCont2Ply6to8, CorrCont2Ply6to8,
                                                   CorrCont2Ply9to11, CorrCont2Ply9to11, CorrCont2Ply9to11};
            for (int i = 0; i < CONT_CORR2_PLIES; i++)
                correction += weights[i] * load((*ctx.contCorr2[i])[ctx.prevPiece2][ctx.prevTo2]);
        }

        return std::clamp<Score>(eval + correction / (1024 * CORR_HIST_GRAIN), -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
    }

    template<Side Me>
    inline void updateCorrection(const Position& pos, Score bestScore, Score staticEval, int depth, const ContCorrContext& ctx) {
        MoveScore diff = (bestScore - staticEval) * CORR_HIST_GRAIN;
        MoveScore weight = std::min(depth + CorrUpdateBase, CorrUpdateMax);

        updateCorrEntry(pawnEntry<Me>(pos), diff, weight);
        updateCorrEntry(nonPawnEntry<Me>(pos, WHITE), diff, weight);
        updateCorrEntry(nonPawnEntry<Me>(pos, BLACK), diff, weight);
        updateCorrEntry(minorEntry<Me>(pos), diff, weight);
        updateCorrEntry(majorEntry<Me>(pos), diff, weight);

        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove)) {
            Piece pc = pos.getPieceAt(moveTo(prevMove));
            Square to = moveTo(prevMove);
            for (int i = 0; i < CONT_CORR_PLIES; i++)
                if (ctx.contCorr[i] != getDefaultContCorr())
                    updateCorrEntry((*ctx.contCorr[i])[pc][to], diff, weight);
        }

        if (ctx.prevPiece2 != NO_PIECE) {
            for (int i = 0; i < CONT_CORR2_PLIES; i++)
                if (ctx.contCorr2[i] != getDefaultContCorr2())
                    updateCorrEntry((*ctx.contCorr2[i])[ctx.prevPiece2][ctx.prevTo2], diff, weight);
        }
    }

private:
    const size_t size;
    const size_t mask;
    std::unique_ptr<MoveScore[]> pawnCorrHist;
    std::unique_ptr<MoveScore[]> nonPawnCorrHist;
    std::unique_ptr<MoveScore[]> minorCorrHist;
    std::unique_ptr<MoveScore[]> majorCorrHist;
    std::unique_ptr<ContinuationHistory> contCorrHist;
    std::unique_ptr<ContinuationHistory> contCorr2Hist;

    template<Side Me>
    inline MoveScore& pawnEntry(const Position& pos) const {
        return pawnCorrHist[Me * size + (pos.pawnHash() & mask)];
    }

    template<Side Me>
    inline MoveScore& nonPawnEntry(const Position& pos, Side side) const {
        return nonPawnCorrHist[(Me * NB_SIDE + side) * size + (pos.nonPawnHash(side) & mask)];
    }

    template<Side Me>
    inline MoveScore& minorEntry(const Position& pos) const {
        return minorCorrHist[Me * size + (pos.minorHash() & mask)];
    }

    template<Side Me>
    inline MoveScore& majorEntry(const Position& pos) const {
        return majorCorrHist[Me * size + (pos.majorHash() & mask)];
    }

    static_assert(std::atomic_ref<MoveScore>::required_alignment == alignof(MoveScore));

    static inline MoveScore load(const MoveScore &entry) {
        return std::atomic_ref(const_cast<MoveScore&>(entry)).load(std::memory_order_relaxed);
    }

    static inline void store(MoveScore &entry, MoveScore value) {
        std::atomic_ref(entry).store(value, std::memory_order_relaxed);
    }

    inline void updateCorrEntry(MoveScore &entry, MoveScore diff, MoveScore weight) {
        MoveScore value = (load(entry) * (256 - weight) + diff * weight) / 256;
        store(entry, std::clamp(value, -CorrLimit * CORR_HIST_GRAIN, CorrLimit * CORR_HIST_GRAIN));
    }
};

} /* namespace Belette */

#endif /* CORRECTIONHISTORY_H_INCLUDED */
