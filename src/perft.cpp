#include <iostream>
#include "perft.h"
#include "move.h"
#include "uci.h"
#include "utils.h"
#include "movepicker.h"

using namespace std;

namespace Belette {

template<bool Div, Side Me>
size_t perft(Position &pos, int depth) {
    size_t total = 0;
    MoveList moves;
    
    if (!Div && depth <= 1) {
        MovePicker<MAIN, Me> mp(pos);
        mp.enumerate([&](Move m, auto doMH, auto undoMH) {
            total += 1;
            return true;
        });

        return total;
    }
    
    MovePicker<MAIN, Me> mp(pos);
    mp.enumerate([&](Move move, auto doMove, auto undoMove) {
        size_t n = 0;

        if (Div && depth == 1) {
            n = 1;
        } else {
            doMove(pos, move);
            n = (depth == 1 ? 1 : perft<false, ~Me>(pos, depth - 1));
            undoMove(pos, move);
        }

        total += n;

        if (Div && n > 0)
            cout << Uci::formatMove(move) << ": " << n << endl;

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

void perft(Position &pos, int depth) {
    std::cout << "perft depth=" << depth << endl;
    auto begin = now();
    size_t n = perft<true>(pos, depth);
    auto end = now();

    auto elapsed = end - begin;
    std::cout << endl << "Nodes: " << n << endl;
	std::cout << "NPS: "
		<< size_t(n * 1000 / elapsed) << endl;
	std::cout << "Time: "
		<< elapsed << "ms" << endl;
}

} /* namespace Belette */