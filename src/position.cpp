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
            setPiece(square(File(f), Rank(r)), p);
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

    os << "   a   b   c   d   e   f   g   h" << std::endl;
    os << "Side to move: " << (pos.getSideToMove() == WHITE ? "White" : "Black") << std::endl;
    os << "FEN: " << pos.fen() << endl;

    return os;
}

template<Side Me>
inline void Position::doMove(Move m) {
    assert(getSideToMove() == Me);
    Square from = moveFrom(m);
    Square to = moveTo(m);
    Piece pc = getPieceAt(from);
    Piece capture = getPieceAt(to);

    assert(side(pc) == Me);
    assert(capture == NO_PIECE || side(capture) == ~Me);
    if (pieceType(capture) == KING) {
        assert(pieceType(capture) != KING);
    }
    
    assert(to == getEpSquare() || moveType(m) != EN_PASSANT);

    State *oldState = state++;
    state->epSquare = SQ_NONE;
    state->castlingRights = oldState->castlingRights;
    state->fiftyMoveRule = oldState->fiftyMoveRule + 1;
    state->halfMoves = oldState->halfMoves + 1;
    state->capture = capture;

    switch(moveType(m)) {
        case NORMAL:
            {
                if (capture != NO_PIECE) {
                    unsetPiece(to);
                    state->fiftyMoveRule = 0;
                }
                
                movePiece(from, to);

                // Update castling right
                state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);

                if (pieceType(pc) == PAWN) {
                    state->fiftyMoveRule = 0;

                    // Set epSquare
                    if ((int(from) ^ int(to)) == int(UP+UP) // Double push
                        && (pawnAttacks(Me, to - pawnDirection(Me)) & getPiecesBB(~Me, PAWN)) // Opponent pawn can enpassant
                    ) {
                        state->epSquare = to - pawnDirection(Me);
                    }
                }
            }
            break;
        case CASTLING:
            {
                CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
                Square rookFrom = CastlingRookFrom[cr];
                Square rookTo = CastlingRookTo[cr];

                assert(pieceType(pc) == KING);
                assert(getPieceAt(rookFrom) == piece(Me, ROOK));
                assert(isEmpty(CastlingPath[cr]));

                movePiece(from, to);
                movePiece(rookFrom, rookTo);

                // Update castling right
                state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
            }
            break;
        case PROMOTION:
            {
                assert(pieceType(pc) == PAWN);

                if (capture != NO_PIECE) {
                    unsetPiece(to);
                }

                unsetPiece(from);
                setPiece(to, piece(Me, movePromotionType(m)));
                state->fiftyMoveRule = 0;

                // Update castling right
                state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
            }
            break;
        case EN_PASSANT: 
            {
                assert(pieceType(pc) == PAWN);

                Square epsq = to - pawnDirection(Me);
                unsetPiece(epsq);
                movePiece(from, to);

                state->fiftyMoveRule = 0;
            }
            break;
    }

    sideToMove = ~Me;
    
    updateBitboards<~Me>();
}

template<Side Me>
inline void Position::undoMove(Move m) {
    assert(getSideToMove() == ~Me);
    Square from = moveFrom(m);
    Square to = moveTo(m);
    Piece capture = state->capture;

    state--;
    sideToMove = Me;

    switch(moveType(m)) {
        case NORMAL: 
            {
                movePiece(to, from);

                if (capture != NO_PIECE) {
                    setPiece(to, capture);
                }
            }   
            break;
        case CASTLING:
            {
                CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
                Square rookFrom = CastlingRookFrom[cr];
                Square rookTo = CastlingRookTo[cr];

                movePiece(to, from);
                movePiece(rookTo, rookFrom);
            }
            break;
        case PROMOTION:
            {
                unsetPiece(to);
                setPiece(from, piece(Me, PAWN));

                if (capture != NO_PIECE) {
                    setPiece(to, capture);
                }
            }
            break;
        case EN_PASSANT:
            {
                movePiece(to, from);

                Square epsq = to - pawnDirection(Me);
                setPiece(epsq, piece(~Me, PAWN));
            }
            break;
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