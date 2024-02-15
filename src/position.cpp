#include <sstream>
#include "position.h"
#include "uci.h"

using namespace std;

namespace BabChess {

const string PIECE_TO_CHAR(" PNBRQK  pnbrqk");

char pieceToChar(Piece p) {
    assert(p>= NO_PIECE && p<= NB_PIECE);
    return PIECE_TO_CHAR[p];
}
Piece charToPiece(char c) {
    return Piece(PIECE_TO_CHAR.find(c));
}

Position::Position() {
    setFromFEN(STARTPOS_FEN);
}

void Position::reset() {
    state = history.data();

    state->fiftyMoveRule = 0;
    state->halfMoves = 0;
    state->epSquare = SQ_NONE;
    state->castlingRights = NO_CASTLING;
    //state->move = MOVE_NONE;


    for(int i=0; i<NB_SQUARE; i++) pieces[i] = NO_PIECE;
    for(int i=0; i<NB_PIECE; i++) piecesBB[i] = EmptyBB;
    //for(int i=0; i<NB_PIECE_TYPE; i++) typeBB[i] = EmptyBB;
    //typeBB[ALL_PIECES] = EmptyBB;
    sideBB[WHITE] = sideBB[BLACK] = EmptyBB;
    sideToMove = WHITE;
}

void Position::setCastlingRights(CastlingRight cr) {
    Side s = (cr & WHITE_CASTLING) ? WHITE : BLACK;

    if (getKingSquare(s) == relativeSquare(s, SQ_E1) && getPieceAt(CastlingRookFrom[cr]) == piece(s, ROOK)) {
        state->castlingRights |= cr;
    }
}

std::string Position::fen() const {
    std::ostringstream ss;

    for (int r = RANK_8; r >= RANK_1; --r) {
        for (int f = FILE_A; f <= FILE_H; ++f) {
            int nbEmpty;

            for (nbEmpty = 0; f <= FILE_H && isEmpty(square(File(f), Rank(r))); ++f)
                ++nbEmpty;

            if (nbEmpty)
                ss << nbEmpty;

            if (f <= FILE_H)
                ss << PIECE_TO_CHAR[getPieceAt(square(File(f), Rank(r)))];
        }

        if (r > RANK_1)
            ss << '/';
    }

    ss << (sideToMove == WHITE ? " w " : " b ");

    if (canCastle(WHITE_KING_SIDE)) ss << 'K';
    if (canCastle(WHITE_QUEEN_SIDE)) ss << 'Q';
    if (canCastle(BLACK_KING_SIDE)) ss << 'k';
    if (canCastle(BLACK_KING_SIDE)) ss << 'q';
    if (!canCastle(ANY_CASTLING)) ss << '-';

    ss << (getEpSquare() == SQ_NONE ? " - " : " " + Uci::formatSquare(getEpSquare()) + " ");
    ss << getFiftyMoveRule() << " " << getFullMoves();

    return ss.str();
}

void Position::setFromFEN(const std::string &fen) {
    reset();

    istringstream parser(fen);
    string token;

    // Pieces
    parser >> skipws >> token;
    int f=0, r=7;
    for(auto c : token) {
        if (c == '/') {
            r--; f=0;
        } else if (c >= '0' && c <= '9') {
            f += c-'0';
        } else {
            Piece p = charToPiece(c);
            Square sq = square(File(f), Rank(r));
            side(p) == WHITE ? setPiece<WHITE>(sq, p) : setPiece<BLACK>(sq, p);
            f++;
        }
    }

    // Side to Move
    parser >> skipws >> token;
    sideToMove = ((token[0] == 'w') ? WHITE : BLACK);

    // Casteling
    parser >> skipws >> token;
    for(auto c : token) {
        if (c == 'K') setCastlingRights(WHITE_KING_SIDE);
        else if (c == 'Q') setCastlingRights(WHITE_QUEEN_SIDE);
        else if (c == 'k') setCastlingRights(BLACK_KING_SIDE);
        else if (c == 'q') setCastlingRights(BLACK_QUEEN_SIDE);
    }

    // En passant
    parser >> skipws >> token;
    state->epSquare = Uci::parseSquare(token);
    if (state->epSquare != SQ_NONE && !(
        pawnAttacks(~sideToMove, state->epSquare) && getPiecesBB(sideToMove, PAWN)
        && (getPiecesBB(~sideToMove, PAWN) & (state->epSquare + pawnDirection(~sideToMove)))
        && !(getPiecesBB() & (state->epSquare | (state->epSquare + pawnDirection(sideToMove))))
    )) {
        
        state->epSquare = SQ_NONE;
    }

    // Rule 50 half move
    parser >> skipws >> state->fiftyMoveRule;

    // Full move
    parser >> skipws >> state->halfMoves;
    state->halfMoves = std::max(2 * (state->halfMoves - 1), 0) + (sideToMove == BLACK);

    updateBitboards();
}

std::ostream& operator<<(std::ostream& os, const Position& pos) {

    os << std::endl << " +---+---+---+---+---+---+---+---+" << std::endl;

    for (int r = RANK_8; r >= RANK_1; --r) {
        for (int f = FILE_A; f <= FILE_H; ++f)
            os << " | " << pieceToChar(pos.getPieceAt(square(File(f), Rank(r))));

        os << " | " << (1 + r) << std::endl << " +---+---+---+---+---+---+---+---+" << std::endl;
    }

    os << "   a   b   c   d   e   f   g   h" << std::endl << std::endl;
    os << "Side to move: " << (pos.getSideToMove() == WHITE ? "White" : "Black") << std::endl;
    os << "FEN: " << pos.fen() << endl;
    os << "Checkers:";
    Bitboard checkers = pos.checkers(); bitscan_loop(checkers) {
        Square sq = bitscan(checkers);
        os << " " << Uci::formatSquare(sq);
    }
    os << endl;

    return os;
}

/*
WIP:
inline bool Position::givesCheck(Move m) {
    Side Me = getSideToMove();
    Side Opp = ~Me;
    Square ksq = getKingSquare(Opp);

    Bitboard bb[NB_PIECE_TYPE] = {
        getPiecesBB(),
        getPiecesBB(Me, PAWN),
        getPiecesBB(Me, KNIGHT),
        getPiecesBB(Me, BISHOP),
        getPiecesBB(Me, ROOK),
        getPiecesBB(Me, QUEEN)
    };

    switch(moveType(m)) {
        case NORMAL:
        case CASTLING:
        case PROMOTION:
        case EN_PASSANT:
        break;
    }

    return ((pawnAttacks(Me, ksq) & bb[PAWN])
        || (attacks<KNIGHT>(ksq) & bb[KNIGHT])
        || (attacks<BISHOP>(ksq, bb[ALL_PIECES]) & (bb[BISHOP] | bb[QUEEN]))
        || (attacks<ROOK>(ksq, bb[ALL_PIECES]) & (bb[ROOK] | bb[QUEEN]))
    );
}*/

template<Side Me>
inline void Position::setPiece(Square sq, Piece p) {
    Bitboard b = bb(sq);
    pieces[sq] = p;
    //typeBB[ALL_PIECES] |= b;
    //typeBB[pieceType(p)] |= b;
    sideBB[Me] |= b;
    piecesBB[p] |= b;
}
template<Side Me>
inline void Position::unsetPiece(Square sq) {
    Bitboard b = bb(sq);
    Piece p = pieces[sq];
    pieces[sq] = NO_PIECE;
    //typeBB[ALL_PIECES] &= ~b;
    //typeBB[pieceType(p)] &= ~b;
    sideBB[Me] &= ~b;
    piecesBB[p] &= ~b;
}
template<Side Me>
inline void Position::movePiece(Square from, Square to) {
    Bitboard fromTo = from | to;
    Piece p = pieces[from];

    pieces[to] = p;
    pieces[from] = NO_PIECE;
    //typeBB[ALL_PIECES] ^= fromTo;
    //typeBB[pieceType(p)] ^= fromTo;
    sideBB[Me] ^= fromTo;
    piecesBB[p] ^= fromTo;
}

template<Side Me>
inline void Position::doMove(Move m) {
    Square from = moveFrom(m);
    Square to = moveTo(m);

    switch(moveType(m)) {
        case NORMAL:      
            if (getPieceAt(from) == piece(Me, PAWN)) {
                (int(from) ^ int(to)) == int(UP+UP) ? doMove<Me, NORMAL, true, true>(from, to) : doMove<Me, NORMAL, true, false>(from, to);
            } else {
                doMove<Me, NORMAL, false, false>(from, to);
            }

            return;
        case CASTLING:    doMove<Me, CASTLING, false, false>(from, to); return;
        case PROMOTION:   doMove<Me, PROMOTION, true, false>(from, to, movePromotionType(m)); return;
        case EN_PASSANT:  doMove<Me, EN_PASSANT, true, false>(from, to); return;
    }
}

template<Side Me, MoveType Mt, bool IsPawn, bool IsDoublePush>
inline void Position::doMove(Square from, Square to, PieceType promotionType) {
    assert(getSideToMove() == Me);
    Piece capture = getPieceAt(to);

    assert(side(getPieceAt(from)) == Me);
    assert(capture == NO_PIECE || side(capture) == ~Me);
    assert(pieceType(capture) != KING);
    assert(to == getEpSquare() || Mt != EN_PASSANT);

    State *oldState = state++;
    state->epSquare = SQ_NONE;
    state->castlingRights = oldState->castlingRights;
    state->fiftyMoveRule = oldState->fiftyMoveRule + 1;
    state->halfMoves = oldState->halfMoves + 1;
    state->capture = capture;

    if constexpr (Mt == NORMAL) {
        if (capture != NO_PIECE) {
            unsetPiece<~Me>(to);
            state->fiftyMoveRule = 0;
        }
        
        movePiece<Me>(from, to);

        // Update castling right
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);

        if constexpr (IsPawn) {
            state->fiftyMoveRule = 0;

            // Set epSquare
            if constexpr (IsDoublePush) {
                if (pawnAttacks(Me, to - pawnDirection(Me)) & getPiecesBB(~Me, PAWN)) { // Opponent pawn can enpassant
                    state->epSquare = to - pawnDirection(Me);
                }
            }
        }
    } else if constexpr (Mt == CASTLING) {
        CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        Square rookFrom = CastlingRookFrom[cr];
        Square rookTo = CastlingRookTo[cr];

        assert(pieceType(getPieceAt(from)) == KING);
        assert(getPieceAt(rookFrom) == piece(Me, ROOK));
        assert(isEmpty(CastlingPath[cr]));

        movePiece<Me>(from, to);
        movePiece<Me>(rookFrom, rookTo);

        // Update castling right
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
    } else if constexpr (Mt == PROMOTION) {
        assert(pieceType(getPieceAt(from)) == PAWN);
        assert(promotionType != NO_PIECE_TYPE && promotionType != PAWN);

        if (capture != NO_PIECE) {
            unsetPiece<~Me>(to);
        }

        unsetPiece<Me>(from);
        setPiece<Me>(to, piece(Me, promotionType));
        state->fiftyMoveRule = 0;

        // Update castling right
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
    } else if constexpr (Mt == EN_PASSANT) {
        assert(pieceType(getPieceAt(from)) == PAWN);

        Square epsq = to - pawnDirection(Me);
        unsetPiece<~Me>(epsq);
        movePiece<Me>(from, to);

        state->fiftyMoveRule = 0;
    }

