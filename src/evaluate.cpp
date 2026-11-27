#include <algorithm>
#include <cassert>
#include "evaluate.h"

namespace Belette {

template<Side Me>
Score evaluate([[maybe_unused]] const Position &pos, const NNUE::Accumulator &acc) {
    assert(acc == NNUE::computeAccumulator(pos));
    Score score = NNUE::evaluate(acc, Me);
    return std::clamp(score, -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
}

template Score evaluate<WHITE>(const Position &pos, const NNUE::Accumulator &acc);
template Score evaluate<BLACK>(const Position &pos, const NNUE::Accumulator &acc);

} /* namespace Belette */
