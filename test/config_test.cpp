#include "imp/config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>


namespace
{
void check(bool condition, const char* message)
{
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}  // namespace

int main(int argc, char** argv)
{
  check(argc == 2, "test needs a fixture directory");
  std::vector<imp::InputSourcePair> pairs{{"previous", "old.alpha", "old.native"}};
  std::istringstream valid(
    "  # comment\r\n\n\t\r\n"
    "custom\tcustom.alpha\tcustom.native\r\n"
    "other other.alpha other.native");
  check(!imp::parse_pairs(valid, pairs, "valid.conf"), "parse whitespace, CRLF, comments, and final line");
  check(pairs.size() == 2 && pairs.front().name == "custom", "preserve file order and first default");
  check(pairs.front().alphanumeric == "custom.alpha" && pairs.front().native == "custom.native",
    "accept arbitrary source IDs without a built-in alias table");
  check(imp::find_pair(pairs, "other") == &pairs[1], "find named pair");
  check(imp::find_pair(pairs, "previous") == nullptr, "file replaces, rather than merges with, old pairs");

  for (const char* content : {"", " # only comments\n", "bad one\n", "bad a b extra\n", "bad a b # inline\n"}) {
    std::istringstream invalid(content);
    const x_error error = imp::parse_pairs(invalid, pairs, "bad.conf");
    check(error && error.value() == EINVAL, "reject empty and malformed records");
    check(std::string(error.message()).find("bad.conf") != std::string::npos, "include file in diagnostics");
    check(pairs.size() == 2 && pairs.front().name == "custom", "failed parse preserves old pairs");
  }
  std::istringstream duplicate("# header\nsame a b\nsame c d\n");
  const x_error duplicate_error = imp::parse_pairs(duplicate, pairs, "duplicate.conf");
  check(duplicate_error && std::string(duplicate_error.message()).find("duplicate.conf:3: duplicate pair name: same")
      != std::string::npos, "duplicate diagnostic includes line and name");
  std::istringstream malformed("# header\nshort a\n");
  const x_error malformed_error = imp::parse_pairs(malformed, pairs, "short.conf");
  check(malformed_error && std::string(malformed_error.message()).find("short.conf:2:") != std::string::npos,
    "malformed diagnostic includes line");
  std::istringstream broken;
  broken.setstate(std::ios::badbit);
  const x_error read_error = imp::parse_pairs(broken, pairs, "broken.conf");
  check(read_error && read_error.value() == EIO, "report stream read errors");

  const std::filesystem::path directory = argv[1];
  std::filesystem::create_directories(directory);
  const x_error not_a_file = imp::load_pairs(directory.string(), pairs);
  check(not_a_file && not_a_file.value() == EINVAL, "reject a directory rather than treating it as empty configuration");
  const auto path = directory / "pairs with spaces.conf";
  {
    std::ofstream file(path);
    file << "uninstalled missing.alpha missing.native\n";
  }
  check(!imp::load_pairs(path.string(), pairs), "load real file without validating system source availability");
  check(pairs.size() == 1 && pairs.front().name == "uninstalled", "loaded file owns source strings");
  std::filesystem::remove(path);
  const x_error missing = imp::load_pairs(path.string(), pairs);
  check(missing && missing.value() == ENOENT, "explicit missing file is an error");
  check(pairs.front().name == "uninstalled", "failed file load preserves old pairs");
}
