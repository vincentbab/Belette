#include <fstream>
#include <random>
#include <sstream>
#include <vector>
#include "binpack.h"
#include "datagen.h"
#include "engine.h"
#include "tt.h"
#include "uci.h"
#include "utils.h"

namespace Belette {

class DatagenEngine : public Engine {
public:
    Move bestMove = MOVE_NONE;
    Score bestScore = SCORE_NONE;

protected:
    virtual void onSearchProgress(const SearchEvent &event) { }
    virtual void onSearchFinish(const SearchEvent &event) {
        bestMove = event.pv.empty() ? MOVE_NONE : event.pv.front();
        bestScore = event.bestScore;
    }
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

static bool generateOpening(DatagenEngine &engine, FastRandGenerator &rng, std::vector<std::string> &book, const GenfensParams &params) {
    Position &pos = engine.position();

    if (book.empty()) {
        pos.setFromFEN(STARTPOS_FEN);
    } else {
        size_t idx = rng.rand(book.size());
        if (!pos.setFromFEN(book[idx])) {
            book[idx] = book.back();
            book.pop_back();
            return false;
        }
    }

    const int plies = params.plies + int(rng.rand(2));

    for (int i = 0; i < plies; i++)
        if (!playMultiPvMove(engine, rng, params, i))
            return false;

    return pos.hasLegalMoves();
}

void genfens(int count, uint64_t seed, const std::string &bookFile, const GenfensParams &params) {
    std::vector<std::string> book = loadBook(bookFile);
    FastRandGenerator rng(seed);
    DatagenEngine engine;

    engine.newGame();

    int generated = 0;
    while (generated < count) {
        if (!generateOpening(engine, rng, book, params))
            continue;

        console << "info string genfens " << engine.position().fen() << std::endl;
        generated++;
    }
}

// One engine and one transposition table per side, like two separate engine processes.
// The active side's table is swapped into the global one, which is restored at the end.
class SelfgenPlayers {
public:
    explicit SelfgenPlayers(size_t hashSize) {
        for (Side s : {WHITE, BLACK})
            use(s).setHashSize(hashSize);
    }

    ~SelfgenPlayers() {
        if (active != NB_SIDE)
            tt.swap(tables[active]);
    }

    inline DatagenEngine &use(Side s) {
        if (s != active) {
            if (active != NB_SIDE)
                tt.swap(tables[active]);
            tt.swap(tables[s]);
            active = s;
        }
        return engines[s];
    }

    inline Position &position(Side s) { return engines[s].position(); }

    inline void newGame() {
        position(BLACK) = position(WHITE);
        for (Side s : {WHITE, BLACK})
            use(s).newGame();
        use(WHITE);
    }

private:
    DatagenEngine engines[NB_SIDE];
    TranspositionTable tables[NB_SIDE];
    Side active = NB_SIDE;
};

struct SelfgenStats {
    size_t games = 0;
    size_t positions = 0;
    size_t results[3] = {0};
};

// Result from white point of view: 1 win, 0 draw, -1 loss
static int playGame(SelfgenPlayers &players, const SelfgenParams &params, BinpackWriter &writer, size_t &positions) {
    Position &pos = players.position(WHITE);
    int resignCount[NB_SIDE] = {0};
    int drawCount = 0;
    int result = 0;

    writer.beginGame();

    SearchLimits limits;
    limits.softNodes = params.nodes;
    limits.maxNodes = HARD_NODES_FACTOR * params.nodes;

    while (true) {
        const Side stm = pos.getSideToMove();

        if (!pos.hasLegalMoves()) {
            result = pos.inCheck() ? (stm == WHITE ? -1 : 1) : 0;
            break;
        }

        if (pos.isFiftyMoveDraw() || pos.isRepetitionDraw(0) || pos.isMaterialDraw())
            break;

        DatagenEngine &engine = players.use(stm);
        engine.search(limits);
        engine.waitForSearchFinish();

        const Move move = engine.bestMove;
        const Score score = engine.bestScore;

        if (move == MOVE_NONE)
            break;

        writer.addMove(pos, move, score);
        positions++;

        for (Side s : {WHITE, BLACK}) {
            Position &p = players.position(s);
            p.doMove(move);
            if (p.historySize() >= MAX_HISTORY - MAX_PLY - 8)
                p.setFromFEN(p.fen());
        }

        if (params.resignMoves > 0) {
            resignCount[stm] = score <= -params.resignScore ? resignCount[stm] + 1 : 0;
            if (resignCount[stm] >= params.resignMoves) {
                result = stm == WHITE ? -1 : 1;
                break;
            }
        }

        if (params.drawMoves > 0) {
            drawCount = pos.getFiftyMoveRule() == 0 ? 0 : std::abs(score) <= params.drawScore ? drawCount + 1 : 0;
            if (pos.getFullMoves() >= params.drawMoveNumber && drawCount >= 2 * params.drawMoves)
                break;
        }
    }

    writer.endGame(result);

    return result;
}

void selfgen(const SelfgenParams &params) {
    std::vector<std::string> book = loadBook(params.book);
    const uint64_t seed = params.seed ? params.seed : (uint64_t(std::random_device()()) << 32) ^ uint64_t(now());
    FastRandGenerator rng(seed);
    SelfgenPlayers players(params.hash * 1024 * 1024);
    BinpackWriter writer(params.output);
    SelfgenStats stats;
    const TimeMs start = now();

    console << "info string selfgen seed " << seed << " output " << params.output << std::endl;

    while (params.games == 0 || stats.games < params.games) {
        if (!generateOpening(players.use(WHITE), rng, book, params.opening))
            continue;

        players.newGame();

        const int result = playGame(players, params, writer, stats.positions);

        stats.games++;
        stats.results[1 - result]++;

        if (stats.games % std::max(1, params.flushGames) == 0 || stats.games == params.games) {
            writer.flush();

            const double elapsed = std::max<TimeMs>(1, now() - start) / 1000.0;
            console << "info string selfgen games " << stats.games
                    << " positions " << stats.positions
                    << " w/d/l " << stats.results[0] << "/" << stats.results[1] << "/" << stats.results[2]
                    << " games/s " << stats.games / elapsed
                    << " positions/s " << size_t(stats.positions / elapsed) << std::endl;
        }
    }
}

} /* namespace Belette */
