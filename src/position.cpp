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
    //sideBB[WHITE] = sideBB[BLACK] = EmptyBB;
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

} /* namespace BabChess */