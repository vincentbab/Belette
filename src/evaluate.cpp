#include <algorithm>
#include "evaluate.h"

namespace Belette {

template<Side Me>
Score evaluate(const Position &pos) {
    Score score = NNUE::evaluate(pos.accumulator(), Me);
    return std::clamp(score, -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
}

template Score evaluate<WHITE>(const Position &pos);
template Score evaluate<BLACK>(const Position &pos);

} /* namespace Belette */
