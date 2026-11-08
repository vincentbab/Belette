#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <vector>
#include "tt.h"
#include "utils.h"

namespace Belette {

// Global Transposition Table
TranspositionTable tt;

TranspositionTable::TranspositionTable(size_t defaultSize): buckets(nullptr), nbBuckets(0), age(0) {
    resize(defaultSize);
}

TranspositionTable::~TranspositionTable(){
    largeFree(buckets);
}

void TranspositionTable::resize(size_t size, size_t nbThreads){
    largeFree(buckets);
    buckets = nullptr;

    nbBuckets = size / sizeof(TTBucket);

    if (nbBuckets > 0) {
        buckets = static_cast<TTBucket *>(largeAlloc(sizeof(TTBucket) * nbBuckets));
        if (!buckets) throw std::runtime_error("failed to allocate memory for transposition table");
    }

    clear(nbThreads);
}

void TranspositionTable::clear(size_t nbThreads) {
    nbThreads = std::max<size_t>(1, std::min(nbThreads, nbBuckets));
    const size_t chunk = (nbBuckets + nbThreads - 1) / nbThreads;
    std::vector<std::thread> workers;

    for (size_t i = 0; i < nbThreads; i++) {
        const size_t start = i * chunk;
        const size_t count = std::min(chunk, nbBuckets - std::min(start, nbBuckets));
        if (count == 0) break;
        workers.emplace_back([this, start, count] { std::memset(&buckets[start], 0, count * sizeof(TTBucket)); });
    }

    for (auto &w : workers) w.join();
    age = 0;
}

void TranspositionTable::newSearch() {
    age += TTEntry::AGE_DELTA;
}

size_t TranspositionTable::usage() const {
    const size_t sampleSize = 1000;
    size_t count = 0;
    
    for (size_t i = 0; i < sampleSize; i++) {
        for (size_t j = 0; j < TT_ENTRIES_PER_BUCKET; j++) {
            const TTEntry &tte = buckets[i].entries[j];
            count += !tte.empty() && tte.age() == age;
        }
    }

    return 1000 * count / (sampleSize * TT_ENTRIES_PER_BUCKET);
}

std::tuple<bool, TTEntry *> TranspositionTable::get(uint64_t hash) {
    TTBucket *bucket = &buckets[index(hash)];

    for (TTEntry *entry = bucket->begin(); entry < bucket->end(); entry++) {
        if (entry->hashEquals(hash) || entry->empty()) {
            entry->refresh(age);

            return TTResult(!entry->empty(), entry);
        }
    }

    TTEntry *toReplace = bucket->begin();

    for (TTEntry *entry = bucket->begin() + 1; entry < bucket->end(); entry++) {
        if (toReplace->isBetterToKeep(*entry, age)) {
            toReplace = entry;
        }
    }

    return TTResult(false, toReplace);
}

// Update TTEntry with fresh informations. Logic is greatly inspired from stockfish
void TranspositionTable::set(TTEntry *tte, uint64_t hash, int depth, int ply, Bound bound, Move move, Score eval, Score score, bool pv) {
    assert(depth >= 0);
    assert(tte != nullptr);
    assert(move != MOVE_NULL);

    if (move != MOVE_NONE || !tte->hashEquals(hash)) {
        tte->move16 = move;
    }

    if (bound == BOUND_EXACT || !tte->hashEquals(hash) || (depth + 2*pv + 2 > tte->depth())) {
        tte->hash16 = (uint16_t)hash;
        tte->eval16 = (int16_t)eval;
        tte->score(score, ply);
        tte->depth8 = (uint8_t)depth;
        tte->ageFlags8 = (uint8_t)(age | (pv << 2) | bound);
    }
}

} /* namespace Belette */