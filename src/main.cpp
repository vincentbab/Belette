

#include <iostream>
#include "uci.h"
#include "engine.h"
#include "bitboard.h"
#include "test.h"
#include "perft.h"

using namespace std;
using namespace BabChess;

int main(int argc, char* argv[])
{
    cout << "BabChess v0.1" << endl;

    BB::init();

#ifdef PROFILING
    Position pos;
    //pos.setFromFEN(STARTPOS_FEN);
    perft(pos, 6);

    return 0;
#else
    Uci uci;
    uci.loop();

    return 0;
#endif
}