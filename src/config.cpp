#include "imp/config.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>


namespace imp
{
const InputSourcePair* find_pair(const std::vector<InputSourcePair>& pairs, const std::string_view name)
{
  for (const InputSourcePair& pair : pairs) {
    if (name == pair.name) {
      return &pair;
    }
  }
  return nullptr;
}

x_error parse_pairs(std::istream& input, std::vector<InputSourcePair>& pairs, const std::string& path)
{
  std::vector<InputSourcePair> parsed;
  std::string line;
  size_t line_number = 0;
  while (std::getline(input, line)) {
    ++line_number;
    std::istringstream record(line);
    record >> std::ws;
    if (record.eof() || record.peek() == '#') {
      continue;
    }

    InputSourcePair pair;
    std::string extra;
    if (!(record >> pair.name >> pair.alphanumeric >> pair.native) || record >> extra) {
      const std::string message = path + ':' + std::to_string(line_number)
        + ": expected three columns: name alphanumeric_source_id native_source_id";
      return x_error("posix", EINVAL, message.c_str());
    }
    if (find_pair(parsed, pair.name) != nullptr) {
      const std::string message = path + ':' + std::to_string(line_number)
        + ": duplicate pair name: " + pair.name;
      return x_error("posix", EINVAL, message.c_str());
    }
    parsed.push_back(std::move(pair));
  }
  if (input.bad() || (input.fail() && !input.eof())) {
    const std::string message = path + ": could not read configuration";
    return x_error("posix", EIO, message.c_str());
  }
  if (parsed.empty()) {
    const std::string message = path + ": configuration must contain at least one pair";
    return x_error("posix", EINVAL, message.c_str());
  }
  pairs = std::move(parsed);
  return x_ok();
}

x_error load_pairs(const std::string& path, std::vector<InputSourcePair>& pairs)
{
  std::error_code error;
  const auto status = std::filesystem::status(path, error);
  if (error) {
    const std::string message = path + ": could not open configuration";
    return x_error("posix", error.value(), message.c_str());
  }
  if (!std::filesystem::is_regular_file(status)) {
    const std::string message = path + ": configuration path is not a regular file";
    return x_error("posix", EINVAL, message.c_str());
  }
  errno = 0;
  std::ifstream input(path);
  if (!input.is_open()) {
    const int code = errno != 0 ? errno : EIO;
    const std::string message = path + ": could not open configuration";
    return x_error("posix", code, message.c_str());
  }
  return parse_pairs(input, pairs, path);
}
}  // namespace imp
