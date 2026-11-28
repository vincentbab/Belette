#include <algorithm>
#include <bit>
#include <cstring>
#include "binpack.h"
#include "bitboard.h"
#include "movegen.h"

namespace Belette {

constexpr size_t CHUNK_SIZE = 1024 * 1024;
constexpr int SCORE_VLE_BLOCK_SIZE = 4;

static uint16_t signedToUnsigned(int16_t a) {
    uint16_t r = uint16_t(a);
    if (r & 0x8000) r ^= 0x7FFF;
    return uint16_t((r << 1) | (r >> 15));
}

static int usedBitsSafe(size_t n) {
    return n == 0 ? 0 : std::bit_width(n - 1);
}

static Bitboard before(Square sq) {
    return bb(sq) - 1;
}

static int16_t clampScore(Score score) {
    return int16_t(std::clamp<int>(score, -32000, 32000));
}

// The format only stores the en passant square when the capture is legal
static Square legalEpSquare(const Position &pos) {
    if (pos.getEpSquare() == SQ_NONE)
        return SQ_NONE;

    bool legal = false;
    enumerateCurrentVariantMoves(pos, [&](Move m) {
        legal = moveType(m) == EN_PASSANT;
        return !legal;
    });

    return legal ? pos.getEpSquare() : SQ_NONE;
}

static Bitboard pieceAttacks(PieceType pt, Square sq, Bitboard occupied) {
    switch (pt) {
    case KNIGHT: return attacks<KNIGHT>(sq);
    case BISHOP: return attacks<BISHOP>(sq, occupied);
    case ROOK:   return attacks<ROOK>(sq, occupied);
    default:     return attacks<QUEEN>(sq, occupied);
    }
}

static uint8_t pieceNibble(const Position &pos, Square sq, Square epSquare) {
    const Piece p = pos.getPieceAt(sq);
    const PieceType pt = pieceType(p);
    const Side s = side(p);
    const Side stm = pos.getSideToMove();
    const CastlingRight cr = pos.getCastlingRights();

    if (pt == PAWN && epSquare != SQ_NONE && fileOf(sq) == fileOf(epSquare)
        && rankOf(sq) == (stm == WHITE ? RANK_5 : RANK_4))
        return 12;

    if (pt == ROOK) {
        if (s == WHITE && ((sq == SQ_A1 && (cr & WHITE_QUEEN_SIDE)) || (sq == SQ_H1 && (cr & WHITE_KING_SIDE))))
            return 13;
        if (s == BLACK && ((sq == SQ_A8 && (cr & BLACK_QUEEN_SIDE)) || (sq == SQ_H8 && (cr & BLACK_KING_SIDE))))
            return 14;
    }

    if (pt == KING && s == BLACK && stm == BLACK)
        return 15;

    return uint8_t((pt - PAWN) * 2 + s);
}

static uint16_t compressMove(const Position &pos, Move m) {
    static constexpr uint16_t TypeIds[4] = { 0, 1, 3, 2 }; // normal, promotion, en passant, castling
    const Square from = moveFrom(m);
    Square to = moveTo(m);
    uint16_t promo = 0;

    if (moveType(m) == CASTLING) {
        const CastlingRight cr = pos.getSideToMove() & (fileOf(to) == FILE_G ? KING_SIDE : QUEEN_SIDE);
        to = Frc960::active ? castlingRookFrom<true>(cr) : castlingRookFrom<false>(cr);
    } else if (moveType(m) == PROMOTION) {
        promo = uint16_t(movePromotionType(m) - KNIGHT);
    }

    return uint16_t((TypeIds[moveType(m) >> 14] << 14) | (from << 8) | (to << 2) | promo);
}

BinpackWriter::BinpackWriter(const std::string &path) : file(path, std::ios::binary | std::ios::app) { }

BinpackWriter::~BinpackWriter() {
    flush();
}

void BinpackWriter::beginGame() {
    movetext.clear();
    bitsLeft = 0;
    nbPlies = 0;
    hasStem = false;
}

void BinpackWriter::addMove(const Position &pos, Move move, Score score) {
    if (hasStem)
        writeMoveScore(pos, move, score);
    else
        writeStem(pos, move, score);
}

void BinpackWriter::endGame(int result) {
    if (!hasStem)
        return;

    const uint16_t pr = uint16_t(stemPly | (signedToUnsigned(int16_t(stemSide == WHITE ? result : -result)) << 14));
    stem[28] = uint8_t(pr >> 8);
    stem[29] = uint8_t(pr);

    if (chunk.size() >= CHUNK_SIZE) {
        flush();
    }

    chunk.insert(chunk.end(), stem, stem + sizeof(stem));
    chunk.push_back(uint8_t(nbPlies >> 8));
    chunk.push_back(uint8_t(nbPlies));
    chunk.insert(chunk.end(), movetext.begin(), movetext.end());

    hasStem = false;
}

void BinpackWriter::flush() {
    if (!chunk.empty()) {
        const uint32_t size = uint32_t(chunk.size());
        const uint8_t header[8] = { 'B', 'I', 'N', 'P', uint8_t(size), uint8_t(size >> 8), uint8_t(size >> 16), uint8_t(size >> 24) };
        file.write(reinterpret_cast<const char *>(header), sizeof(header));
        file.write(reinterpret_cast<const char *>(chunk.data()), chunk.size());
        chunk.clear();
    }

    file.flush();
}

void BinpackWriter::addBits(uint8_t bits, int count) {
    if (count == 0) return;

    if (bitsLeft == 0) {
        movetext.push_back(uint8_t(bits << (8 - count)));
        bitsLeft = 8;
    } else if (count <= bitsLeft) {
        movetext.back() |= uint8_t(bits << (bitsLeft - count));
    } else {
        const int spill = count - bitsLeft;
        movetext.back() |= uint8_t(bits >> spill);
        movetext.push_back(uint8_t(bits << (8 - spill)));
        bitsLeft += 8;
    }

    bitsLeft -= count;
}

void BinpackWriter::addVle16(uint16_t value, int blockSize) {
    const uint16_t mask = uint16_t((1 << blockSize) - 1);

    do {
        addBits(uint8_t((value & mask) | ((value > mask) << blockSize)), blockSize + 1);
        value >>= blockSize;
    } while (value);
}

void BinpackWriter::writeStem(const Position &pos, Move move, Score score) {
    const Square epSquare = legalEpSquare(pos);
    const Bitboard occupied = pos.getPiecesBB();
    const int16_t s = clampScore(score);

    std::memset(stem, 0, sizeof(stem));

    for (int i = 0; i < 8; i++)
        stem[i] = uint8_t(occupied >> (56 - 8 * i));

    int i = 0;
    for (Bitboard b = occupied; b; b &= b - 1, i++)
        stem[8 + i / 2] |= uint8_t(pieceNibble(pos, bitscan(b), epSquare) << (4 * (i & 1)));

    const uint16_t m = compressMove(pos, move);
    stem[24] = uint8_t(m >> 8);
    stem[25] = uint8_t(m);

    const uint16_t us = signedToUnsigned(s);
    stem[26] = uint8_t(us >> 8);
    stem[27] = uint8_t(us);

    const uint8_t rule50 = uint8_t(pos.getFiftyMoveRule());
    stem[30] = 0;
    stem[31] = rule50;

    stemSide = pos.getSideToMove();
    stemPly = uint16_t(pos.getFullMoves() * 2 - (stemSide == WHITE));
    lastScore = int16_t(-s);
    hasStem = true;
}

void BinpackWriter::writeMoveScore(const Position &pos, Move move, Score score) {
    const Side stm = pos.getSideToMove();
    const Bitboard ours = pos.getPiecesBB(stm);
    const Bitboard theirs = pos.getPiecesBB(~stm);
    const Bitboard occupied = ours | theirs;
    const Square from = moveFrom(move), to = moveTo(move);
    const int16_t s = clampScore(score);

    int moveId = 0, nbMoves = 0;

    switch (pieceType(pos.getPieceAt(from))) {
    case PAWN: {
        const Square epSquare = legalEpSquare(pos);
        const Bitboard targets = theirs | (epSquare != SQ_NONE ? bb(epSquare) : 0);
        Bitboard destinations = pawnAttacks(stm, from) & targets;
        const Square forward = from + pawnDirection(stm);

        if (!(occupied & forward)) {
            destinations |= bb(forward);
            if (relativeRank(stm, from) == RANK_2 && !(occupied & (forward + pawnDirection(stm))))
                destinations |= bb(forward + pawnDirection(stm));
        }

        moveId = popcount(destinations & before(to));
        nbMoves = popcount(destinations);

        if (relativeRank(stm, from) == RANK_7) {
            moveId = moveId * 4 + (movePromotionType(move) - KNIGHT);
            nbMoves *= 4;
        }
        break;
    }
    case KING: {
        const CastlingRight cr = pos.getCastlingRights();
        const Bitboard destinations = attacks<KING>(from) & ~ours;
        const int nbAttacks = popcount(destinations);

        nbMoves = nbAttacks + popcount(Bitboard(cr & (stm == WHITE ? WHITE_CASTLING : BLACK_CASTLING)));

        if (moveType(move) == CASTLING)
            moveId = nbAttacks - 1 + !!(cr & (stm & QUEEN_SIDE)) + (fileOf(to) == FILE_G);
        else
            moveId = popcount(destinations & before(to));
        break;
    }
    default: {
        const Bitboard destinations = pieceAttacks(pieceType(pos.getPieceAt(from)), from, occupied) & ~ours;
        moveId = popcount(destinations & before(to));
        nbMoves = popcount(destinations);
    }
    }

    addBits(uint8_t(popcount(ours & before(from))), usedBitsSafe(popcount(ours)));
    addBits(uint8_t(moveId), usedBitsSafe(nbMoves));
    addVle16(signedToUnsigned(int16_t(s - lastScore)), SCORE_VLE_BLOCK_SIZE);

    lastScore = int16_t(-s);
    nbPlies++;
}

} /* namespace Belette */
