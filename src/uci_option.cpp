#include "uci_option.h"

using namespace std;

namespace Belette {

UciOption::UciOption(OnUpdate onUpdate_) : type("button"), onUpdate(onUpdate_) { }

UciOption::UciOption(bool defaultValue_, OnUpdate onUpdate_) : type("check"), onUpdate(onUpdate_)
{ 
    defaultValue = value = (defaultValue_ ? "true" : "false");
}

UciOption::UciOption(const std::string &defaultValue_, OnUpdate onUpdate_) : type("string"), defaultValue(defaultValue_), value(defaultValue_), onUpdate(onUpdate_) { }
UciOption::UciOption(const char *defaultValue_, OnUpdate onUpdate_) : type("string"), defaultValue(defaultValue_), value(defaultValue_), onUpdate(onUpdate_) { }

UciOption::UciOption(int defaultValue_, int min_, int max_, OnUpdate onUpdate_) : type("spin"), min(min_), max(max_), onUpdate(onUpdate_)
{ 
    defaultValue = value = std::to_string(defaultValue_);
}

UciOption::UciOption(const std::string &defaultValue_, const OptionValues &allowedValues_, OnUpdate onUpdate_) : defaultValue(defaultValue_), value(defaultValue_), allowedValues(allowedValues_), onUpdate(onUpdate_)
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
    if (isSpin() && parseInt(newValue) < min && parseInt(newValue) > max) return *this;
    if (isCombo() && this->allowedValues.count(newValue) != 1) return *this;

    if (!isButton())
        value = newValue;

    if (onUpdate != nullptr)
        onUpdate(*this);

    return *this;
}

} /* namespace Belette */