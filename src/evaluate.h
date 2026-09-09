#ifndef EVALUATE_H_INCLUDED
#define EVALUATE_H_INCLUDED

#include <array>
#include "chess.h"
#include "position.h"

namespace Belette {

constexpr Score Tempo = 19;

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

constexpr int PHASE_TOTAL = 24;

constexpr Score PSQT[NB_PIECE_TYPE][NB_PHASE][NB_SQUARE] = {
    {},
    // Pawn
    {
        {
               0,    0,    0,    0,    0,    0,    0,    0,
             -54,  -30,  -34,  -43,  -26,  -11,    9,  -33,
             -52,  -29,  -30,  -29,  -16,  -25,    3,  -25,
             -50,  -26,  -27,  -13,  -13,  -19,  -10,  -30,
             -41,  -18,  -16,  -13,    6,   -2,    3,  -19,
             -25,  -13,   18,   22,   24,   45,   29,  -11,
              45,   72,   45,   72,   47,   41,  -15,  -45,
               0,    0,    0,    0,    0,    0,    0,    0,
        },
        {
               0,    0,    0,    0,    0,    0,    0,    0,
              17,   18,    5,   10,   17,    5,    5,   -2,
              13,   15,   -1,   10,    3,    1,    5,   -3,
              18,   16,    0,   -3,   -5,   -3,    8,    1,
              41,   31,   13,    5,   -3,    1,   17,   17,
             105,  111,   79,   61,   54,   39,   82,   81,
             164,  153,  158,  113,  116,  120,  159,  177,
               0,    0,    0,    0,    0,    0,    0,    0,
        }
    },
    // Knight
    {
        {
            -161, -116, -124, -110, -105,  -96, -113, -125,
            -122, -110,  -96,  -87,  -86,  -82,  -94,  -98,
            -110,  -89,  -80,  -74,  -63,  -76,  -71,  -96,
             -93,  -78,  -66,  -65,  -57,  -62,  -62,  -83,
             -78,  -68,  -46,  -28,  -46,  -21,  -59,  -47,
             -78,  -43,  -24,  -18,   19,   13,  -20,  -51,
             -90,  -65,  -43,  -28,  -51,    9,  -74,  -50,
            -206, -197, -112,  -74,  -55, -117, -226, -158,
        },
        {
               3,  -14,   14,   18,   15,    7,   -7,  -14,
               5,   19,   27,   30,   29,   25,   10,   12,
              10,   29,   39,   51,   50,   35,   24,   14,
              29,   37,   58,   57,   60,   52,   39,   19,
              28,   45,   57,   58,   60,   54,   46,   21,
              23,   30,   44,   45,   32,   29,   19,    8,
               9,   23,   26,   25,   18,    8,   20,  -11,
             -48,   10,   20,    8,   14,  -15,   26,  -66,
        }
    },
    // Bishop
    {
        {
             -96,  -77,  -95,  -99,  -94,  -98,  -71,  -80,
             -77,  -77,  -65,  -86,  -80,  -67,  -63,  -73,
             -78,  -72,  -72,  -72,  -70,  -74,  -70,  -65,
             -85,  -74,  -72,  -51,  -53,  -71,  -75,  -80,
             -80,  -69,  -47,  -38,  -41,  -44,  -69,  -79,
             -71,  -51,  -48,  -28,  -42,  -12,  -35,  -47,
             -74,  -50,  -61,  -82,  -48,  -55,  -57,  -81,
             -94, -114,  -87, -140, -130, -130,  -79, -118,
        },
        {
               8,   25,    5,   24,   20,   21,    7,  -11,
              22,   21,   20,   33,   34,   25,   25,    3,
              25,   34,   42,   42,   45,   42,   27,   16,
              26,   42,   49,   46,   46,   44,   40,   18,
              30,   45,   39,   52,   46,   41,   42,   32,
              36,   29,   39,   27,   32,   35,   29,   27,
               9,   26,   30,   33,   23,   22,   29,   13,
              22,   30,   23,   41,   35,   30,   18,   19,
        }
    },
    // Rook
    {
        {
            -130, -126, -118, -113, -109, -121, -106, -129,
            -142, -132, -118, -121, -118, -116, -103, -128,
            -140, -129, -123, -124, -119, -124,  -92, -112,
            -133, -129, -121, -108, -109, -124, -104, -114,
            -115, -102, -100,  -91,  -87,  -88,  -84,  -80,
             -98,  -81,  -78,  -75,  -49,  -49,  -15,  -36,
             -81,  -78,  -64,  -43,  -60,  -32,  -56,  -27,
             -70,  -76,  -71,  -64,  -49,    2,  -21,  -38,
        },
        {
              40,   50,   57,   55,   47,   43,   39,   31,
              47,   49,   51,   51,   43,   40,   31,   38,
              52,   51,   50,   54,   50,   44,   26,   26,
              57,   60,   62,   60,   57,   54,   44,   42,
              64,   61,   70,   66,   52,   49,   47,   42,
              62,   64,   64,   61,   51,   46,   39,   35,
              60,   69,   72,   63,   63,   53,   55,   43,
              60,   65,   73,   66,   60,   40,   46,   51,
        }
    },
    // Queen
    {
        {
            -271, -277, -272, -263, -267, -279, -250, -270,
            -266, -263, -255, -256, -257, -247, -242, -228,
            -264, -260, -264, -267, -262, -258, -245, -249,
            -264, -263, -265, -259, -258, -261, -250, -247,
            -263, -259, -253, -255, -251, -242, -245, -239,
            -248, -250, -246, -237, -233, -192, -193, -200,
            -244, -262, -254, -260, -252, -229, -241, -210,
            -286, -273, -253, -209, -213, -203, -198, -247,
        },
        {
              35,   41,   42,   36,   40,   37,   -6,   17,
              38,   42,   38,   48,   51,   27,    1,  -41,
              42,   58,   80,   79,   80,   74,   52,   39,
              55,   82,   88,  109,  105,   98,   79,   65,
              64,   78,   93,  113,  123,  111,   98,   78,
              54,   71,   99,  107,  119,   96,   62,   58,
              44,   79,  105,  118,  133,  105,   87,   70,
              69,   79,   96,   74,   72,   62,   30,   65,
        }
    },
    // King
    {
        {
              73,   91,   64,  -23,   31,    1,   72,   75,
              77,   35,   25,   -7,   -8,    9,   51,   60,
              -3,   15,  -44,  -51,  -44,  -47,   -3,  -19,
             -47,  -47,  -61,  -96, -102,  -68,  -80,  -76,
             -48,  -65,  -77, -127, -112,  -69,  -81, -109,
             -94,   11,  -39,  -59,  -24,   43,   12,   -4,
             -96,  -34,  -77,   50,   -8,    6,   28,    2,
              15,   31,   49,  -80,    0,   43,   62,   91,
        },
        {
             -75,  -57,  -41,  -24,  -46,  -26,  -51,  -77,
             -45,  -19,   -8,    2,    5,   -4,  -21,  -39,
             -23,   -4,   16,   26,   25,   19,    0,  -11,
             -13,   13,   32,   46,   47,   36,   27,    6,
              -2,   29,   44,   57,   56,   51,   46,   24,
              11,   22,   38,   47,   50,   44,   44,   16,
               2,   22,   29,    9,   31,   38,   32,    7,
             -81,  -43,  -35,    8,  -16,  -13,  -16, -110,
        }
    }
};

struct PSQPair { int16_t mg, eg; };

constexpr auto PSQ = []() {
    std::array<std::array<PSQPair, NB_SQUARE>, NB_PIECE> psq{};

    for (int pt = PAWN; pt <= KING; pt++) {
        for (int sq = 0; sq < NB_SQUARE; sq++) {
            psq[piece(WHITE, PieceType(pt))][sq] = {
                int16_t(PIECE_TYPE_VALUE[pt][MG] + PSQT[pt][MG][sq]),
                int16_t(PIECE_TYPE_VALUE[pt][EG] + PSQT[pt][EG][sq])
            };
            psq[piece(BLACK, PieceType(pt))][sq] = {
                int16_t(-(PIECE_TYPE_VALUE[pt][MG] + PSQT[pt][MG][sq ^ 56])),
                int16_t(-(PIECE_TYPE_VALUE[pt][EG] + PSQT[pt][EG][sq ^ 56]))
            };
        }
    }

    return psq;
}();

template<Side Me>
Score evaluate(const Position &pos);

inline Score evaluate(const Position &pos) {
    return pos.getSideToMove() == WHITE ? evaluate<WHITE>(pos) : evaluate<BLACK>(pos);
};

} /* namespace Belette */

#endif /* EVALUATE_H_INCLUDED */
