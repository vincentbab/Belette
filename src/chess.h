#ifndef CHESS_H_INCLUDED
#define CHESS_H_INCLUDED

#include <cstdint>
#include <cassert>
#include <string>

namespace BabChess {

constexpr int MAX_DEPTH = 256;
constexpr int MAX_HISTORY   = 2048;
constexpr int MAX_MOVE   = 256;

typedef uint64_t Bitboard;
constexpr Bitboard EmptyBB = 0ULL;

// bit  0- 5: destination square (from 0 to 63)
// bit  6-11: origin square (from 0 to 63)
// 
// bit 14-15: promotion: pieceType-2 (from KNIGHT-2 to QUEEN-2)
// bit 12-13: special move flag: castling (1), en passant (2), promotion (3)
enum Move : uint16_t {
    MOVE_NONE,
    MOVE_NULL = 65
};

enum MoveType {
    NORMAL,
    PROMOTION = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING  = 3 << 14
};

enum Side {
    WHITE, BLACK, NB_SIDE = 2
};

enum PieceType {
    ALL_PIECES = 0,
    NO_PIECE_TYPE = 0, 
    PAWN = 1, KNIGHT, BISHOP, ROOK, QUEEN, KING,
    NB_PIECE_TYPE = 7
};

enum Piece {
    NO_PIECE,
    W_PAWN = PAWN,     W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
    B_PAWN = PAWN + 8, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
    NB_PIECE = 16
};

enum Square : int {
    SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE,

    //SQUARE_ZERO = 0,
    NB_SQUARE   = 64
};

enum File : int {
    FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_NB
};

enum Rank : int {
    RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_NB
};

enum Direction : int {
    UP =  8,
    DOWN = -UP,
    RIGHT  =  1,
    LEFT  = -RIGHT,

    UP_RIGHT = UP + RIGHT,
    UP_LEFT = UP + LEFT,
    DOWN_RIGHT = DOWN + RIGHT,
    DOWN_LEFT = DOWN + LEFT,
};

constexpr int dir8(Direction d) {
    switch (d) {
        case UP: return 0;
        case DOWN: return 1;
        case RIGHT: return 2;
        case LEFT: return 3;

        case UP_RIGHT: return 4;
        case UP_LEFT: return 5;
        case DOWN_RIGHT: return 6;
        case DOWN_LEFT: return 7;
    }

    static_assert(true, "invalid dir");
    return -1;
}

enum CastlingRight {
    NO_CASTLING = 0,
    WHITE_KING_SIDE = 1,
    WHITE_QUEEN_SIDE = 2,
    BLACK_KING_SIDE  = 4,
    BLACK_QUEEN_SIDE = 8,

    KING_SIDE      = WHITE_KING_SIDE  | BLACK_KING_SIDE,
    QUEEN_SIDE     = WHITE_QUEEN_SIDE | BLACK_QUEEN_SIDE,
    WHITE_CASTLING = WHITE_KING_SIDE  | WHITE_QUEEN_SIDE,
    BLACK_CASTLING = BLACK_KING_SIDE  | BLACK_QUEEN_SIDE,
    ANY_CASTLING   = WHITE_CASTLING | BLACK_CASTLING,

