#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include "chess.h"
#include "position.h"
#include "fixed_vector.h"
#include "utils.h"

namespace Belette {

using MoveList = fixed_vector<Move, MAX_MOVE, uint8_t>;

std::ostream& operator<<(std::ostream& os, const MoveList& pos);

enum MoveGenType {
    QUIET_MOVES = 1,
    TACTICAL_MOVES = 2,
    ALL_MOVES = QUIET_MOVES | TACTICAL_MOVES,
};

#define CALL_HANDLER(...) if (!handler(__VA_ARGS__)) return false

#define CALL_ENUMERATOR(...) if (!__VA_ARGS__) return false


template<Side Me, PieceType PromotionType, typename Handler>
inline bool enumeratePromotion(Square from, Square to, const Handler& handler) {
    CALL_HANDLER(makeMove<PROMOTION>(from, to, PromotionType));

    return true;
}

template<Side Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePromotions(Square from, Square to, const Handler& handler) {
    if constexpr (MGType & TACTICAL_MOVES) {
        CALL_ENUMERATOR(enumeratePromotion<Me, QUEEN>(from, to, handler));
    }
    
    if constexpr (MGType & QUIET_MOVES) {
        CALL_ENUMERATOR(enumeratePromotion<Me, KNIGHT>(from, to, handler));
        CALL_ENUMERATOR(enumeratePromotion<Me, ROOK>(from, to, handler));
        CALL_ENUMERATOR(enumeratePromotion<Me, BISHOP>(from, to, handler));
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnPromotionMoves(const Position &pos, Bitboard source, const Handler& handler) {
    constexpr Side Opp = ~Me;
    //constexpr Bitboard Rank3 = (Me == WHITE) ? Rank3BB : Rank6BB;
    constexpr Bitboard Rank7 = (Me == WHITE) ? Rank7BB : Rank2BB;
    constexpr Direction Up = (Me == WHITE) ? UP : DOWN;
    constexpr Direction UpLeft = (Me == WHITE) ? UP_LEFT : DOWN_RIGHT;
    constexpr Direction UpRight = (Me == WHITE) ? UP_RIGHT : DOWN_LEFT;

    Bitboard emptyBB = pos.getEmptyBB();
    Bitboard pinOrtho = pos.pinOrtho();
    Bitboard pinDiag = pos.pinDiag();
    Bitboard checkMask = pos.checkMask();

    Bitboard pawnsCanPromote = source & Rank7;
    if (pawnsCanPromote) {
        // Capture Promotion
        {
            Bitboard pawns = pawnsCanPromote & ~pinOrtho;
            Bitboard capLPromotions = (shift<UpLeft>(pawns & ~pinDiag) | (shift<UpLeft>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);
            Bitboard capRPromotions = (shift<UpRight>(pawns & ~pinDiag) | (shift<UpRight>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);

            if constexpr (InCheck) {
                capLPromotions &= checkMask;
                capRPromotions &= checkMask;
            }

            bitscan_loop(capLPromotions) {
                Square to = bitscan(capLPromotions);
                Square from = to - UpLeft;
                CALL_ENUMERATOR(enumeratePromotions<Me, MGType>(from, to, handler));
            }
            bitscan_loop(capRPromotions) {
                Square to = bitscan(capRPromotions);
                Square from = to - UpRight;
                CALL_ENUMERATOR(enumeratePromotions<Me, MGType>(from, to, handler));
            }
        }

        // Quiet Promotion
        {
            Bitboard pawns = pawnsCanPromote & ~pinDiag;
            Bitboard quietPromotions = (shift<Up>(pawns & ~pinOrtho) | (shift<Up>(pawns & pinOrtho) & pinOrtho)) & emptyBB;

            if constexpr (InCheck) quietPromotions &= checkMask;

            bitscan_loop(quietPromotions) {
                Square to = bitscan(quietPromotions);
                Square from = to - Up;
                CALL_ENUMERATOR(enumeratePromotions<Me, MGType>(from, to, handler));
            }
        }
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnEnpassantMoves(const Position &pos, Bitboard source, const Handler& handler) {
    constexpr Side Opp = ~Me;

    Bitboard pinOrtho = pos.pinOrtho();
    Bitboard pinDiag = pos.pinDiag();
    Bitboard checkMask = pos.checkMask();

    // Enpassant
    if (pos.getEpSquare() != SQ_NONE) {
        assert(rankOf(pos.getEpSquare()) == relativeRank(Me, RANK_6));
        assert(pos.isEmpty(pos.getEpSquare()));
        assert(pieceType(pos.getPieceAt(pos.getEpSquare() - pawnDirection(Me))) == PAWN);

        Bitboard epcapture = bb(pos.getEpSquare() - pawnDirection(Me)); // Pawn who will be captured enpassant
        Bitboard enpassants = pawnAttacks(Opp, pos.getEpSquare()) & source & ~pinOrtho;

        if constexpr (InCheck) {
            // Case: 3k4/8/8/4pP2/3K4/8/8/8 w - e6 0 2
            // If we are in check by the pawn who can be captured enpasant don't use the checkMask because it does not contain the EpSquare
            if (!(pos.checkers() & epcapture)) {
                enpassants &= checkMask;
            }
        }

        bitscan_loop(enpassants) {
            Square from = bitscan(enpassants);
            Bitboard epRank = bb(rankOf(from));

            // Case: 2r1k2r/Pp3ppp/1b3nbN/nPPp4/BB2P3/q2P1N2/Pp4PP/R2Q1RK1 w k d6 0 3
            // Pawn is pinned diagonaly and EpSquare is not part of the pin mask
            if ( (bb(from) & pinDiag) && !(bb(pos.getEpSquare()) & pinDiag))
                continue;
            
            // Case: 3k4/8/8/2KpP2r/8/8/8/8 w - - 0 2
            // Case: 8/2p5/8/KP1p2kr/5pP1/8/4P3/6R1 b - g3 0 3
            // Check for the special case where our king is on the same rank than an opponent rook(or queen) and enpassant capture will put us in check
            if ( !((epRank & pos.getPiecesBB(Me, KING)) && (epRank & pos.getPiecesBB(Opp, ROOK, QUEEN)))
                || !(attacks<ROOK>(pos.getKingSquare(Me), pos.getPiecesBB() ^ bb(from) ^ epcapture) & pos.getPiecesBB(Opp, ROOK, QUEEN)) )
            {
                Square to = pos.getEpSquare();
                CALL_HANDLER(makeMove<EN_PASSANT>(from, to));
            }
        }
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnNormalMoves(const Position &pos, Bitboard source, const Handler& handler) {
    constexpr Side Opp = ~Me;
    constexpr Bitboard Rank3 = (Me == WHITE) ? Rank3BB : Rank6BB;
    constexpr Bitboard Rank7 = (Me == WHITE) ? Rank7BB : Rank2BB;
    constexpr Direction Up = (Me == WHITE) ? UP : DOWN;
    constexpr Direction UpLeft = (Me == WHITE) ? UP_LEFT : DOWN_RIGHT;
    constexpr Direction UpRight = (Me == WHITE) ? UP_RIGHT : DOWN_LEFT;

    Bitboard emptyBB = pos.getEmptyBB();
    Bitboard pinOrtho = pos.pinOrtho();
    Bitboard pinDiag = pos.pinDiag();
    Bitboard checkMask = pos.checkMask();

    // Single & Double Push
    if constexpr (MGType & QUIET_MOVES) {
        Bitboard pawns = source & ~Rank7 & ~pinDiag;
        Bitboard singlePushes = (shift<Up>(pawns & ~pinOrtho) | (shift<Up>(pawns & pinOrtho) & pinOrtho)) & emptyBB;
        Bitboard doublePushes = shift<Up>(singlePushes & Rank3) & emptyBB;

        if constexpr (InCheck) {
            singlePushes &= checkMask;
            doublePushes &= checkMask;
        }

        bitscan_loop(singlePushes) {
            Square to = bitscan(singlePushes);
            Square from = to - Up;
            CALL_HANDLER(makeMove(from, to));
        }

        bitscan_loop(doublePushes) {
            Square to = bitscan(doublePushes);
            Square from = to - Up - Up;
            CALL_HANDLER(makeMove(from, to));
        }
    }

    // Normal Capture
    if constexpr (MGType & TACTICAL_MOVES) {
        Bitboard pawns = source & ~Rank7 & ~pinOrtho;
        Bitboard capL = (shift<UpLeft>(pawns & ~pinDiag) | (shift<UpLeft>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);
        Bitboard capR = (shift<UpRight>(pawns & ~pinDiag) | (shift<UpRight>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);

        if constexpr (InCheck) {
            capL &= checkMask;
            capR &= checkMask;
        }

        bitscan_loop(capL) {
            Square to = bitscan(capL);
            Square from = to - UpLeft;
            CALL_HANDLER(makeMove(from, to));
        }
        bitscan_loop(capR) {
            Square to = bitscan(capR);
            Square from = to - UpRight;
            CALL_HANDLER(makeMove(from, to));
        }
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumeratePawnMoves(const Position &pos, Bitboard source, const Handler& handler) {
    CALL_ENUMERATOR(enumeratePawnNormalMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    CALL_ENUMERATOR(enumeratePawnPromotionMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    
    if constexpr (MGType & TACTICAL_MOVES) {
        CALL_ENUMERATOR(enumeratePawnEnpassantMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    }

    return true;
}

template<Side Me, typename Handler>
inline bool enumerateCastlingMoves(const Position &pos, const Handler& handler) {
    Square kingSquare = pos.getKingSquare(Me);

    for (auto cr : {Me & KING_SIDE, Me & QUEEN_SIDE}) {
        if (pos.canCastle(cr) && pos.isEmpty(CastlingPath[cr]) && !(pos.checkedSquares() & CastlingKingPath[cr])) {
            Square from = kingSquare;
            Square to = CastlingKingTo[cr];
            CALL_HANDLER(makeMove<CASTLING>(from, to));
        }
    }

    return true;
}

template<Side Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateKingMoves(const Position &pos, Square from, const Handler& handler) {
    Bitboard dest = attacks<KING>(from) & ~pos.getPiecesBB(Me) & ~pos.checkedSquares();

    if constexpr (MGType == TACTICAL_MOVES) dest &= pos.getPiecesBB(~Me);
    if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

    bitscan_loop(dest) {
        Square to = bitscan(dest);
        CALL_HANDLER(makeMove(from, to));
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateKnightMoves(const Position &pos, Bitboard source, const Handler& handler) {
    Bitboard pieces = source & ~(pos.pinDiag() | pos.pinOrtho());

    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<KNIGHT>(from) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateBishopSliderMoves(const Position &pos, Bitboard source, const Handler& handler) {
    Bitboard pieces;
    Bitboard oppPiecesBB = pos.getPiecesBB(~Me);

    // Non-pinned Bishop & Queen
    pieces = source & ~pos.pinOrtho() & ~pos.pinDiag();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    // Pinned Bishop & Queen
    pieces = source & ~pos.pinOrtho() & pos.pinDiag();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinDiag();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    return true;
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateRookSliderMoves(const Position &pos, Bitboard source, const Handler& handler) {
    Bitboard pieces;
    Bitboard oppPiecesBB = pos.getPiecesBB(~Me);

    // Non-pinned Rook & Queen
    pieces = source & ~pos.pinDiag() & ~pos.pinOrtho();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    // Pinned Rook & Queen
    pieces = source & ~pos.pinDiag() & pos.pinOrtho();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinOrtho();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == TACTICAL_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    return true;
}

// Used only for Position::isLegal
template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateQueenSliderMoves(const Position &pos, Bitboard source, const Handler& handler) {
    CALL_ENUMERATOR(enumerateBishopSliderMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    CALL_ENUMERATOR(enumerateRookSliderMoves<Me, InCheck, MGType, Handler>(pos, source, handler));
    return true;
}

template<Side Me, MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    assert(pos.nbCheckers() < 3);

    switch(pos.nbCheckers()) {
        case 0:
            CALL_ENUMERATOR(enumeratePawnMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, PAWN), handler));
            CALL_ENUMERATOR(enumerateKnightMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, KNIGHT), handler));
            CALL_ENUMERATOR(enumerateBishopSliderMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, BISHOP, QUEEN), handler));
            CALL_ENUMERATOR(enumerateRookSliderMoves<Me, false, MGType, Handler>(pos, pos.getPiecesBB(Me, ROOK, QUEEN), handler));
            if constexpr (MGType & QUIET_MOVES) CALL_ENUMERATOR(enumerateCastlingMoves<Me, Handler>(pos, handler));
            CALL_ENUMERATOR(enumerateKingMoves<Me, MGType, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;
        case 1:
            CALL_ENUMERATOR(enumeratePawnMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, PAWN), handler));
            CALL_ENUMERATOR(enumerateKnightMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, KNIGHT), handler));
            CALL_ENUMERATOR(enumerateBishopSliderMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, BISHOP, QUEEN), handler));
            CALL_ENUMERATOR(enumerateRookSliderMoves<Me, true, ALL_MOVES, Handler>(pos, pos.getPiecesBB(Me, ROOK, QUEEN), handler));
            CALL_ENUMERATOR(enumerateKingMoves<Me, ALL_MOVES, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;
        default: //case 2:
            // If we are in double check only king moves are allowed
            CALL_ENUMERATOR(enumerateKingMoves<Me, ALL_MOVES, Handler>(pos, pos.getKingSquare(Me), handler));

            return true;
    }
}

template<MoveGenType MGType = ALL_MOVES, typename Handler>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    return pos.getSideToMove() == WHITE 
            ? enumerateLegalMoves<WHITE, MGType, Handler>(pos, handler)
            : enumerateLegalMoves<BLACK, MGType, Handler>(pos, handler);
}

inline void generateLegalMoves(const Position &pos, MoveList &moves) {
    enumerateLegalMoves(pos, [&](Move m) {
        moves.push_back(m); return true;
    });
}

} /* namespace Belette */

#endif /* MOVE_H_INCLUDED */
