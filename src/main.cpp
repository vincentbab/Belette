

#include <iostream>
#include "uci.h"
#include "engine.h"
#include "bitboard.h"
#include "test.h"
#include "perft.h"
#include "zobrist.h"

using namespace std;
using namespace BabChess;

int main(int argc, char* argv[])
{
    cout << "BabChess v0.1 by Vincent Bab" << endl;

    BB::init();
    Zobrist::init();

#ifdef PROFILING
    Position pos;
    //pos.setFromFEN(STARTPOS_FEN);
    perft(pos, 6);
#else
    Uci uci;
    uci.loop();
#endif

    return 0;
}