#include <algorithm>
#include <cassert>
#include "evaluate.h"

namespace Belette {

static inline Score clampEval(int score) {
    return std::clamp(score, -SCORE_MATE_MAX_PLY + 1, SCORE_MATE_MAX_PLY - 1);
}

template<Side Me>
Score evaluate([[maybe_unused]] const Position &pos, NNUE::AccumulatorStack &accumulators) {
    Score score = accumulators.evaluate(Me);
    assert(accumulators.current() == NNUE::computeAccumulator(pos));
    return clampEval(score);
}

template Score evaluate<WHITE>(const Position &pos, NNUE::AccumulatorStack &accumulators);
template Score evaluate<BLACK>(const Position &pos, NNUE::AccumulatorStack &accumulators);

Score evaluate(const Position &pos) {
    return clampEval(NNUE::evaluate(NNUE::computeAccumulator(pos), pos.getSideToMove()));
}

} /* namespace Belette */
