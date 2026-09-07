#include "evaluate.h"

namespace Belette {

template<Side Me>
Score evaluate(const Position &pos) {
    int mg = pos.psq(MG), eg = pos.psq(EG);

    if constexpr (Me == BLACK) {
        mg = -mg;
        eg = -eg;
    }

    int phase = 4 * pos.nbPieceTypes(QUEEN)
              + 2 * pos.nbPieceTypes(ROOK)
              + 1 * pos.nbPieceTypes(KNIGHT)
              + 1 * pos.nbPieceTypes(BISHOP);

    Score score = (mg*phase + eg*(PHASE_TOTAL - phase)) / PHASE_TOTAL;
    score += Tempo;

    return score;
}

template Score evaluate<WHITE>(const Position &pos);
template Score evaluate<BLACK>(const Position &pos);

} /* namespace Belette */
