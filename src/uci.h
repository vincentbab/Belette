#ifndef UCI_H_INCLUDED
#define UCI_H_INCLUDED

#include <map>
#include <string>
#include <sstream>
#include <istream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include "uci_option.h"
#include "engine.h"

#define VERSION "2.0.0"

namespace Belette {

typedef std::ostream& (*Manipulator) (std::ostream&);

// Used to log stdin & stdout to a file for debugging
class Console {
public:
    Console() = default;
    Console(const Console &) = delete;
    ~Console();
    Console &operator=(const Console &) = delete;

    void setLogFile(const std::string &filename);
    std::istream& getline(std::string& str);

    template <class T> friend Console& operator<<(Console& console, const T& x);
    friend Console& operator<<(Console& console, Manipulator manip);
private:
    template <class T> Console &log(const T& x, bool isInput = false);

    std::stringstream buffer;
    std::ofstream *file;
};

extern Console console;

inline Console& operator<<(Console& console, Manipulator x) {
    std::cout << x;
    console.log(x);
    return console;
}

template <class T>
inline Console& operator<<(Console& console, const T& x) { 
    std::cout << x;
    console.log(x);
    return console;
}

template <class T> Console& Console::log(const T& x, bool isInput) {
    if (file != nullptr) {
        if (buffer.rdbuf()->in_avail() == 0) {
            auto now = time(nullptr);
            buffer << "[" << std::put_time(std::localtime(&now), "%F %T") << "] " << (isInput ? "<< " : ">> ");
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



class UciEngine : public Engine {
protected:
    virtual void onSearchProgress(const SearchEvent &event);
    virtual void onSearchFinish(const SearchEvent &event);
};

class Uci {
public:
    Uci(int argc, char* argv[]);
    ~Uci() = default;
    void loop();

    Move parseMove(std::string str) const;

    static Square parseSquare(std::string str);
    static std::string formatSquare(Square sq);
    static std::string formatMove(Move m);
    static std::string formatScore(Score s);
    
private:
    typedef bool (Uci::*UciCommandHandler)(std::istringstream& is);
    struct CaseInsensitiveComparator {
        bool operator() (const std::string&s1, const std::string&s2) const {
            return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(), [](char c1, char c2) { 
                return tolower(c1) < tolower(c2);
            });
        }
    };

    std::map<std::string, UciOption, CaseInsensitiveComparator> options;
    std::map<std::string, UciCommandHandler> commands;
    UciEngine engine;

    bool cmdUci(std::istringstream& is);
    bool cmdIsReady(std::istringstream& is);
    bool cmdUciNewGame(std::istringstream& is);
    bool cmdSetOption(std::istringstream& is);
    bool cmdPosition(std::istringstream& is);
    bool cmdGo(std::istringstream& is);
    bool cmdStop(std::istringstream& is);
    bool cmdQuit(std::istringstream& is);

    bool cmdDebug(std::istringstream& is);
    bool cmdEval(std::istringstream& is);
    bool cmdPerft(std::istringstream& is);
    bool cmdPerftmp(std::istringstream& is);
    bool cmdTest(std::istringstream& is);
    bool cmdBench(std::istringstream& is);
};

} /* namespace Belette */

#endif /* UCI_H_INCLUDED */
