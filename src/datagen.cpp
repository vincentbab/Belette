#include <fstream>
#include <sstream>
#include <vector>
#include "datagen.h"
#include "engine.h"
#include "uci.h"
#include "utils.h"

namespace Belette {

class DatagenEngine : public Engine {
protected:
    virtual void onSearchProgress(const SearchEvent &event) { }
    virtual void onSearchFinish(const SearchEvent &event) { }
};

static bool isNumber(const std::string &str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](char c) { return c >= '0' && c <= '9'; });
}

static std::string parseBookLine(const std::string &line) {
    std::istringstream is(line.substr(0, line.find(';')));
    std::vector<std::string> tokens;
    std::string token;

    while (tokens.size() < 6 && is >> token)
        tokens.push_back(token);

    if (tokens.size() < 4)
        return "";

    std::string fen = tokens[0] + " " + tokens[1] + " " + tokens[2] + " " + tokens[3];

    if (tokens.size() >= 6 && isNumber(tokens[4]) && isNumber(tokens[5]))
        return fen + " " + tokens[4] + " " + tokens[5];

    return fen + " 0 1";
}

static std::vector<std::string> loadBook(const std::string &bookFile) {
    std::vector<std::string> book;

    if (bookFile.empty() || bookFile == "None" || bookFile == "none")
        return book;

    std::ifstream file(bookFile);
    if (!file) {
        console << "info string Unable to open book '" << bookFile << "'" << std::endl;
        return book;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::string fen = parseBookLine(line);
        if (!fen.empty())
            book.push_back(fen);
    }

    return book;
}

static int multiPvAtPly(const GenfensParams &params, int ply) {
    const int minPv = std::max(1, params.multiPvMin);
    const int maxPv = std::max(minPv, params.multiPvMax);
    const int start = params.multiPvStep >= 0 ? minPv : maxPv;

    return int(std::clamp(start + ply * params.multiPvStep, double(minPv), double(maxPv)));
}

static bool playMultiPvMove(DatagenEngine &engine, FastRandGenerator &rng, const GenfensParams &params, int ply) {
    Position &pos = engine.position();

    int nbLegalMoves = 0;
    enumerateCurrentVariantMoves(pos, [&](Move) { nbLegalMoves++; return true; });

    if (nbLegalMoves == 0)
        return false;

    const int multiPv = multiPvAtPly(params, ply);

    SearchLimits limits;
    limits.multiPv = multiPv;
    limits.softNodes = params.nodes * std::min(multiPv, nbLegalMoves);
    limits.maxNodes = HARD_NODES_FACTOR * limits.softNodes;

    engine.search(limits);
    engine.waitForSearchFinish();

    Score bestScore = -SCORE_INFINITE;
    for (const PvLine &line : engine.pvLines())
        if (!line.pv.empty())
            bestScore = std::max(bestScore, line.score);

    MoveList candidates;
    for (const PvLine &line : engine.pvLines())
        if (!line.pv.empty() && line.score >= bestScore - params.margin)
            candidates.push_back(line.pv.front());

    if (candidates.empty())
        return false;

    pos.doMove(candidates[uint8_t(rng.rand(candidates.size()))]);

    return true;
}

void genfens(int count, uint64_t seed, const std::string &bookFile, const GenfensParams &params) {
    std::vector<std::string> book = loadBook(bookFile);
    FastRandGenerator rng(seed);
    DatagenEngine engine;

    engine.newGame();

    int generated = 0;
    while (generated < count) {
        Position &pos = engine.position();

        if (book.empty()) {
            pos.setFromFEN(STARTPOS_FEN);
        } else {
            size_t idx = rng.rand(book.size());
            if (!pos.setFromFEN(book[idx])) {
                book[idx] = book.back();
                book.pop_back();
                continue;
            }
        }

        const int plies = params.plies + int(rng.rand(2));
        bool valid = true;

        for (int i = 0; i < plies && valid; i++)
            valid = playMultiPvMove(engine, rng, params, i);

        if (!valid || !pos.hasLegalMoves())
            continue;

        console << "info string genfens " << pos.fen() << std::endl;
        generated++;
    }
}

} /* namespace Belette */
