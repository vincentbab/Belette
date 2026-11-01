#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "test.h"
#include "uci.h"
#include "position.h"
#include "perft.h"

namespace Belette::Test {

struct TestCase {
    std::string fen;
    int depth;
    size_t nbNodes;
};

std::vector<TestCase> ALL_TESTS = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324},             // Startpos
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690}, // Kiwipete
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 6, 11030083},                                 // En passant & pins
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6, 706045033},     // Castling
    {"8/r3P1K1/3P4/8/8/2k5/1p6/B1N5 w - - 0 1", 6, 24491602},                               // Promotion & pins
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194},             
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551},
    {"2kq4/8/b5b1/1N6/2PNB3/r1BK1R1r/2R1N3/1b3b2 w - - 0 1", 6, 302150595},                 // Pins !
    {"3r1b2/1k6/8/1PpP4/3K4/8/1R6/8 w - c6 0 2", 7, 167705139},                             // En passant
    {"3k4/8/8/2KpP2r/8/8/8/8 w - - 0 2", 6, 1441479},                                       // En passant
    {"bqnb1rkr/pp3ppp/3ppn2/2p5/5P2/P2P4/NPP1P1PP/BQ1BNRKR w HFhf - 2 9", 4, 326672},       // Chess960
    {"2nnrbkr/p1qppppp/8/1ppb4/6PP/3PP3/PPP2P2/BQNNRBKR w HEhe - 1 9", 4, 667366},          // Chess960
    {"b1q1rrkb/pppppppp/3nn3/8/P7/1PPP4/4PPPP/BQNNRKRB w GE - 1 9", 4, 273318},             // Chess960
    {"1rkr4/8/8/8/8/8/8/1RKR4 w DBdb - 0 1", 5, 3087356},                                   // Chess960 adjacent king and rooks
    {"r1k4r/8/8/8/8/8/8/R1K4R w HAha - 0 1", 5, 7571396},                                   // Chess960 king on castling square
    {"5rkr/8/8/8/8/8/8/5RKR w HFhf - 0 1", 5, 3126967},                                     // Chess960 rook on castling square
    {"4k3/8/8/8/8/8/8/rRK5 w B - 0 1", 5, 201102},                                          // Chess960 pinned castling rook
    {"4k3/8/8/8/8/8/8/1qRK3R w C - 0 1", 4, 98210}                                          // Chess960 pinned castling rook
};

void run() {
    Position pos;
    int i = 1, nbTest = ALL_TESTS.size(), nbFailed = 0;

    for(auto t : ALL_TESTS) {
        console << "[Test " << i << "/" << nbTest << "] \"" << t.fen << "\"" << std::endl;

        pos.setFromFEN(t.fen);
        size_t result = perft<false>(pos, t.depth);

        if (result == t.nbNodes) {
            console << "  SUCCESS - " << t.nbNodes << " == " << result << std::endl;
        } else {
            console << "  FAILED! - " << t.nbNodes << " != " << result << std::endl;
            nbFailed++;
        }

        i++;
    }

    console << std::endl << std::endl;

    if (nbFailed > 0) {
        console << "##############################" << std::endl;
        console << "/!\\ Some tests failed :( /!\\" << std::endl;
        console << "##############################" << std::endl;
    } else {
        console << "----------------------------------------" << std::endl;
        console << " Congratulations! All tests succeeded ! " << std::endl;
        console << "----------------------------------------" << std::endl;
    }
    
}

} /* namespace Belette::Test */
