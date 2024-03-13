#ifndef TT_H_INCLUDED
#define TT_H_INCLUDED

#include <cstdlib>
#include <cstdint>
#include <tuple>
#include "chess.h"

namespace BabChess {

constexpr size_t TT_DEFAULT_SIZE = 1024*1024*16;

constexpr int TT_ENTRIES_PER_BUCKET = 3;

enum Bound {
    BOUND_NONE = 0,
    BOUND_LOWER = 1,
    BOUND_UPPER = 2,
    BOUND_EXACT = BOUND_LOWER | BOUND_UPPER
};

class TTEntry {
public:
    static constexpr uint8_t AGE_MASK   = 0b11111000;
    static constexpr uint8_t PV_MASK    = 0b00000100;
    static constexpr uint8_t BOUND_MASK = 0b00000011;
    static constexpr int AGE_DELTA = 0x8;
    static constexpr int AGE_CYCLE = 0xFF + AGE_DELTA;

    inline bool empty() const { return hash16 == 0; }
    inline bool hashEquals(uint64_t hash) const { return hash16 == (uint16_t)hash; }
    inline Move move() const { return (Move)move16; }
    inline Score eval() const { return eval16; }
    inline Score score(int ply) const {
        return score16 == SCORE_NONE ? SCORE_NONE
             : score16 >=  SCORE_MATE_MAX_PLY ? score16 - ply
             : score16 <= -SCORE_MATE_MAX_PLY ? score16 + ply 
             : score16;
    }
    inline void score(Score s, int ply) {
        score16 = (int16_t) (s ==  SCORE_NONE   ? SCORE_NONE
                           : s >=  SCORE_MATE_MAX_PLY ? s + ply
                           : s <= -SCORE_MATE_MAX_PLY ? s - ply : s);
    }
    inline int depth() const { return depth8; }
    inline uint8_t age() const { return ageFlags8 & AGE_MASK; }
    inline bool isPv() const { return bool(ageFlags8 & PV_MASK); }
    inline Bound bound() const { return Bound(ageFlags8 & BOUND_MASK); }
    inline bool isExactBound() const { return bound() & BOUND_EXACT; }
    inline bool isLowerBound() const { return bound() & BOUND_LOWER; }
    inline bool isUpperBound() const { return bound() & BOUND_UPPER; }
    inline bool canCutoff(Score score, Score beta) { return score != SCORE_NONE && (bound() & (score >= beta ? BOUND_LOWER : BOUND_UPPER)); }

    inline void refresh(uint8_t age) { ageFlags8 = age | (ageFlags8 & (PV_MASK | BOUND_MASK)); }

    inline isBetterToKeep(const TTEntry &other, uint8_t age) const {
        return this->depth8 - ((AGE_CYCLE + age - this->ageFlags8) & AGE_MASK)
             > other.depth8 - ((AGE_CYCLE + age - other.ageFlags8) & AGE_MASK);
    }
private:
    friend class TranspositionTable;

    uint16_t hash16;
    Move move16;
    int16_t eval16;
    int16_t score16;
    uint8_t depth8;
    uint8_t ageFlags8;
}; // 10 Bytes

class TranspositionTable {
public:
    using TTResult = std::tuple<bool, TTEntry *>;

    TranspositionTable(size_t defaultSize = TT_DEFAULT_SIZE);
    ~TranspositionTable();

    void resize(size_t size);
    void clear();
    void newSearch();

    TTResult get(uint64_t hash);
    void set(TTEntry *tte, uint64_t hash, int depth, int ply, Bound bound, Move move, Score eval, Score score, bool pv);

    size_t usage() const;
    inline size_t size() const { return nbBuckets; }

private:
    struct TTBucket {
        TTEntry entries[TT_ENTRIES_PER_BUCKET];
        uint16_t padding;

        inline TTEntry *begin() { return &entries[0]; }
        inline TTEntry *end() { return &entries[TT_ENTRIES_PER_BUCKET]; }
    }; // 32 Bytes

    TTBucket *buckets;
    size_t nbBuckets;
    uint8_t age;

    //inline uint64_t index(uint64_t hash) { return hash % nbBuckets; }
    inline uint64_t index(uint64_t hash) { return ((unsigned __int128)hash * (unsigned __int128)nbBuckets) >> 64; }
};

} /* namespace BabChess */

#endif /* TT_H_INCLUDED */
