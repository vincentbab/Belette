#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <string>
#include <array>
#include <vector>
#include "chess.h"
#include "bitboard.h"
#include "zobrist.h"

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
#define KIWIPETE_FEN "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"

namespace Belette {

struct State {
    CastlingRight castlingRights;
    Square epSquare;
    int fiftyMoveRule;
    int halfMoves;
    int pliesFromNull;
    int repetition;
    Move move;

    Piece capture;

    uint64_t hash;
    Bitboard threatsFor[NB_PIECE_TYPE];
    Bitboard checkers;
    Bitboard checkMask;
    Bitboard pinDiag;
    Bitboard pinOrtho;
    Bitboard oppPins;

    inline State& prev() { return *(this-1); }
    inline const State& prev() const { return *(this-1); }

    inline State& next() { return *(this+1); }
    inline const State& next() const { return *(this+1); }
};

class Position {
public:
    static void init();

    Position();
    Position(const Position &other);
    Position& operator=(const Position &other);

    void reset();
    bool setFromFEN(const std::string &fen);
    std::string fen() const;
    
    inline void doMove(Move m) { getSideToMove() == WHITE ? doMove<WHITE>(m) : doMove<BLACK>(m); }
    template<Side Me> inline void doMove(Move m);

    inline void undoMove(Move m) { getSideToMove() == WHITE ? undoMove<WHITE>(m) : undoMove<BLACK>(m); }
    template<Side Me> inline void undoMove(Move m);

    template<Side Me> void doNullMove();
    template<Side Me> void undoNullMove();

    //bool givesCheck(Move m);

    inline Side getSideToMove() const { return sideToMove; }
    inline int getFiftyMoveRule() const { return state->fiftyMoveRule; }
    inline int getHalfMoves() const { return state->halfMoves; }
    inline int getFullMoves() const { return 1 + (getHalfMoves() - (sideToMove == BLACK)) / 2; }
    inline Square getEpSquare() const { return state->epSquare; }
    inline Piece getPieceAt(Square sq) const { return pieces[sq]; }
    inline bool isEmpty(Square sq) const { return getPieceAt(sq) == NO_PIECE; }
    inline bool isEmpty(Bitboard b) const { return !(b & getPiecesBB()); }
    inline bool canCastle(CastlingRight cr) const { return state->castlingRights & cr; }
    inline CastlingRight getCastlingRights() const { return state->castlingRights; }
    
    /*inline Bitboard getPiecesBB() const { return typeBB[ALL_PIECES]; }
    inline Bitboard getPiecesBB(Side side) const { return sideBB[side]; }
    inline Bitboard getPiecesBB(Side side, PieceType pt) const { return sideBB[side] & typeBB[pt]; }
    inline Bitboard getPiecesBB(Side side, PieceType pt1, PieceType pt2) const { return sideBB[side] & (typeBB[pt1] | typeBB[pt2]); }
    inline Square getKingSquare(Side side) const { return Square(bitscan(sideBB[side] & typeBB[KING])); }*/

    //inline Bitboard getPiecesBB(Side side) const { return side == WHITE ? piecesBB[W_PAWN]|piecesBB[W_KING]|piecesBB[W_KNIGHT]|piecesBB[W_BISHOP]|piecesBB[W_ROOK]|piecesBB[W_QUEEN] : piecesBB[B_PAWN]|piecesBB[B_KING]|piecesBB[B_KNIGHT]|piecesBB[B_BISHOP]|piecesBB[B_ROOK]|piecesBB[B_QUEEN]; }
    inline Bitboard getPiecesBB(Side side) const { return sideBB[side]; }
    inline Bitboard getPiecesBB() const { return getPiecesBB(WHITE) | getPiecesBB(BLACK); }
    inline Bitboard getPiecesBB(Side side, PieceType pt) const { return piecesBB[piece(side, pt)]; }
    //template<Side Me> inline Bitboard getPiecesBB(PieceType pt) const { return piecesBB[piece(Me, pt)]; }
    inline Bitboard getPiecesBB(Side side, PieceType pt1, PieceType pt2) const { return piecesBB[piece(side, pt1)] | piecesBB[piece(side, pt2)]; }
    //template<Side Me>  inline Bitboard getPiecesBB(PieceType pt1, PieceType pt2) const { return piecesBB[piece(Me, pt1)] | piecesBB[piece(Me, pt2)]; }
    inline Square getKingSquare(Side side) const { return Square(bitscan(piecesBB[side == WHITE ? W_KING : B_KING])); }
    //template<Side Me> inline Square getKingSquare() const { return Square(bitscan(piecesBB[Me == WHITE ? W_KING : B_KING])); }

