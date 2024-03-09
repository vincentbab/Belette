#include <sstream>
#include <cstring>
#include "position.h"
#include "uci.h"
#include "zobrist.h"

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

Position::Position(const Position &other) {
    std::memcpy(this, &other, sizeof(Position));
    this->state = this->history + (other.state - other.history);
}

Position& Position::operator=(const Position &other) {
    std::memcpy(this, &other, sizeof(Position));
    this->state = this->history + (other.state - other.history);

    return *this;
}

void Position::reset() {
    state = &history[0];

    state->fiftyMoveRule = 0;
    state->halfMoves = 0;
    state->epSquare = SQ_NONE;
    state->castlingRights = NO_CASTLING;
    state->move = MOVE_NONE;


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

bool Position::setFromFEN(const std::string &fen) {
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

            if (!isValidPiece(p) || !isValidSq(sq)) {
                reset();
                return false;
            }

            side(p) == WHITE ? setPiece<WHITE>(sq, p) : setPiece<BLACK>(sq, p);
            f++;
        }
    }

    // Side to Move
    parser >> skipws >> token;
    if (token[0] != 'w' && token[0] != 'b') {
        reset();
        return false;
    }
    sideToMove = ((token[0] == 'w') ? WHITE : BLACK);

    // Casteling
    parser >> skipws >> token;
    for(auto c : token) {
        if (c == 'K') setCastlingRights(WHITE_KING_SIDE);
        else if (c == 'Q') setCastlingRights(WHITE_QUEEN_SIDE);
        else if (c == 'k') setCastlingRights(BLACK_KING_SIDE);
        else if (c == 'q') setCastlingRights(BLACK_QUEEN_SIDE);
        else if (c == '-') continue;
        else {
            reset();
            return false;
        }
    }

    // En passant
    parser >> skipws >> token;
    state->epSquare = Uci::parseSquare(token);
    if (state->epSquare != SQ_NONE && !isValidSq(state->epSquare)) {
        reset();
        return false;
    }
    if (state->epSquare != SQ_NONE && !(
        pawnAttacks(~sideToMove, state->epSquare) && getPiecesBB(sideToMove, PAWN)
        && (getPiecesBB(~sideToMove, PAWN) & (state->epSquare + pawnDirection(~sideToMove)))
        && !(getPiecesBB() & (state->epSquare | (state->epSquare + pawnDirection(sideToMove))))
    )) {
        state->epSquare = SQ_NONE;
    }

    // Rule 50 half move
    parser >> skipws >> state->fiftyMoveRule;
    if (state->fiftyMoveRule < 0) {
        reset();
        return false;
    }

    // Full move
    parser >> skipws >> state->halfMoves;
    state->halfMoves = std::max(2 * (state->halfMoves - 1), 0) + (sideToMove == BLACK);
    if (state->halfMoves < 0) {
        reset();
        return false;
    }

    updateBitboards();
    this->state->hash = computeHash();

    return true;
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
    os << "Hash: " << pos.computeHash() << endl;
    os << "Checkers:";
    Bitboard checkers = pos.checkers(); 
    bitscan_loop(checkers) {
        Square sq = bitscan(checkers);
        os << " " << Uci::formatSquare(sq);
    }
    os << endl;

    if (pos.isFiftyMoveDraw())
        os << "[Draw by fifty move rule]" << std::endl;
    if (pos.isRepetitionDraw())
        os << "[Draw by 3-fold repetition]" << std::endl;
    if (pos.isMaterialDraw())
        os << "[Draw by insufficient material]" << std::endl;

    return os;
}

std::string Position::debugHistory() {
    stringstream ss;

    assert(this->state - this->history < MAX_HISTORY && this->state - this->history >= 0);

    for(State *s = this->state; s > this->history; s--) {
        ss << Uci::formatMove(s->move) << " ";
    }

    return ss.str();
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
}


*/

template<Side Me>
bool Position::isLegal(Move move) const {
    if (!isValidMove(move)) return false;

    Square from = moveFrom(move);
    Square to = moveTo(move);
    if (from == to) return false;

    Piece pc = getPieceAt(from);
    if (pc == NO_PIECE || side(pc) != Me) return false;

    Piece capture = getPieceAt(to);
    if (pieceType(capture) == KING) return false;
    if (capture != NO_PIECE && side(capture) != ~Me) return false;

    if (checkers()) {
        return capture != NO_PIECE ? isLegal<Me, true, true>(move, pc) : isLegal<Me, true, false>(move, pc);
    }

    return capture != NO_PIECE ? isLegal<Me, false, true>(move, pc) : isLegal<Me, false, false>(move, pc);
}

template bool Position::isLegal<WHITE>(Move move) const;
template bool Position::isLegal<BLACK>(Move move) const;

