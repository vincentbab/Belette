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
#include "tune.h"

namespace Belette {

using MoveScore = int32_t;

using PartialMoveList = fixed_vector<Move, 32, uint8_t>;

// Indexed by [piece][to] of the current move
using PieceToHistory = std::array<std::array<MoveScore, NB_SQUARE>, NB_PIECE>;

// Indexed by [piece][to] of a previous move
using ContinuationHistory = std::array<std::array<PieceToHistory, NB_SQUARE>, NB_PIECE>;

using ButterflyHistory = std::array<std::array<std::array<std::array<MoveScore, NB_SQUARE*NB_SQUARE>, 2>, 2>, NB_SIDE>;

constexpr int CONT_HIST_PLIES = 3;
constexpr int CONT_HIST_MAX_PLY = 4;
constexpr int QSEARCH_CONT_HIST_PLIES = 1;
constexpr int CONT_CORR_PLIES = 10;
constexpr int CONT_CORR_MAX_PLY = 11;
constexpr int CONT_CORR2_PLIES = 9;
constexpr int CONT_CORR2_MAX_PLY = 11;
constexpr int NODE_SENTINELS = std::max({CONT_HIST_MAX_PLY, CONT_CORR_MAX_PLY, CONT_CORR2_MAX_PLY});

struct ContCorrContext {
    PieceToHistory* contCorr[CONT_CORR_PLIES];
    PieceToHistory* contCorr2[CONT_CORR2_PLIES];
    Piece prevPiece2;
    Square prevTo2;
};

constexpr int PAWN_HIST_SIZE = 4096;

using PawnHistory = std::array<std::array<std::array<int16_t, NB_SQUARE>, NB_PIECE>, PAWN_HIST_SIZE>;

constexpr int CORR_HIST_SIZE = 16384;
constexpr MoveScore CORR_HIST_GRAIN = 256;

using CaptureHistory = std::array<std::array<std::array<std::array<std::array<MoveScore, NB_PIECE_TYPE>, NB_SQUARE>, NB_PIECE>, 2>, 2>;
using NonPawnCorrHist = std::array<std::array<std::array<MoveScore, CORR_HIST_SIZE>, NB_SIDE>, NB_SIDE>;
using MinorCorrHist = std::array<std::array<MoveScore, CORR_HIST_SIZE>, NB_SIDE>;
using MajorCorrHist = std::array<std::array<MoveScore, CORR_HIST_SIZE>, NB_SIDE>;

class MoveHistory {
public:
    MoveHistory(): counterMoves{}, killerMoves{}, history(std::make_unique<ButterflyHistory>()), captureHistory(std::make_unique<CaptureHistory>()), corrHist{},
        nonPawnCorrHist(std::make_unique<NonPawnCorrHist>()),
        minorCorrHist(std::make_unique<MinorCorrHist>()),
        majorCorrHist(std::make_unique<MajorCorrHist>()),
        contCorrHist(std::make_unique<ContinuationHistory>()),
        contCorr2Hist(std::make_unique<ContinuationHistory>()),
        continuationHistory(std::make_unique<ContinuationHistory>()),
        pawnHistory(std::make_unique<PawnHistory>()) { }

    inline void clear() {
        std::memset(counterMoves, 0, sizeof(counterMoves));
        std::memset(killerMoves, 0, sizeof(killerMoves));
        std::memset(history.get(), 0, sizeof(ButterflyHistory));
        std::memset(captureHistory.get(), 0, sizeof(CaptureHistory));
        std::memset(corrHist, 0, sizeof(corrHist));
        std::memset(nonPawnCorrHist.get(), 0, sizeof(NonPawnCorrHist));
        std::memset(minorCorrHist.get(), 0, sizeof(MinorCorrHist));
        std::memset(majorCorrHist.get(), 0, sizeof(MajorCorrHist));
        std::memset(contCorrHist.get(), 0, sizeof(ContinuationHistory));
        std::memset(contCorr2Hist.get(), 0, sizeof(ContinuationHistory));
        std::memset(continuationHistory.get(), 0, sizeof(ContinuationHistory));
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

    template<Side Me>
    inline MoveScore getQuietHistory(const Position& pos, Move m, const PieceToHistory* const* contHist) const {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);

        return (OrderMain * getButterfly<Me>(pos, m)
              + OrderCont1 * (*contHist[0])[pc][to]
              + OrderCont2 * (*contHist[1])[pc][to]
              + OrderCont4 * (*contHist[2])[pc][to]
              + OrderPawn * getPawnHistory(pos, m)) / 1024;
    }

    template<Side Me>
    inline MoveScore getStatScore(const Position& pos, Move m, const PieceToHistory* const* contHist) const {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);

        return (StatMain * getButterfly<Me>(pos, m)
              + StatCont1 * (*contHist[0])[pc][to]
              + StatCont2 * (*contHist[1])[pc][to]) / 1024;
    }

