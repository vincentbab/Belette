#ifndef NNUE_H_INCLUDED
#define NNUE_H_INCLUDED

#include <cstdint>
#include "chess.h"

namespace Belette::NNUE {

constexpr int INPUT_SIZE = 768;
constexpr int HIDDEN_SIZE = 256;

constexpr int QA = 255;
constexpr int QB = 64;
constexpr int EVAL_SCALE = 400;

struct Network {
    alignas(64) int16_t ftWeights[INPUT_SIZE * HIDDEN_SIZE];
    alignas(64) int16_t ftBiases[HIDDEN_SIZE];
    alignas(64) int16_t outWeights[2 * HIDDEN_SIZE];
    int16_t outBias;
};

extern Network network;

void init();

constexpr int featureIndex(Side perspective, Piece p, Square sq) {
    return (side(p) != perspective) * 384
         + (pieceType(p) - 1) * 64
         + (perspective == WHITE ? int(sq) : int(sq) ^ 56);
}

struct Accumulator {
    alignas(64) int16_t values[NB_SIDE][HIDDEN_SIZE];

    inline void reset() {
        for (Side s : {WHITE, BLACK})
            for (int i = 0; i < HIDDEN_SIZE; i++)
                values[s][i] = network.ftBiases[i];
    }

    inline void add(Piece p, Square sq) {
        for (Side s : {WHITE, BLACK}) {
            const int16_t *w = &network.ftWeights[featureIndex(s, p, sq) * HIDDEN_SIZE];
            for (int i = 0; i < HIDDEN_SIZE; i++)
                values[s][i] += w[i];
        }
    }

    inline void sub(Piece p, Square sq) {
        for (Side s : {WHITE, BLACK}) {
            const int16_t *w = &network.ftWeights[featureIndex(s, p, sq) * HIDDEN_SIZE];
            for (int i = 0; i < HIDDEN_SIZE; i++)
                values[s][i] -= w[i];
        }
    }

    inline void move(Piece p, Square from, Square to) {
        for (Side s : {WHITE, BLACK}) {
            const int16_t *wf = &network.ftWeights[featureIndex(s, p, from) * HIDDEN_SIZE];
            const int16_t *wt = &network.ftWeights[featureIndex(s, p, to) * HIDDEN_SIZE];
            for (int i = 0; i < HIDDEN_SIZE; i++)
                values[s][i] += wt[i] - wf[i];
        }
    }

    inline bool operator==(const Accumulator &other) const {
        for (Side s : {WHITE, BLACK})
            for (int i = 0; i < HIDDEN_SIZE; i++)
                if (values[s][i] != other.values[s][i]) return false;
        return true;
    }
};

int evaluate(const Accumulator &acc, Side stm);

} /* namespace Belette::NNUE */

#endif /* NNUE_H_INCLUDED */
