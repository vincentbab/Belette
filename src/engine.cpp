#include <iostream>
#include <thread>
#include <cmath>
#include "engine.h"
#include "movegen.h"
#include "evaluate.h"
#include "movepicker.h"

namespace Belette {

int Engine::LMRTable[MAX_PLY][MAX_MOVE];

void Engine::init() {
    for (int d=1; d<MAX_PLY; d++) {
        for (int m=1; m<MAX_MOVE; m++) {
            LMRTable[d][m] = int(0.25 + 0.46 * std::log(d) * std::log(m));
        }
    }
}

void updatePv(MoveList &pv, Move move, const MoveList &childPv) {
    pv.clear();
    pv.push_back(move);
    pv.insert(childPv.begin(), childPv.end());
}

void SearchData::initAllocatedTime() {
    int64_t moves = limits.movesToGo > 0 ? limits.movesToGo + 5 : 30;
    Side stm = position.getSideToMove();

    hardTimeLimit = 0.49 * limits.timeLeft[stm];
    softTimeLimit = std::min<TimeMs>(hardTimeLimit, limits.timeLeft[stm] / moves + 0.9 * limits.increment[stm]);
}

Engine::~Engine() {
    stop();
    waitForSearchFinish();
}

void Engine::waitForSearchFinish() {
    if (searchThread.joinable()) {
        searchThread.join();
    }
}

// Search entry point
void Engine::search(const SearchLimits &limits) {
    waitForSearchFinish();

    moveHistory.clearAllKillers();

    sd = std::make_unique<SearchData>(position(), limits, moveHistory);
    aborted = false;
    searching = true;
    
    tt.newSearch();

    searchThread = std::thread([this] {
        this->idSearch();
    });
}

void Engine::stop() {
    aborted = true;
}

// Iterative deepening loop
template<Side Me>
void Engine::idSearch() {
    MoveList bestPv;
    Score bestScore = SCORE_DRAW;
    int depth, searchDepth, completedDepth = 0;

    for (depth = 1; depth < MAX_PLY; depth++) {
        Score alpha = -SCORE_INFINITE, beta = SCORE_INFINITE;
        Score delta = 0, score = -SCORE_INFINITE;

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
            if (alpha < -SCORE_MATE_MAX_PLY) alpha = -SCORE_INFINITE;
            if (beta > SCORE_MATE_MAX_PLY) beta = SCORE_INFINITE;
            //std::cout << "  depth=" << searchDepth << " d=" << delta << std::endl;
            score = pvSearch<Me, NodeType::Root>(alpha, beta, searchDepth, 0, false);

            if (searchAborted()) break;

            if (score <= alpha) { // Fail low
                //std::cout << "  Fail Low: a=" << alpha << " b=" << beta << " score=" << score << std::endl;
                beta = (alpha + beta) / 2;
                alpha = std::max(score - delta, -SCORE_INFINITE);
                searchDepth = depth;
            } else if (score >= beta) { // Fail high
                //std::cout << "  Fail High: a=" << alpha << " b=" << beta << " score=" << score << std::endl;
                beta = std::min(score + delta, SCORE_INFINITE);
                //searchDepth = std::max(std::max(1, depth - 4), searchDepth - 1);
                searchDepth = std::max(1, searchDepth - (std::abs(score) < 1000));
            } else {
                break;
            }

            delta += delta / 2;
        }

        if (searchAborted() && (depth > 1 || score == -SCORE_INFINITE)) break;

        bestPv = sd->node(0).pv;
        bestScore = score;
        completedDepth = depth;

        onSearchProgress(SearchEvent(depth, sd->selDepth, bestPv, bestScore, sd->nbNodes, sd->getElapsed(), tt.usage()));

        if (sd->limits.maxDepth > 0 && depth >= sd->limits.maxDepth) break;

        if (sd->shouldStopSoft()) break;
    }

    SearchEvent event(depth, sd->selDepth, bestPv, bestScore, sd->nbNodes, sd->getElapsed(), tt.usage());

    if (depth != completedDepth) {
        event.depth = completedDepth;
        onSearchProgress(event);
    }

    onSearchFinish(event);

    searching = false;
}

