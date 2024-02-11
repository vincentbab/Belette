#include <iostream>
#include "move.h"

using namespace std;

namespace BabChess {

inline void generatePromotions(MoveList &moves, Square from, Square to) {
    moves.push_back(makeMove<PROMOTION>(from, to, QUEEN));
    moves.push_back(makeMove<PROMOTION>(from, to, KNIGHT));
    moves.push_back(makeMove<PROMOTION>(from, to, ROOK));
    moves.push_back(makeMove<PROMOTION>(from, to, BISHOP));
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES>
inline void generatePawnMoves(const Position &pos, MoveList &moves) {
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
            moves.push_back(makeMove(to - Up, to));
        }
        bitscan_loop(doublePushes) {
            Square to = bitscan(doublePushes);
            moves.push_back(makeMove(to - Up - Up, to));
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
            moves.push_back(makeMove(to - UpLeft, to));
        }
        bitscan_loop(capR) {
            Square to = bitscan(capR);
            moves.push_back(makeMove(to - UpRight, to));
        }
    }

    if constexpr (MGType & NON_QUIET_MOVES) {
        Bitboard pawnsCanPromote = pos.getPiecesBB(Me, PAWN) & rank7;
        if (pawnsCanPromote) {
            // Quiet Promotion
            {
                Bitboard pawns = pawnsCanPromote & ~pos.pinDiag();
                Bitboard quietPromotions = (shift<Up>(pawns & ~pos.pinOrtho()) | (shift<Up>(pawns & pos.pinOrtho()) & pos.pinOrtho())) & pos.getEmptyBB();

                if constexpr (InCheck) quietPromotions &= pos.checkMask();

                bitscan_loop(quietPromotions) {
                    Square to = bitscan(quietPromotions);
                    generatePromotions(moves, to - Up, to);
                }
            }

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
                    moves.push_back(makeMove<EN_PASSANT>(from, pos.getEpSquare()));
                }
            }
        }
    }

    
}

template<Side Me>
inline void generateCastlingMoves(const Position &pos, MoveList &moves) {
    Square kingSquare = pos.getKingSquare(Me);

    for (auto cr : {Me & KING_SIDE, Me & QUEEN_SIDE}) {
        if (pos.canCastle(cr) && pos.isEmpty(CastlingPath[cr]) && !(pos.checkedSquares() & CastlingKingPath[cr])) {
            moves.push_back(makeMove<CASTLING>(kingSquare, CastlingKingTo[cr]));
        }
    }
}

template<Side Me, MoveGenType MGType = ALL_MOVES>
inline void generateKingMoves(const Position &pos, MoveList &moves) {
    Square from = pos.getKingSquare(Me);

    Bitboard dest = attacks<KING>(from) & ~pos.getPiecesBB(Me) & ~pos.checkedSquares();

    if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
    if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

    bitscan_loop(dest) {
        moves.push_back(makeMove(from, bitscan(dest)));
    }
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES>
inline void generateKnightMoves(const Position &pos, MoveList &moves) {
    Bitboard pieces = pos.getPiecesBB(Me, KNIGHT) & ~(pos.pinDiag() | pos.pinOrtho());

    bitscan_loop(pieces) {
        Square from = bitscan(pieces);
        Bitboard dest = attacks<KNIGHT>(from) & ~pos.getPiecesBB(Me);

        if constexpr (InCheck) dest &= pos.checkMask();
        if constexpr (MGType == NON_QUIET_MOVES) dest &= pos.getPiecesBB(~Me);
        if constexpr (MGType == QUIET_MOVES) dest &= ~pos.getPiecesBB(~Me);

        bitscan_loop(dest) {
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }
}

template<Side Me, bool InCheck, MoveGenType MGType = ALL_MOVES>
inline void generateSliderMoves(const Position &pos, MoveList &moves) {
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
            moves.push_back(makeMove(from, bitscan(dest)));
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
            moves.push_back(makeMove(from, bitscan(dest)));
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
            moves.push_back(makeMove(from, bitscan(dest)));
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
            moves.push_back(makeMove(from, bitscan(dest)));
        }
    }
}

template<Side Me, MoveGenType MGType>
void generateLegalMoves(const Position &pos, MoveList &moves) {
    //Bitboard checkedSquares = pos.checkedSquares();
    //Bitboard checkers = pos.checkers();

    switch(popcount(pos.checkers())) {
        case 2:
            {
                // If we are in double check only king moves are allowed
                generateKingMoves<Me, MGType>(pos, moves);

                return;
            }
        case 1:
            {
                //CheckInfo ci = getCheckInfo<Me, true>(pos, ci);
                generatePawnMoves<Me, true, MGType>(pos, moves);
                generateKnightMoves<Me, true, MGType>(pos, moves);
                generateSliderMoves<Me, true, MGType>(pos, moves);
                generateKingMoves<Me, MGType>(pos, moves);

                return;
            }
        case 0:
            {
                //CheckInfo ci = getCheckInfo<Me, false>(pos, ci);
                generatePawnMoves<Me, false, MGType>(pos, moves);
                generateKnightMoves<Me, false, MGType>(pos, moves);
                generateSliderMoves<Me, false, MGType>(pos, moves);
                if constexpr (MGType & QUIET_MOVES) generateCastlingMoves<Me>(pos, moves);
                generateKingMoves<Me, MGType>(pos, moves);

                return;
            }
    }
}

/*template<> void generateLegalMoves<WHITE, ALL_MOVES>(const Position &pos, MoveList &moves) {
    generateLegalMoves<WHITE, NON_QUIET_MOVES>(pos, moves);
    generateLegalMoves<WHITE, QUIET_MOVES>(pos, moves);
}
template<> void generateLegalMoves<BLACK, ALL_MOVES>(const Position &pos, MoveList &moves) {
    generateLegalMoves<BLACK, NON_QUIET_MOVES>(pos, moves);
    generateLegalMoves<BLACK, QUIET_MOVES>(pos, moves);
}*/

template void generateLegalMoves<WHITE, ALL_MOVES>(const Position &pos, MoveList &moves);
template void generateLegalMoves<WHITE, NON_QUIET_MOVES>(const Position &pos, MoveList &moves);
template void generateLegalMoves<WHITE, QUIET_MOVES>(const Position &pos, MoveList &moves);

template void generateLegalMoves<BLACK, ALL_MOVES>(const Position &pos, MoveList &moves);
template void generateLegalMoves<BLACK, NON_QUIET_MOVES>(const Position &pos, MoveList &moves);
template void generateLegalMoves<BLACK, QUIET_MOVES>(const Position &pos, MoveList &moves);

} /* namespace BabChess*/