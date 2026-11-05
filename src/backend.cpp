#include "imp/backend.h"

#include <algorithm>
#include <cctype>


namespace imp
{
x_error Backend::load_config(const std::string& path)
{
  if (path.empty()) {
    return x_ok();
  }
  return x_error("posix", ENOTSUP, "Input-source configuration is not supported by this backend");
}

x_error Backend::configure_pair(const std::string&)
{
  return x_error("posix", ENOTSUP, "Input-source pair selection is not supported by this backend");
}

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