// Negamax search
template<Side Me, NodeType NT>
Score Engine::pvSearch(Score alpha, Score beta, int depth, int ply, bool cutNode) {
    constexpr bool PvNode = (NT != NodeType::NonPV);
    constexpr bool RootNode = (NT == NodeType::Root);
    constexpr NodeType QNodeType = PvNode ? NodeType::PV : NodeType::NonPV;

    // Quiescence
    if (depth <= 0) {
        return qSearch<Me, QNodeType>(alpha, beta, depth, ply);
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

    Position &pos = sd->position;

    if (!RootNode && pos.isDraw(ply)) {
        return scoreDraw(sd->nbNodes);
    }

    if (!RootNode && alpha < SCORE_DRAW && pos.hasUpcomingRepetition(ply)) {
        alpha = scoreDraw(sd->nbNodes);
        if (alpha >= beta)
            return alpha;
    }

    Node& node = sd->node(ply);
    Score bestScore = -SCORE_INFINITE;
    Move bestMove = MOVE_NONE;
    bool inCheck = pos.inCheck();
    Score eval;
    bool improving = false;

    // continuation history
    PieceToHistory* contHist[CONT_HIST_PLIES] = {
        sd->node(ply-1).contHist,
        sd->node(ply-2).contHist
    };

    if (RootNode) {
        node.pv.clear();
    }

    if (ply >= MAX_PLY) [[unlikely]] {
        return evaluate<Me>(pos); // TODO: verify if we are in check ?
    }

    // Query Transposition Table
    auto&&[ttHit, tte] = tt.get(pos.hash());
    Score ttScore = tte->score(ply);
    bool ttPv = PvNode || (ttHit && tte->isPv());
    Move ttMove = ttHit ? tte->move() : MOVE_NONE;
    bool ttTactical = ttHit ? pos.isTactical(ttMove) : false;

    // Transposition Table cutoff
    if (!PvNode && ttHit && tte->depth() >= depth && tte->canCutoff(ttScore, beta)) {
        return ttScore;
    }

    // Static eval
    Score rawEval = SCORE_NONE;
    if (!inCheck) {
        if (ttHit) {
            rawEval = (tte->eval() != SCORE_NONE ? tte->eval() : evaluate<Me>(pos));
        } else {
            rawEval = evaluate<Me>(pos);
            tt.set(tte, pos.hash(), 0, ply, BOUND_NONE, MOVE_NONE, rawEval, SCORE_NONE, ttPv);
        }

        node.staticEval = eval = sd->moveHistory.correctEval<Me>(pos, rawEval);

        // Use score instead of eval if available.
        if (ttHit && tte->canCutoff(ttScore, eval)) {
            eval = tte->score(ply);
        }

        // Improving
        if (ply >= 2 && sd->node(ply - 2).staticEval != SCORE_NONE)
            improving = (node.staticEval > sd->node(ply - 2).staticEval);
        else if (ply >= 4 && sd->node(ply - 4).staticEval != SCORE_NONE)
            improving = (node.staticEval > sd->node(ply - 4).staticEval);
    } else {
        node.staticEval = eval = SCORE_NONE;
    }

    // Internal Iterative Reduction (IIR)
    if (depth >= 4 && ttMove == MOVE_NONE) {
        depth--;
    }

    // Reverse futility pruning (RFP)
    if (!PvNode && !inCheck && depth <= 8
        && eval - ((improving ? 60 : 120) * depth) >= beta)
    {
        return eval;
    }

    // Razoring
    if (!PvNode && !inCheck && depth <= 2
        && eval + (400 * depth) <= alpha)
    {
        Score score = qSearch<Me, QNodeType>(alpha, beta, depth, ply);
        if (score <= alpha)
            return score;
    }

    // Null move pruning (NMP)
    if (!PvNode && !inCheck
        && pos.previousMove() != MOVE_NULL && pos.hasNonPawnMateriel<Me>() && eval >= beta)
    {
        tt.prefetch(pos.getHashAfterNullMove());
        int R = 4 + depth / 4;

        node.contHist = sd->moveHistory.getDefaultContHist();
        pos.doNullMove<Me>();
        Score score = -pvSearch<~Me, NodeType::NonPV>(-beta, -beta+1, depth-R, ply+1, !cutNode);
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

    sd->moveHistory.clearKillers(ply+1);

    int nbMoves = 0;
    MovePicker mp(pos, ttMove, &sd->moveHistory, ply, contHist);
    //MovePicker *mp = new (&node.mp) MovePicker(pos, ttMove, &sd->moveHistory, ply);
    PartialMoveList quietMoves, captureMoves;

    mp.enumerate<MAIN, Me>([&](Move move, bool& skipQuiets) -> bool {
        // Honor UCI searchmoves
        if (RootNode && sd->limits.searchMoves.size() > 0 && !sd->limits.searchMoves.contains(move))
            return true; // continue

        nbMoves++;

        bool moveIsTactical = pos.isTactical(move);

        // Combined quiet history, must be computed before the move is played
        MoveScore statScore = moveIsTactical ? 0 : sd->moveHistory.getHistory<Me>(pos, move, contHist);

        // Late move pruning
        if (!RootNode && bestScore > -SCORE_MATE_MAX_PLY) {
            // Move count pruning
            skipQuiets = (nbMoves >= 3 + depth*depth/(improving ? 1 : 2));

            // Futility pruning
            Score futilityValue = eval + 100 + 120*depth;
            if (!inCheck && !moveIsTactical && depth <= 6 && futilityValue <= alpha) {
                skipQuiets = true;
                //if (bestScore < futilityValue && futilityValue < SCORE_MATE_MAX_PLY)
                //    bestScore = futilityValue;
                return true; // continue;
            }

            // History pruning
            if (!inCheck && !moveIsTactical && depth <= 4 && statScore < -4096 * depth) {
                return true; // continue;
            }

            // SEE Pruning
            if (depth <= 8 && !pos.see(move, moveIsTactical ? -100*depth : -60*depth)) {
                return true; // continue;
            }
        }

        sd->nbNodes++;

        if (PvNode)
            sd->node(ply+1).pv.clear();

        // Continuation history
        node.contHist = sd->moveHistory.getContHistEntry(pos, move);

        // Do move
        pos.doMove<Me>(move);

        Score score;

        // Late move reduction (LMR)
        if (depth >= 2 && nbMoves > 1) {
            int R = LMRTable[depth][nbMoves];

            R -= PvNode;
            R -= pos.inCheck();
            R += !ttPv;
            R += ttTactical;
            R += 2*cutNode;
            R += !improving;
            R -= statScore / 4096;

            R = std::min(depth - 1, std::max(1, R));

            // Reduced depth, Zero window
            score = -pvSearch<~Me, NodeType::NonPV>(-alpha-1, -alpha, depth-R, ply+1, true);

            if (score > alpha && R != 1) {
                // Full depth, Zero window
                score = -pvSearch<~Me, NodeType::NonPV>(-alpha-1, -alpha, depth-1, ply+1, !cutNode);
            }

        } else if (!PvNode || nbMoves > 1) {
            // Zero window (PVS)
            score = -pvSearch<~Me, NodeType::NonPV>(-alpha-1, -alpha, depth-1, ply+1, !cutNode);
        }

        if (PvNode && (nbMoves == 1 || (score > alpha && (RootNode || score < beta)))) {
            // Full window (PVS)
            score = -pvSearch<~Me, NodeType::PV>(-beta, -alpha, depth-1, ply+1, false);
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
                    updatePv(node.pv, move, sd->node(ply+1).pv);

                if (alpha >= beta) {
                    sd->moveHistory.update<Me>(pos, bestMove, ply, depth, quietMoves, captureMoves, contHist);
                    return false; // break
                }
            }
        }

        if (move != bestMove) {
            if (moveIsTactical) {
                if (captureMoves.size() < captureMoves.capacity())
                    captureMoves.push_back(move);
            } else if (quietMoves.size() < quietMoves.capacity()) {
                quietMoves.push_back(move);
            }
        }

        return true;
    }); if (searchAborted()) return bestScore;

    // Checkmate / Stalemate detection
    if (nbMoves == 0) {
        return inCheck ? -SCORE_MATE + ply : SCORE_DRAW;
    }

    // Update correction history
    if (!inCheck && !(bestMove != MOVE_NONE && pos.isTactical(bestMove)) && std::abs(bestScore) < SCORE_MATE_MAX_PLY
        && ((bestScore < node.staticEval && bestScore < beta) || (bestScore > node.staticEval && bestMove != MOVE_NONE)))
    {
        sd->moveHistory.updateCorrection<Me>(pos, bestScore, node.staticEval, depth);
    }

    // Update Transposition Table
    Bound ttBound = bestScore >= beta                ? BOUND_LOWER :
                    PvNode && bestMove != MOVE_NONE  ? BOUND_EXACT : BOUND_UPPER;
    tt.set(tte, pos.hash(), depth, ply, ttBound, bestMove, rawEval, bestScore, ttPv);

    return bestScore;
}

// Quiescence search
template<Side Me, NodeType NT>
Score Engine::qSearch(Score alpha, Score beta, int depth, int ply) {
    constexpr bool PvNode = (NT != NodeType::NonPV);

    // Check if we should stop according to limits
    if (sd->shouldStop()) [[unlikely]] {
        stop();
    }

    // If search has been aborted (either by the gui or by limits) exit here
    if (searchAborted()) [[unlikely]] {
        return -SCORE_INFINITE;
    }

    Position &pos = sd->position;

    if (pos.isDraw(ply)) {
        return scoreDraw(sd->nbNodes);
    }

    if (alpha < SCORE_DRAW && pos.hasUpcomingRepetition(ply)) {
        alpha = scoreDraw(sd->nbNodes);
        if (alpha >= beta)
            return alpha;
    }

    // bestScore stays at -SCORE_INFINITE until a move is actually searched. That's what makes the mate detection trustworthy.
    Score bestScore = -SCORE_INFINITE;
    Move bestMove = MOVE_NONE;
    //Node& node = sd->node(ply);

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
    Score rawEval = SCORE_NONE;
    if (!inCheck) {
        if (ttHit) {
            rawEval = (tte->eval() != SCORE_NONE ? tte->eval() : evaluate<Me>(pos));
        } else {
            rawEval = evaluate<Me>(pos);
            tt.set(tte, pos.hash(), ttDepth, ply, BOUND_NONE, MOVE_NONE, rawEval, SCORE_NONE, ttPv);
        }

        eval = sd->moveHistory.correctEval<Me>(pos, rawEval);

        // Use score instead of eval if available.
        if (ttHit && tte->canCutoff(ttScore, eval)) {
            eval = tte->score(ply);
        }

        if (eval >= beta) {
            return eval;
        }

        if (eval > alpha)
            alpha = eval;

        bestScore = eval;
    }

    Move ttMove = tte->move();
    // If ttMove is quiet we don't want to use it past a certain depth to allow qSearch to stabilize
    bool useTTMove = ttHit && isValidMove(ttMove) && (depth >= -7 || pos.inCheck() || pos.isTactical(ttMove));
    MovePicker mp(pos, useTTMove ? ttMove : MOVE_NONE);
    //MovePicker *mp = new (&node.mp) MovePicker(pos, useTTMove ? ttMove : MOVE_NONE);

    mp.enumerate<QUIESCENCE, Me>([&](Move move, /*unused*/bool& skipQuiets) -> bool {
        // SEE Pruning. Gated on bestScore no longer being a loss so that the first evasion
        // is always searched: otherwise every evasion could be pruned and the mate detection
        // below would report a mate that does not exist.
        if (bestScore > -SCORE_MATE_MAX_PLY && !pos.see(move, 0)) return true; // continue;
        
        sd->nbNodes++;

        pos.doMove<Me>(move);
        Score score = -qSearch<~Me, NT>(-beta, -alpha, depth-1, ply+1);
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

    // Checkmate detection. Only reachable when no evasion was searched at all, ie there is
    // no legal move: the pruning above cannot produce this state.
    if (inCheck && bestScore == -SCORE_INFINITE) {
        bestScore = -SCORE_MATE + ply;
    }

    // Update Transposition Table
    Bound ttBound = bestScore >= beta ? BOUND_LOWER : BOUND_UPPER;
    tt.set(tte, pos.hash(), ttDepth, ply, ttBound, bestMove, rawEval, bestScore, ttPv);

    return bestScore;
}

} /* namespace Belette */