    template<Side Me, int NbContHist>
    inline MoveScore getEvasionHistory(const Position& pos, Move m, const PieceToHistory* const* contHist) const {
        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);

        MoveScore score = 2048 * getButterfly<Me>(pos, m);
        for (int i = 0; i < NbContHist; i++)
            score += EvasionCont * (*contHist[i])[pc][to];
        score += EvasionPawn * getPawnHistory(pos, m);

        return score / 1024;
    }

    inline MoveScore getPawnHistory(const Position& pos, Move m) const {
        return (*pawnHistory)[pos.pawnHash() & (PAWN_HIST_SIZE - 1)][pos.getPieceAt(moveFrom(m))][moveTo(m)];
    }

    inline MoveScore getCaptureHistory(const Position& pos, Move m) const {
        return captureEntry(pos, m);
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

    inline PieceToHistory* getContCorr2Entry(const Position& pos, Move m) {
        return &(*contCorr2Hist)[pos.getPieceAt(moveFrom(m))][moveTo(m)];
    }

    inline PieceToHistory* getDefaultContCorr2() {
        return &(*contCorr2Hist)[NO_PIECE][SQ_FIRST];
    }

    template<Side Me>
    inline void prefetchCorrection(const Position& pos) const {
        __builtin_prefetch(&corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)]);
        __builtin_prefetch(&(*nonPawnCorrHist)[Me][WHITE][pos.nonPawnHash(WHITE) & (CORR_HIST_SIZE - 1)]);
        __builtin_prefetch(&(*nonPawnCorrHist)[Me][BLACK][pos.nonPawnHash(BLACK) & (CORR_HIST_SIZE - 1)]);
        __builtin_prefetch(&(*minorCorrHist)[Me][pos.minorHash() & (CORR_HIST_SIZE - 1)]);
        __builtin_prefetch(&(*majorCorrHist)[Me][pos.majorHash() & (CORR_HIST_SIZE - 1)]);
    }

    template<Side Me>
    inline Score correctEval(const Position& pos, Score eval, const ContCorrContext& ctx) const {
        MoveScore correction = CorrPawn * corrHist[Me][pos.pawnHash() & (CORR_HIST_SIZE - 1)]
                             + CorrNonPawn * ((*nonPawnCorrHist)[Me][WHITE][pos.nonPawnHash(WHITE) & (CORR_HIST_SIZE - 1)]
                                            + (*nonPawnCorrHist)[Me][BLACK][pos.nonPawnHash(BLACK) & (CORR_HIST_SIZE - 1)])
                             + CorrMinor * (*minorCorrHist)[Me][pos.minorHash() & (CORR_HIST_SIZE - 1)]
                             + CorrMajor * (*majorCorrHist)[Me][pos.majorHash() & (CORR_HIST_SIZE - 1)];

        Move prevMove = pos.previousMove();
        if (isValidMove(prevMove)) {
            Piece pc = pos.getPieceAt(moveTo(prevMove));
            Square to = moveTo(prevMove);
            const int weights[CONT_CORR_PLIES] = {CorrContPly2, CorrContPly3, CorrContPly4, CorrContPly5,
                                                  CorrContPly6to8, CorrContPly6to8, CorrContPly6to8,
                                                  CorrContPly9to11, CorrContPly9to11, CorrContPly9to11};
            for (int i = 0; i < CONT_CORR_PLIES; i++)
                correction += weights[i] * (*ctx.contCorr[i])[pc][to];
        }

        if (ctx.prevPiece2 != NO_PIECE) {
            const int weights[CONT_CORR2_PLIES] = {CorrCont2Ply3, CorrCont2Ply4, CorrCont2Ply5,
                                                   CorrCont2Ply6to8, CorrCont2Ply6to8, CorrCont2Ply6to8,
                                                   CorrCont2Ply9to11, CorrCont2Ply9to11, CorrCont2Ply9to11};
            for (int i = 0; i < CONT_CORR2_PLIES; i++)
                correction += weights[i] * (*ctx.contCorr2[i])[ctx.prevPiece2][ctx.prevTo2];
        }

        return std::clamp<Score>(eval + correction / (1024 * CORR_HIST_GRAIN), -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
    }

    template<Side Me>
    inline void updateCorrection(const Position& pos, Score bestScore, Score staticEval, int depth, const ContCorrContext& ctx) {
        MoveScore diff = (bestScore - staticEval) * CORR_HIST_GRAIN;
        MoveScore weight = std::min(depth + CorrUpdateBase, CorrUpdateMax);

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
                if (ctx.contCorr[i] != getDefaultContCorr())
                    updateCorrEntry((*ctx.contCorr[i])[pc][to], diff, weight);
        }

        if (ctx.prevPiece2 != NO_PIECE) {
            for (int i = 0; i < CONT_CORR2_PLIES; i++)
                if (ctx.contCorr2[i] != getDefaultContCorr2())
                    updateCorrEntry((*ctx.contCorr2[i])[ctx.prevPiece2][ctx.prevTo2], diff, weight);
        }
    }

    template<Side Me, bool Frc = false>
    inline void update(const Position& pos, Move bestMove, int ply, int depth, const PartialMoveList& quietMoves,
                       const PartialMoveList& captureMoves, PieceToHistory* const* contHist) {
        MoveScore captureMalus = historyValue(depth, MalusCaptMult, MalusCaptBase, MalusCaptMax);

        if (!pos.isTactical<Frc>(bestMove)) {
            updateKiller(bestMove, ply);
            updateCounter(pos, bestMove);

            updateQuiet<Me>(pos, bestMove, historyValue(depth, BonusMainMult, BonusMainBase, BonusMainMax),
                                           historyValue(depth, BonusContMult, BonusContBase, BonusContMax),
                                           historyValue(depth, BonusPawnMult, BonusPawnBase, BonusPawnMax), contHist);

            MoveScore mainMalus = historyValue(depth, MalusMainMult, MalusMainBase, MalusMainMax);
            MoveScore contMalus = historyValue(depth, MalusContMult, MalusContBase, MalusContMax);
            MoveScore pawnMalus = historyValue(depth, MalusPawnMult, MalusPawnBase, MalusPawnMax);

            for (auto m : quietMoves) {
                updateQuiet<Me>(pos, m, -mainMalus, -contMalus, -pawnMalus, contHist);
            }
        } else {
            updateCapture(pos, bestMove, historyValue(depth, BonusCaptMult, BonusCaptBase, BonusCaptMax));
        }

        for (auto m : captureMoves) {
            updateCapture(pos, m, -captureMalus);
        }
    }
