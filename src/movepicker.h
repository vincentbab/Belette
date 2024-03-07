#ifndef MOVEPICKER_H_INCLUDED
#define MOVEPICKER_H_INCLUDED

#include <cstdint>
#include <algorithm>
#include "fixed_vector.h"
#include "chess.h"
#include "position.h"
#include "move.h"
#include "evaluate.h"

namespace BabChess {

struct ScoredMove {
    void (Position::*doMove)(Move m);
    void (Position::*undoMove)(Move m);
    Move move;
    int16_t score;
};

using ScoredMoveList = fixed_vector<ScoredMove, MAX_MOVE, uint8_t>;

enum MovePickerType {
    MAIN,
    QUIESCENCE
};

template<MovePickerType Type, Side Me>
class MovePicker {
public:
    MovePicker(const Position &pos_, Move ttMove_, Move killer1_ = MOVE_NONE, Move killer2_ = MOVE_NONE, Move counter_ = MOVE_NONE)
        : pos(pos_), ttMove(ttMove_), refutations{killer1_, killer2_, counter_} { }

    template<typename Handler>
    inline bool enumerate(const Handler &handler);

private:
    const Position &pos;
    Move ttMove;
    Move refutations[3];

    Bitboard threatenedPieces;

    inline int16_t scoreEvasion(Move m);
    inline int16_t scoreTactical(Move m);
    inline int16_t scoreQuiet(Move m);
};


template<MovePickerType Type, Side Me>
template<typename Handler>
bool MovePicker<Type, Me>::enumerate(const Handler &handler) {
    // TTMove
    if (pos.isLegal<Me>(ttMove)) {
        if (!handler(ttMove, &Position::doMove<Me>, &Position::undoMove<Me>)) return false;
    }
    
    ScoredMoveList moves;

    // Evasions
    if (pos.inCheck()) {
        enumerateLegalMoves<Me, ALL_MOVES>(pos, [&](Move m, auto doMove, auto undoMove) {
            if (m == ttMove) return true; // continue;

            ScoredMove smove = {doMove, undoMove, m, scoreEvasion(m)};
            moves.push_back(smove);
            //return handler(m, doMove, undoMove);
            return true;
        });

        std::sort(moves.begin(), moves.end(), [](const ScoredMove &a, const ScoredMove &b) {
            return a.score > b.score;
        });

        for (auto m : moves) {
            if (m.move == ttMove) continue;
            if (!handler(m.move, m.doMove, m.undoMove)) return false;
        }

        return true;
    }

    // Tacticals
    enumerateLegalMoves<Me, NON_QUIET_MOVES>(pos, [&](Move m, auto doMove, auto undoMove) {
        if (m == ttMove) return true; // continue;
        
        ScoredMove smove = {doMove, undoMove, m, scoreTactical(m)};
        moves.push_back(smove);
        return true;
    });

    std::sort(moves.begin(), moves.end(), [](const ScoredMove &a, const ScoredMove &b) {
        return a.score > b.score;
    });

    for (auto m : moves) {
        if (!handler(m.move, m.doMove, m.undoMove)) return false;
    }

    // Stop here for Quiescence
    if constexpr(Type == QUIESCENCE) return true;

    // Refutations (killers & counter)
    for (auto m : refutations) {
        if (pos.isLegal<Me>(m)) {
            if (!handler(m, &Position::doMove<Me>, &Position::undoMove<Me>)) return false;
        }
    }

    // Quiets
    moves.clear();
    threatenedPieces = (pos.getPiecesBB(Me, KING, BISHOP) & pos.threatenedByPawns())
                     | (pos.getPiecesBB(Me, ROOK) & pos.threatenedByMinors())
                     | (pos.getPiecesBB(Me, QUEEN) & pos.threatenedByRooks());

    enumerateLegalMoves<Me, QUIET_MOVES>(pos, [&](Move move, auto doMove, auto undoMove) {
        if (move == ttMove) return true; // continue;
        for (auto m : refutations) if (move == m) return true; // continue;

        ScoredMove smove = {doMove, undoMove, move, scoreQuiet(move)};
        moves.push_back(smove);
        return true;
    });

    std::sort(moves.begin(), moves.end(), [](const ScoredMove &a, const ScoredMove &b) {
        return a.score > b.score;
    });

    for (auto m : moves) {
        if (!handler(m.move, m.doMove, m.undoMove)) return false;
    }

    return true;
}

template<MovePickerType Type, Side Me>
int16_t MovePicker<Type, Me>::scoreEvasion(Move m) {
    if (pos.isCapture(m)) {
        return scoreTactical(m);
    }

    return 0;
}

template<MovePickerType Type, Side Me>
int16_t MovePicker<Type, Me>::scoreTactical(Move m) {
    return PieceTypeValue[pieceType(pos.getPieceAt(moveTo(m)))][MG] - (int)pieceType(pos.getPieceAt(moveFrom(m))); // MVV-LVA
}

template<MovePickerType Type, Side Me>
int16_t MovePicker<Type, Me>::scoreQuiet(Move m) {
    Square from = moveFrom(m);
    PieceType pt = pieceType(pos.getPieceAt(from));
    
    if (threatenedPieces & from) {
        return pt == QUEEN && !(moveTo(m) & pos.threatenedByRooks()) ? 1000
             : pt == ROOK && !(moveTo(m) & pos.threatenedByMinors()) ? 500
             : (pt == BISHOP || pt == KNIGHT) && !(moveTo(m) & pos.threatenedByPawns()) ? 300
             : -(int)pt;
    }

    return -(int)pt;
}

} /* namespace BabChess */

#endif /* MOVEPICKER_H_INCLUDED */
