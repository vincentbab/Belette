#ifndef MOVEPICKER_H_INCLUDED
#define MOVEPICKER_H_INCLUDED

#include <cstdint>
#include <algorithm>
#include "fixed_vector.h"
#include "chess.h"
#include "position.h"
#include "move.h"
#include "evaluate.h"

namespace Belette {

struct ScoredMove {
    ScoredMove() { };
    inline ScoredMove(Move move_, int16_t score_) 
        : move(move_), score(score_) { }
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
    MovePicker(const Position &pos_, Move ttMove_ = MOVE_NONE, Move killer1_ = MOVE_NONE, Move killer2_ = MOVE_NONE, Move counter_ = MOVE_NONE)
    : pos(pos_), ttMove(ttMove_), refutations{killer1_, killer2_, counter_} 
    { 
        assert(killer1_ != killer2_ || killer1_ == MOVE_NONE);
    }

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
    // TT Move
    if (pos.isLegal<Me>(ttMove)) {
        CALL_HANDLER(ttMove);
    }
    
    ScoredMoveList moves;
    ScoredMove *current, *endBadTacticals, *beginQuiets, *endBadQuiets;

    // Evasions
    if (pos.inCheck()) {
        enumerateLegalMoves<Me, ALL_MOVES>(pos, [&](Move m) {
            if (m == ttMove) return true; // continue;

            moves.emplace_back(m, scoreEvasion(m));
            return true;
        });

        std::sort(moves.begin(), moves.end(), [](const ScoredMove &a, const ScoredMove &b) {
            return a.score > b.score;
        });

        for (auto m : moves) {
            if (m.move == ttMove) continue;
            CALL_HANDLER(m.move);
        }

        return true;
    }

    // Tacticals
    enumerateLegalMoves<Me, TACTICAL_MOVES>(pos, [&](Move m) {
        if (m == ttMove) return true; // continue;

        if constexpr(Type == QUIESCENCE) {
            if (!pos.see(m, 0)) return true; // continue;
        }
        
        moves.emplace_back(m, scoreTactical(m));
        return true;
    });

    std::sort(moves.begin(), moves.end(), [](const ScoredMove &a, const ScoredMove &b) {
        return a.score > b.score;
    });

    // Good tacticals
    for (current = endBadTacticals = moves.begin(); current != moves.end(); current++) {
        if constexpr(Type == MAIN) { // For quiescence bad moves are already pruned in move enumeration
            if (!pos.see(current->move, -50)) { // Allow Bishop takes Knight
                *endBadTacticals++ = *current;
                continue;
            }
        }

        CALL_HANDLER(current->move);
    }

    // Stop here for Quiescence
    if constexpr(Type == QUIESCENCE) return true;

    // Killer 1
    if (refutations[0] != ttMove && !pos.isTactical(refutations[0]) && pos.isLegal<Me>(refutations[0])) {
        CALL_HANDLER(refutations[0]);
    }

    // Killer 2
    if (refutations[1] != ttMove && !pos.isTactical(refutations[1]) && pos.isLegal<Me>(refutations[1])) {
        CALL_HANDLER(refutations[1]);
    }

    // Counter
    if (refutations[2] != ttMove && !pos.isTactical(refutations[2]) && refutations[2] != refutations[0] && refutations[2] != refutations[1] && pos.isLegal<Me>(refutations[2])) {
        CALL_HANDLER(refutations[2]);
    }

    // Quiets
    moves.resize(endBadTacticals - moves.begin()); // Keep only bad tacticals
    beginQuiets = endBadTacticals;
    threatenedPieces = (pos.getPiecesBB(Me, KNIGHT, BISHOP) & pos.threatenedByPawns())
                     | (pos.getPiecesBB(Me, ROOK) & pos.threatenedByMinors())
                     | (pos.getPiecesBB(Me, QUEEN) & pos.threatenedByRooks());

    enumerateLegalMoves<Me, QUIET_MOVES>(pos, [&](Move move) {
        if (move == ttMove) return true; // continue;
        if (refutations[0] == move || refutations[1] == move || refutations[2] == move) return true; // continue

        moves.emplace_back(move, scoreQuiet(move));
        return true;
    });

    std::sort(beginQuiets, moves.end(), [](const ScoredMove &a, const ScoredMove &b) {
        return a.score > b.score;
    });

    // Good quiets
    for (current = endBadQuiets = beginQuiets; current != moves.end(); current++) {
        if (current->score < 0) {
            *endBadQuiets++ = *current;
            continue;
        }

        CALL_HANDLER(current->move);
    }

    // Bad tacticals
    for (current = moves.begin(); current != endBadTacticals; current++) {
        CALL_HANDLER(current->move);
    }

    // Bad quiets
    for (current = beginQuiets; current != endBadQuiets; current++) {
        CALL_HANDLER(current->move);
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
    return PieceValue<MG>(pos.getPieceAt(moveTo(m))) - (int)pieceType(pos.getPieceAt(moveFrom(m))); // MVV-LVA
}

template<MovePickerType Type, Side Me>
int16_t MovePicker<Type, Me>::scoreQuiet(Move m) {
    Square from = moveFrom(m), to = moveTo(m);
    PieceType pt = pieceType(pos.getPieceAt(from));
    Score score = NB_PIECE_TYPE-(int)pt;

    if (moveType(m) == PROMOTION) [[unlikely]]
        return -100;

    if (threatenedPieces & from) {
        score += pt == QUEEN && !(to & pos.threatenedByRooks()) ? 1000
             : pt == ROOK && !(to & pos.threatenedByMinors()) ? 500
             : (pt == BISHOP || pt == KNIGHT) && !(to & pos.threatenedByPawns()) ? 300
             : 0;
    }

    // TODO: refactor this!
    switch (pt) {
        case PAWN:
            if (pawnAttacks<Me>(bb(to)) & pos.getPiecesBB(~Me, KING)) {
                score += 10;
            }
            break;
        case KNIGHT:
            if (attacks<KNIGHT>(to, pos.getPiecesBB()) & pos.getPiecesBB(~Me, KING)) {
                score += 10;
            }
            break;
        case BISHOP:
            if (attacks<BISHOP>(to, pos.getPiecesBB()) & pos.getPiecesBB(~Me, KING)) {
                score += 10;
            }
        case ROOK:
            if (attacks<ROOK>(to, pos.getPiecesBB()) & pos.getPiecesBB(~Me, KING)) {
                score += 10;
            }
        case QUEEN:
            if (attacks<QUEEN>(to, pos.getPiecesBB()) & pos.getPiecesBB(~Me, KING)) {
                score += 10;
            }
        default:
            break;
    }

    return score;
}

} /* namespace Belette */

#endif /* MOVEPICKER_H_INCLUDED */
