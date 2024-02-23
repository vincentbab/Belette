#include "evaluate.h"

namespace BabChess {

template<Side Me, Phase P>
Score evaluateMaterial(const Position &pos) {
    static_assert(P == MG || P == EG);
    constexpr Side Opp = ~Me;
    Score score = 0;

    score += PieceValue[PAWN][P]*pos.nbPieces(Me, PAWN) - PieceValue[PAWN][P]*pos.nbPieces(Opp, PAWN);
    score += PieceValue[KNIGHT][P]*pos.nbPieces(Me, KNIGHT) - PieceValue[KNIGHT][P]*pos.nbPieces(Opp, KNIGHT);
    score += PieceValue[BISHOP][P]*pos.nbPieces(Me, BISHOP) - PieceValue[BISHOP][P]*pos.nbPieces(Opp, BISHOP);
    score += PieceValue[ROOK][P]*pos.nbPieces(Me, ROOK) - PieceValue[ROOK][P]*pos.nbPieces(Opp, ROOK);
    score += PieceValue[QUEEN][P]*pos.nbPieces(Me, QUEEN) - PieceValue[QUEEN][P]*pos.nbPieces(Opp, QUEEN);

    return score;
}

template<Side Me, Phase P>
Score evaluatePSQT(const Position &pos) {
    static_assert(P == MG || P == EG);
    constexpr Side Opp = ~Me;
    Score score = 0;

    Bitboard myPieces = pos.getPiecesBB(Me);
    bitscan_loop(myPieces) {
        Square sq = bitscan(myPieces);
        PieceType pt = pieceType(pos.getPieceAt(sq));
        score += PSQT[pt][P][relativeSquare(Me, sq)];
    }

    Bitboard oppPieces = pos.getPiecesBB(Opp);
    bitscan_loop(oppPieces) {
        Square sq = bitscan(oppPieces);
        PieceType pt = pieceType(pos.getPieceAt(sq));
        score -= PSQT[pt][P][relativeSquare(Opp, sq)];
    }

    return score;
}

template<Side Me, Phase P>
Score evaluate(const Position &pos) {
    Score score = 0;
    score += evaluateMaterial<Me, P>(pos);
    score += evaluatePSQT<Me, P>(pos);

    return score;
}

template<Side Me>
Score evaluate(const Position &pos) {
    Score mg = evaluate<Me, MG>(pos);
    Score eg = evaluate<Me, EG>(pos);

    int phase = 4 * pos.nbPieceTypes(QUEEN)
              + 2 * pos.nbPieceTypes(ROOK)
              + 1 * pos.nbPieceTypes(KNIGHT)
              + 1 * pos.nbPieceTypes(BISHOP);

    Score score = (mg*phase +  eg*(PHASE_TOTAL - phase)) / PHASE_TOTAL;

    return score;
}

template Score evaluate<WHITE>(const Position &pos);
template Score evaluate<BLACK>(const Position &pos);

} /* namespace BabChess */