#include <iostream>
#include <chrono>
#include "perft.h"
#include "move.h"
#include "uci.h"

using namespace std;

namespace BabChess {

template<bool Div, Side Me>
size_t perft(Position &pos, int depth) {
    size_t total = 0;
    MoveList moves;
    
    //if (depth <= 0) return 1;
    if (!Div && depth <= 1) {
        enumerateLegalMoves<Me>(pos, [&](Move m, auto doMH, auto undoMH) {
            total += 1;
            return true;
        });

        return total;
    }
    
    enumerateLegalMoves<Me>(pos, [&](Move move, auto doMove, auto undoMove) {
        size_t n = 0;

        if (Div && depth == 1) {
            n = 1;
        } else {
            //pos.doMove<Me>(move);
            (pos.*doMove)(move);
            n = (depth == 1 ? 1 : perft<false, ~Me>(pos, depth - 1));
            //n = perft<false, ~Me>(pos, depth - 1);
            //pos.undoMove<Me>(move);
            (pos.*undoMove)(move);
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
    auto begin = std::chrono::steady_clock::now();
    size_t n = perft<true>(pos, depth);
    auto end = std::chrono::steady_clock::now();

    auto elapsed = end - begin;
    std::cout << endl << "Nodes: " << n << endl;
	std::cout << "NPS: "
		<< int(n * 1000000.0 / std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) << endl;
	std::cout << "Time: "
		<< std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() << " us" << endl;
}

} /* namespace BabChess */