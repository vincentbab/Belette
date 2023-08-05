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

template<Side Me>
inline void generatePawnMoves(const Position &pos, MoveList &moves, const CheckInfo& ci, Bitboard checkers) {
    
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
        Bitboard doublePushes = shift<Up>(singlePushes & rank3) & pos.getEmptyBB() & ci.checkMask;
        singlePushes &= ci.checkMask;

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
        Bitboard cap1 = (shift<UpLeft>(pawns & ~ci.pinDiag) | (shift<UpLeft>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp) & ci.checkMask;
        Bitboard cap2 = (shift<UpRight>(pawns & ~ci.pinDiag) | (shift<UpRight>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp) & ci.checkMask;

        bitscan_loop(cap1) {
            Square to = bitscan(cap1);
            moves.push_back(makeMove(to - UpLeft, to));
        }
        bitscan_loop(cap2) {
            Square to = bitscan(cap2);
            moves.push_back(makeMove(to - UpRight, to));
        }
    }

    Bitboard pawnsCanPromote = pos.getPiecesBB(Me, PAWN) & rank7;
    if (pawnsCanPromote) {
        // Quiet Promotion
        {
            Bitboard pawns = pawnsCanPromote & ~ci.pinDiag;
            Bitboard quietPromotions = (shift<Up>(pawns & ~ci.pinOrtho) | (shift<Up>(pawns & ci.pinOrtho) & ci.pinOrtho)) & pos.getEmptyBB() & ci.checkMask;

            bitscan_loop(quietPromotions) {
                Square to = bitscan(quietPromotions);
                generatePromotions(moves, to - Up, to);
            }
        }

        // Capture Promotion
        {
            Bitboard pawns = pawnsCanPromote & ~ci.pinOrtho;
            //Bitboard cap1Promotions = shift<UpLeft>(pos.getPiecesBB(Me, PAWN) & rank7 & ~ci.pinOrtho) & pos.getPiecesBB(Opp) & ci.checkMask;
            //Bitboard cap2Promotions = shift<UpRight>(pos.getPiecesBB(Me, PAWN) & rank7 & ~ci.pinOrtho) & pos.getPiecesBB(Opp) & ci.checkMask;
            Bitboard cap1Promotions = (shift<UpLeft>(pawns & ~ci.pinDiag) | (shift<UpLeft>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp) & ci.checkMask;
            Bitboard cap2Promotions = (shift<UpRight>(pawns & ~ci.pinDiag) | (shift<UpRight>(pawns & ci.pinDiag) & ci.pinDiag)) & pos.getPiecesBB(Opp) & ci.checkMask;

            bitscan_loop(cap1Promotions) {
                Square to = bitscan(cap1Promotions);
                generatePromotions(moves, to - UpLeft, to);
            }
            bitscan_loop(cap2Promotions) {
                Square to = bitscan(cap2Promotions);
                generatePromotions(moves, to - UpRight, to);
            }
        }
    }

    // Enpassant
    if (pos.getEpSquare() != SQ_NONE) {
        assert(rankOf(pos.getEpSquare()) == relativeRank(Me, RANK_6));
        assert(pos.isEmpty(pos.getEpSquare()));
        Bitboard epcapture = bb(pos.getEpSquare() - pawnDirection(Me)); // Captured pawn bitboard

        // Case: 3k4/8/8/4pP2/3K4/8/8/8 w - e6 0 2
        Bitboard epmask = ci.checkMask;
        if (checkers & epcapture) {
            epmask = ~EmptyBB;
        }

        Bitboard enpassants = pawnAttacks(Opp, pos.getEpSquare()) & pos.getPiecesBB(Me, PAWN) & ~ci.pinOrtho & epmask;
        //enpassants = (enpassants & ~ci.pinDiag) | (enpassants & ci.pinDiag & 

        bitscan_loop(enpassants) {
            Square from = bitscan(enpassants);
            Bitboard epRank = bb(rankOf(from));

            // Case: 2r1k2r/Pp3ppp/1b3nbN/nPPp4/BB2P3/q2P1N2/Pp4PP/R2Q1RK1 w k d6 0 3
            if ( (bb(from) & ci.pinDiag) && !(bb(pos.getEpSquare()) & ci.pinDiag))
                continue;
            
            // Case: 3k4/8/8/2KpP2r/8/8/8/8 w - - 0 2
            // Case: 8/2p5/8/KP1p2kr/5pP1/8/4P3/6R1 b - g3 0 3
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

    for(auto cr : {Me & KING_SIDE, Me & QUEEN_SIDE}) {
        if (pos.canCastle(cr) && pos.isEmpty(CastlingPath[cr]) && !(checkedSquares & CastlingKingPath[cr])) {
            moves.push_back(makeMove<CASTLING>(kingSquare, CastlingKingTo[cr]));
        }
    }
}

template<Side Me>
inline void generateKingMoves(const Position &pos, MoveList &moves, Bitboard checkedSquares) {
    Square from = pos.getKingSquare(Me);

    // King moves
    Bitboard dest = attacks<KING>(from) & ~pos.getPiecesBB(Me) & ~checkedSquares;

    bitscan_loop(dest) {
        moves.push_back(makeMove(from, bitscan(dest)));
    }
}

template<Side Me>
inline void generateKnightMoves(const Position &pos, MoveList &moves, const CheckInfo& ci) {
    Bitboard pieces = pos.getPiecesBB(Me, KNIGHT) & ~(ci.pinDiag | ci.pinOrtho);
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<KNIGHT>(from) & ~pos.getPiecesBB(Me) & ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }
}

template<Side Me>
inline void generateSliderMoves(const Position &pos, MoveList &moves, const CheckInfo& ci) {
    Bitboard pieces;

    // Non-pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~ci.pinOrtho & ~ci.pinDiag;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }

    // Pinned Bishop & Queen
    pieces = pos.getPiecesBB(Me, BISHOP, QUEEN) & ~ci.pinOrtho & ci.pinDiag;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<BISHOP>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & ci.pinDiag & ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }

    // Non-pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~ci.pinDiag & ~ci.pinOrtho;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & ci.checkMask;

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }

    // Pinned Rook & Queen
    pieces = pos.getPiecesBB(Me, ROOK, QUEEN) & ~ci.pinDiag & ci.pinOrtho;
    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<ROOK>(from, pos.getPiecesBB()) & ~pos.getPiecesBB(Me) & ci.pinOrtho & ci.checkMask;

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

template<Side Me>
inline CheckInfo& getCheckInfo(const Position& pos, CheckInfo& ci) {
    constexpr Side Opp = ~Me;
    Square ksq = pos.getKingSquare(Me);
    ci.checkMask = (pawnAttacks(Me, ksq) & pos.getPiecesBB(Opp, PAWN)) | (attacks<KNIGHT>(ksq) & pos.getPiecesBB(Opp, KNIGHT));
    ci.pinDiag = ci.pinOrtho = EmptyBB;

    // TODO: merge diag & ortho loops
    Bitboard pinners = (attacks<BISHOP>(ksq, pos.getPiecesBB(Opp)) & pos.getPiecesBB(Opp, BISHOP, QUEEN));
    bitscan_loop(pinners) {
        Square s = bitscan(pinners);
        Bitboard b = betweenBB(ksq, s);

        switch(popcount(b & pos.getPiecesBB(Me))) {
            case 0: ci.checkMask |= b | bb(s); break;
            case 1: ci.pinDiag |= b | bb(s);
        }
    }

    pinners = (attacks<ROOK>(ksq, pos.getPiecesBB(Opp)) & pos.getPiecesBB(Opp, ROOK, QUEEN));
    bitscan_loop(pinners) {
        Square s = bitscan(pinners);
        Bitboard b = betweenBB(ksq, s);

        switch(popcount(b & pos.getPiecesBB(Me))) {
            case 0: ci.checkMask |= b | bb(s); break;
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
            // FIXME: check if line of diagonal !
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

    // If we are in double check only king moves are allowed
    if (popcount(checkers) == 2) {
        generateKingMoves<Me>(pos, moves, checkedSquares);

        return;
    }

    CheckInfo ci = getCheckInfo<Me>(pos, ci);
    if (!checkers) ci.checkMask = ~EmptyBB;

    generatePawnMoves<Me>(pos, moves, ci, checkers);
    generateKnightMoves<Me>(pos, moves, ci);
    
    generateSliderMoves<Me>(pos, moves, ci);

    generateCastlingMoves<Me>(pos, moves, checkedSquares);
    generateKingMoves<Me>(pos, moves, checkedSquares);
}

inline void generateLegalMoves(const Position &pos, MoveList &moves) {
    pos.getSideToMove() == WHITE ? generateLegalMoves<WHITE>(pos, moves) : generateLegalMoves<BLACK>(pos, moves);
}

} /* namespace BabChess */

#endif /* MOVE_H_INCLUDED */
