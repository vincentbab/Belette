#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include "chess.h"
#include "position.h"
#include "array.h"

namespace BabChess {

typedef Array<Move, MAX_MOVE, uint8_t> MoveList;

struct CheckInfo {
    Bitboard checkMask;
    Bitboard pinDiag;
    Bitboard pinOrtho;
};

inline void generatePromotions(MoveList &moves, Square from, Square to) {
    moves.push_back(makeMove<PROMOTION>(from, to, QUEEN));
    moves.push_back(makeMove<PROMOTION>(from, to, KNIGHT));
    moves.push_back(makeMove<PROMOTION>(from, to, ROOK));
    moves.push_back(makeMove<PROMOTION>(from, to, BISHOP));
}

template<Side Me, bool InCheck>
inline void generatePawnMoves(const Position &pos, MoveList &moves, const CheckInfo& ci, Bitboard checkers = EmptyBB) {
    
    constexpr Side Opp = (Me == WHITE) ? BLACK : WHITE;
    constexpr Bitboard rank3 = (Me == WHITE) ? Rank3BB : Rank6BB;
    constexpr Bitboard rank7 = (Me == WHITE) ? Rank7BB : Rank2BB;
    constexpr Direction Up = (Me == WHITE) ? UP : DOWN;
    constexpr Direction UpLeft = (Me == WHITE) ? UP_LEFT : DOWN_RIGHT;
    constexpr Direction UpRight = (Me == WHITE) ? UP_RIGHT : DOWN_LEFT;

    // Single & Double Push
    {
        Bitboard pawns = pos.getPiecesBB(Me, PAWN) & ~rank7 & ~ci.pinDiag;
        Bitboard singlePushes = (shift<Up>(pawns & ~ci.pinOrtho) | (shift<Up>(pawns & ci.pinOrtho) & ci.pinOrtho)) & pos.getEmptyBB();
        Bitboard doublePushes = shift<Up>(singlePushes & rank3) & pos.getEmptyBB();

        if constexpr (InCheck) {
            singlePushes &= ci.checkMask;
            doublePushes &= ci.checkMask;
        }

        bitscan_loop(singlePushes) {
            Square to = bitscan(singlePushes);
            moves.push_back(makeMove(to - Up, to));
        }
        bitscan_loop(doublePushes) {
            Square to = bitscan(doublePushes);
            moves.push_back(makeMove(to - Up - Up, to));
        }
    }

    // Normal Capture
    {
        Bitboard pawns = pos.getPiecesBB(Me, PAWN) & ~rank7 & ~ci.pinOrtho;
        Bitboard capL = (shift<UpLeft>(pawns & ~ci.pinDiag) | (shift<UpLeft>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp);
        Bitboard capR = (shift<UpRight>(pawns & ~ci.pinDiag) | (shift<UpRight>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp);

        if constexpr (InCheck) {
            capL &= ci.checkMask;
            capR &= ci.checkMask;
        }

        bitscan_loop(capL) {
            Square to = bitscan(capL);
            moves.push_back(makeMove(to - UpLeft, to));
        }
        bitscan_loop(capR) {
            Square to = bitscan(capR);
            moves.push_back(makeMove(to - UpRight, to));
        }
    }

    Bitboard pawnsCanPromote = pos.getPiecesBB(Me, PAWN) & rank7;
    if (pawnsCanPromote) {
        // Quiet Promotion
        {
            Bitboard pawns = pawnsCanPromote & ~ci.pinDiag;
            Bitboard quietPromotions = (shift<Up>(pawns & ~ci.pinOrtho) | (shift<Up>(pawns & ci.pinOrtho) & ci.pinOrtho)) & pos.getEmptyBB();

            if constexpr (InCheck) quietPromotions &= ci.checkMask;

            bitscan_loop(quietPromotions) {
                Square to = bitscan(quietPromotions);
                generatePromotions(moves, to - Up, to);
            }
        }

        // Capture Promotion
        {
            Bitboard pawns = pawnsCanPromote & ~ci.pinOrtho;
            Bitboard capLPromotions = (shift<UpLeft>(pawns & ~ci.pinDiag) | (shift<UpLeft>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp);
            Bitboard capRPromotions = (shift<UpRight>(pawns & ~ci.pinDiag) | (shift<UpRight>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp);

            if constexpr (InCheck) {
                capLPromotions &= ci.checkMask;
                capRPromotions &= ci.checkMask;
            }

            bitscan_loop(capLPromotions) {
                Square to = bitscan(capLPromotions);
                generatePromotions(moves, to - UpLeft, to);
            }
            bitscan_loop(capRPromotions) {
                Square to = bitscan(capRPromotions);
                generatePromotions(moves, to - UpRight, to);
            }
        }
    }

    // Enpassant
    if (pos.getEpSquare() != SQ_NONE) {
        assert(rankOf(pos.getEpSquare()) == relativeRank(Me, RANK_6));
        assert(pos.isEmpty(pos.getEpSquare()));
        assert(pieceType(pos.getPieceAt(pos.getEpSquare() - pawnDirection(Me))) == PAWN);

        Bitboard epcapture = bb(pos.getEpSquare() - pawnDirection(Me)); // Pawn who will be captured enpassant
        Bitboard enpassants = pawnAttacks(Opp, pos.getEpSquare()) & pos.getPiecesBB(Me, PAWN) & ~ci.pinOrtho;

        if constexpr (InCheck) {
            // Case: 3k4/8/8/4pP2/3K4/8/8/8 w - e6 0 2
            // If we are in check by the pawn who can be captured enpasant don't use the checkMask because it does not contain the EpSquare
            if (!(checkers & epcapture)) {
                enpassants &= ci.checkMask;
            }
        }

        bitscan_loop(enpassants) {
            Square from = bitscan(enpassants);
            Bitboard epRank = bb(rankOf(from));

            // Case: 2r1k2r/Pp3ppp/1b3nbN/nPPp4/BB2P3/q2P1N2/Pp4PP/R2Q1RK1 w k d6 0 3
            // Pawn is pinned diagonaly and EpSquare is not part of the pin mask
            if ( (bb(from) & ci.pinDiag) && !(bb(pos.getEpSquare()) & ci.pinDiag))
                continue;
            
            // Case: 3k4/8/8/2KpP2r/8/8/8/8 w - - 0 2
            // Case: 8/2p5/8/KP1p2kr/5pP1/8/4P3/6R1 b - g3 0 3
            // Check for the special case where our king is on the same rank than an opponent rook(or queen) and enpassant capture will put us in check
            if ( !((epRank & pos.getPiecesBB(Me, KING)) && (epRank & pos.getPiecesBB(Opp, ROOK, QUEEN)))
                || !(attacks<ROOK>(pos.getKingSquare(Me), pos.getPiecesBB() ^ bb(from) ^ epcapture) & pos.getPiecesBB(Opp, ROOK, QUEEN)) )
            {
                moves.push_back(makeMove<EN_PASSANT>(from, pos.getEpSquare()));
            }
        }
    }
}

template<Side Me>
inline void generateCastlingMoves(const Position &pos, MoveList &moves, Bitboard checkedSquares) {
    Square kingSquare = pos.getKingSquare(Me);

    for (auto cr : {Me & KING_SIDE, Me & QUEEN_SIDE}) {
        if (pos.canCastle(cr) && pos.isEmpty(CastlingPath[cr]) && !(checkedSquares & CastlingKingPath[cr])) {
            moves.push_back(makeMove<CASTLING>(kingSquare, CastlingKingTo[cr]));
        }
    }
}

template<Side Me>
inline void generateKingMoves(const Position &pos, MoveList &moves, Bitboard checkedSquares) {
    Square from = pos.getKingSquare(Me);

    Bitboard dest = attacks<KING>(from) & ~pos.getPiecesBB(Me) & ~checkedSquares;

    bitscan_loop(dest) {
        moves.push_back(makeMove(from, bitscan(dest)));
    }
}

template<Side Me, bool InCheck>
inline void generateKnightMoves(const Position &pos, MoveList &moves, const CheckInfo& ci) {
    Bitboard pieces = pos.getPiecesBB(Me, KNIGHT) & ~(ci.pinDiag | ci.pinOrtho);

    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<KNIGHT>(from) & ~pos.getPiecesBB(Me);
        if constexpr (InCheck) dest &= ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }
}

template<Side Me, bool InCheck>
inline void generateSliderMoves(const Position &pos, MoveList &moves, const CheckInfo& ci) {
    Bitboard pieces;

    // Non-pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~ci.pinOrtho & ~ci.pinDiag;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);
        if constexpr (InCheck) dest &= ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }

    // Pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~ci.pinOrtho & ci.pinDiag;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & ci.pinDiag;
        if constexpr (InCheck) dest &= ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }

    // Non-pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~ci.pinDiag & ~ci.pinOrtho;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me);
        if constexpr (InCheck) dest &= ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }

    // Pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~ci.pinDiag & ci.pinOrtho;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & ci.pinOrtho;
        if constexpr (InCheck) dest &= ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }
}