    sideToMove = ~Me;
    
    updateBitboards<~Me>();
}

template<Side Me>
inline void Position::undoMove(Move m) {
    Square from = moveFrom(m);
    Square to = moveTo(m);

    switch(moveType(m)) {
        case NORMAL:      undoMove<Me, NORMAL>(from, to); return;
        case CASTLING:    undoMove<Me, CASTLING>(from, to); return;
        case PROMOTION:   undoMove<Me, PROMOTION>(from, to); return;
        case EN_PASSANT:  undoMove<Me, EN_PASSANT>(from, to); return;
    }
}

template<Side Me, MoveType Mt>
inline void Position::undoMove(Square from, Square to) {
    assert(getSideToMove() == ~Me);
    Piece capture = state->capture;

    state--;
    sideToMove = Me;

    if constexpr (Mt == NORMAL) {
        movePiece<Me>(to, from);

        if (capture != NO_PIECE) {
            setPiece<~Me>(to, capture);
        }
    } else if constexpr (Mt == CASTLING) {
        CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        Square rookFrom = CastlingRookFrom[cr];
        Square rookTo = CastlingRookTo[cr];

        movePiece<Me>(to, from);
        movePiece<Me>(rookTo, rookFrom);
    } else if constexpr (Mt == PROMOTION){
        unsetPiece<Me>(to);
        setPiece<Me>(from, piece(Me, PAWN));

        if (capture != NO_PIECE) {
            setPiece<~Me>(to, capture);
        }
    } else if constexpr (Mt == EN_PASSANT) {
        movePiece<Me>(to, from);

        Square epsq = to - pawnDirection(Me);
        setPiece<~Me>(epsq, piece(~Me, PAWN));
    }
}

