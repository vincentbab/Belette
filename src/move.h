#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include "chess.h"
#include "position.h"
#include "fixed_vector.h"

namespace BabChess {

typedef fixed_vector<Move, MAX_MOVE, uint8_t> MoveList;

enum MoveGenType {
    QUIET_MOVES = 1,
    NON_QUIET_MOVES = 2,
    ALL_MOVES = QUIET_MOVES | NON_QUIET_MOVES,
};

#define MAKE_MOVE_HANDLER(type) \
    auto doMoveHandler = [from, to](Position &p) { p.doMove<Me, type, false, false>(from, to); }; \
    auto undoMoveHandler = [from, to](Position &p) { p.undoMove<Me, type>(from, to); }

#define MAKE_MOVE_HANDLER_PAWN(type, isDoublePush) \
    auto doMoveHandler = [from, to](Position &p) { p.doMove<Me, type, true, isDoublePush>(from, to); }; \
    auto undoMoveHandler = [from, to](Position &p) { p.undoMove<Me, type>(from, to); }

#define MAKE_MOVE_HANDLER_PROMOTION(type, promotionType) \
    auto doMoveHandler = [from, to](Position &p) { p.doMove<Me, type, true, false>(from, to, promotionType); }; \
    auto undoMoveHandler = [from, to](Position &p) { p.undoMove<Me, type>(from, to); }

#define CALL_HANDLER(move) if (!handler(move, doMoveHandler, undoMoveHandler)) return false


template<Side Me, PieceType PromotionType, typename Handler>
inline bool enumeratePromotion(Square from, Square to, const Handler& handler) {
    MAKE_MOVE_HANDLER_PROMOTION(PROMOTION, PromotionType);
    CALL_HANDLER(makeMove<PROMOTION>(from, to, QUEEN));

    return true;
}

template<Side Me, typename Handler>
inline bool enumeratePromotions(Square from, Square to, const Handler& handler) {
    if (!enumeratePromotion<Me, QUEEN>(from, to, handler)) return false;
    if (!enumeratePromotion<Me, KNIGHT>(from, to, handler)) return false;
    if (!enumeratePromotion<Me, ROOK>(from, to, handler)) return false;
    if (!enumeratePromotion<Me, BISHOP>(from, to, handler)) return false;

    return true;
}


template<Side Me, bool InCheck, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumeratePawnMoves(const Position &pos, const Handler& handler) {
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
        Bitboard pawns = pos.getPiecesBB(Me, PAWN) & ~Rank7 & ~pinDiag;
        Bitboard singlePushes = (shift<Up>(pawns & ~pinOrtho) | (shift<Up>(pawns & pinOrtho) & pinOrtho)) & emptyBB;
        Bitboard doublePushes = shift<Up>(singlePushes & Rank3) & emptyBB;

        if constexpr (InCheck) {
            singlePushes &= checkMask;
            doublePushes &= checkMask;
        }

        bitscan_loop(singlePushes) {
            Square to = bitscan(singlePushes);
            Square from = to - Up;
            MAKE_MOVE_HANDLER_PAWN(NORMAL, false);
            CALL_HANDLER(makeMove(from, to));
        }
        bitscan_loop(doublePushes) {
            Square to = bitscan(doublePushes);
            Square from = to - Up - Up;
            MAKE_MOVE_HANDLER_PAWN(NORMAL, true);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    // Normal Capture
    if constexpr (MGType & NON_QUIET_MOVES) {
        Bitboard pawns = pos.getPiecesBB(Me, PAWN) & ~Rank7 & ~pinOrtho;
        Bitboard capL = (shift<UpLeft>(pawns & ~pinDiag) | (shift<UpLeft>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);
        Bitboard capR = (shift<UpRight>(pawns & ~pinDiag) | (shift<UpRight>(pawns & pinDiag) & pinDiag)) & pos.getPiecesBB(Opp);

        if constexpr (InCheck) {
            capL &= checkMask;
            capR &= checkMask;
        }

        bitscan_loop(capL) {
            Square to = bitscan(capL);
            Square from = to - UpLeft;
            MAKE_MOVE_HANDLER_PAWN(NORMAL, false);
            CALL_HANDLER(makeMove(from, to));
        }
        bitscan_loop(capR) {
            Square to = bitscan(capR);
            Square from = to - UpRight;
            MAKE_MOVE_HANDLER_PAWN(NORMAL, false);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    if constexpr (MGType & NON_QUIET_MOVES) {
        Bitboard pawnsCanPromote = pos.getPiecesBB(Me, PAWN) & Rank7;
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
                    if (!enumeratePromotions<Me>(from, to, handler)) return false;
                }
                bitscan_loop(capRPromotions) {
                    Square to = bitscan(capRPromotions);
                    Square from = to - UpRight;
                    if (!enumeratePromotions<Me>(from, to, handler)) return false;
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
                    if (!enumeratePromotions<Me>(from, to, handler)) return false;
                }
            }
        }

        // Enpassant
        if (pos.getEpSquare() != SQ_NONE) {
            assert(rankOf(pos.getEpSquare()) == relativeRank(Me, RANK_6));
            assert(pos.isEmpty(pos.getEpSquare()));
            assert(pieceType(pos.getPieceAt(pos.getEpSquare() - pawnDirection(Me))) == PAWN);

            Bitboard epcapture = bb(pos.getEpSquare() - pawnDirection(Me)); // Pawn who will be captured enpassant
            Bitboard enpassants = pawnAttacks(Opp, pos.getEpSquare()) & pos.getPiecesBB(Me, PAWN) & ~pinOrtho;

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
                    MAKE_MOVE_HANDLER_PAWN(EN_PASSANT, false);
                    CALL_HANDLER(makeMove<EN_PASSANT>(from, to));
                }
            }
        }

        return true;
    }

    
}

