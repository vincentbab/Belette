#include "bitboard.h"
#include <iostream>

using namespace std;

namespace BabChess {

Bitboard PAWN_ATTACK[NB_SIDE][NB_SQUARE];
Bitboard KNIGHT_MOVE[NB_SQUARE];
Bitboard KING_MOVE[NB_SQUARE];
PextEntry ROOK_MOVE[NB_SQUARE];
PextEntry BISHOP_MOVE[NB_SQUARE];

Bitboard BETWEEN_BB[NB_SQUARE][NB_SQUARE];

Bitboard ROOK_DATA[0x19000];
Bitboard BISHOP_DATA[0x1480];

namespace BB {

template<Direction Dir>
inline Bitboard slidingRay(Square sq, Bitboard occupied) 
{
    Bitboard attacks = 0;
    Bitboard b = (1ULL << sq);

    do {
        attacks |= (b = shift<Dir>(b));
    } while(b & ~occupied);

    return attacks;
}

template<PieceType Pt>
Bitboard slidingAttacks(Square sq, Bitboard occupied) 
{
    if constexpr (Pt == ROOK)
        return slidingRay<UP>(sq, occupied) 
            | slidingRay<DOWN>(sq, occupied) 
            | slidingRay<RIGHT>(sq, occupied) 
            | slidingRay<LEFT>(sq, occupied);
    else
        return slidingRay<UP_RIGHT>(sq, occupied) 
            | slidingRay<UP_LEFT>(sq, occupied) 
            | slidingRay<DOWN_RIGHT>(sq, occupied) 
            | slidingRay<DOWN_LEFT>(sq, occupied);
}



template<PieceType Pt>
void init_pext(Bitboard table[], PextEntry magics[]) {
    int size = 0;

    for (int i=0; i<NB_SQUARE; i++) {
        Square s = Square(i);

        Bitboard edges = ((Rank1BB | Rank8BB) & ~bb(rankOf(s))) | ((FileABB | FileHBB) & ~bb(fileOf(s)));

        PextEntry& m = magics[s];
        m.mask  = slidingAttacks<Pt>(s, 0) & ~edges;
        m.data = s == SQ_A1 ? table : magics[s - 1].data + size;

        size = 0;
        Bitboard b = 0;
        do {
            m.data[pext(b, m.mask)] = slidingAttacks<Pt>(s, b);

            size++;
            b = (b - m.mask) & m.mask;
        } while (b);
    }
}

void debug(Bitboard bb)
{
	cout << "Bitboard:" << endl;
	int sq = 0;
	for (; sq != 64; ++sq) {
		if (sq && !(sq & 7))
			cout << endl;

		if ((1ULL << (sq ^ 56)) & bb)
			cout << "X  ";
		else
			cout << "-  ";
	}
	cout << endl;
}



void init()
{
    for(Square s=SQ_A1; s<NB_SQUARE; ++s) {
        Bitboard b = bb(s);

        PAWN_ATTACK[WHITE][s] = shift<UP_LEFT>(b) | shift<UP_RIGHT>(b);
        PAWN_ATTACK[BLACK][s] = shift<DOWN_RIGHT>(b) | shift<DOWN_LEFT>(b);

        KING_MOVE[s] = shift<UP>(b) | shift<DOWN>(b) | shift<RIGHT>(b) | shift<LEFT>(b)
                     | shift<UP_RIGHT>(b) | shift<UP_LEFT>(b) | shift<DOWN_RIGHT>(b) | shift<DOWN_LEFT>(b);

        KNIGHT_MOVE[s] = shift<UP_LEFT>( shift<UP>(b) ) | shift<UP_RIGHT>( shift<UP>(b) )
                       | shift<UP_RIGHT>( shift<RIGHT>(b) ) | shift<DOWN_RIGHT>( shift<RIGHT>(b) )
                       | shift<DOWN_RIGHT>( shift<DOWN>(b) ) | shift<DOWN_LEFT>( shift<DOWN>(b) )
                       | shift<DOWN_LEFT>( shift<LEFT>(b) ) | shift<UP_LEFT>( shift<LEFT>(b) )
                       ;

        for(Square s2=SQ_A1; s2<NB_SQUARE; ++s2) {
            Bitboard b2 = bb(s2);

            if (slidingAttacks<ROOK>(s, 0) & s2) {
                BETWEEN_BB[s][s2] |= slidingAttacks<ROOK>(s, b2) & slidingAttacks<ROOK>(s2, b);
            } else if (slidingAttacks<BISHOP>(s, 0) & s2) {
                BETWEEN_BB[s][s2] = slidingAttacks<BISHOP>(s, b2) & slidingAttacks<BISHOP>(s2, b);
            }
        }
    }

    init_pext<ROOK>(ROOK_DATA, ROOK_MOVE);
    init_pext<BISHOP>(BISHOP_DATA, BISHOP_MOVE);
}

} // namespace Bitboard


} /* namespace BabChess */