    inline Bitboard getPiecesTypeBB(PieceType pt) const { return getPiecesBB(WHITE, pt) | getPiecesBB(BLACK, pt); }
    inline Bitboard getPiecesTypeBB(PieceType pt1, PieceType pt2) const { return getPiecesBB(WHITE, pt1, pt2) | getPiecesBB(BLACK, pt1, pt2); }

    inline Bitboard nbPieces(Side side) const { return popcount(getPiecesBB(side)); }
    inline Bitboard nbPieces() const { return popcount(getPiecesBB(WHITE) | getPiecesBB(BLACK)); }
    inline Bitboard nbPieces(Side side, PieceType pt) const { return popcount(getPiecesBB(side, pt)); }
    inline Bitboard nbPieces(Side side, PieceType pt1, PieceType pt2) const { return popcount(getPiecesBB(side, pt1, pt2)); }
    inline Bitboard nbPieceTypes(PieceType pt) const { return popcount(getPiecesBB(WHITE, pt) | getPiecesBB(BLACK, pt)); }

    inline Bitboard getEmptyBB() const { return ~getPiecesBB(); }

    inline Bitboard getAttackers(Square sq, Bitboard occupied) const;

    inline Bitboard threatsFor(PieceType pt) const { return state->threatsFor[pt]; }
    inline Bitboard checkedSquares() const { return threatsFor(KING); }
    inline Bitboard checkers() const { return state->checkers; }
    inline Bitboard nbCheckers() const { return popcount(state->checkers); }
    inline bool inCheck() const { return !!state->checkers; }

    inline uint64_t hash() const { return state->hash; }
    inline int psq(Phase p) const { return psqValue[p]; }
    inline uint64_t pawnHash() const { return pawnKey; }
    inline uint64_t nonPawnHash(Side side) const { return nonPawnKeys[side]; }
    inline uint64_t minorHash() const { return minorKey; }
    uint64_t computeHash() const;
    inline uint64_t computePawnHash() const;
    inline uint64_t computeNonPawnHash(Side side) const;
    inline uint64_t computeMinorHash() const;
    inline uint64_t getHashAfter(Move m) const;
    inline uint64_t getHashAfterNullMove() const { return hash() ^ Zobrist::sideToMoveKey; };

    inline Bitboard checkMask() const { return state->checkMask; }
    inline Bitboard pinDiag() const { return state->pinDiag; }
    inline Bitboard pinOrtho() const { return state->pinOrtho; }
    inline Bitboard oppPins() const { return state->oppPins; }

    inline size_t historySize() const { return state - history; }

    inline bool isRepetitionDraw(int ply = 0) const { return state->repetition && state->repetition < ply; }
    bool hasUpcomingRepetition(int ply) const;
    bool hasLegalMoves() const;

    inline bool isFiftyMoveDraw() const { return state->fiftyMoveRule > 99 && (!inCheck() || hasLegalMoves()); }
    inline bool isMaterialDraw() const;
    inline bool isDraw(int ply) const { return isFiftyMoveDraw() || isRepetitionDraw(ply) || isMaterialDraw(); }
    template<Side Me> inline bool hasNonPawnMateriel() { return getPiecesBB(Me, PAWN, KING) != getPiecesBB(Me); }