private:
    Move counterMoves[NB_PIECE][NB_SQUARE];
    Move killerMoves[MAX_PLY+1][2];
    std::unique_ptr<ButterflyHistory> history;
    std::unique_ptr<CaptureHistory> captureHistory;
    MoveScore corrHist[NB_SIDE][CORR_HIST_SIZE];
    std::unique_ptr<NonPawnCorrHist> nonPawnCorrHist;
    std::unique_ptr<MinorCorrHist> minorCorrHist;
    std::unique_ptr<MajorCorrHist> majorCorrHist;
    std::unique_ptr<ContinuationHistory> contCorrHist;
    std::unique_ptr<ContinuationHistory> contCorr2Hist;
    std::unique_ptr<ContinuationHistory> continuationHistory;
    std::unique_ptr<PawnHistory> pawnHistory;

    template<Side Me>
    inline MoveScore& getButterfly(const Position& pos, Move m) const {
        Bitboard threats = pos.threatsFor(KING);
        return (*history)[Me][bool(threats & moveFrom(m))][bool(threats & moveTo(m))][moveFromTo(m)];
    }

    inline MoveScore& captureEntry(const Position& pos, Move m) const {
        Bitboard threats = pos.threatsFor(KING);
        return (*captureHistory)[bool(threats & moveFrom(m))][bool(threats & moveTo(m))][pos.getPieceAt(moveFrom(m))][moveTo(m)][capturedType(pos, m)];
    }

    inline MoveScore historyValue(int depth, int mult, int base, int max) {
        return std::clamp(mult*depth - base, 0, max);
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
    inline void updateHistoryEntry(T &entry, MoveScore bonus, MoveScore gravity = 8192) {
        entry += bonus - entry * std::abs(bonus) / gravity;
    }

    inline void updateCorrEntry(MoveScore &entry, MoveScore diff, MoveScore weight) {
        entry = (entry * (256 - weight) + diff * weight) / 256;
        entry = std::clamp(entry, -CorrLimit * CORR_HIST_GRAIN, CorrLimit * CORR_HIST_GRAIN);
    }

    inline PieceType capturedType(const Position& pos, Move m) const {
        return moveType(m) == EN_PASSANT ? PAWN : pieceType(pos.getPieceAt(moveTo(m)));
    }

    inline void updateCapture(const Position& pos, Move m, MoveScore bonus) {
        updateHistoryEntry(captureEntry(pos, m), bonus, GravityCapt * 1024);
    }

    template<Side Me>
    inline void updateQuiet(const Position& pos, Move m, MoveScore main, MoveScore cont, MoveScore pawn, PieceToHistory* const* contHist) {
        updateHistoryEntry(getButterfly<Me>(pos, m), main);

        Piece pc = pos.getPieceAt(moveFrom(m));
        Square to = moveTo(m);
        for (int i = 0; i < CONT_HIST_PLIES; i++)
            if (contHist[i] != getDefaultContHist())
                updateHistoryEntry((*contHist[i])[pc][to], cont, GravityCont * 1024);

        updateHistoryEntry((*pawnHistory)[pos.pawnHash() & (PAWN_HIST_SIZE - 1)][pc][to], pawn);
    }
};

} /* namespace Belette */

#endif /* MOVEHISTORY_H_INCLUDED */