template<Side Me>
inline Bitboard getCheckedSquares(const Position& pos) {
    constexpr Side Opp = ~Me;

    Bitboard checkedSquares = pawnAttacks<Opp>(pos.getPiecesBB(Opp, PAWN)) | (attacks<KING>(pos.getKingSquare(Opp)));
    //knight
    Bitboard enemies = pos.getPiecesBB(Opp, KNIGHT);
    bitscan_loop(enemies) {
        checkedSquares |= attacks<KNIGHT>(bitscan(enemies), pos.getPiecesBB());
    }

    //bishop & queen
    enemies = pos.getPiecesBB(Opp, BISHOP, QUEEN);
    bitscan_loop(enemies) {
        checkedSquares |= attacks<BISHOP>(bitscan(enemies), pos.getPiecesBB() ^ pos.getPiecesBB(Me, KING));
    }
    //rook & queen
    enemies = pos.getPiecesBB(Opp, ROOK, QUEEN);
    bitscan_loop(enemies) {
        checkedSquares |= attacks<ROOK>(bitscan(enemies), pos.getPiecesBB() ^ pos.getPiecesBB(Me, KING));
    }

    return checkedSquares;
}

template<Side Me, bool InCheck>
inline CheckInfo& getCheckInfo(const Position& pos, CheckInfo& ci) {
    constexpr Side Opp = ~Me;
    Square ksq = pos.getKingSquare(Me);

    if constexpr (InCheck) {
        ci.checkMask = (pawnAttacks(Me, ksq) & pos.getPiecesBB(Opp, PAWN)) | (attacks<KNIGHT>(ksq) & pos.getPiecesBB(Opp, KNIGHT));
    }

    ci.pinDiag = ci.pinOrtho = EmptyBB;

    // TODO: merge diag & ortho loops
    Bitboard pinners = (attacks<BISHOP>(ksq, pos.getPiecesBB(Opp)) & pos.getPiecesBB(Opp, BISHOP, QUEEN));
    bitscan_loop(pinners) {
        Square s = bitscan(pinners);
        Bitboard b = betweenBB(ksq, s);

        switch(popcount(b & pos.getPiecesBB(Me))) {
            case 0: if constexpr (InCheck) ci.checkMask |= b | bb(s); break;
            case 1: ci.pinDiag |= b | bb(s);
        }
    }

    pinners = (attacks<ROOK>(ksq, pos.getPiecesBB(Opp)) & pos.getPiecesBB(Opp, ROOK, QUEEN));
    bitscan_loop(pinners) {
        Square s = bitscan(pinners);
        Bitboard b = betweenBB(ksq, s);

        switch(popcount(b & pos.getPiecesBB(Me))) {
            case 0: if constexpr (InCheck) ci.checkMask |= b | bb(s); break;
            case 1: ci.pinOrtho |= b | bb(s);
        }
    }

    /*Bitboard pinners = (attacks<BISHOP>(ksq, pos.getPiecesBB(Opp)) & pos.getPiecesBB(Opp, BISHOP, QUEEN)) 
                     | (attacks<ROOK>  (ksq, pos.getPiecesBB(Opp)) & pos.getPiecesBB(Opp, ROOK, QUEEN));

    bitscan_loop(pinners) {
        Square s = bitscan(pinners);
        Bitboard b = betweenBB(ksq, s);

        if (popcount(b & pos.getPiecesBB(Me)) == 0) {
            ci.checkMask |= b | bb(s);
        } else if (popcount(b & pos.getPiecesBB(Me)) == 1) {
            // FIXME: check if line or diagonal !
            if (bb(s) & pos.getPiecesBB(Opp, BISHOP, QUEEN))
                ci.pinDiag |= b | bb(s);
            if (bb(s) & pos.getPiecesBB(Opp, ROOK, QUEEN))
                ci.pinOrtho |= b | bb(s);
        }
    }*/

    return ci;
}

