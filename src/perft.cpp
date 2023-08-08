#include <iostream>
#include "perft.h"
#include "move.h"
#include "uci.h"

using namespace std;

namespace BabChess {

template<bool Div>
size_t perft(Position &pos, int depth) {
    //if (!Div && depth <= 0) return 1;
    //constexpr Side Opp = ~Me;
    size_t total = 0;
    MoveList moves;
    generateLegalMoves(pos, moves);

    if (!Div && depth <= 1) return moves.size();

    for (Move m : moves) {
        size_t n = 0;

        if (Div && depth == 1) {
            n = 1;
        } else {
            pos.doMove(m);
            /*if (pos.getAttackers(pos.getSideToMove(), pos.getKingSquare(~pos.getSideToMove()), pos.getPiecesBB())) {
                pos.undoMove(m);
                cout << pos << endl;
                cout << "Move: " << Uci::formatMove(m) << endl;
                exit(1);
            }*/
            n = perft<false>(pos, depth - 1);
            pos.undoMove(m);
        }

        total += n;

        if (Div && n > 0)
            cout << Uci::formatMove(m) << ": " << n << endl;
    }

    return total;
}

/*template size_t perft<true, WHITE>(Position &pos, int depth);
template size_t perft<false, WHITE>(Position &pos, int depth);
template size_t perft<true, BLACK>(Position &pos, int depth);
template size_t perft<false, BLACK>(Position &pos, int depth);

template<bool Div>
size_t perft(Position &pos, int depth) {
    return pos.getSideToMove() == WHITE ? perft<Div, WHITE>(pos, depth) : perft<Div, BLACK>(pos, depth);
}*/

template size_t perft<true>(Position &pos, int depth);
template size_t perft<false>(Position &pos, int depth);

} /* namespace BabChess */