template<Side Me, bool InCheck, bool IsCapture>
inline bool Position::isLegal(Move move, Piece pc) const {
    constexpr MoveGenType Type = IsCapture ? TACTICAL_MOVES : QUIET_MOVES;

    Square from = moveFrom(move);
    Bitboard src = bb(from);
    PieceType pt = pieceType(pc);

    switch(moveType(move)) {
        case NORMAL:
            if (nbCheckers() > 1 && pt != KING) return false; // Only king move when in double check

            switch(pt) {
                case PAWN:
                    return !enumeratePawnNormalMoves<Me, InCheck, Type>(*this, src, [move](Move m, auto doMove, auto undoMove){ return move != m; });
                case KNIGHT:
                    return !enumerateKnightMoves<Me, InCheck, Type>(*this, src, [move](Move m, auto doMove, auto undoMove){ return move != m; });
                case BISHOP:
                    return !enumerateBishopSliderMoves<Me, InCheck, Type>(*this, src, [move](Move m, auto doMove, auto undoMove){ return move != m; });
                case ROOK:
                    return !enumerateRookSliderMoves<Me, InCheck, Type>(*this, src, [move](Move m, auto doMove, auto undoMove){ return move != m; });
                case QUEEN:
                    return !enumerateQueenSliderMoves<Me, InCheck, Type>(*this, src, [move](Move m, auto doMove, auto undoMove){ return move != m; });
                case KING:
                    return !enumerateKingMoves<Me, Type>(*this, from, [move](Move m, auto doMove, auto undoMove){ return move != m; });
                default:
                    return false;
            }
        case CASTLING:
            if (nbCheckers() > 0) return false; // No castling when in check

            return !enumerateCastlingMoves<Me>(*this, [move](Move m, auto doMove, auto undoMove) { return move != m; });
        case PROMOTION:
            if (nbCheckers() > 1) return false; // Only king move when in double check
            if (pt != PAWN) return false;

            return !enumeratePawnPromotionMoves<Me, InCheck, ALL_MOVES>(*this, src, [move](Move m, auto doMove, auto undoMove) { return (move != m); });
        case EN_PASSANT:
            if (nbCheckers() > 1) return false; // Only king move when in double check
            if (getEpSquare() == SQ_NONE || getEpSquare() != moveTo(move)) return false;
            if (pt != PAWN) return false;

            return !enumeratePawnEnpassantMoves<Me, InCheck>(*this, src, [move](Move m, auto doMove, auto undoMove){ return move != m; });
        break;
    }

    return false;
}

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
    switch(moveType(m)) {
        case NORMAL:
            {
                const Square from = moveFrom(m);

                if (getPieceAt(from) == piece(Me, PAWN)) {
                    const Square to = moveTo(m);
                    (int(from) ^ int(to)) == int(UP+UP) ? doMove<Me, NORMAL, true, true>(m) : doMove<Me, NORMAL, true, false>(m);
                } else {
                    doMove<Me, NORMAL, false, false>(m);
                }

                return;
            }
        case CASTLING:    doMove<Me, CASTLING, false, false>(m); return;
        case PROMOTION:   doMove<Me, PROMOTION, true, false>(m); return;
        case EN_PASSANT:  doMove<Me, EN_PASSANT, true, false>(m); return;
    }
}

