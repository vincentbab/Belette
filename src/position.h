#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <string>
#include <array>
#include <vector>
#include "chess.h"
#include "bitboard.h"

#define STARTPOS_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

namespace BabChess {

class State {
public:
    CastlingRight castlingRights;
    Square epSquare;
    int fiftyMoveRule;
    int halfMoves;
    //Move move;

    Piece capture;

    Bitboard checkedSquares;
    Bitboard checkers;
    Bitboard checkMask;
    Bitboard pinDiag;
    Bitboard pinOrtho;
};

typedef std::array<State, MAX_HISTORY> StateList;

class Position {
public:
    Position();
    void reset();
    void setFromFEN(const std::string &fen);
    std::string fen() const;
    
    inline void doMove(Move m) { getSideToMove() == WHITE ? doMove<WHITE>(m) : doMove<BLACK>(m); }
    template<Side Me> void doMove(Move m);

    template<Side Me> void undoMove(Move m);

    inline Side getSideToMove() const { return sideToMove; }
    inline int getFiftyMoveRule() const { return state->fiftyMoveRule; }
    inline int getHalfMoves() const { return state->halfMoves; }
    inline int getFullMoves() const { return 1 + (getHalfMoves() - (sideToMove == BLACK)) / 2; }
    inline Square getEpSquare() const { return state->epSquare; }
    inline Piece getPieceAt(Square sq) const { return pieces[sq]; }
    inline bool isEmpty(Square sq) const { return getPieceAt(sq) == NO_PIECE; }
    inline bool isEmpty(Bitboard b) const { return !(b & getPiecesBB()); }
    inline bool canCastle(CastlingRight cr) const { return state->castlingRights & cr; }
    
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

    inline Bitboard getEmptyBB() const { return ~getPiecesBB(); }

    inline Bitboard getAttackers(Side side, Square sq, Bitboard occupied) const;

    inline Bitboard checkedSquares() const { return state->checkedSquares; }
    inline Bitboard checkers() const { return state->checkers; }
    inline Bitboard checkMask() const { return state->checkMask; }
    inline Bitboard pinDiag() const { return state->pinDiag; }
    inline Bitboard pinOrtho() const { return state->pinOrtho; }

private:
    void setCastlingRights(CastlingRight cr);

    inline void setPiece(Square sq, Piece p);
    inline void unsetPiece(Square sq);
    inline void movePiece(Square from, Square to);

    template<Side Me> inline void updateCheckedSquares();
    template<Side Me> inline void updateCheckers();
    template<Side Me, bool InCheck> inline void updatePinsAndCheckMask();

    inline void updateBitboards();
    template<Side Me> inline void updateBitboards();

    Piece pieces[NB_SQUARE];
    //Bitboard typeBB[NB_PIECE_TYPE];
    Bitboard sideBB[NB_SIDE];
    Bitboard piecesBB[NB_PIECE];

    Side sideToMove;

    State *state;
    StateList history;
};

std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Bitboard Position::getAttackers(Side side, Square sq, Bitboard occupied) const {
    return ((pawnAttacks(~side, sq) & getPiecesBB(side, PAWN))
        | (attacks<KNIGHT>(sq) & getPiecesBB(side, KNIGHT))
        | (attacks<BISHOP>(sq, occupied) & getPiecesBB(side, BISHOP, QUEEN))
        | (attacks<ROOK>(sq, occupied) & getPiecesBB(side, ROOK, QUEEN))
        //| (attacks<KING>(sq) & getPiecesBB(side, KING))
    );
}
/*template<Side Me>
inline Bitboard Position::getAttackers(Square sq, Bitboard occupied) const {
    return ((pawnAttacks(~Me, sq) & getPiecesBB(Me, PAWN))
        | (attacks<KNIGHT>(sq) & getPiecesBB(Me, KNIGHT))
        | (attacks<BISHOP>(sq, occupied) & getPiecesBB(Me, BISHOP, QUEEN))
        | (attacks<ROOK>(sq, occupied) & getPiecesBB(Me, ROOK, QUEEN))
        | (attacks<KING>(sq) & getPiecesBB(Me, KING))
    );
}*/

inline void Position::setPiece(Square sq, Piece p) {
    Bitboard b = bb(sq);
    pieces[sq] = p;
    //typeBB[ALL_PIECES] |= b;
    //typeBB[pieceType(p)] |= b;
    sideBB[side(p)] |= b;
    piecesBB[p] |= b;
}
inline void Position::unsetPiece(Square sq) {
    Bitboard b = bb(sq);
    Piece p = pieces[sq];
    pieces[sq] = NO_PIECE;
    //typeBB[ALL_PIECES] &= ~b;
    //typeBB[pieceType(p)] &= ~b;
    sideBB[side(p)] &= ~b;
    piecesBB[p] &= ~b;
}
inline void Position::movePiece(Square from, Square to) {
    Bitboard fromTo = from | to;
    Piece p = pieces[from];

    pieces[to] = p;
    pieces[from] = NO_PIECE;
    //typeBB[ALL_PIECES] ^= fromTo;
    //typeBB[pieceType(p)] ^= fromTo;
    sideBB[side(p)] ^= fromTo;
    piecesBB[p] ^= fromTo;
}



} /* namespace BabChess */

#endif /* POSITION_H_INCLUDED */