    NB_CASTLING_RIGHT = 16
};

constexpr Bitboard FileABB = 0x0101010101010101ULL;
constexpr Bitboard FileBBB = FileABB << 1;
constexpr Bitboard FileCBB = FileABB << 2;
constexpr Bitboard FileDBB = FileABB << 3;
constexpr Bitboard FileEBB = FileABB << 4;
constexpr Bitboard FileFBB = FileABB << 5;
constexpr Bitboard FileGBB = FileABB << 6;
constexpr Bitboard FileHBB = FileABB << 7;

constexpr Bitboard Rank1BB = 0xFF;
constexpr Bitboard Rank2BB = Rank1BB << (8 * 1);
constexpr Bitboard Rank3BB = Rank1BB << (8 * 2);
constexpr Bitboard Rank4BB = Rank1BB << (8 * 3);
constexpr Bitboard Rank5BB = Rank1BB << (8 * 4);
constexpr Bitboard Rank6BB = Rank1BB << (8 * 5);
constexpr Bitboard Rank7BB = Rank1BB << (8 * 6);
constexpr Bitboard Rank8BB = Rank1BB << (8 * 7);

/*constexpr Bitboard SQUARE_BB[65] = {
	0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
	0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000,
	0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x200000, 0x400000, 0x800000,
	0x1000000, 0x2000000, 0x4000000, 0x8000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000,
	0x100000000, 0x200000000, 0x400000000, 0x800000000, 0x1000000000, 0x2000000000, 0x4000000000, 0x8000000000,
	0x10000000000, 0x20000000000, 0x40000000000, 0x80000000000, 0x100000000000, 0x200000000000, 0x400000000000, 0x800000000000,
	0x1000000000000, 0x2000000000000, 0x4000000000000, 0x8000000000000, 0x10000000000000, 0x20000000000000, 0x40000000000000, 0x80000000000000,
	0x100000000000000, 0x200000000000000, 0x400000000000000, 0x800000000000000, 0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000,
	0
};*/
constexpr Bitboard bb(Square s) { return (1ULL << s); }
//constexpr Bitboard bb(Square s) { return SQUARE_BB[s]; }

inline Square& operator++(Square& sq) { return sq = Square(int(sq) + 1); }
inline Square& operator--(Square& sq) { return sq = Square(int(sq) - 1); }

constexpr Square operator+(Square sq, Direction dir) { return Square(int(sq) + int(dir)); }
constexpr Square operator-(Square sq, Direction dir) { return Square(int(sq) - int(dir)); }
inline Square& operator+=(Square& sq, Direction dir) { return sq = sq + dir; }
inline Square& operator-=(Square& sq, Direction dir) { return sq = sq - dir; }

constexpr CastlingRight operator|(CastlingRight cr1, CastlingRight cr2) { return CastlingRight(int(cr1) | int(cr2)); }
constexpr CastlingRight operator&(CastlingRight cr1, CastlingRight cr2) { return CastlingRight(int(cr1) & int(cr2)); }
constexpr CastlingRight operator~(CastlingRight cr1) { return CastlingRight(~int(cr1)); }
inline CastlingRight& operator|=(CastlingRight& cr, CastlingRight other) { return cr = cr | other; }
inline CastlingRight& operator&=(CastlingRight& cr, CastlingRight other) { return cr = cr & other; }
constexpr CastlingRight operator&(Side s, CastlingRight cr) { return CastlingRight((s == WHITE ? WHITE_CASTLING : BLACK_CASTLING) & cr); }

constexpr Side operator~(Side c) { return Side(c ^ BLACK); }


constexpr Square CastlingKingTo[NB_CASTLING_RIGHT] = {
    SQ_NONE, // NO_CASTLING
    SQ_G1,   // WHITE_KING_SIDE = 1,
    SQ_C1,   // WHITE_QUEEN_SIDE = 2,
    SQ_NONE,
    SQ_G8,   // BLACK_KING_SIDE  = 4,
    SQ_NONE,SQ_NONE,SQ_NONE,
    SQ_C8    // BLACK_QUEEN_SIDE = 8,
};
constexpr Square CastlingRookFrom[NB_CASTLING_RIGHT] = {
    SQ_NONE, // NO_CASTLING
    SQ_H1,   // WHITE_KING_SIDE = 1,
    SQ_A1,   // WHITE_QUEEN_SIDE = 2,
    SQ_NONE,
    SQ_H8,   // BLACK_KING_SIDE  = 4,
    SQ_NONE,SQ_NONE,SQ_NONE,
    SQ_A8    // BLACK_QUEEN_SIDE = 8,
};
constexpr Square CastlingRookTo[NB_CASTLING_RIGHT] = {
    SQ_NONE, // NO_CASTLING
    SQ_F1,   // WHITE_KING_SIDE = 1,
    SQ_D1,   // WHITE_QUEEN_SIDE = 2,
    SQ_NONE,
    SQ_F8,   // BLACK_KING_SIDE  = 4,
    SQ_NONE,SQ_NONE,SQ_NONE,
    SQ_D8    // BLACK_QUEEN_SIDE = 8,
};

constexpr CastlingRight CastlingRightsMask[NB_SQUARE] = {
    WHITE_QUEEN_SIDE, NO_CASTLING, NO_CASTLING, NO_CASTLING, WHITE_QUEEN_SIDE|WHITE_KING_SIDE, NO_CASTLING, NO_CASTLING, WHITE_KING_SIDE,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING, NO_CASTLING,
    BLACK_QUEEN_SIDE, NO_CASTLING, NO_CASTLING, NO_CASTLING, BLACK_QUEEN_SIDE|BLACK_KING_SIDE, NO_CASTLING, NO_CASTLING, BLACK_KING_SIDE,
};

// Squares that need to be empty for castling
constexpr Bitboard CastlingPath[NB_CASTLING_RIGHT] = {
    EmptyBB,                           // NO_CASTLING
    bb(SQ_F1) | bb(SQ_G1),              // WHITE_KING_SIDE = 1,
    bb(SQ_D1) | bb(SQ_C1) | bb(SQ_B1),  // WHITE_QUEEN_SIDE = 2,
    EmptyBB,
    bb(SQ_F8) | bb(SQ_G8),              // BLACK_KING_SIDE  = 4,
    EmptyBB,EmptyBB,EmptyBB,
    bb(SQ_D8) | bb(SQ_C8) | bb(SQ_B8)   // BLACK_QUEEN_SIDE = 8,
};

// Squares that need not to be attacked by ennemy for castling
constexpr Bitboard CastlingKingPath[NB_CASTLING_RIGHT] = {
    EmptyBB,                           // NO_CASTLING
    bb(SQ_E1) | bb(SQ_F1) | bb(SQ_G1),  // WHITE_KING_SIDE = 1,
    bb(SQ_E1) | bb(SQ_D1) | bb(SQ_C1),  // WHITE_QUEEN_SIDE = 2,
    EmptyBB,
    bb(SQ_E8) | bb(SQ_F8) | bb(SQ_G8),  // BLACK_KING_SIDE  = 4,
    EmptyBB,EmptyBB,EmptyBB,
    bb(SQ_E8) | bb(SQ_D8) | bb(SQ_C8)   // BLACK_QUEEN_SIDE = 8,
};

constexpr bool isValidSq(Square s) {
    return s >= SQ_A1 && s <= SQ_H8;
}

constexpr Square square(File f, Rank r) {
    return Square((r << 3) + f);
}

constexpr File fileOf(Square sq) {
    return File(sq & 7);
}

constexpr Rank rankOf(Square sq) {
    return Rank(sq >> 3);
}

constexpr Bitboard bb(Rank r) {
  return Rank1BB << (8 * r);
}

constexpr Bitboard bb(File f) {
  return FileABB << f;
}

constexpr Square relativeSquare(Side side, Square sq) {
  return Square(sq ^ (side * 56));
}

constexpr Rank relativeRank(Side side, Rank r) {
  return Rank(r ^ (side * 7));
}

constexpr Rank relativeRank(Side side, Square sq) {
  return relativeRank(side, rankOf(sq));
}

constexpr Piece piece(Side side, PieceType p) {
    return Piece((side << 3) + p);
}

inline PieceType pieceType(Piece p) {
    return PieceType(p & 7);
}

inline Side side(Piece p) {
    //if (p == NO_PIECE)
    assert(p != NO_PIECE);
    return Side(p >> 3);
}

constexpr Move makeMove(Square from, Square to) {
    return Move((from << 6) + to);
}

template<MoveType T>
constexpr Move makeMove(Square from, Square to, PieceType pt = KNIGHT) {
    return Move(T + ((pt - KNIGHT) << 12) + (from << 6) + to);
}

constexpr Square moveTo(Move m) {
    return Square(m & 0x3F);
}

constexpr Square moveFrom(Move m) {
    return Square((m >> 6) & 0x3F);
}

constexpr MoveType moveType(Move m) {
    return MoveType(m & (3 << 14));
}

constexpr PieceType movePromotionType(Move m) {
    return PieceType(((m >> 12) & 3) + KNIGHT);
}

constexpr Direction pawnDirection(Side s) {
    return s == WHITE ? UP : DOWN;
}

inline Bitboard operator&(Bitboard b, Square s) { return b & bb(s); }
inline Bitboard operator|(Bitboard b, Square s) { return b | bb(s); }
inline Bitboard operator^(Bitboard b, Square s) { return b ^ bb(s); }
inline Bitboard operator&(Square s, Bitboard b) { return b & s; }
inline Bitboard operator|(Square s, Bitboard b) { return b | s; }
inline Bitboard operator^(Square s, Bitboard b) { return b ^ s; }

inline Bitboard& operator|=(Bitboard& b, Square s) { return b |= bb(s); }
inline Bitboard& operator^=(Bitboard& b, Square s) { return b ^= bb(s); }
inline Bitboard  operator|(Square s1, Square s2) { return bb(s1) | bb(s2); }

} /* namespace BabChess */

#endif /* CHESS_H_INCLUDED */
