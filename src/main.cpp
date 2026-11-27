

#include <iostream>
#include "uci.h"
#include "engine.h"
#include "bitboard.h"
#include "test.h"
#include "perft.h"
#include "zobrist.h"
#include "nnue.h"

using namespace Belette;

int main(int argc, char* argv[])
{
    Engine::init();
    BB::init();
    Zobrist::init();
    Position::init();
    NNUE::init();

    Uci uci;
    uci.loop(argc, argv);

    return 0;
}