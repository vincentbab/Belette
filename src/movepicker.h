#ifndef MOVEPICKER_H_INCLUDED
#define MOVEPICKER_H_INCLUDED

#include <cstdint>
#include <algorithm>
#include "fixed_vector.h"
#include "chess.h"
#include "position.h"
#include "movegen.h"
#include "movehistory.h"
#include "evaluate.h"
#include "tt.h"

namespace Belette {

constexpr MoveScore PieceThreatenedValue[NB_PIECE_TYPE] = {
    0, 0, 15000, 15000, 25000, 50000, 0
};

struct ScoredMove {
    ScoredMove() { };

    inline ScoredMove(Move move_, MoveScore score_) 
    : score(score_), move(move_) { }

    MoveScore score;
    Move move;
};

using ScoredMoveList = fixed_vector<ScoredMove, MAX_MOVE, uint8_t>;

enum MovePickerType {
    MAIN,
    QUIESCENCE
};

class MovePicker {
public:
    MovePicker(): pos(nullptr), moveHistory(nullptr) { }
    MovePicker(const Position &pos_, Move ttMove_ = MOVE_NONE)
    : pos(&pos_),  moveHistory(nullptr), ttMove(ttMove_), refutations{}
    { }

    MovePicker(const Position &pos_, Move ttMove_, const MoveHistory* moveHistory_, int ply_)
    : pos(&pos_), moveHistory(moveHistory_), ttMove(ttMove_),
      refutations{moveHistory->getKiller<0>(ply_), moveHistory->getKiller<1>(ply_), moveHistory->getCounter(pos_)}
    {
        assert(refutations[0] != refutations[1] || refutations[0] == MOVE_NONE);
    }

    template<MovePickerType Type, Side Me, typename Handler>
    inline bool enumerate(const Handler &handler);

    template<MovePickerType Type, typename Handler>
    inline bool enumerate(const Handler &handler) { return pos->getSideToMove() == WHITE ? enumerate<Type, WHITE, Handler>(handler) : enumerate<Type, BLACK, Handler>(handler); }

private:
    const Position* const pos;
    const MoveHistory* const moveHistory;
    Move ttMove;
    Move refutations[3];

