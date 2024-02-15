#include <iostream>
#include <cassert>
#include <algorithm>
#include <chrono>
#include "uci.h"
#include "move.h"
#include "test.h"
#include "perft.h"

using namespace std;

namespace BabChess {

Uci::Uci()  {
    options["Debug"] = UciOption(false);

    commands["uci"] = &Uci::cmdUci;
    commands["isready"] = &Uci::cmdIsReady;
    commands["ucinewgame"] = &Uci::cmdUciNewGame;
    commands["setoption"] = &Uci::cmdSetOption;
    commands["position"] = &Uci::cmdPosition;
    commands["go"] = &Uci::cmdGo;
    commands["stop"] = &Uci::cmdStop;
    commands["quit"] = &Uci::cmdQuit;

    commands["debug"] = &Uci::cmdDebug;
    commands["d"] = &Uci::cmdDebug;
    commands["perft"] = &Uci::cmdPerft;
    commands["test"] = &Uci::cmdTest;
}

Square Uci::parseSquare(std::string str) {
    if (str.length() < 2) return SQ_NONE;

    char col = str[0], row = str[1];
    if ( col < 'a' || col > 'h') return SQ_NONE;
    if ( row < '1' || row > '8') return SQ_NONE;

    return square(File(col - 'a'), Rank(row - '1'));
}

std::string Uci::formatSquare(Square sq) {
    std::string str;
    str += 'a'+fileOf(sq);
    str += '1'+rankOf(sq);

    return str;
}

std::string Uci::formatMove(Move m) {
    std::string str = formatSquare(moveFrom(m)) + formatSquare(moveTo(m));

    if (moveType(m) == PROMOTION) {
        str += " pnbrqk"[movePromotionType(m)];
    }

    return str;
}

Move Uci::parseMove(std::string str) const {
    if (str.length() == 5) str[4] = char(tolower(str[4]));

    Move move;
    enumerateLegalMoves(engine.position(), [&](Move m) {
        if (str == formatMove(m)) {
            move = m;
            return false;
        }

        return true;
    });

    return move;
}

void Uci::loop() {
    string line, token;

    while(getline(cin, line)) {
        istringstream parser(line);

        if (line.empty()) continue;

        token.clear();
        parser >> skipws >> token;

        bool unknowCommand = true, exit = false;
        for (auto const& [cmd, handler] : commands) {
            if (cmd != token) continue;

            unknowCommand = false;

            if (!(this->*handler)(parser)) {
                exit = true;
            }

            break;
        }

        if (unknowCommand) {
            cout << "Unknow command '" << token << "'" << endl;
        }

        if (exit) {
            break;
        }
    }

    // cleanup
    cout << "Exiting UCI loop" << endl;
}

bool Uci::cmdUci(istringstream &is) {
    cout << "id name BabChess engine v0.1" << endl;
    cout << "id author Vincent Bab" << endl;

    cout << endl;

    for (auto const& [name, option] : options) {
        cout << "option name " << name << " " << option << endl;
    }

    cout << "uciok" << endl;

    return true;
}

bool Uci::cmdIsReady(istringstream& is) {
    cout << "readyok" << endl;

    return true;
}

bool Uci::cmdUciNewGame(istringstream& is) {
    return true;
}

bool Uci::cmdSetOption(istringstream& is) {
    string token, name, value;

    // name
    is >> token;
    while (is >> token && token != "value") {
        name += (name.empty() ? "" : " ") + token;
    }

    while (is >> token) {
        value += (value.empty() ? "" : " ") + token;
    }

    if (!options.count(name))
        cout << "Unknow option '" << name << "'" << endl;
        
    options[name] = value;

    return true;
}

bool Uci::cmdPosition(istringstream& is) {
    string token, fen;

    is >> token;

    if (token == "startpos") {
        fen = STARTPOS_FEN;
    } else if (token == "fen") {
        while (is >> token && token != "moves") {
            fen += token + " ";
        }
    } else {
        return true;
    }

    engine.position().setFromFEN(fen);

    while (is >> token) {
        Move m = parseMove(token);
        
        if (m == MOVE_NONE) continue;

        engine.position().doMove(m);
    }

    return true;
}

bool Uci::cmdGo(istringstream& is) {
    string token;
    ThinkParams params;

    while (is >> token) {
        if (token == "perft") {
            return cmdPerft(is);
        } else if (token == "searchmoves") {
            // unsupported
        } else if (token == "ponder") {
            // unsupported
        } else if (token == "wtime") {
            is >> token;
            params.timeLeft[WHITE] = stoi(token);
        } else if (token == "btime") {
            is >> token;
            params.timeLeft[BLACK] = stoi(token);
        } else if (token == "winc") {
            is >> token;
            params.increment[WHITE] = stoi(token);
        } else if (token == "binc") {
            is >> token;
            params.increment[BLACK] = stoi(token);
        } else if (token == "movestogo") {
            is >> token;
            params.movesToGo = stoi(token);
        } else if (token == "depth") {
            is >> token;
            params.maxDepth = stoi(token);
        } else if (token == "nodes") {
            is >> token;
            params.maxNodes = stoi(token);
        } else if (token == "mate") {
            // unsupported
        } else if (token == "movetime") {
            is >> token;
            params.maxTime = stoi(token);
        } else if (token == "infinite") {
             
        }
    }

    engine.think(params);
    return true;
}

bool Uci::cmdDebug(istringstream& is) {
    cout << engine.position() << endl;
    return true;
}

bool Uci::cmdPerft(istringstream& is) {
    int depth;
    is >> depth;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    size_t n = perft<true>(engine.position(), depth);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto elapsed = end - begin;
    std::cout << endl << "Nodes: " << n << endl;
	std::cout << "NPS: "
		<< int(n * 1000000.0 / std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()) << endl;
	std::cout << "Time: "
		<< std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() << " us" << endl;

    return true;
}

bool Uci::cmdStop(istringstream& is) {
    //Engine.stop();
    return true;
}

bool Uci::cmdQuit(istringstream& is) {
    return false;
}

bool Uci::cmdTest(istringstream& is) {
    Test::run();
    
    return true;
}

void UciEngine::onThinkProgress() {
    cout << "onThinkProgress()" << endl;
}

void UciEngine::onThinkFinish() {
    cout << "onThinkFinish()" << endl;
}

} /* namespace BabChess */