template<Side Me, typename Handler>
inline bool enumerateCastlingMoves(const Position &pos, const Handler& handler) {
    Square kingSquare = pos.getKingSquare(Me);

    for (auto cr : {Me & KING_SIDE, Me & QUEEN_SIDE}) {
        if (pos.canCastle(cr) && pos.isEmpty(CastlingPath[cr]) && !(pos.checkedSquares() & CastlingKingPath[cr])) {
            Square from = kingSquare;
            Square to = CastlingKingTo[cr];
            MAKE_MOVE_HANDLER(CASTLING);
            CALL_HANDLER(makeMove<CASTLING>(from, to));
        }
    }

    return true;
}

template<Side Me, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumerateKingMoves(const Position &pos, const Handler& handler) {
    Square from = pos.getKingSquare(Me);

    Bitboard dest = attacks<KING>(from) & ~pos.getPiecesBB(Me) & ~pos.checkedSquares();

    if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
    if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

    bitscan_loop(dest) {
        Square to = bitscan(dest);
        MAKE_MOVE_HANDLER(NORMAL);
        CALL_HANDLER(makeMove(from, to));
    }

    return true;
}

template<Side Me, bool InCheck, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumerateKnightMoves(const Position &pos, const Handler& handler) {
    Bitboard pieces = pos.getPiecesBB(Me, KNIGHT) & ~(pos.pinDiag() | pos.pinOrtho());

    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<KNIGHT>(from) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            MAKE_MOVE_HANDLER(NORMAL);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    return true;
}

template<Side Me, bool InCheck, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumerateSliderMoves(const Position &pos, const Handler& handler) {
    Bitboard pieces;
    Bitboard oppPiecesBB = pos.getPiecesBB(~Me);

    // Non-pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~pos.pinOrtho() & ~pos.pinDiag();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            MAKE_MOVE_HANDLER(NORMAL);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    // Pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~pos.pinOrtho() & pos.pinDiag();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinDiag();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            MAKE_MOVE_HANDLER(NORMAL);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    // Non-pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~pos.pinDiag() & ~pos.pinOrtho();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            MAKE_MOVE_HANDLER(NORMAL);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    // Pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~pos.pinDiag() & pos.pinOrtho();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinOrtho();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= oppPiecesBB;
        if constexpr (MGType == QUIET_MOVES) dest &= ~oppPiecesBB;

        bitscan_loop(dest) {
            Square to = bitscan(dest);
            MAKE_MOVE_HANDLER(NORMAL);
            CALL_HANDLER(makeMove(from, to));
        }
    }

    return true;
}

template<Side Me, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    //static_assert( std::is_same_v<typename std::invoke_result_t<Handler, Move>, bool>, "Handler should have bool return type");

    switch(popcount(pos.checkers())) {
        case 0:
            {
                if (!enumeratePawnMoves<Me, false, Handler, MGType>(pos, handler)) return false;
                if (!enumerateKnightMoves<Me, false, Handler, MGType>(pos, handler)) return false;
                if (!enumerateSliderMoves<Me, false, Handler, MGType>(pos, handler)) return false;
                if constexpr (MGType & QUIET_MOVES) if (!enumerateCastlingMoves<Me, Handler>(pos, handler)) return false;
                if (!enumerateKingMoves<Me, Handler, MGType>(pos, handler)) return false;

                return true;
            }
        case 1:
            {
                if (!enumeratePawnMoves<Me, true, Handler, MGType>(pos, handler)) return false;
                if (!enumerateKnightMoves<Me, true, Handler, MGType>(pos, handler)) return false;
                if (!enumerateSliderMoves<Me, true, Handler, MGType>(pos, handler)) return false;
                if (!enumerateKingMoves<Me, Handler, MGType>(pos, handler)) return false;

                return true;
            }
        case 2:
        default:
            {
                // If we are in double check only king moves are allowed
                if (!enumerateKingMoves<Me, Handler, MGType>(pos, handler)) return false;

                return true;
            }
    }
}

template<typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    return pos.getSideToMove() == WHITE 
            ? enumerateLegalMoves<WHITE, Handler, MGType>(pos, handler)
            : enumerateLegalMoves<BLACK, Handler, MGType>(pos, handler);
}

inline void generateLegalMoves(const Position &pos, MoveList &moves) {
    enumerateLegalMoves(pos, [&](Move m, auto doMH, auto undoMH) {
        moves.push_back(m); return true;
    });
}

} /* namespace BabChess */

#undef CALL_HANDLER
#undef MAKE_MOVE_HANDLER
#undef MAKE_MOVE_HANDLER_PAWN
#undef MAKE_MOVE_HANDLER_PROMOTION

#endif /* MOVE_H_INCLUDED */
