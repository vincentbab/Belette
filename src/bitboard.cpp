#include "bitboard.h"
#include <iostream>

namespace Belette {

Bitboard PAWN_ATTACK[NB_SIDE][NB_SQUARE];
Bitboard KNIGHT_MOVE[NB_SQUARE];
Bitboard KING_MOVE[NB_SQUARE];
SliderEntry ROOK_MOVE[NB_SQUARE];
SliderEntry BISHOP_MOVE[NB_SQUARE];

Bitboard BETWEEN_BB[NB_SQUARE][NB_SQUARE];
Bitboard LINE_BB[NB_SQUARE][NB_SQUARE];

#ifdef USE_PEXT
Bitboard ROOK_DATA[0x19000];
Bitboard BISHOP_DATA[0x1480];
#else
// Black magic bitboards, fixed shift
Bitboard SLIDER_DATA[88507];

struct MagicInit {
    uint32_t offset;
    uint64_t magic;
};

constexpr MagicInit BISHOP_MAGIC[NB_SQUARE] = {
    { 66157, 0x107ac08050500bffull }, { 71730, 0x7fffdfdfd823fffdull },
    { 37781, 0x0400c00fe8000200ull }, { 21015, 0x103f802004000000ull },
    { 47590, 0xc03fe00100000000ull }, {   835, 0x24c00bffff400000ull },
    { 23592, 0x0808101f40007f04ull }, { 30599, 0x100808201ec00080ull },
    { 68776, 0xffa2feffbfefb7ffull }, { 19959, 0x083e3ee040080801ull },
    { 21783, 0x040180bff7e80080ull }, { 64836, 0x0440007fe0031000ull },
    { 23417, 0x2010007ffc000000ull }, { 66724, 0x1079ffe000ff8000ull },
    { 74542, 0x7f83ffdfc03fff80ull }, { 67266, 0x080614080fa00040ull },
    { 26575, 0x7ffe7fff817fcff9ull }, { 67543, 0x7ffebfffa01027fdull },
    { 24409, 0x20018000c00f3c01ull }, { 30779, 0x407e0001000ffb8aull },
    { 17384, 0x201fe000fff80010ull }, { 18778, 0xffdfefffde39ffefull },
    { 65109, 0x7ffff800203fbfffull }, { 20184, 0x7ff7fbfff8203fffull },
    { 38240, 0x000000fe04004070ull }, { 16459, 0x7fff7f9fffc0eff9ull },
    { 17432, 0x7ffeff7f7f01f7fdull }, { 81040, 0x3f6efbbf9efbffffull },
    { 84946, 0x0410008f01003ffdull }, { 18276, 0x20002038001c8010ull },
    {  8512, 0x087ff038000fc001ull }, { 78544, 0x00080c0c00083007ull },
    { 19974, 0x00000080fc82c040ull }, { 23850, 0x000000407e416020ull },
    { 11056, 0x00600203f8008020ull }, { 68019, 0xd003fefe04404080ull },
    { 85965, 0x100020801800304aull }, { 80524, 0x7fbffe700bffe800ull },
    { 38221, 0x107ff00fe4000f90ull }, { 64647, 0x7f8fffcff1d007f8ull },
    { 61320, 0x0000004100f88080ull }, { 67281, 0x00000020807c4040ull },
    { 79076, 0x00000041018700c0ull }, { 17115, 0x0010000080fc4080ull },
    { 50718, 0x1000003c80180030ull }, { 24659, 0x2006001cf00c0018ull },
    { 38291, 0xffffffbfeff80fdcull }, { 30605, 0x000000101003f812ull },
    { 37759, 0x0800001f40808200ull }, {  4639, 0x084000101f3fd208ull },
    { 21759, 0x080000000f808081ull }, { 67799, 0x0004000008003f80ull },
    { 22841, 0x08000001001fe040ull }, { 66689, 0x085f7d8000200a00ull },
    { 62548, 0xfffffeffbfeff81dull }, { 66597, 0xffbfffefefdff70full },
    { 86749, 0x100000101ec10082ull }, { 69558, 0x7fbaffffefe0c02full },
    { 61589, 0x7f83fffffff07f7full }, { 62533, 0xfff1fffffff7ffc1ull },
    { 64387, 0x0878040000ffe01full }, { 26581, 0x005d00000120200aull },
    { 76355, 0x0840800080200fdaull }, { 11140, 0x100000c05f582008ull },
};

constexpr MagicInit ROOK_MAGIC[NB_SQUARE] = {
    { 10890, 0x80280013ff84ffffull }, { 56054, 0x5ffbfefdfef67fffull },
    { 67495, 0xffeffaffeffdffffull }, { 72797, 0x003000900300008aull },
    { 17179, 0x0030018003500030ull }, { 63978, 0x0020012120a00020ull },
    { 56650, 0x0030006000c00030ull }, { 15929, 0xffa8008dff09fff8ull },
    { 55905, 0x7fbff7fbfbeafffcull }, { 26301, 0x0000140081050002ull },
    { 78100, 0x0000180043800048ull }, { 86245, 0x7fffe800021fffb8ull },
    { 75228, 0xffffcffe7fcfffafull }, { 31661, 0x00001800c0180060ull },
    { 38053, 0xffffe7ff8fbfffe8ull }, { 37433, 0x0000180030620018ull },
    { 74747, 0x00300018010c0003ull }, { 53847, 0x0003000c0085ffffull },
    { 70952, 0xfffdfff7fbfefff7ull }, { 49447, 0x7fc1ffdffc001fffull },
    { 62629, 0xfffeffdffdffdfffull }, { 58996, 0x7c108007befff81full },
    { 36009, 0x20408007bfe00810ull }, { 21230, 0x0400800558604100ull },
    { 51882, 0x0040200010080008ull }, { 11841, 0x0010020008040004ull },
    { 25794, 0xfffdfefff7fbfff7ull }, { 49689, 0xfebf7dfff8fefff9ull },
    { 63400, 0xc00000ffe001ffe0ull }, { 33958, 0x2008208007004007ull },
    { 21991, 0xbffbfafffb683f7full }, { 45618, 0x0807f67ffa102040ull },
    { 70134, 0x200008e800300030ull }, { 75944, 0x0000008780180018ull },
    { 68392, 0x0000010300180018ull }, { 66472, 0x4000008180180018ull },
    { 23236, 0x008080310005fffaull }, { 19067, 0x4000188100060006ull },
    {     0, 0xffffff7fffbfbfffull }, { 43566, 0x0000802000200040ull },
    { 29810, 0x20000202ec002800ull }, { 65558, 0xfffff9ff7cfff3ffull },
    { 77684, 0x000000404b801800ull }, { 73350, 0x2000002fe03fd000ull },
    { 61765, 0xffffff6ffe7fcffdull }, { 49282, 0xbff7efffbfc00fffull },
    { 78840, 0x000000100800a804ull }, { 82904, 0xfffbffefa7ffa7feull },
    { 24594, 0x0000052800140028ull }, {  9513, 0x00000085008a0014ull },
    { 29012, 0x8000002b00408028ull }, { 27684, 0x4000002040790028ull },
    { 27901, 0x7800002010288028ull }, { 61477, 0x0000001800e08018ull },
    { 25719, 0x1890000810580050ull }, { 50020, 0x2003d80000500028ull },
    { 41547, 0xfffff37eefefdfbeull }, {  4750, 0x40000280090013c1ull },
    {  6014, 0xbf7ffeffbffaf71full }, { 41529, 0xfffdffff777b7d6eull },
    { 84192, 0xeeffffeff0080bfeull }, { 33433, 0xafe0000fff780402ull },
    {  8555, 0xee73fffbffbb77feull }, {  1009, 0x0002000308482882ull },
};
#endif

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
Bitboard relevantMask(Square s) {
    Bitboard edges = ((Rank1BB | Rank8BB) & ~bb(rankOf(s))) | ((FileABB | FileHBB) & ~bb(fileOf(s)));
    return slidingAttacks<Pt>(s, 0) & ~edges;
}

#ifdef USE_PEXT
template<PieceType Pt>
void initSliders(Bitboard table[], SliderEntry entries[]) {
    int size = 0;

    for (int i=0; i<NB_SQUARE; i++) {
        Square s = Square(i);

        SliderEntry& e = entries[s];
        e.mask = relevantMask<Pt>(s);
        e.data = s == SQ_A1 ? table : entries[s - 1].data + size;

        size = 0;
        Bitboard b = 0;
        do {
            e.data[_pext_u64(b, e.mask)] = slidingAttacks<Pt>(s, b);

            size++;
            b = (b - e.mask) & e.mask;
        } while (b);
    }
}
#else
template<PieceType Pt>
void initSliders(const MagicInit magics[], SliderEntry entries[]) {
    constexpr int Shift = Pt == ROOK ? 64 - 12 : 64 - 9;

    for (int i=0; i<NB_SQUARE; i++) {
        Square s = Square(i);
        Bitboard mask = relevantMask<Pt>(s);

        SliderEntry& e = entries[s];
        e.data = SLIDER_DATA + magics[s].offset;
        e.mask = ~mask;
        e.magic = magics[s].magic;

        Bitboard b = 0;
        do {
            Bitboard attacks = slidingAttacks<Pt>(s, b);
            Bitboard& entry = e.data[((b | e.mask) * e.magic) >> Shift];
            assert(!entry || entry == attacks);
            entry = attacks;

            b = (b - mask) & mask;
        } while (b);
    }
}
#endif

void debug(Bitboard bb)
{
	std::cout << "Bitboard:" << std::endl;
	int sq = 0;
	for (; sq != 64; ++sq) {
		if (sq && !(sq & 7))
			std::cout << std::endl;

		if ((1ULL << (sq ^ 56)) & bb)
			std::cout << "X  ";
		else
			std::cout << "-  ";
	}
	std::cout << std::endl;
}



void init()
{
    for (Square s=SQ_A1; s<NB_SQUARE; ++s) {
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
                LINE_BB[s][s2] = (slidingAttacks<ROOK>(s, 0) & slidingAttacks<ROOK>(s2, 0)) | b | b2;
            } else if (slidingAttacks<BISHOP>(s, 0) & s2) {
                BETWEEN_BB[s][s2] = slidingAttacks<BISHOP>(s, b2) & slidingAttacks<BISHOP>(s2, b);
                LINE_BB[s][s2] = (slidingAttacks<BISHOP>(s, 0) & slidingAttacks<BISHOP>(s2, 0)) | b | b2;
            }
        }
    }

#ifdef USE_PEXT
    initSliders<ROOK>(ROOK_DATA, ROOK_MOVE);
    initSliders<BISHOP>(BISHOP_DATA, BISHOP_MOVE);
#else
    initSliders<ROOK>(ROOK_MAGIC, ROOK_MOVE);
    initSliders<BISHOP>(BISHOP_MAGIC, BISHOP_MOVE);
#endif
}

} // namespace Bitboard


} /* namespace Belette */