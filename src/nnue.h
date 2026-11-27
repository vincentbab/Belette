#ifndef NNUE_H_INCLUDED
#define NNUE_H_INCLUDED

#include <cassert>
#include <cstdint>
#include "chess.h"

#if defined(__AVX512F__) && defined(__AVX512BW__)
#define USE_AVX512
#elif defined(__AVX2__)
#define USE_AVX2
#endif

#if defined(USE_AVX512) || defined(USE_AVX2)
#define USE_SIMD
#include <immintrin.h>
#endif

namespace Belette { class Position; }

namespace Belette::NNUE {

constexpr int INPUT_SIZE = 768;
constexpr int HIDDEN_SIZE = 512;

constexpr int QA = 255;
constexpr int QB = 64;
constexpr int EVAL_SCALE = 400;

#if defined(USE_AVX512)
using Vec = __m512i;

inline Vec vecLoad(const int16_t *p) { return _mm512_load_si512(p); }
inline void vecStore(int16_t *p, Vec v) { _mm512_store_si512(p, v); }
inline Vec vecZero() { return _mm512_setzero_si512(); }
inline Vec vecSet16(int16_t x) { return _mm512_set1_epi16(x); }
inline Vec vecAdd16(Vec a, Vec b) { return _mm512_add_epi16(a, b); }
inline Vec vecSub16(Vec a, Vec b) { return _mm512_sub_epi16(a, b); }
inline Vec vecMax16(Vec a, Vec b) { return _mm512_max_epi16(a, b); }
inline Vec vecMin16(Vec a, Vec b) { return _mm512_min_epi16(a, b); }
inline Vec vecMullo16(Vec a, Vec b) { return _mm512_mullo_epi16(a, b); }
inline Vec vecMadd16(Vec a, Vec b) { return _mm512_madd_epi16(a, b); }
inline Vec vecAdd32(Vec a, Vec b) { return _mm512_add_epi32(a, b); }
inline int vecHsum32(Vec v) { return _mm512_reduce_add_epi32(v); }
#elif defined(USE_AVX2)
using Vec = __m256i;

inline Vec vecLoad(const int16_t *p) { return _mm256_load_si256(reinterpret_cast<const Vec*>(p)); }
inline void vecStore(int16_t *p, Vec v) { _mm256_store_si256(reinterpret_cast<Vec*>(p), v); }
inline Vec vecZero() { return _mm256_setzero_si256(); }
inline Vec vecSet16(int16_t x) { return _mm256_set1_epi16(x); }
inline Vec vecAdd16(Vec a, Vec b) { return _mm256_add_epi16(a, b); }
inline Vec vecSub16(Vec a, Vec b) { return _mm256_sub_epi16(a, b); }
inline Vec vecMax16(Vec a, Vec b) { return _mm256_max_epi16(a, b); }
inline Vec vecMin16(Vec a, Vec b) { return _mm256_min_epi16(a, b); }
inline Vec vecMullo16(Vec a, Vec b) { return _mm256_mullo_epi16(a, b); }
inline Vec vecMadd16(Vec a, Vec b) { return _mm256_madd_epi16(a, b); }
inline Vec vecAdd32(Vec a, Vec b) { return _mm256_add_epi32(a, b); }
inline int vecHsum32(Vec v) {
    __m128i s = _mm_add_epi32(_mm256_castsi256_si128(v), _mm256_extracti128_si256(v, 1));
    s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(1, 0, 3, 2)));
    s = _mm_add_epi32(s, _mm_shuffle_epi32(s, _MM_SHUFFLE(2, 3, 0, 1)));
    return _mm_cvtsi128_si32(s);
}
#endif

#if defined(USE_SIMD)
constexpr int VEC_SIZE = sizeof(Vec) / sizeof(int16_t);
static_assert(HIDDEN_SIZE % VEC_SIZE == 0);
#endif

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

#if defined(USE_SIMD)
            for (int i = 0; i < HIDDEN_SIZE; i += VEC_SIZE) {
                Vec v = vecLoad(&src.values[s][i]);
                for (int k = 0; k < NA; k++) v = vecAdd16(v, vecLoad(&wa[k][i]));
                for (int k = 0; k < NS; k++) v = vecSub16(v, vecLoad(&ws[k][i]));
                vecStore(&values[s][i], v);
            }
#else
            for (int i = 0; i < HIDDEN_SIZE; i++) {
                int16_t v = src.values[s][i];
                for (int k = 0; k < NA; k++) v += wa[k][i];
                for (int k = 0; k < NS; k++) v -= ws[k][i];
                values[s][i] = v;
            }
#endif
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

int evaluate(const Accumulator &acc, Side stm);
int evaluateUpdate(Accumulator &dst, const Accumulator &src, const DirtyPiece &dp, Side stm);

class AccumulatorStack {
public:
    inline void reset(const Position &pos) { idx = 0; accs[0] = computeAccumulator(pos); computed[0] = true; }
    inline void push(const DirtyPiece &dp) { assert(idx < MAX_PLY); idx++; dirty[idx] = dp; computed[idx] = false; }
    inline void pop() { assert(idx > 0); idx--; }

    inline const Accumulator &current() {
        if (!computed[idx]) {
            int i = idx;
            while (!computed[i - 1]) i--;

            for (; i <= idx; i++) {
                accs[i].update(accs[i - 1], dirty[i]);
                computed[i] = true;
            }
        }

        return accs[idx];
    }

    inline int evaluate(Side stm) {
        if (computed[idx])
            return NNUE::evaluate(accs[idx], stm);

        int i = idx;
        while (!computed[i - 1]) i--;

        for (; i < idx; i++) {
            accs[i].update(accs[i - 1], dirty[i]);
            computed[i] = true;
        }

        computed[idx] = true;
        return evaluateUpdate(accs[idx], accs[idx - 1], dirty[idx], stm);
    }

private:
    Accumulator accs[MAX_PLY + 1];
    DirtyPiece dirty[MAX_PLY + 1];
    bool computed[MAX_PLY + 1];
    int idx = 0;
};

} /* namespace Belette::NNUE */

#endif /* NNUE_H_INCLUDED */
