#include <iostream>
#include "move.h"
#include "uci.h"

using namespace std;

namespace BabChess {

std::ostream& operator<<(std::ostream& os, const MoveList& moves) {
    bool first = true;

    for(Move m : moves) {
        if (!first) os << " ";
        os << Uci::formatMove(m);
        first = false;
        
    }

    return os;
}

} /* namespace BabChess*/