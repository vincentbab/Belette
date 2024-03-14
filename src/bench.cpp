#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include "bench.h"
#include "uci.h"
#include "utils.h"

namespace Belette {

std::vector<std::string> BENCH_POSITIONS = {
    "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    "3r1rk1/B4p1p/3p2p1/8/5P2/3P3P/bb1N2P1/3R1RK1 w - - 0 12",
    "2b2k1r/bp1p2p1/r1pP1pB1/7P/4NP2/3R3R/1p2P3/1K6 w - - 0 28",
    "2r2rk1/1ppq2b1/1n6/1N1Ppp2/p7/Pn2BP2/1PQ1BP2/3RK2R w K - 0 23",
    "rn3rk1/pbppq1pp/1p2pb2/4N2Q/3PN3/3B4/PPP2PPP/R3K2R w KQ - 7 11",
    "2q1nk1r/4Rp2/1ppp1P2/6Pp/3p1B2/3P3P/PPP1Q3/6K1 w - - 0 1",
};

class BenchEngine : public UciEngine {
public:
    size_t nbNodes = 0;
    TimeMs elapsed = 0;

    size_t nps() { return 1000ull * nbNodes / std::max((uint64_t)elapsed, 1ull); }

private:
    virtual void onSearchProgress(const SearchEvent &event) {
        UciEngine::onSearchProgress(event);
    }
    virtual void onSearchFinish(const SearchEvent &event) {
        UciEngine::onSearchFinish(event);
        nbNodes += event.nbNodes;
        elapsed += event.elapsed;
    }
};


void bench(int depth) {
    BenchEngine engine;

    for (auto fen : BENCH_POSITIONS) {
        SearchLimits limits;
        limits.maxDepth = depth;

        console << "position fen " << fen << std::endl;
        console << "go depth " << depth << std::endl;

        engine.newGame();
        engine.position().setFromFEN(fen);
        engine.search(limits);
        engine.waitForSearchFinish();
    }

    console << std::endl << "-----------------------------" << std::endl;
    console << "Elapsed: " << engine.elapsed << std::endl;
    console << "Nodes: " << engine.nbNodes << std::endl;
    console << "NPS: " << engine.nps() << std::endl;
}

} /* namespace Belette  */