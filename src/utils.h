#ifndef TIMEMANAGER_H_INCLUDED
#define TIMEMANAGER_H_INCLUDED

#include <chrono>
#include <string>

namespace BabChess {

using TimeMs = std::chrono::milliseconds::rep;

inline TimeMs now() {
    return std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline int parseInt(const std::string &str) {
    try {
        return std::stoi(str);
    } catch (const std::invalid_argument & e) {
        return 0;
    } catch (const std::out_of_range & e) {
        return 0;
    }

    return 0;
}

} /* namespace BabChess */

#endif /* TIMEMANAGER_H_INCLUDED */
