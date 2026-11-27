#ifndef NNUE_H_INCLUDED
#define NNUE_H_INCLUDED

#include <cassert>
#include <cstdint>
#include "chess.h"

namespace Belette { class Position; }

namespace Belette::NNUE {

constexpr int INPUT_SIZE = 768;
constexpr int HIDDEN_SIZE = 512;

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

struct Feature {
    Piece p;
    Square sq;
};

struct DirtyPiece {
    int nAdd, nSub;
    Feature add[2], sub[2];

    inline void set(Feature a, Feature s) { nAdd = 1; nSub = 1; add[0] = a; sub[0] = s; }
    inline void set(Feature a, Feature s1, Feature s2) { nAdd = 1; nSub = 2; add[0] = a; sub[0] = s1; sub[1] = s2; }
    inline void set(Feature a1, Feature a2, Feature s1, Feature s2) { nAdd = 2; nSub = 2; add[0] = a1; add[1] = a2; sub[0] = s1; sub[1] = s2; }
};

inline const int16_t *featureWeights(Side perspective, Feature f) {
    return &network.ftWeights[featureIndex(perspective, f.p, f.sq) * HIDDEN_SIZE];
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
            const int16_t *w = featureWeights(s, {p, sq});
            for (int i = 0; i < HIDDEN_SIZE; i++)
                values[s][i] += w[i];
        }
    }

    template<int NA, int NS>
    inline void updateFrom(const Accumulator &src, const DirtyPiece &dp) {
        for (Side s : {WHITE, BLACK}) {
            const int16_t *wa[NA], *ws[NS];
            for (int k = 0; k < NA; k++) wa[k] = featureWeights(s, dp.add[k]);
            for (int k = 0; k < NS; k++) ws[k] = featureWeights(s, dp.sub[k]);

            for (int i = 0; i < HIDDEN_SIZE; i++) {
                int16_t v = src.values[s][i];
                for (int k = 0; k < NA; k++) v += wa[k][i];
                for (int k = 0; k < NS; k++) v -= ws[k][i];
                values[s][i] = v;
            }
        }
    }

    inline void update(const Accumulator &src, const DirtyPiece &dp) {
        if (dp.nAdd == 1 && dp.nSub == 1) updateFrom<1, 1>(src, dp);
        else if (dp.nAdd == 1) updateFrom<1, 2>(src, dp);
        else updateFrom<2, 2>(src, dp);
    }

    inline bool operator==(const Accumulator &other) const {
        for (Side s : {WHITE, BLACK})
            for (int i = 0; i < HIDDEN_SIZE; i++)
                if (values[s][i] != other.values[s][i]) return false;
        return true;
    }
};

Accumulator computeAccumulator(const Position &pos);

class AccumulatorStack {
public:
    inline void reset(const Position &pos) { idx = 0; accs[0] = computeAccumulator(pos); }
    inline void push(const DirtyPiece &dp) { assert(idx < MAX_PLY); accs[idx + 1].update(accs[idx], dp); idx++; }
    inline void pop() { assert(idx > 0); idx--; }
    inline const Accumulator &current() const { return accs[idx]; }

private:
    Accumulator accs[MAX_PLY + 1];
    int idx = 0;
};

int evaluate(const Accumulator &acc, Side stm);

} /* namespace Belette::NNUE */

#endif /* NNUE_H_INCLUDED */
