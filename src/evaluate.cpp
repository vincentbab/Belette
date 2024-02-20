#include "evaluate.h"

namespace BabChess {

template<Side Me>
Score evaluateMaterial(const Position &pos) {
    constexpr Side Opp = ~Me;
    Score mg = 0, eg = 0;

    mg += PawnValueMg*pos.nbPieces(Me, PAWN) - PawnValueMg*pos.nbPieces(Opp, PAWN);
    mg += KnightValueMg*pos.nbPieces(Me, KNIGHT) - KnightValueMg*pos.nbPieces(Opp, KNIGHT);
    mg += BishopValueMg*pos.nbPieces(Me, BISHOP) - BishopValueMg*pos.nbPieces(Opp, BISHOP);
    mg += RookValueMg*pos.nbPieces(Me, ROOK) - RookValueMg*pos.nbPieces(Opp, ROOK);
    mg += QueenValueMg*pos.nbPieces(Me, QUEEN) - QueenValueMg*pos.nbPieces(Opp, QUEEN);

    eg += PawnValueEg*pos.nbPieces(Me, PAWN) - PawnValueEg*pos.nbPieces(Opp, PAWN);
    eg += KnightValueEg*pos.nbPieces(Me, KNIGHT) - KnightValueEg*pos.nbPieces(Opp, KNIGHT);
    eg += BishopValueEg*pos.nbPieces(Me, BISHOP) - BishopValueEg*pos.nbPieces(Opp, BISHOP);
    eg += RookValueEg*pos.nbPieces(Me, ROOK) - RookValueEg*pos.nbPieces(Opp, ROOK);
    eg += QueenValueEg*pos.nbPieces(Me, QUEEN) - QueenValueEg*pos.nbPieces(Opp, QUEEN);

    return makeScore(mg, eg);
}

template<Side Me>
Score evaluatePSQT(const Position &pos) {
    constexpr Side Opp = ~Me;
    Score mg = 0, eg = 0;

    Bitboard myPieces = pos.getPiecesBB(Me);
    bitscan_loop(myPieces) {
        Square sq = bitscan(myPieces);
        Piece p = pos.getPieceAt(sq);
        PieceType pt = pieceType(p);

        if (pt == PAWN) {
            mg += PawnMgPSQT[relativeSquare(Me, sq)];
            eg += PawnEgPSQT[relativeSquare(Me, sq)];
        } else if (pt == KNIGHT) {
            mg += KnightMgPSQT[relativeSquare(Me, sq)];
            eg += KnightEgPSQT[relativeSquare(Me, sq)];
        } else if (pt == BISHOP) {
            mg += BishopMgPSQT[relativeSquare(Me, sq)];
            eg += BishopEgPSQT[relativeSquare(Me, sq)];
        } else if (pt == ROOK) {
            mg += RookMgPSQT[relativeSquare(Me, sq)];
            eg += RookEgPSQT[relativeSquare(Me, sq)];
        } else if (pt == QUEEN) {
            mg += QueenMgPSQT[relativeSquare(Me, sq)];
            eg += QueenEgPSQT[relativeSquare(Me, sq)];
        } else if (pt == KING) {
            mg += KingMgPSQT[relativeSquare(Me, sq)];
            eg += KingEgPSQT[relativeSquare(Me, sq)];
        }
    }

    Bitboard oppPieces = pos.getPiecesBB(Opp);
    bitscan_loop(oppPieces) {
        Square sq = bitscan(oppPieces);
        Piece p = pos.getPieceAt(sq);
        PieceType pt = pieceType(p);

        if (pt == PAWN) {
            mg -= PawnMgPSQT[relativeSquare(Opp, sq)];
            eg -= PawnEgPSQT[relativeSquare(Opp, sq)];
        } else if (pt == KNIGHT) {
            mg -= KnightMgPSQT[relativeSquare(Opp, sq)];
            eg -= KnightEgPSQT[relativeSquare(Opp, sq)];
        } else if (pt == BISHOP) {
            mg -= BishopMgPSQT[relativeSquare(Opp, sq)];
            eg -= BishopEgPSQT[relativeSquare(Opp, sq)];
        } else if (pt == ROOK) {
            mg -= RookMgPSQT[relativeSquare(Opp, sq)];
            eg -= RookEgPSQT[relativeSquare(Opp, sq)];
        } else if (pt == QUEEN) {
            mg -= QueenMgPSQT[relativeSquare(Opp, sq)];
            eg -= QueenEgPSQT[relativeSquare(Opp, sq)];
        } else if (pt == KING) {
            mg -= KingMgPSQT[relativeSquare(Opp, sq)];
            eg -= KingEgPSQT[relativeSquare(Opp, sq)];
        }
    }

    return makeScore(mg, eg);
}

template<Side Me>
Score evaluate(const Position &pos) {
    constexpr Side Opp = ~Me;
    Score mg = 0, eg = 0;

    Score material = evaluateMaterial<Me>(pos);
    Score psqt = evaluatePSQT<Me>(pos);

    mg += mgScore(material);
    eg += egScore(material);

    mg += mgScore(psqt);
    eg += egScore(psqt);

    int phase = 4 * (pos.nbPieces(Me, QUEEN) + pos.nbPieces(Opp, QUEEN))
              + 2 * (pos.nbPieces(Me, ROOK) + pos.nbPieces(Opp, ROOK))
              + 1 * (pos.nbPieces(Me, KNIGHT, BISHOP) + pos.nbPieces(Opp, KNIGHT, BISHOP));

    Score score = (mg*phase +  eg*(PHASE_MAX - phase)) / PHASE_MAX;

    return score;
}

template Score evaluate<WHITE>(const Position &pos);
template Score evaluate<BLACK>(const Position &pos);

} /* namespace BabChess */