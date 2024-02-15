#include <iostream>
#include "engine.h"
#include "move.h"

using namespace std;

namespace BabChess {

void Engine::think(const ThinkParams &params) {
    stopThinking = false;
    onThinkProgress();

    onThinkFinish();
}

void Engine::stop() {
    stopThinking = true;
}

void idSearch() {

}

void pvSearch() {

}

void qSearch() {
    
}

} /* namespace BabChess */