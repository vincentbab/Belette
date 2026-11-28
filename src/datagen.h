#ifndef DATAGEN_H_INCLUDED
#define DATAGEN_H_INCLUDED

#include <cstdint>
#include <string>

namespace Belette {

struct GenfensParams {
    int plies = 8;
    int multiPvMin = 4;
    int multiPvMax = 4;
    double multiPvStep = 0;
    int margin = 50;
    size_t nodes = 5000;
};

struct SelfgenParams {
    std::string output = "selfgen.binpack";
    std::string book;
    uint64_t seed = 0;
    size_t games = 0;
    size_t nodes = 5000;
    size_t hash = 16;
    GenfensParams opening;
    int resignMoves = 0;
    int resignScore = 0;
    int drawMoveNumber = 0;
    int drawMoves = 0;
    int drawScore = 0;
    int flushGames = 64;
};

void genfens(int count, uint64_t seed, const std::string &bookFile, const GenfensParams &params);
void selfgen(const SelfgenParams &params);

} /* namespace Belette */

#endif /* DATAGEN_H_INCLUDED */
