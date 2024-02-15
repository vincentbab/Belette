#include <iostream>
#include "perft.h"
#include "move.h"
#include "uci.h"

using namespace std;

namespace BabChess {

template<bool Div, Side Me>
size_t perft(Position &pos, int depth) {
    size_t total = 0;
    MoveList moves;
    
    if (!Div && depth <= 1) {
        enumerateLegalMoves<Me>(pos, [&](Move m, auto doMH, auto undoMH) {
            total += 1;
            return true;
        });

        return total;
    }

    
    enumerateLegalMoves<Me>(pos, [&](Move m, auto doMoveHandler, auto undoMoveHandler) {
        size_t n = 0;

        if (Div && depth == 1) {
            n = 1;
        } else {
            //pos.doMove<Me>(m);
            doMoveHandler(pos);
            n = perft<false, ~Me>(pos, depth - 1);
            //pos.undoMove<Me>(m);
            undoMoveHandler(pos);
        }

        total += n;

        if (Div && n > 0)
            cout << Uci::formatMove(m) << ": " << n << endl;

        return true;
    });

    return total;
}

template size_t perft<true, WHITE>(Position &pos, int depth);
template size_t perft<false, WHITE>(Position &pos, int depth);
template size_t perft<true, BLACK>(Position &pos, int depth);
template size_t perft<false, BLACK>(Position &pos, int depth);

template<bool Div>
size_t perft(Position &pos, int depth) {
    return pos.getSideToMove() == WHITE ? perft<Div, WHITE>(pos, depth) : perft<Div, BLACK>(pos, depth);
}

template size_t perft<true>(Position &pos, int depth);
template size_t perft<false>(Position &pos, int depth);

} /* namespace BabChess */