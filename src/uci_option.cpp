#include "uci_option.h"

using namespace std;

namespace BabChess {

UciOption::UciOption(OnUpdate _onUpdate) : type("button"), onUpdate(_onUpdate) { }

UciOption::UciOption(bool _defaultValue, OnUpdate _onUpdate) : type("check"), onUpdate(_onUpdate)
{ 
    defaultValue = value = (_defaultValue ? "true" : "false");
}

UciOption::UciOption(std::string _defaultValue, OnUpdate _onUpdate) : type("string"), defaultValue(_defaultValue), value(_defaultValue), onUpdate(_onUpdate) { }

UciOption::UciOption(int _defaultValue, int _min, int _max, OnUpdate _onUpdate) : type("spin"), min(_min), max(_max), onUpdate(_onUpdate)
{ 
    defaultValue = value = _defaultValue;
}

UciOption::UciOption(std::string _defaultValue, const OptionValues &_allowedValues, OnUpdate _onUpdate) : defaultValue(_defaultValue), value(_defaultValue), allowedValues(_allowedValues), onUpdate(_onUpdate)
{
    assert(allowedValues.count(value) == 1);
}

std::ostream &operator<<(std::ostream &s, const UciOption &option) {
    if (option.isButton()) {
        s << "type button";
    } else if (option.isCheck()) {
        s << "type check default " << "false";
    } else if (option.isString()) {
        s << "type string default " << option.defaultValue;
    } else if (option.isSpin()) {
        s << "type spin default " << option.defaultValue << " min " << option.min << " max " << option.max;
    } else if (option.isCombo()) {   
        s << "type combo default " << option.defaultValue;
        for(auto const &v : option.allowedValues) {
            s << " var " << v;
        }
    }

    return s;
}

UciOption& UciOption::operator=(const string& newValue)
{
    if (!isButton() && newValue.empty()) return *this;
    if (isCheck() && newValue != "true" && newValue != "false") return *this;
    if (isSpin() && stoi(newValue) < min && stoi(newValue) > max) return *this;
    if (isCombo() && this->allowedValues.count(newValue) != 1) return *this;

    if (!isButton())
        value = newValue;

    if (onUpdate != nullptr)
        onUpdate(*this);

    return *this;
}

} /* namespace BabChess */