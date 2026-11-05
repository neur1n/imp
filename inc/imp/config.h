#ifndef IMP_CONFIG_H
#define IMP_CONFIG_H


#include "x.h"

#include <istream>
#include <string>
#include <string_view>
#include <vector>


namespace imp
{
/// @brief A named pair of platform-specific input-source IDs.
struct InputSourcePair
{
  std::string name;
  std::string alphanumeric;
  std::string native;
};

/// @brief Find a named pair; returns nullptr if the name is not registered.
const InputSourcePair* find_pair(const std::vector<InputSourcePair>& pairs, std::string_view name);

/// @brief Parse three-column pair records, ignoring blank lines and full-line comments.
/// @param input Configuration stream.
/// @param pairs Replaced on success; the first record is the default pair.
/// @param path Name used in diagnostics, including line numbers.
/// @return An error for malformed, duplicate, empty, or unreadable input.
/// @note Failures preserve the previous pair list. No input sources are queried or switched.
x_error parse_pairs(std::istream& input, std::vector<InputSourcePair>& pairs, const std::string& path);

/// @brief Load a pair file; failures preserve the previous pair list.
x_error load_pairs(const std::string& path, std::vector<InputSourcePair>& pairs);
}  // namespace imp


#endif  // IMP_CONFIG_H
