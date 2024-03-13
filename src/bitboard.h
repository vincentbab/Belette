#ifndef BITBOARD_H_INCLUDED
#define BITBOARD_H_INCLUDED

#include <immintrin.h>
#include <cassert>
#include "chess.h"

namespace Belette {


namespace BB {

void debug(Bitboard b);
void init();

} // namespace Bitboard

inline uint64_t pext(uint64_t b, uint64_t m) {
    return _pext_u64(b, m);
}

inline int popcount(Bitboard b) {
    return __builtin_popcountll(b);
}

inline Square bitscan(Bitboard b) {
    return Square(__builtin_ctzll(b));
    //return Square(_tzcnt_u64(b));
}
/*inline Square bitscan_reverse(Bitboard b) {
    return Square(63 - __builtin_clzll(b));
}*/

#define bitscan_loop(B) for(; B; B &= B - 1)
//#define bitscan_loop(B) for(; B; B = _blsr_u64(B))

struct PextEntry {
    Bitboard  mask;
    Bitboard *data;

    inline Bitboard attacks(Bitboard occupied) const {
        return data[pext(occupied, mask)];
    }
};

extern Bitboard PAWN_ATTACK[NB_SIDE][NB_SQUARE];
extern Bitboard KNIGHT_MOVE[NB_SQUARE];
extern Bitboard KING_MOVE[NB_SQUARE];
extern PextEntry BISHOP_MOVE[NB_SQUARE];
extern PextEntry ROOK_MOVE[NB_SQUARE];

extern Bitboard BETWEEN_BB[NB_SQUARE][NB_SQUARE];

template<Direction D>
constexpr Bitboard shift(Bitboard b)
{
    switch(D) {
        case UP:         return b << 8;
        case DOWN:       return b >> 8;
        case RIGHT:      return (b & ~FileHBB) << 1;
        case LEFT:       return (b & ~FileABB) >> 1;
        case UP_RIGHT:   return (b & ~FileHBB) << 9;
        case UP_LEFT:    return (b & ~FileABB) << 7;
        case DOWN_RIGHT: return (b & ~FileHBB) >> 7;
        case DOWN_LEFT:  return (b & ~FileABB) >> 9;
        default: static_assert(true, "invalid shift direction");
    }
}

inline Bitboard pawnAttacks(Side side, Square sq) {
    return PAWN_ATTACK[side][sq];
}

template<Side Me>
inline Bitboard pawnAttacks(Bitboard b) {
    if constexpr (Me == WHITE)
        return shift<UP_LEFT>(b) | shift<UP_RIGHT>(b);
    else
        return shift<DOWN_RIGHT>(b) | shift<DOWN_LEFT>(b);
}

template<PieceType Pt>
inline Bitboard sliderAttacks(Square sq, Bitboard occupied) {
    static_assert(Pt == ROOK || Pt == BISHOP);

    if constexpr (Pt == ROOK)
        return ROOK_MOVE[sq].attacks(occupied);
    else
        return BISHOP_MOVE[sq].attacks(occupied);
}

template<PieceType Pt>
inline Bitboard attacks(Square sq, Bitboard occupied = 0) {
    if constexpr (Pt == KING) return KING_MOVE[sq];
    if constexpr (Pt == KNIGHT) return KNIGHT_MOVE[sq];
    if constexpr (Pt == BISHOP) return sliderAttacks<BISHOP>(sq, occupied);
    if constexpr (Pt == ROOK) return sliderAttacks<ROOK>(sq, occupied);
    if constexpr (Pt == QUEEN) return sliderAttacks<BISHOP>(sq, occupied) | sliderAttacks<ROOK>(sq, occupied);
}

inline Bitboard betweenBB(Square from, Square to) {
    assert(isValidSq(from) && isValidSq(to));
    return BETWEEN_BB[from][to];
}

} /* namespace Belette */

#endif /* BITBOARD_H_INCLUDED */
