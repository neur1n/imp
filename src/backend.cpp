#include "imp/backend.h"

#include <algorithm>
#include <cctype>


namespace imp
{
const char* mode_name(const Mode mode)
{
  switch (mode) {
    case Mode::Native:
      return "native";
    case Mode::Alphanumeric:
      return "alphanumeric";
    case Mode::Unknown:
      return "unknown";
  }

  return "unknown";
}

bool parse_mode(Mode& mode, const char* value)
{
  std::string normalized(value);

  std::transform(
      normalized.begin(), normalized.end(), normalized.begin(),
      [](char c) {
        return static_cast<char>(std::tolower(c));
      });

  if (normalized == "native") {
    mode = Mode::Native;
    return true;
  } else if (normalized == "alphanumeric" || normalized == "alpha") {
    mode = Mode::Alphanumeric;
    return true;
  }

  return false;
}
}  // namespace imp
