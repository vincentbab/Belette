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
    MovePicker(const Position &pos_, Move ttMove_): pos(pos_), ttMove(ttMove_) { }

    template<typename Handler>
    bool enumerate(const Handler &handler);

private:
    const Position &pos;
    Move ttMove;

    inline int16_t scoreEvasion(Move m);
    inline int16_t scoreTactical(Move m);
    inline int16_t scoreQuiet(Move m);
};


template<MovePickerType Type, Side Me>
template<typename Handler>
bool MovePicker<Type, Me>::enumerate(const Handler &handler) {
    //constexpr MoveGenType MGType = Type == MAIN ? ALL_MOVES : NON_QUIET_MOVES;

    //ttMove = MOVE_NONE;

    // TTMove
    if (pos.isLegal<Me>(ttMove)) {
        MoveList legals; enumerateLegalMoves<Me>(pos, [&](Move m, auto doMove, auto undoMove) { legals.push_back(m); return true; });
        if (!legals.contains(ttMove))
            assert(pos.isLegal<Me>(ttMove));

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

    if constexpr(Type == QUIESCENCE) return true;

    // Quiets
    moves.clear();
    enumerateLegalMoves<Me, QUIET_MOVES>(pos, [&](Move m, auto doMove, auto undoMove) {
        if (m == ttMove) return true; // continue;

        ScoredMove smove = {doMove, undoMove, m, scoreQuiet(m)};
        moves.push_back(smove);
        return true;
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
    return PieceTypeValue[pieceType(pos.getPieceAt(moveTo(m)))][MG];
}

template<MovePickerType Type, Side Me>
int16_t MovePicker<Type, Me>::scoreQuiet(Move m) {
    return 0;
}

} /* namespace BabChess */

#endif /* MOVEPICKER_H_INCLUDED */
