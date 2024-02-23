

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
    BB::init();
    Zobrist::init();

#ifdef PROFILING
    Position pos;
    //pos.setFromFEN(STARTPOS_FEN);
    perft(pos, 6);
#else
    Uci uci(argc, argv);
    uci.loop();
#endif

    return 0;
}