template<Side Me, MoveType Mt, bool IsPawn, bool IsDoublePush>
inline void Position::doMove(Move m) {
    assert(m != MOVE_NONE);
    assert(getSideToMove() == Me);
    const Square from = moveFrom(m); 
    const Square to = moveTo(m);
    const Piece p = getPieceAt(from);
    const Piece capture = getPieceAt(to);

    assert(side(getPieceAt(from)) == Me);
    assert(capture == NO_PIECE || side(capture) == ~Me);
    assert(pieceType(capture) != KING);
    assert(to == getEpSquare() || Mt != EN_PASSANT);

    uint64_t h = state->hash;

    // Reset epSquare (branchless)
    h ^= Zobrist::enpassantKeys[fileOf(state->epSquare) + NB_FILE*(state->epSquare == SQ_NONE)];
    /*if (state->epSquare != SQ_NONE) {
        h ^= Zobrist::enpassantKeys[fileOf(state->epSquare)];
    }*/

    State *oldState = state++;
    state->epSquare = SQ_NONE;
    state->castlingRights = oldState->castlingRights;
    state->fiftyMoveRule = oldState->fiftyMoveRule + 1;
    state->halfMoves = oldState->halfMoves + 1;
    state->capture = capture;
    state->move = m;

    if constexpr (Mt == NORMAL) {
        // TODO: Try to remove branching using xor
        if (capture != NO_PIECE) {
            h ^= Zobrist::keys[capture][to];
            unsetPiece<~Me>(to);
            state->fiftyMoveRule = 0;
        }
        
        h ^= Zobrist::keys[p][from] ^ Zobrist::keys[p][to];
        movePiece<Me>(from, to);

        // Update castling right (no branching)
        h ^= Zobrist::castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        h ^= Zobrist::castlingKeys[state->castlingRights];

        if constexpr (IsPawn) {
            state->fiftyMoveRule = 0;

            // Set epSquare on double push
            if constexpr (IsDoublePush) {
                const Square epsq = to - pawnDirection(Me);

                // Only if opponent pawn can do enpassant
                if (pawnAttacks(Me, epsq) & getPiecesBB(~Me, PAWN)) {
                    h ^= Zobrist::enpassantKeys[fileOf(epsq)];
                    state->epSquare = epsq;
                }
            }
        }
    } else if constexpr (Mt == CASTLING) {
        CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        const Square rookFrom = CastlingRookFrom[cr];
        const Square rookTo = CastlingRookTo[cr];

        assert(pieceType(getPieceAt(from)) == KING);
        assert(getPieceAt(rookFrom) == piece(Me, ROOK));
        assert(isEmpty(CastlingPath[cr]));

        h ^= Zobrist::keys[piece(Me, KING)][from] ^ Zobrist::keys[piece(Me, KING)][to];
        h ^= Zobrist::keys[piece(Me, ROOK)][rookFrom] ^ Zobrist::keys[piece(Me, ROOK)][rookTo];
        movePiece<Me>(from, to);
        movePiece<Me>(rookFrom, rookTo);

        // Update castling right
        h ^= Zobrist::castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        h ^= Zobrist::castlingKeys[state->castlingRights];
    } else if constexpr (Mt == PROMOTION) {
        const PieceType promotionType = movePromotionType(m);
        assert(pieceType(getPieceAt(from)) == PAWN);
        assert(promotionType != NO_PIECE_TYPE && promotionType != PAWN);

        // TODO: Try to remove branching using xor
        if (capture != NO_PIECE) {
            h ^= Zobrist::keys[capture][to];
            unsetPiece<~Me>(to);
        }

        h ^= Zobrist::keys[piece(Me, PAWN)][from] ^ Zobrist::keys[piece(Me, promotionType)][to];
        unsetPiece<Me>(from);
        setPiece<Me>(to, piece(Me, promotionType));
        state->fiftyMoveRule = 0;

        // Update castling right
        h ^= Zobrist::castlingKeys[state->castlingRights];
        state->castlingRights &= ~(CastlingRightsMask[from] | CastlingRightsMask[to]);
        h ^= Zobrist::castlingKeys[state->castlingRights];
    } else if constexpr (Mt == EN_PASSANT) {
        assert(pieceType(getPieceAt(from)) == PAWN);

        const Square epsq = to - pawnDirection(Me);

        h ^= Zobrist::keys[piece(~Me, PAWN)][epsq];
        h ^= Zobrist::keys[piece(Me, PAWN)][from] ^ Zobrist::keys[piece(Me, PAWN)][to];
        unsetPiece<~Me>(epsq);
        movePiece<Me>(from, to);

        state->fiftyMoveRule = 0;
    }

    sideToMove = ~Me;
    h ^= Zobrist::sideToMoveKey;

    state->hash = h;
    assert(computeHash() == hash());
    
    updateBitboards<~Me>();
}

template<Side Me>
inline void Position::undoMove(Move m) {
    switch(moveType(m)) {
        case NORMAL:      undoMove<Me, NORMAL>(m); return;
        case CASTLING:    undoMove<Me, CASTLING>(m); return;
        case PROMOTION:   undoMove<Me, PROMOTION>(m); return;
        case EN_PASSANT:  undoMove<Me, EN_PASSANT>(m); return;
    }
}

