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

constexpr int PAWN_HIST_SIZE = 4096;

using PawnHistory = std::array<std::array<std::array<int16_t, NB_SQUARE>, NB_PIECE>, PAWN_HIST_SIZE>;

using CaptureHistory = std::array<std::array<std::array<std::array<std::array<MoveScore, NB_PIECE_TYPE>, NB_SQUARE>, NB_PIECE>, 2>, 2>;

class MoveHistory {
public:
    MoveHistory(): counterMoves{}, killerMoves{}, history(std::make_unique<ButterflyHistory>()), captureHistory(std::make_unique<CaptureHistory>()),
        continuationHistory(std::make_unique<ContinuationHistory>()),
        pawnHistory(std::make_unique<PawnHistory>()) { }

    inline void clear() {
        std::memset(counterMoves, 0, sizeof(counterMoves));
        std::memset(killerMoves, 0, sizeof(killerMoves));
        std::memset(history.get(), 0, sizeof(ButterflyHistory));
        std::memset(captureHistory.get(), 0, sizeof(CaptureHistory));
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
