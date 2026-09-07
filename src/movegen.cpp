#include <iostream>
#include "movegen.h"
#include "uci.h"

namespace Belette {

template<typename List>
static std::ostream& printMoves(std::ostream& os, const List& moves) {
    bool first = true;

    for(Move m : moves) {
        if (!first) os << " ";
        os << Uci::formatMove(m);
        first = false;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const MoveList& moves) { return printMoves(os, moves); }
std::ostream& operator<<(std::ostream& os, const PvList& moves) { return printMoves(os, moves); }

} /* namespace Belette*/