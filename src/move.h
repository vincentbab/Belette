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

template<typename Handler>
inline bool enumeratePromotions(Square from, Square to, const Handler& handler) {
    if (!handler(makeMove<PROMOTION>(from, to, QUEEN))) return false;
    if (!handler(makeMove<PROMOTION>(from, to, KNIGHT))) return false;
    if (!handler(makeMove<PROMOTION>(from, to, ROOK))) return false;
    if (!handler(makeMove<PROMOTION>(from, to, BISHOP))) return false;

    return true;
}


template<Side Me, bool InCheck, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumeratePawnMoves(const Position &pos, const Handler& handler) {
    constexpr Side Opp = ~Me;
    constexpr Bitboard rank3 = (Me == WHITE) ? Rank3BB : Rank6BB;
    constexpr Bitboard rank7 = (Me == WHITE) ? Rank7BB : Rank2BB;
    constexpr Direction Up = (Me == WHITE) ? UP : DOWN;
    constexpr Direction UpLeft = (Me == WHITE) ? UP_LEFT : DOWN_RIGHT;
    constexpr Direction UpRight = (Me == WHITE) ? UP_RIGHT : DOWN_LEFT;

    // Single & Double Push
    if constexpr (MGType & QUIET_MOVES) {
        Bitboard pawns = pos.getPiecesBB(Me, PAWN) & ~rank7 & ~pos.pinDiag();
        Bitboard singlePushes = (shift<Up>(pawns & ~pos.pinOrtho()) | (shift<Up>(pawns & pos.pinOrtho()) & pos.pinOrtho())) & pos.getEmptyBB();
        Bitboard doublePushes = shift<Up>(singlePushes & rank3) & pos.getEmptyBB();

        if constexpr (InCheck) {
            singlePushes &= pos.checkMask();
            doublePushes &= pos.checkMask();
        }

        bitscan_loop(singlePushes) {
            Square to = bitscan(singlePushes);
            if (!handler(makeMove(to - Up, to))) return false;
        }
        bitscan_loop(doublePushes) {
            Square to = bitscan(doublePushes);
            if (!handler(makeMove(to - Up - Up, to))) return false;
        }
    }

    // Normal Capture
    if constexpr (MGType & NON_QUIET_MOVES) {
        Bitboard pawns = pos.getPiecesBB(Me, PAWN) & ~rank7 & ~pos.pinOrtho();
        Bitboard capL = (shift<UpLeft>(pawns & ~pos.pinDiag()) | (shift<UpLeft>(pawns & pos.pinDiag()) & pos.pinDiag())) & pos.getPiecesBB(Opp);
        Bitboard capR = (shift<UpRight>(pawns & ~pos.pinDiag()) | (shift<UpRight>(pawns & pos.pinDiag()) & pos.pinDiag())) & pos.getPiecesBB(Opp);

        if constexpr (InCheck) {
            capL &= pos.checkMask();
            capR &= pos.checkMask();
        }

        bitscan_loop(capL) {
            Square to = bitscan(capL);
            if (!handler(makeMove(to - UpLeft, to))) return false;
        }
        bitscan_loop(capR) {
            Square to = bitscan(capR);
            if (!handler(makeMove(to - UpRight, to))) return false;
        }
    }

    if constexpr (MGType & NON_QUIET_MOVES) {
        Bitboard pawnsCanPromote = pos.getPiecesBB(Me, PAWN) & rank7;
        if (pawnsCanPromote) {
            // Capture Promotion
            {
                Bitboard pawns = pawnsCanPromote & ~pos.pinOrtho();
                Bitboard capLPromotions = (shift<UpLeft>(pawns & ~pos.pinDiag()) | (shift<UpLeft>(pawns & pos.pinDiag()) & pos.pinDiag())) & pos.getPiecesBB(Opp);
                Bitboard capRPromotions = (shift<UpRight>(pawns & ~pos.pinDiag()) | (shift<UpRight>(pawns & pos.pinDiag()) & pos.pinDiag())) & pos.getPiecesBB(Opp);

                if constexpr (InCheck) {
                    capLPromotions &= pos.checkMask();
                    capRPromotions &= pos.checkMask();
                }

                bitscan_loop(capLPromotions) {
                    Square to = bitscan(capLPromotions);
                    if (!enumeratePromotions(to - UpLeft, to, handler)) return false;
                }
                bitscan_loop(capRPromotions) {
                    Square to = bitscan(capRPromotions);
                    if (!enumeratePromotions(to - UpRight, to, handler)) return false;
                }
            }

            // Quiet Promotion
            {
                Bitboard pawns = pawnsCanPromote & ~pos.pinDiag();
                Bitboard quietPromotions = (shift<Up>(pawns & ~pos.pinOrtho()) | (shift<Up>(pawns & pos.pinOrtho()) & pos.pinOrtho())) & pos.getEmptyBB();

                if constexpr (InCheck) quietPromotions &= pos.checkMask();

                bitscan_loop(quietPromotions) {
                    Square to = bitscan(quietPromotions);
                    if (!enumeratePromotions(to - Up, to, handler)) return false;
                }
            }
        }

        // Enpassant
        if (pos.getEpSquare() != SQ_NONE) {
            assert(rankOf(pos.getEpSquare()) == relativeRank(Me, RANK_6));
            assert(pos.isEmpty(pos.getEpSquare()));
            assert(pieceType(pos.getPieceAt(pos.getEpSquare() - pawnDirection(Me))) == PAWN);

            Bitboard epcapture = bb(pos.getEpSquare() - pawnDirection(Me)); // Pawn who will be captured enpassant
            Bitboard enpassants = pawnAttacks(Opp, pos.getEpSquare()) & pos.getPiecesBB(Me, PAWN) & ~pos.pinOrtho();

            if constexpr (InCheck) {
                // Case: 3k4/8/8/4pP2/3K4/8/8/8 w - e6 0 2
                // If we are in check by the pawn who can be captured enpasant don't use the checkMask because it does not contain the EpSquare
                if (!(pos.checkers() & epcapture)) {
                    enpassants &= pos.checkMask();
                }
            }

            bitscan_loop(enpassants) {
                Square from = bitscan(enpassants);
                Bitboard epRank = bb(rankOf(from));

                // Case: 2r1k2r/Pp3ppp/1b3nbN/nPPp4/BB2P3/q2P1N2/Pp4PP/R2Q1RK1 w k d6 0 3
                // Pawn is pinned diagonaly and EpSquare is not part of the pin mask
                if ( (bb(from) & pos.pinDiag()) && !(bb(pos.getEpSquare()) & pos.pinDiag()))
                    continue;
                
                // Case: 3k4/8/8/2KpP2r/8/8/8/8 w - - 0 2
                // Case: 8/2p5/8/KP1p2kr/5pP1/8/4P3/6R1 b - g3 0 3
                // Check for the special case where our king is on the same rank than an opponent rook(or queen) and enpassant capture will put us in check
                if ( !((epRank & pos.getPiecesBB(Me, KING)) && (epRank & pos.getPiecesBB(Opp, ROOK, QUEEN)))
                    || !(attacks<ROOK>(pos.getKingSquare(Me), pos.getPiecesBB() ^ bb(from) ^ epcapture) & pos.getPiecesBB(Opp, ROOK, QUEEN)) )
                {
                    if (!handler(makeMove<EN_PASSANT>(from, pos.getEpSquare()))) return false;
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
            if (!handler(makeMove<CASTLING>(kingSquare, CastlingKingTo[cr]))) return false;
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
        if (!handler(makeMove(from, bitscan(dest)))) return false;
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
            if (!handler(makeMove(from, bitscan(dest)))) return false;
        }
    }

    return true;
}

template<Side Me, bool InCheck, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumerateSliderMoves(const Position &pos, const Handler& handler) {
    Bitboard pieces;

    // Non-pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~pos.pinOrtho() & ~pos.pinDiag();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitscan_loop(dest) {
            if (!handler(makeMove(from, bitscan(dest)))) return false;
        }
    }

    // Pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~pos.pinOrtho() & pos.pinDiag();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinDiag();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitscan_loop(dest) {
            if (!handler(makeMove(from, bitscan(dest)))) return false;
        }
    }

    // Non-pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~pos.pinDiag() & ~pos.pinOrtho();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitscan_loop(dest) {
            if (!handler(makeMove(from, bitscan(dest)))) return false;
        }
    }

    // Pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~pos.pinDiag() & pos.pinOrtho();
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & pos.pinOrtho();

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitscan_loop(dest) {
            if (!handler(makeMove(from, bitscan(dest)))) return false;
        }
    }

    return true;
}

template<Side Me, typename Handler, MoveGenType MGType = ALL_MOVES>
inline bool enumerateLegalMoves(const Position &pos, const Handler& handler) {
    static_assert( std::is_same_v<typename std::invoke_result_t<Handler, Move>, bool>, "Handler should have bool return type");

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
    enumerateLegalMoves(pos, [&](Move m) {
        moves.push_back(m); return true;
    });
}

} /* namespace BabChess */

#endif /* MOVE_H_INCLUDED */
