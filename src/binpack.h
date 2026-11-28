#ifndef BINPACK_H_INCLUDED
#define BINPACK_H_INCLUDED

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include "chess.h"
#include "position.h"

namespace Belette {

// Stockfish .binpack writer (nnue-pytorch format)
class BinpackWriter {
public:
    explicit BinpackWriter(const std::string &path);
    ~BinpackWriter();

    void beginGame();
    void addMove(const Position &pos, Move move, Score score);
    // Result from white point of view: 1 win, 0 draw, -1 loss
    void endGame(int result);
    void flush();

private:
    void addBits(uint8_t bits, int count);
    void addVle16(uint16_t value, int blockSize);
    void writeStem(const Position &pos, Move move, Score score);
    void writeMoveScore(const Position &pos, Move move, Score score);

    std::ofstream file;
    std::vector<uint8_t> chunk;
    std::vector<uint8_t> movetext;
    uint8_t stem[32];
    Side stemSide = WHITE;
    uint16_t stemPly = 0;
    uint16_t nbPlies = 0;
    bool hasStem = false;
    int bitsLeft = 0;
    int16_t lastScore = 0;
};

} /* namespace Belette */

#endif /* BINPACK_H_INCLUDED */
