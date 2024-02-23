#ifndef UCI_OPTION_H_INCLUDED
#define UCI_OPTION_H_INCLUDED

#include <string>
#include <map>
#include <set>
#include <cassert>
#include <sstream>
#include "utils.h"

namespace BabChess {

class UciOption {
    typedef void (*OnUpdate)(const UciOption&);
    typedef std::set<std::string> OptionValues;

    std::string type; // button, check, string, spin, combo
    std::string defaultValue;
    std::string value;
    int min;
    int max;
    std::set<std::string> allowedValues;
    OnUpdate onUpdate;

public:
    UciOption(OnUpdate = nullptr); // button
    explicit UciOption(bool _defaultValue, OnUpdate = nullptr); // check
    explicit UciOption(std::string _defaultValue, OnUpdate = nullptr); // string
    explicit UciOption(int _defaultValue, int _min, int _max, OnUpdate = nullptr); // spin
    explicit UciOption(std::string _defaultValue, const OptionValues &_allowedValues, OnUpdate = nullptr); // combo

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
};

} /* namespace BabChess */

#endif /* UCI_OPTION_H_INCLUDED */