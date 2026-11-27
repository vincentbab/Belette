#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "incbin.h"
#include "nnue.h"

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

} /* namespace Belette::NNUE */