template<Side Me>
inline Bitboard getCheckers(const Position& pos) {
    Square ksq = pos.getKingSquare(Me);
    constexpr Side Opp = ~Me;

    return ((pawnAttacks(Me, ksq) & pos.getPiecesBB(Opp, PAWN))
        | (attacks<KNIGHT>(ksq) & pos.getPiecesBB(Opp, KNIGHT))
        | (attacks<BISHOP>(ksq, pos.getPiecesBB()) & pos.getPiecesBB(Opp, BISHOP, QUEEN))
        | (attacks<ROOK>(ksq, pos.getPiecesBB()) & pos.getPiecesBB(Opp, ROOK, QUEEN))
    );
}

template<Side Me>
inline void generateLegalMoves(const Position &pos, MoveList &moves) {

    Bitboard checkedSquares = getCheckedSquares<Me>(pos);
    Bitboard checkers = getCheckers<Me>(pos);

    switch(popcount(checkers)) {
        case 2:
            {
                // If we are in double check only king moves are allowed
                generateKingMoves<Me>(pos, moves, checkedSquares);

                return;
            }
        case 1:
            {
                CheckInfo ci = getCheckInfo<Me, true>(pos, ci);
                generatePawnMoves<Me, true>(pos, moves, ci, checkers);
                generateKnightMoves<Me, true>(pos, moves, ci);
                generateSliderMoves<Me, true>(pos, moves, ci);
                generateKingMoves<Me>(pos, moves, checkedSquares);

                return;
            }
        case 0:
            {
                CheckInfo ci = getCheckInfo<Me, false>(pos, ci);
                generatePawnMoves<Me, false>(pos, moves, ci);
                generateKnightMoves<Me, false>(pos, moves, ci);
                generateSliderMoves<Me, false>(pos, moves, ci);
                generateCastlingMoves<Me>(pos, moves, checkedSquares);
                generateKingMoves<Me>(pos, moves, checkedSquares);

                return;
            }
    }
}

inline void generateLegalMoves(const Position &pos, MoveList &moves) {
    pos.getSideToMove() == WHITE ? generateLegalMoves<WHITE>(pos, moves) : generateLegalMoves<BLACK>(pos, moves);
}

} /* namespace BabChess */

#endif /* MOVE_H_INCLUDED */
