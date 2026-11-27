#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "incbin.h"
#include "nnue.h"
#include "position.h"

INCBIN(Network, EVALFILE);

namespace Belette::NNUE {

constexpr size_t NETWORK_SIZE = sizeof(int16_t) * (INPUT_SIZE * HIDDEN_SIZE + HIDDEN_SIZE + 2 * HIDDEN_SIZE + 1);

Network network;

void init() {
    if (gNetworkSize < NETWORK_SIZE) {
        std::fprintf(stderr, "Invalid network file: %s\n", EVALFILE);
        std::exit(EXIT_FAILURE);
    }

    const unsigned char *data = gNetworkData;

    std::memcpy(network.ftWeights, data, sizeof(network.ftWeights));
    data += sizeof(network.ftWeights);
    std::memcpy(network.ftBiases, data, sizeof(network.ftBiases));
    data += sizeof(network.ftBiases);
    std::memcpy(network.outWeights, data, sizeof(network.outWeights));
    data += sizeof(network.outWeights);
    std::memcpy(&network.outBias, data, sizeof(network.outBias));
}

Accumulator computeAccumulator(const Position &pos) {
    Accumulator acc;
    acc.reset();
    Bitboard occupied = pos.getPiecesBB();

    bitscan_loop(occupied) {
        Square sq = bitscan(occupied);
        acc.add(pos.getPieceAt(sq), sq);
    }

    return acc;
}

#if defined(USE_SIMD)
constexpr int NB_SUMS = 4;
static_assert(HIDDEN_SIZE % (NB_SUMS * VEC_SIZE) == 0);

static inline void screluDot(const int16_t *acc, const int16_t *weights, Vec (&sums)[NB_SUMS]) {
    const Vec zero = vecZero();
    const Vec qa = vecSet16(QA);

    for (int i = 0; i < HIDDEN_SIZE; i += NB_SUMS * VEC_SIZE) {
        for (int j = 0; j < NB_SUMS; j++) {
            const Vec v = vecMin16(vecMax16(vecLoad(&acc[i + j * VEC_SIZE]), zero), qa);
            const Vec vw = vecMullo16(v, vecLoad(&weights[i + j * VEC_SIZE]));
            sums[j] = vecAdd32(sums[j], vecMadd16(vw, v));
        }
    }
}

int evaluate(const Accumulator &acc, Side stm) {
    Vec sums[NB_SUMS];
    for (int j = 0; j < NB_SUMS; j++) sums[j] = vecZero();

    screluDot(acc.values[stm], network.outWeights, sums);
    screluDot(acc.values[~stm], network.outWeights + HIDDEN_SIZE, sums);

    for (int j = 1; j < NB_SUMS; j++) sums[0] = vecAdd32(sums[0], sums[j]);

    int output = vecHsum32(sums[0]) / QA + network.outBias;
    return output * EVAL_SCALE / (QA * QB);
}
#else
static inline int screluDot(const int16_t *acc, const int16_t *weights) {
    int sum = 0;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        const int16_t v = std::clamp<int16_t>(acc[i], 0, QA);
        const int16_t vw = int16_t(v * weights[i]);
        sum += int(vw) * int(v);
    }
    return sum;
}

int evaluate(const Accumulator &acc, Side stm) {
    int output = screluDot(acc.values[stm], network.outWeights)
               + screluDot(acc.values[~stm], network.outWeights + HIDDEN_SIZE);

    output = output / QA + network.outBias;
    return output * EVAL_SCALE / (QA * QB);
}
#endif

} /* namespace Belette::NNUE */
