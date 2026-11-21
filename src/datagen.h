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

void genfens(int count, uint64_t seed, const std::string &bookFile, const GenfensParams &params);

} /* namespace Belette */

#endif /* DATAGEN_H_INCLUDED */