template<Side Me, MoveType Mt>
inline void Position::undoMove(Move m) {
    assert(getSideToMove() == ~Me);

    const Square from = moveFrom(m);
    const Square to = moveTo(m);
    const Piece capture = state->capture;

    state--;
    sideToMove = Me;

    if constexpr (Mt == NORMAL) {
        movePiece<Me>(to, from);

        if (capture != NO_PIECE) {
            setPiece<~Me>(to, capture);
        }
    } else if constexpr (Mt == CASTLING) {
        const CastlingRight cr = Me & (to > from ? KING_SIDE : QUEEN_SIDE);
        const Square rookFrom = CastlingRookFrom[cr];
        const Square rookTo = CastlingRookTo[cr];

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

        const Square epsq = to - pawnDirection(Me);
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

    // Pawns
    Bitboard threatened = pawnAttacks<Opp>(getPiecesBB(Opp, PAWN));
    state->threatenedByPawns = threatened;

    // Knights
    Bitboard enemies = getPiecesBB(Opp, KNIGHT);
    bitscan_loop(enemies) {
        threatened |= attacks<KNIGHT>(bitscan(enemies), getPiecesBB());
    }
    state->threatenedByKnights = threatened;

    Bitboard occupied = getPiecesBB() ^ getPiecesBB(Me, KING); // x-ray through king

    // Bishops
    enemies = getPiecesBB(Opp, BISHOP);
    bitscan_loop(enemies) {
        threatened |= attacks<BISHOP>(bitscan(enemies), occupied); // x-ray through king
    }
    state->threatenedByMinors = threatened;

    // Rooks
    enemies = getPiecesBB(Opp, ROOK);
    bitscan_loop(enemies) {
        threatened |= attacks<ROOK>(bitscan(enemies), occupied);
    }
    state->threatenedByRooks = threatened;

    // Queens
    enemies = getPiecesBB(Opp, QUEEN);
    bitscan_loop(enemies) {
        Square enemy = bitscan(enemies);
        threatened |= attacks<BISHOP>(enemy, occupied);
        threatened |= attacks<ROOK>(enemy, occupied);
    }

    // King
    threatened |= attacks<KING>(getKingSquare(Opp));

    state->checkedSquares = threatened;
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

uint64_t Position::computeHash() const {
    uint64_t h = 0;

    for (Square sq=SQ_FIRST; sq<NB_SQUARE; ++sq) {
        Piece p = getPieceAt(sq);
        if (p == NO_PIECE) continue;

        h ^= Zobrist::keys[p][sq];
    }

    h ^= Zobrist::castlingKeys[getCastlingRights()];
    if (getEpSquare() != SQ_NONE) h ^= Zobrist::enpassantKeys[fileOf(getEpSquare())];
    if (getSideToMove() == BLACK) h ^= Zobrist::sideToMoveKey;

    return h;
}

// Static exchange evaluation. Algorithm from stockfish
bool Position::see(Move move, int threshold) const {
    assert(isValidMove(move));
    assert(getSideToMove() == side(getPieceAt(moveFrom(move))));

    Square from = moveFrom(move);
    Square to = moveTo(move);

    Score value = PieceValue<MG>(getPieceAt(to)) - threshold;
    if (value < 0) return false;

    value = PieceValue<MG>(getPieceAt(from)) - value;
    if (value <= 0) return true;

    Bitboard occupied = getPiecesBB() ^ from ^ to;
    Side me = ~getSideToMove();
    Bitboard allAttackers = getAttackers(to, occupied) & occupied;
    int result = 1;

    while (true) {
        Bitboard myAttackers = allAttackers & getPiecesBB(me);
        if (!myAttackers) break; // No more attackers

        // TODO: remove pinned pieces

        result ^= 1;

        Bitboard nextAttacker;

        if ((nextAttacker = myAttackers & getPiecesTypeBB(PAWN))) { // Pawn
            value = PieceValue<MG>(PAWN) - value;
            if (value < result) break;

            occupied ^= bitscan(nextAttacker);
            allAttackers |= attacks<BISHOP>(to, occupied) & getPiecesTypeBB(BISHOP, QUEEN); // Add X-Ray attackers
        } else if ((nextAttacker = myAttackers & getPiecesTypeBB(KNIGHT))) { // Knight
            value = PieceValue<MG>(KNIGHT) - value;
            if (value < result) break;

            occupied ^= bitscan(nextAttacker);
        } else if ((nextAttacker = myAttackers & getPiecesTypeBB(BISHOP))) { // Bishop
            value = PieceValue<MG>(BISHOP) - value;
            if (value < result) break;

            occupied ^= bitscan(nextAttacker);
            allAttackers |= attacks<BISHOP>(to, occupied) & getPiecesTypeBB(BISHOP, QUEEN); // Add X-Ray attackers
        } else if ((nextAttacker = myAttackers & getPiecesTypeBB(ROOK))) { // Rook
            value = PieceValue<MG>(ROOK) - value;
            if (value < result) break;

            occupied ^= bitscan(nextAttacker);
            allAttackers |= attacks<ROOK>(to, occupied) & getPiecesTypeBB(ROOK, QUEEN); // Add X-Ray attackers
        } else if ((nextAttacker = myAttackers & getPiecesTypeBB(QUEEN))) { // Queen
            value = PieceValue<MG>(QUEEN) - value;
            if (value < result) break;

            occupied ^= bitscan(nextAttacker);
            allAttackers |= (attacks<BISHOP>(to, occupied) & getPiecesTypeBB(BISHOP, QUEEN)) // Add X-Ray attackers
                         |  (attacks<ROOK>(to, occupied) & getPiecesTypeBB(ROOK, QUEEN));
        } else { // King
            // King cannot capture if opponent still has attackers
            return (allAttackers & ~getPiecesBB(me)) ? result ^ 1 : result;
        }

        me = ~me;
        allAttackers &= occupied;
    }

    return (bool)result;
}

} /* namespace BabChess */