    template<Side Me> inline MoveScore scoreEvasion(Move m);
    template<Side Me> inline MoveScore scoreTactical(Move m);
    template<Side Me> inline MoveScore scoreQuiet(Move m);
};

template<MovePickerType Type, Side Me, typename Handler>
bool MovePicker::enumerate(const Handler &handler) {
    assert(pos != nullptr);
    assert(pos->getSideToMove() == Me);
    
    bool skipQuiets = false;

    tt.prefetch(pos->getHashAfter(ttMove));

    // TT Move
    if (pos->isLegal<Me>(ttMove)) {
        CALL_HANDLER(ttMove, skipQuiets);
    }
    
    ScoredMoveList moves;
    ScoredMove *current, *endBadTacticals, *beginQuiets, *endBadQuiets;

    auto compare = [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; };

    // Evasions
    if (pos->inCheck()) {
        enumerateLegalMoves<Me, ALL_MOVES>(*pos, [&](Move m) {
            if (m == ttMove) return true; // continue;

            tt.prefetch(pos->getHashAfter(m));

            ScoredMove newMove = ScoredMove(m, scoreEvasion<Me>(m));
            moves.insert_sorted(newMove, compare);
            
            return true;
        });

        for (auto m : moves) {
            CALL_HANDLER(m.move, skipQuiets);
        }

        return true;
    }

    // Tacticals
    enumerateLegalMoves<Me, TACTICAL_MOVES>(*pos, [&](Move m) {
        if (m == ttMove) return true; // continue;
        
        if (moves.size() < 16)
            tt.prefetch(pos->getHashAfter(m));

        ScoredMove newMove = ScoredMove(m, scoreTactical<Me>(m));
        moves.insert_sorted(newMove, compare);

        return true;
    });

    for (current = endBadTacticals = moves.begin(); current != moves.end(); current++) {
        if constexpr(Type == MAIN) { // For quiescence prunning of bad captures is done in search
            if (!pos->see(current->move, -50)) { // Allow Bishop takes Knight
                *endBadTacticals++ = *current;
                continue;
            }
        }

        CALL_HANDLER(current->move, skipQuiets);
    }

    // Stop here for Quiescence
    if constexpr(Type == QUIESCENCE) return true;

    if (moveHistory != nullptr) [[likely]] {
        tt.prefetch(pos->getHashAfter(refutations[0]));
        tt.prefetch(pos->getHashAfter(refutations[1]));
        tt.prefetch(pos->getHashAfter(refutations[2]));
        
        // Killer 1
        if (refutations[0] != ttMove && !pos->isTactical(refutations[0]) && pos->isLegal<Me>(refutations[0])) {
            CALL_HANDLER(refutations[0], skipQuiets);
        }

        // Killer 2
        if (refutations[1] != ttMove && !pos->isTactical(refutations[1]) && pos->isLegal<Me>(refutations[1])) {
            CALL_HANDLER(refutations[1], skipQuiets);
        }

        // Counter
        if (refutations[2] != ttMove && !pos->isTactical(refutations[2]) && refutations[2] != refutations[0] && refutations[2] != refutations[1] && pos->isLegal<Me>(refutations[2])) {
            CALL_HANDLER(refutations[2], skipQuiets);
        }
    }

    // Quiets
    moves.resize(endBadTacticals - moves.begin()); // Keep only bad tacticals
    beginQuiets = endBadTacticals;

    enumerateLegalMoves<Me, QUIET_MOVES>(*pos, [&](Move m) {
        if (m == ttMove) return true; // continue;
        if (refutations[0] == m || refutations[1] == m || refutations[2] == m) return true; // continue

        if (moves.size() < 48)
            tt.prefetch(pos->getHashAfter(m));

        ScoredMove newMove = ScoredMove(m, scoreQuiet<Me>(m));
        moves.insert_sorted(newMove, compare);
        
        return true;
    });

    // Good quiets
    for (current = endBadQuiets = beginQuiets; current != moves.end() && !skipQuiets; current++) {
        if (current->score < -4000) {
            *endBadQuiets++ = *current;
            continue;
        }

        CALL_HANDLER(current->move, skipQuiets);
    }

    // Bad tacticals
    for (current = moves.begin(); current != endBadTacticals; current++) {
        tt.prefetch(pos->getHashAfter(current->move));
        CALL_HANDLER(current->move, skipQuiets);
    }

    // Bad quiets
    for (current = beginQuiets; current != endBadQuiets && !skipQuiets; current++) {
        tt.prefetch(pos->getHashAfter(current->move));
        CALL_HANDLER(current->move, skipQuiets);
    }

    return true;
}

template<Side Me>
MoveScore MovePicker::scoreEvasion(Move m) {
    if (pos->isTactical(m)) {
        return scoreTactical<Me>(m) + 1000000;
    } else {
        if (moveHistory != nullptr) [[likely]]
            return moveHistory->getHistory<Me>(m);
    }

    return 0;
}

template<Side Me>
MoveScore MovePicker::scoreTactical(Move m) {
    return PieceValue<MG>(pos->getPieceAt(moveTo(m))) - (int)pieceType(pos->getPieceAt(moveFrom(m))); // MVV-LVA
}

template<Side Me>
MoveScore MovePicker::scoreQuiet(Move m) {
    assert(movePromotionType(m) != QUEEN);

    Square from = moveFrom(m), to = moveTo(m);
    PieceType pt = pieceType(pos->getPieceAt(from));
    MoveScore score = NB_PIECE_TYPE-(int)pt;

    if (moveType(m) == PROMOTION) [[unlikely]]
        return -10000;

    Bitboard threatened = pos->threatsFor(pt);
    score += ((threatened & from) && !(threatened & to)) * PieceThreatenedValue[pt];

    if (moveHistory != nullptr) [[likely]]
        score += moveHistory->getHistory<Me>(m);

    // TODO: refactor this!
    switch (pt) {
        case PAWN:
            if (pawnAttacks<Me>(bb(to)) & pos->getPiecesBB(~Me, KING)) {
                score += 10000;
            }
            break;
        case KNIGHT:
            if (attacks<KNIGHT>(to, pos->getPiecesBB()) & pos->getPiecesBB(~Me, KING)) {
                score += 10000;
            }
            break;
        case BISHOP:
            if (attacks<BISHOP>(to, pos->getPiecesBB()) & pos->getPiecesBB(~Me, KING)) {
                score += 10000;
            }
        case ROOK:
            if (attacks<ROOK>(to, pos->getPiecesBB()) & pos->getPiecesBB(~Me, KING)) {
                score += 10000;
            }
        case QUEEN:
            if (attacks<QUEEN>(to, pos->getPiecesBB()) & pos->getPiecesBB(~Me, KING)) {
                score += 10000;
            }
        default:
            break;
    }

    return score;
}

} /* namespace Belette */

#endif /* MOVEPICKER_H_INCLUDED */