    template<Side Me> bool isLegal(Move m) const;
    inline bool isLegal(Move m) const { return getSideToMove() == WHITE ? isLegal<WHITE>(m) : isLegal<BLACK>(m); };
    inline bool isCapture(Move m) const { return getPieceAt(moveTo(m)) != NO_PIECE || moveType(m) == EN_PASSANT; }
    inline bool isTactical(Move m) const { return isCapture(m) || (moveType(m) == PROMOTION && movePromotionType(m) == QUEEN); }

    inline Move previousMove() const { return state->move; }
    
    bool see(Move m, int threshold) const ;
    template<Side Me> inline bool givesCheck(Move m) const;

    std::string debugHistory();

private:
    void setCastlingRights(CastlingRight cr);

    template<Side Me, MoveType Mt> void doMove(Move m);
    template<Side Me, MoveType Mt> void undoMove(Move m);

    template<Side Me, bool InCheck, bool IsCapture> bool isLegal(Move m, Piece pc) const;

    template<Side Me> inline void setPiece(Square sq, Piece p);
    template<Side Me> inline void unsetPiece(Square sq);
    template<Side Me> inline void movePiece(Square from, Square to);

    template<Side Me> inline void updateThreatenedSquares();
    template<Side Me> inline void updateCheckers();
    template<Side Me, bool InCheck> inline void updatePinsAndCheckMask();
    template<Side Me> inline void updateOppPins();

    inline void updateBitboards();
    template<Side Me> inline void updateBitboards();

    Piece pieces[NB_SQUARE];
    //Bitboard typeBB[NB_PIECE_TYPE];
    Bitboard sideBB[NB_SIDE];
    Bitboard piecesBB[NB_PIECE];

    uint64_t pawnKey;
    uint64_t nonPawnKeys[NB_SIDE];
    uint64_t minorKey;
    int psqValue[NB_PHASE];

    Side sideToMove;

