#ifndef UCI_OPTION_H_INCLUDED
#define UCI_OPTION_H_INCLUDED

#include <string>
#include <map>
#include <set>
#include <cassert>
#include <sstream>
#include <functional>
#include "utils.h"

namespace BabChess {

class UciOption {
public:
    using OnUpdate = std::function<void(const UciOption&)>;
    using OptionValues = std::set<std::string>;

    UciOption(OnUpdate = nullptr); // button
    explicit UciOption(bool defaultValue_, OnUpdate = nullptr); // check
    explicit UciOption(const std::string &defaultValue_, OnUpdate = nullptr); // string
    explicit UciOption(const char *defaultValue_, OnUpdate = nullptr); // string
    explicit UciOption(int defaultValue_, int min_, int max_, OnUpdate = nullptr); // spin
    explicit UciOption(const std::string &defaultValue_, const OptionValues &allowedValues_, OnUpdate = nullptr); // combo

    inline bool isButton() const { return type == "button"; }
    inline bool isCheck() const { return type == "check"; }
    inline bool isString() const { return type == "string"; }
    inline bool isSpin() const { return type == "spin"; }
    inline bool isCombo() const { return type == "combo"; }

    inline operator int() const {
        assert(type == "spin");
        return parseInt(value);
    }
    inline operator std::string() const {
        assert(type != "button");
        return value;
    }
    inline operator bool() const {
        if (isCheck()) return value == "true";
        if (isSpin()) return parseInt(value) != 0;
        
        return !value.empty();
    }

    UciOption& operator=(const std::string& v);
    friend std::ostream &operator<<(std::ostream &, UciOption const &);

private:
    std::string type; // button, check, string, spin, combo
    std::string defaultValue;
    std::string value;
    int min;
    int max;
    std::set<std::string> allowedValues;
    OnUpdate onUpdate;
};

} /* namespace BabChess */

#endif /* UCI_OPTION_H_INCLUDED */