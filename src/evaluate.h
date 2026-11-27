#ifndef EVALUATE_H_INCLUDED
#define EVALUATE_H_INCLUDED

#include <array>
#include "chess.h"
#include "position.h"

namespace Belette {

constexpr Score PawnValueMg = 85;
constexpr Score PawnValueEg = 100;

constexpr Score KnightValueMg = 335;
constexpr Score KnightValueEg = 285;

constexpr Score BishopValueMg = 360;
constexpr Score BishopValueEg = 300;

constexpr Score RookValueMg = 455;
constexpr Score RookValueEg = 525;

constexpr Score QueenValueMg = 1025;
constexpr Score QueenValueEg = 985;

constexpr Score PIECE_TYPE_VALUE[NB_PIECE_TYPE][NB_PHASE] = {
    {},
    {PawnValueMg, PawnValueEg},
    {KnightValueMg, KnightValueEg},
    {BishopValueMg, BishopValueEg},
    {RookValueMg, RookValueEg},
    {QueenValueMg, QueenValueEg},
};

constexpr Score PIECE_VALUE[NB_PIECE][NB_PHASE] = {
    {},
    {PawnValueMg, PawnValueEg}, {KnightValueMg, KnightValueEg}, {BishopValueMg, BishopValueEg}, {RookValueMg, RookValueEg}, {QueenValueMg, QueenValueEg},
    {}, {}, {},
    {PawnValueMg, PawnValueEg}, {KnightValueMg, KnightValueEg}, {BishopValueMg, BishopValueEg}, {RookValueMg, RookValueEg}, {QueenValueMg, QueenValueEg},
};

template <Phase P>
constexpr Score PieceValue(PieceType pt) { return PIECE_TYPE_VALUE[pt][P]; }

template <Phase P>
constexpr Score PieceValue(Piece p) { return PIECE_VALUE[p][P]; }

template<Side Me>
Score evaluate(const Position &pos, NNUE::AccumulatorStack &accumulators);

Score evaluate(const Position &pos);

} /* namespace Belette */

#endif /* EVALUATE_H_INCLUDED */