    State *state;
    State history[MAX_HISTORY];
};

std::ostream& operator<<(std::ostream& os, const Position& pos);

template<Side Me>
inline bool Position::givesCheck(Move m) const {
    constexpr Side Opp = ~Me;
    const Square from = moveFrom(m), to = moveTo(m);
    const Square ksq = getKingSquare(Opp);
    const MoveType mt = moveType(m);

    Bitboard occupied = (getPiecesBB() ^ bb(from)) | bb(to);
    Bitboard vacated = bb(from);

    if (mt == EN_PASSANT) {
        const Square epsq = to - pawnDirection(Me);
        occupied ^= bb(epsq);
        vacated |= bb(epsq);
    } else if (mt == CASTLING) {
        const CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        occupied ^= bb(CastlingRookFrom[cr]) | bb(CastlingRookTo[cr]);

        return !!(attacks<ROOK>(CastlingRookTo[cr], occupied) & ksq);
    }

    // Direct check
    switch (mt == PROMOTION ? movePromotionType(m) : pieceType(getPieceAt(from))) {
        case PAWN:   if (pawnAttacks(Opp, ksq) & to) return true; break;
        case KNIGHT: if (attacks<KNIGHT>(ksq) & to) return true; break;
        case BISHOP: if (attacks<BISHOP>(ksq, occupied) & to) return true; break;
        case ROOK:   if (attacks<ROOK>(ksq, occupied) & to) return true; break;
        case QUEEN:  if (attacks<QUEEN>(ksq, occupied) & to) return true; break;
        default: break;
    }

    // Discovered check
    if (mt != EN_PASSANT) {
        const Bitboard line = lineBB(ksq, from);
        if (line == EmptyBB || (line & to)) return false;
    }

    vacated = ~vacated;
    return !!(((attacks<BISHOP>(ksq, occupied) & getPiecesBB(Me, BISHOP, QUEEN))
             | (attacks<ROOK>(ksq, occupied) & getPiecesBB(Me, ROOK, QUEEN))) & vacated);
}

inline Bitboard Position::getAttackers(Square sq, Bitboard occupied) const {
    return ((pawnAttacks(BLACK, sq) & getPiecesBB(WHITE, PAWN))
          | (pawnAttacks(WHITE, sq) & getPiecesBB(BLACK, PAWN))
          | (attacks<KNIGHT>(sq) & getPiecesTypeBB(KNIGHT))
          | (attacks<BISHOP>(sq, occupied) & getPiecesTypeBB(BISHOP, QUEEN))
          | (attacks<ROOK>(sq, occupied) & getPiecesTypeBB(ROOK, QUEEN))
          | (attacks<KING>(sq) & getPiecesTypeBB(KING))
    );
}

inline bool Position::isMaterialDraw() const {
    if ((getPiecesTypeBB(PAWN) | getPiecesTypeBB(ROOK) | getPiecesTypeBB(QUEEN)) != 0)
        return false;

    Bitboard knights = getPiecesTypeBB(KNIGHT);
    Bitboard bishops = getPiecesTypeBB(BISHOP);

    if (popcount(knights | bishops) <= 1)
        return true;

    constexpr Bitboard DarkSquaresBB = 0xAA55AA55AA55AA55ULL;
    if (!knights && ((bishops & DarkSquaresBB) == bishops || !(bishops & DarkSquaresBB)))
        return true;

    return false;
}

template<Side Me>
inline void Position::doMove(Move m) {
    switch(moveType(m)) {
        case NORMAL:     doMove<Me, NORMAL>(m); return;
        case CASTLING:   doMove<Me, CASTLING>(m); return;
        case PROMOTION:  doMove<Me, PROMOTION>(m); return;
        case EN_PASSANT: doMove<Me, EN_PASSANT>(m); return;
    }
}

template<Side Me>
inline void Position::undoMove(Move m) {
    switch(moveType(m)) {
        case NORMAL:     undoMove<Me, NORMAL>(m); return;
        case CASTLING:   undoMove<Me, CASTLING>(m); return;
        case PROMOTION:  undoMove<Me, PROMOTION>(m); return;
        case EN_PASSANT: undoMove<Me, EN_PASSANT>(m); return;
    }
}

inline uint64_t Position::computePawnHash() const {
    uint64_t h = 0;
    Bitboard wp = getPiecesBB(WHITE, PAWN), bp = getPiecesBB(BLACK, PAWN);

    bitscan_loop(wp) {
        h ^= Zobrist::keys[W_PAWN][bitscan(wp)];
    }
    bitscan_loop(bp) {
        h ^= Zobrist::keys[B_PAWN][bitscan(bp)];
    }

    return h;
}

inline uint64_t Position::computeNonPawnHash(Side side) const {
    uint64_t h = 0;
    Bitboard b = getPiecesBB(side) ^ getPiecesBB(side, PAWN);

    bitscan_loop(b) {
        Square sq = bitscan(b);
        h ^= Zobrist::keys[getPieceAt(sq)][sq];
    }

    return h;
}

inline uint64_t Position::computeMinorHash() const {
    uint64_t h = 0;
    Bitboard b = getPiecesTypeBB(KNIGHT, BISHOP) | getPiecesTypeBB(KING);

    bitscan_loop(b) {
        Square sq = bitscan(b);
        h ^= Zobrist::keys[getPieceAt(sq)][sq];
    }

    return h;
}

inline uint64_t Position::getHashAfter(Move m) const {
    uint64_t h = hash();
    const Square from = moveFrom(m), to = moveTo(m);
    const Piece p = getPieceAt(from);
    const Piece capture = getPieceAt(to);

    h ^= Zobrist::sideToMoveKey;
    h ^= Zobrist::keys[p][from] ^ Zobrist::keys[p][to];
    if (capture != NO_PIECE) 
        h ^= Zobrist::keys[capture][to];

    return h;
}

} /* namespace Belette */

#endif /* POSITION_H_INCLUDED */