template void Position::doMove<WHITE>(Move m);
template void Position::doMove<BLACK>(Move m);
template void Position::undoMove<WHITE>(Move m);
template void Position::undoMove<BLACK>(Move m);

inline void Position::updateBitboards() { 
    sideToMove == WHITE ? updateBitboards<WHITE>() : updateBitboards<BLACK>(); 
}

template<Side Me>
inline void Position::updateBitboards() {
    updateCheckedSquares<Me>();
    updateCheckers<Me>();
    checkers() ? updatePinsAndCheckMask<Me, true>() : updatePinsAndCheckMask<Me, false>();
}

template<Side Me>
inline void Position::updateCheckedSquares() {
    constexpr Side Opp = ~Me;

    //pawns
    Bitboard csq = pawnAttacks<Opp>(getPiecesBB(Opp, PAWN)) | (attacks<KING>(getKingSquare(Opp)));

    //knight
    Bitboard enemies = getPiecesBB(Opp, KNIGHT);
    bitscan_loop(enemies) {
        csq |= attacks<KNIGHT>(bitscan(enemies), getPiecesBB());
    }

    //bishop & queen
    enemies = getPiecesBB(Opp, BISHOP, QUEEN);
    bitscan_loop(enemies) {
        csq |= attacks<BISHOP>(bitscan(enemies), getPiecesBB() ^ getPiecesBB(Me, KING));
    }

    //rook & queen
    enemies = getPiecesBB(Opp, ROOK, QUEEN);
    bitscan_loop(enemies) {
        csq |= attacks<ROOK>(bitscan(enemies), getPiecesBB() ^ getPiecesBB(Me, KING));
    }

    state->checkedSquares = csq;
}

