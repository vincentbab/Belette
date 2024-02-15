

#include <iostream>
#include "uci.h"
#include "engine.h"
#include "bitboard.h"
#include "test.h"

using namespace std;
using namespace BabChess;

int main(int argc, char* argv[])
{
    cout << "BabChess v0.1" << endl;

    BB::init();

    Uci uci;
    uci.loop();

    return 0;
}