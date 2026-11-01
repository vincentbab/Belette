#include <iostream>
#include "perft.h"
#include "movegen.h"
#include "uci.h"
#include "utils.h"
#include "movepicker.h"

namespace Belette {

/**
 * Perft using the MovePicker (slower)
 */
template<bool Div, Side Me, bool Frc>
size_t perftmp(Position &pos, int depth) {
    size_t total = 0;
    MoveList moves;

    if (!Div && depth <= 1) {
        MovePicker mp(pos);
        mp.enumerate<MAIN, Me, Frc>([&](Move m, bool& skipQuiets) {
            total += 1;
            return true;
        });

        return total;
    }

    MovePicker mp(pos);
    mp.enumerate<MAIN, Me, Frc>([&](Move move, bool& skipQuiets) {
        size_t n = 0;

        if (Div && depth == 1) {
            n = 1;
        } else {
            pos.doMove<Me, Frc>(move);
            n = (depth == 1 ? 1 : perftmp<false, ~Me, Frc>(pos, depth - 1));
            pos.undoMove<Me, Frc>(move);
        }

        total += n;

        if (Div && n > 0)
            console << Uci::formatMove(move) << ": " << n << std::endl;

        return true;
    });

    return total;
}

template size_t perftmp<true, WHITE, false>(Position &pos, int depth);
template size_t perftmp<false, WHITE, false>(Position &pos, int depth);
template size_t perftmp<true, BLACK, false>(Position &pos, int depth);
template size_t perftmp<false, BLACK, false>(Position &pos, int depth);
template size_t perftmp<true, WHITE, true>(Position &pos, int depth);
template size_t perftmp<false, WHITE, true>(Position &pos, int depth);
template size_t perftmp<true, BLACK, true>(Position &pos, int depth);
template size_t perftmp<false, BLACK, true>(Position &pos, int depth);

template<bool Div>
size_t perftmp(Position &pos, int depth) {
    if (Frc960::active)
        return pos.getSideToMove() == WHITE ? perftmp<Div, WHITE, true>(pos, depth) : perftmp<Div, BLACK, true>(pos, depth);

    return pos.getSideToMove() == WHITE ? perftmp<Div, WHITE, false>(pos, depth) : perftmp<Div, BLACK, false>(pos, depth);
}

template size_t perftmp<true>(Position &pos, int depth);
template size_t perftmp<false>(Position &pos, int depth);

void perftmp(Position &pos, int depth) {
    console << "perft depth=" << depth << std::endl;
    auto begin = now();
    size_t n = perftmp<true>(pos, depth);
    auto end = now();

    auto elapsed = end - begin;
    console << std::endl << "Nodes: " << n << std::endl;
	console << "NPS: " << size_t(n * 1000 / elapsed) << std::endl;
	console << "Time: " << elapsed << "ms" << std::endl;
}

/**
 * Perft using the legal move generator (faster)
 */
template<bool Div, Side Me, bool Frc>
size_t perft(Position &pos, int depth) {
    size_t total = 0;
    MoveList moves;

    if (!Div && depth <= 1) {
        enumerateLegalMoves<Me, ALL_MOVES, Frc>(pos, [&](Move m) {
            total += 1;
            return true;
        });

        return total;
    }

    enumerateLegalMoves<Me, ALL_MOVES, Frc>(pos, [&](Move move) {
        size_t n = 0;

        if (Div && depth == 1) {
            n = 1;
        } else {
            pos.doMove<Me, Frc>(move);
            n = (depth == 1 ? 1 : perft<false, ~Me, Frc>(pos, depth - 1));
            pos.undoMove<Me, Frc>(move);
        }

        total += n;

        if (Div && n > 0)
            console << Uci::formatMove(move) << ": " << n << std::endl;

        return true;
    });

    return total;
}

template size_t perft<true, WHITE, false>(Position &pos, int depth);
template size_t perft<false, WHITE, false>(Position &pos, int depth);
template size_t perft<true, BLACK, false>(Position &pos, int depth);
template size_t perft<false, BLACK, false>(Position &pos, int depth);
template size_t perft<true, WHITE, true>(Position &pos, int depth);
template size_t perft<false, WHITE, true>(Position &pos, int depth);
template size_t perft<true, BLACK, true>(Position &pos, int depth);
template size_t perft<false, BLACK, true>(Position &pos, int depth);

template<bool Div>
size_t perft(Position &pos, int depth) {
    if (Frc960::active)
        return pos.getSideToMove() == WHITE ? perft<Div, WHITE, true>(pos, depth) : perft<Div, BLACK, true>(pos, depth);

    return pos.getSideToMove() == WHITE ? perft<Div, WHITE, false>(pos, depth) : perft<Div, BLACK, false>(pos, depth);
}

template size_t perft<true>(Position &pos, int depth);
template size_t perft<false>(Position &pos, int depth);

void perft(Position &pos, int depth) {
    console << "perft depth=" << depth << std::endl;
    auto begin = now();
    size_t n = perft<true>(pos, depth);
    auto end = now();

    auto elapsed = end - begin;
    console << std::endl << "Nodes: " << n << std::endl;
	console << "NPS: " << size_t(n * 1000 / elapsed) << std::endl;
	console << "Time: " << elapsed << "ms" << std::endl;
}

} /* namespace Belette */