template<Side Me, bool InCheck>
inline void Position::updatePinsAndCheckMask() {
    constexpr Side Opp = ~Me;
    Square ksq = getKingSquare(Me);
    Bitboard pd = EmptyBB, po = EmptyBB, cm;

    if constexpr (InCheck) {
        cm = (pawnAttacks(Me, ksq) & getPiecesBB(Opp, PAWN)) | (attacks<KNIGHT>(ksq) & getPiecesBB(Opp, KNIGHT));
    }

    Bitboard pinners = (attacks<BISHOP>(ksq, getPiecesBB(Opp)) & getPiecesBB(Opp, BISHOP, QUEEN));
    bitscan_loop(pinners) {
        Square s = bitscan(pinners);
        Bitboard b = betweenBB(ksq, s);

        switch(popcount(b & getPiecesBB(Me))) {
            case 0: if constexpr (InCheck) cm |= b | bb(s); break;
            case 1: pd |= b | bb(s);
        }
    }

    pinners = (attacks<ROOK>(ksq, getPiecesBB(Opp)) & getPiecesBB(Opp, ROOK, QUEEN));
    bitscan_loop(pinners) {
        Square s = bitscan(pinners);
        Bitboard b = betweenBB(ksq, s);

        switch(popcount(b & getPiecesBB(Me))) {
            case 0: if constexpr (InCheck) cm |= b | bb(s); break;
            case 1: po |= b | bb(s);
        }
    }

    state->pinDiag = pd;
    state->pinOrtho = po;
    if constexpr (InCheck) state->checkMask = cm;
}

template<Side Me>
inline void Position::updateCheckers() {
    Square ksq = getKingSquare(Me);
    constexpr Side Opp = ~Me;

    state->checkers = ((pawnAttacks(Me, ksq) & getPiecesBB(Opp, PAWN))
        | (attacks<KNIGHT>(ksq) & getPiecesBB(Opp, KNIGHT))
        | (attacks<BISHOP>(ksq, getPiecesBB()) & getPiecesBB(Opp, BISHOP, QUEEN))
        | (attacks<ROOK>(ksq, getPiecesBB()) & getPiecesBB(Opp, ROOK, QUEEN))
    );
}

} /* namespace BabChess */