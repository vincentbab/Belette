#include <iostream>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <format>
#include "uci.h"
#include "move.h"
#include "test.h"
#include "perft.h"

using namespace std;

namespace BabChess {

static Console console;

Console::Console() {
    setLogFile("babchess.log");
}

Console::~Console() {
    if (file != nullptr) delete file;
}

template <class T> Console& Console::log(const T& x, bool isInput) {
    if (file != nullptr) {
        if (buffer.rdbuf()->in_avail() == 0) {
            auto now = std::chrono::system_clock::now();
            buffer << "[" << std::format("{0:%F_%T}", now) << "]:" << (isInput ? "<<" : ">>");
        }
        buffer << x;

        if (buffer.str().ends_with('\n')) {
            (*file) << buffer.str();
            file->flush();
            buffer.str(std::string()); // clear buffer
        }
        
    }

    return *this;
}

Console& operator<<(Console& console, Manipulator x){
    std::cout << x;
    console.log(x);
    return console;
}

template <class T>
Console& operator<<(Console& console, const T& x) { 
    std::cout << x;
    console.log(x);
    return console;
}
istream& Console::getline(string& x) {
    std::getline(std::cin, x);
    console.log(x, true).log('\n');
    return std::cin;
}

void Console::setLogFile(const std::string &filename) {
    if (file != nullptr) delete file;
    file = new std::ofstream(filename);
}



Uci::Uci()  {
    options["Debug Log File"] = UciOption(false);

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
    commands["eval"] = &Uci::cmdEval;
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

std::string Uci::formatScore(Score score) {
    stringstream ss;

    if (abs(score) >= (SCORE_MATE - MAX_PLY)) {
        ss << "mate " << (score > 0 ? SCORE_MATE - score + 1 : -SCORE_MATE - score) / 2;
    } else {
        ss << "cp " << score;
    }
    
    return ss.str();
}

std::string Uci::formatMove(Move m) {
    if (m == MOVE_NONE) {
        return "(none)";
    } else if (m == MOVE_NULL) {
        return "(null)";
    }
    
    std::string str = formatSquare(moveFrom(m)) + formatSquare(moveTo(m));
    
    if (moveType(m) == PROMOTION) {
        str += " pnbrqk"[movePromotionType(m)];
    }

    return str;
}

Move Uci::parseMove(std::string str) const {
    if (str.length() == 5) str[4] = char(tolower(str[4]));

    Move move;
    enumerateLegalMoves(engine.position(), [&](Move m, auto doMH, auto undoMH) {
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

    while(console.getline(line)) {
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
            console << "Unknow command '" << token << "'" << endl;
        }

        if (exit) {
            break;
        }
    }

    // cleanup
    console << "Exiting UCI loop" << endl;
}

bool Uci::cmdUci(istringstream &is) {
    console << "id name BabChess engine v0.1" << endl;
    console << "id author Vincent Bab" << endl;

    console << endl;

    for (auto const& [name, option] : options) {
        console << "option name " << name << " " << option << endl;
    }

    console << "uciok" << endl;

    return true;
}

bool Uci::cmdIsReady(istringstream& is) {
    console << "readyok" << endl;

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
        console << "Unknow option '" << name << "'" << endl;
        
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

    if (!engine.position().setFromFEN(fen)) {
        console << "Invalid FEN position" << endl;
    }

    while (is >> token) {
        Move m = parseMove(token);
        
        if (m == MOVE_NONE) continue;

        engine.position().doMove(m);
    }

    return true;
}

bool Uci::cmdGo(istringstream& is) {
    string token;
    SearchLimits params;

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

    engine.search(params);
    return true;
}

bool Uci::cmdDebug(istringstream& is) {
    console << engine.position() << endl;
    return true;
}

bool Uci::cmdEval(istringstream& is) {
    console << "Static eval: " << evaluate(engine.position()) << endl;
    return true;
}

bool Uci::cmdPerft(istringstream& is) {
    int depth;
    is >> depth;

    perft(engine.position(), depth);

    return true;
}

bool Uci::cmdStop(istringstream& is) {
    engine.stop();
    return true;
}

bool Uci::cmdQuit(istringstream& is) {
    return false;
}

bool Uci::cmdTest(istringstream& is) {
    Test::run();
    
    return true;
}

void UciEngine::onSearchProgress(const SearchEvent &event) {
    console << "info"
        << " depth " << event.depth 
        << " seldepth " << event.depth 
        << " multipv " << 1
        << " score " << Uci::formatScore(event.bestScore)
        << " nodes " << 0
        << " nps " << 0
        << " time " << 0
        << " hashfull " << 0
        << " tbhits " << 0;

    if (!event.pv.empty()) 
        console << " pv " << event.pv;
    
    console << endl;
}

void UciEngine::onSearchFinish(const SearchEvent &event) {
    Move bestMove = MOVE_NONE;
    if (!event.pv.empty()) bestMove = event.pv.front();

    console << "bestmove " << Uci::formatMove(bestMove) << endl;
}



} /* namespace BabChess */
