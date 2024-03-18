#include <iostream>
#include <thread>
#include "engine.h"
#include "move.h"
#include "evaluate.h"
#include "movepicker.h"

using namespace std;

namespace Belette {

void updatePv(MoveList &pv, Move move, const MoveList &childPv) {
    pv.clear();
    pv.push_back(move);
    pv.insert(childPv.begin(), childPv.end());
}

void SearchData::initAllocatedTime() {
    int64_t moves = limits.movesToGo > 0 ? limits.movesToGo : 40;
    Side stm = position.getSideToMove();

    allocatedTime = limits.timeLeft[stm] / moves + limits.increment[stm];
}

void Engine::waitForSearchFinish() {
    // TODO: use condition variable ?
    while (isSearching()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

// Search entry point
void Engine::search(const SearchLimits &limits) {
    if (searching) return;

    sd = std::make_unique<SearchData>(position(), limits);
    aborted = false;
    searching = true;
    
    tt.newSearch();

    std::thread th([&] { 
        this->idSearch();
    });
    th.detach();
}

void Engine::stop() {
    aborted = true;
}

// Iterative deepening loop
template<Side Me>
void Engine::idSearch() {
    MoveList bestPv;
    Score bestScore;
    int depth, searchDepth, completedDepth = 0;

    for (depth = 1; depth < MAX_PLY; depth++) {
        Score alpha = -SCORE_INFINITE, beta = SCORE_INFINITE;
        Score delta, score;
        MoveList pv;

        // Reset selDepth
        sd->selDepth = 0;

        searchDepth = depth;

        // Aspiration window
        if (depth > 4) {
            delta = 16 + std::abs(bestScore)/100;
            alpha = std::max(-SCORE_INFINITE, bestScore - delta);
            beta  = std::min( SCORE_INFINITE, bestScore + delta);
        }

        while (true) {
            //std::cout << "  depth=" << searchDepth << " d=" << delta << std::endl;
            score = pvSearch<Me, NodeType::Root>(alpha, beta, searchDepth, 0, pv);

            if (searchAborted()) break;

            if (score <= alpha) { // Fail low
                //std::cout << "  Fail Low: a=" << alpha << " b=" << beta << " score=" << score << std::endl;
                beta = (alpha + beta) / 2;
                alpha = std::max(score - delta, -SCORE_INFINITE);
                //searchDepth = depth;
            } else if (score >= beta) { // Fail high
                //std::cout << "  Fail High: a=" << alpha << " b=" << beta << " score=" << score << std::endl;
                beta = std::min(score + delta, SCORE_INFINITE);
                //searchDepth = std::max(std::max(1, depth - 4), searchDepth - 1);
            } else {
                break;
            }

            delta += delta / 2;
        }

        if (depth > 1 && searchAborted()) break;

        bestPv = pv;
        bestScore = score;
        completedDepth = depth;

        onSearchProgress(SearchEvent(depth, sd->selDepth, pv, bestScore, sd->nbNodes, sd->getElapsed(), tt.usage()));

        if (sd->limits.maxDepth > 0 && depth >= sd->limits.maxDepth) break;
    }

    SearchEvent event(depth, sd->selDepth, bestPv, bestScore, sd->nbNodes, sd->getElapsed(), tt.usage());
    if (depth != completedDepth)
        onSearchProgress(event);
    onSearchFinish(event);

    searching = false;
}

// Negamax search
template<Side Me, NodeType NT>
Score Engine::pvSearch(Score alpha, Score beta, int depth, int ply, MoveList &pv) {
    constexpr bool PvNode = (NT != NodeType::NonPV);
    constexpr bool RootNode = (NT == NodeType::Root);

    // Quiescence
    if (depth <= 0) {
        constexpr NodeType QNodeType = PvNode ? NodeType::PV : NodeType::NonPV;
        return qSearch<Me, QNodeType>(alpha, beta, depth, ply, pv);
    }

    // Update selDepth
    if (PvNode && sd->selDepth < ply + 1) {
        sd->selDepth = ply + 1;
    }

    // Check if we should stop according to limits
    if (!RootNode && sd->shouldStop()) [[unlikely]] {
        stop();
    }

    // If search has been aborted (either by the gui or by reaching limits) exit here
    if (!RootNode && searchAborted()) [[unlikely]] {
        return -SCORE_INFINITE;
    }

    // Mate distance pruning
    if (!RootNode) {
        alpha = std::max(alpha, -SCORE_MATE + ply);
        beta  = std::min(beta, SCORE_MATE - ply - 1);

        if (alpha >= beta) return alpha;
    }

    Score alphaOrig = alpha;
    Score bestScore = -SCORE_INFINITE;
    Move bestMove = MOVE_NONE;
    Position &pos = sd->position;
    bool inCheck = pos.inCheck();
    Score eval = SCORE_NONE;
    MoveList childPv;

    if (pos.isFiftyMoveDraw() || pos.isMaterialDraw() || pos.isRepetitionDraw()) {
        // "Random" either -1 or 1, avoid blindness to 3-fold repetitions
        return 1-(sd->nbNodes & 2);
        //return SCORE_DRAW;
    }

    if (ply >= MAX_PLY) [[unlikely]] {
        return evaluate<Me>(pos); // TODO: verify if we are in check ?
    }

    // Query Transposition Table
    auto&&[ttHit, tte] = tt.get(pos.hash());
    Score ttScore = tte->score(ply);
    bool ttPv = PvNode || (ttHit && tte->isPv());

    // Transposition Table cutoff
    if (!PvNode && ttHit && tte->depth() >= depth && tte->canCutoff(ttScore, beta)) {
        return ttScore;
    }

    // Static eval
    if (!inCheck) {
        if (ttHit) {
            eval = (tte->eval() != SCORE_NONE ? tte->eval() : evaluate<Me>(pos));

            // Use score instead of eval if available. 
            if (tte->canCutoff(ttScore, eval)) {
                eval = tte->score(ply);
            }
        } else {
            eval = evaluate<Me>(pos);
            tt.set(tte, pos.hash(), 0, ply, BOUND_NONE, MOVE_NONE, eval, SCORE_NONE, ttPv);
        }
    }

    // Reverse futility pruning
    if (!PvNode && !inCheck && depth <= 4
        && eval - (100 * depth) >= beta)
    {
        return eval;
    }

    // Null move pruning
    if (!PvNode && !inCheck
        && pos.previousMove() != MOVE_NULL && pos.hasNonPawnMateriel<Me>() && eval >= beta)
    {
        int R = 4 + depth / 4;

        pos.doNullMove<Me>();
        Score score = -pvSearch<~Me, NodeType::NonPV>(-beta, -beta+1, depth-R, ply+1, childPv);
        pos.undoNullMove<Me>();

        if (score >= beta) {
            // TODO: verification search ?
            return score >= SCORE_MATE_MAX_PLY ? beta : score;
        }
    }

    // Check extension
    if (PvNode && inCheck && depth <= 2) {
        depth++;
    }

    sd->clearKillers(ply+1);

    int nbMoves = 0;
    MovePicker<MAIN, Me> mp(pos, ttHit ? tte->move() : MOVE_NONE, sd->killerMoves[ply][0], sd->killerMoves[ply][1], sd->getCounter());
    
    mp.enumerate([&](Move move) -> bool {
        // Honor UCI searchmoves
        if (RootNode && sd->limits.searchMoves.size() > 0 && !sd->limits.searchMoves.contains(move))
            return true; // continue

        if (PvNode)
            childPv.clear();

        nbMoves++;
        sd->nbNodes++;

        // Do move
        pos.doMove<Me>(move);

        Score score;

        // PVS
        if (!PvNode || nbMoves > 1) {
            score = -pvSearch<~Me, NodeType::NonPV>(-alpha-1, -alpha, depth-1, ply+1, childPv);
        }

        if (PvNode && (nbMoves == 1 || (score > alpha && (RootNode || score < beta)))) {
            score = -pvSearch<~Me, NodeType::PV>(-beta, -alpha, depth-1, ply+1, childPv);
        }

        // Undo move
        pos.undoMove<Me>(move);

        if (searchAborted()) return false; // break

        if (score > bestScore) {
            bestScore = score;
            
            if (bestScore > alpha) {
                bestMove = move;
                alpha = bestScore;
                if (PvNode)
                    updatePv(pv, move, childPv);

                if (alpha >= beta) {
                    if (!pos.isTactical(bestMove)) {
                        sd->updateKillers(bestMove, ply);
                        sd->updateCounter(bestMove);
                    }
                    return false; // break
                }
            }
        }

        return true;
    }); if (searchAborted()) return bestScore;

    // Checkmate / Stalemate detection
    if (nbMoves == 0) {
        return inCheck ? -SCORE_MATE + ply : SCORE_DRAW;
    }

    // Update Transposition Table
    Bound ttBound =         bestScore >= beta         ? BOUND_LOWER : 
                    !PvNode || bestScore <= alphaOrig ? BOUND_UPPER : BOUND_EXACT;
    tt.set(tte, pos.hash(), depth, ply, ttBound, bestMove, SCORE_NONE, bestScore, ttPv);

    return bestScore;
}

// Quiescence search
template<Side Me, NodeType NT>
Score Engine::qSearch(Score alpha, Score beta, int depth, int ply, MoveList &pv) {
    constexpr bool PvNode = (NT != NodeType::NonPV);
    if (PvNode)
        pv.clear();

    // Check if we should stop according to limits
    if (sd->shouldStop()) [[unlikely]] {
        stop();
    }

    // If search has been aborted (either by the gui or by limits) exit here
    if (searchAborted()) [[unlikely]] {
        return -SCORE_INFINITE;
    }

    // Default bestScore for mate detection, if InCheck and there is no move this score will be returned
    Score bestScore = -SCORE_MATE + ply;
    Move bestMove = MOVE_NONE;
    Position &pos = sd->position;

    if (pos.isFiftyMoveDraw() || pos.isMaterialDraw() || pos.isRepetitionDraw()) {
        // "Random" either -1 or 1, avoid blindness to 3-fold repetitions
        return 1-(sd->nbNodes & 2);
        //return SCORE_DRAW;
    }

    if (ply >= MAX_PLY) [[unlikely]] {
        return evaluate<Me>(pos); // TODO: check if we are in check ?
    }

    bool inCheck = pos.inCheck();
    Score eval = SCORE_NONE;

    // Query Transposition Table
    auto&&[ttHit, tte] = tt.get(pos.hash());
    bool ttPv = PvNode || (ttHit && tte->isPv());
    int ttDepth = inCheck ? 1 : 0; // If we are in check use depth=1 because when we are in check we go through all moves
    Score ttScore = tte->score(ply);

    // Transposition Table cutoff
    if (!PvNode && ttHit && tte->depth() >= ttDepth && tte->canCutoff(ttScore, beta)) {
        return ttScore;
    }

    // Standing Pat
    if (!inCheck) {
        if (ttHit) {
            eval = (tte->eval() != SCORE_NONE ? tte->eval() : evaluate<Me>(pos));

            // Use score instead of eval if available. 
            if (tte->canCutoff(ttScore, beta)) {
                eval = tte->score(ply);
            }
        } else {
            eval = evaluate<Me>(pos);
            tt.set(tte, pos.hash(), ttDepth, ply, BOUND_NONE, MOVE_NONE, eval, SCORE_NONE, ttPv);
        }

        if (eval >= beta) {
            return eval;
        }

        if (eval > alpha)
            alpha = eval;

        bestScore = eval;
    }

    int nbMoves = 0;
    MoveList childPv;
    Move ttMove = tte->move();
    // If ttMove is quiet we don't want to use it past a certain depth to allow qSearch to stabilize
    bool useTTMove = ttHit && isValidMove(ttMove) && (depth >= -7 || pos.inCheck() || pos.isTactical(ttMove));
    MovePicker<QUIESCENCE, Me> mp(pos, useTTMove ? ttMove : MOVE_NONE);

    mp.enumerate([&](Move move) -> bool {
        nbMoves++;
        sd->nbNodes++;

        pos.doMove<Me>(move);
        Score score = -qSearch<~Me, NT>(-beta, -alpha, depth-1, ply+1, childPv);
        pos.undoMove<Me>(move);

        if (searchAborted()) return false; // break

        if (score > bestScore) {
            bestScore = score;
            
            if (bestScore > alpha) {
                bestMove = move;
                alpha = bestScore;

                if (alpha >= beta) {
                    return false; // break
                }
            }
        }

        return true;
    }); if (searchAborted()) return bestScore;

    // Update Transposition Table
    Bound ttBound = bestScore >= beta ? BOUND_LOWER : BOUND_UPPER;
    tt.set(tte, pos.hash(), ttDepth, ply, ttBound, bestMove, eval, bestScore, ttPv);

    return bestScore;
}

} /* namespace Belette */