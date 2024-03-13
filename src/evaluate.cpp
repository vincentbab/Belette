#include "evaluate.h"

namespace Belette {

template<Side Me, Phase P>
Score evaluateMaterial(const Position &pos) {
    static_assert(P == MG || P == EG);
    constexpr Side Opp = ~Me;
    Score score = 0;

    score += PieceValue<P>(PAWN)*pos.nbPieces(Me, PAWN) - PieceValue<P>(PAWN)*pos.nbPieces(Opp, PAWN);
    score += PieceValue<P>(KNIGHT)*pos.nbPieces(Me, KNIGHT) - PieceValue<P>(KNIGHT)*pos.nbPieces(Opp, KNIGHT);
    score += PieceValue<P>(BISHOP)*pos.nbPieces(Me, BISHOP) - PieceValue<P>(BISHOP)*pos.nbPieces(Opp, BISHOP);
    score += PieceValue<P>(ROOK)*pos.nbPieces(Me, ROOK) - PieceValue<P>(ROOK)*pos.nbPieces(Opp, ROOK);
    score += PieceValue<P>(QUEEN)*pos.nbPieces(Me, QUEEN) - PieceValue<P>(QUEEN)*pos.nbPieces(Opp, QUEEN);

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
    score += Tempo;

    return score;
}

template Score evaluate<WHITE>(const Position &pos);
template Score evaluate<BLACK>(const Position &pos);

} /* namespace Belette */