#include <map>
#include <vector>
#include <string>
#include <iostream>
#include "test.h"
#include "uci.h"
#include "position.h"
#include "perft.h"

using namespace std;

namespace BabChess::Test {

struct TestCase {
    string fen;
    int depth;
    size_t nbNodes;
};

vector<TestCase> ALL_TESTS = {
    {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6, 119060324},             // Startpos
    {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5, 193690690}, // Kiwipete
    {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -", 6, 11030083},                                 // En passant & pins
    {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6, 706045033},     // Castling
    {"8/r3P1K1/3P4/8/8/2k5/1p6/B1N5 w - - 0 1", 6, 24491602},                               // Promotion & pins
    {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194},             
    {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551},
    {"2kq4/8/b5b1/1N6/2PNB3/r1BK1R1r/2R1N3/1b3b2 w - - 0 1", 6, 302150595},                 // Pins !
    {"3r1b2/1k6/8/1PpP4/3K4/8/1R6/8 w - c6 0 2", 7, 167705139},                             // En passant
    {"3k4/8/8/2KpP2r/8/8/8/8 w - - 0 2", 6, 1441479}                                        // En passant
};

void run() {
    Position pos;
    int i = 1, nbTest = ALL_TESTS.size(), nbSuccess = 0, nbFailed = 0;

    for(auto t : ALL_TESTS) {
        cout << "[Test " << i << "/" << nbTest << "] \"" << t.fen << "\"" << endl;

        pos.setFromFEN(t.fen);
        size_t result = perft<false>(pos, t.depth);

        if (result == t.nbNodes) {
            cout << "  SUCCESS - " << t.nbNodes << " == " << result << endl;
            nbSuccess++;
        } else {
            cout << "  FAILED! - " << t.nbNodes << " != " << result << endl;
            nbFailed++;
        }

        i++;
    }

    cout << endl << endl;

    if (nbFailed > 0) {    
        cout << "##############################" << endl;
        cout << "/!\\ Some tests failed :( /!\\" << endl;
        cout << "##############################" << endl;
    } else {
        cout << "----------------------------------------" << endl;
        cout << " Congratulations! All tests succeeded ! " << endl;
        cout << "----------------------------------------" << endl;
    }
    
}

}
