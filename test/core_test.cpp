#include "imp/backend.h"

#if defined(__APPLE__)
#include "detail/macos_error.h"
#include "detail/macos_source.h"
#endif

#include <cstdlib>
#include <iostream>
#include <limits>


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

int main()
{
  using imp::Mode;

  check(imp::mode_name(Mode::Native) == std::string("native"), "native mode name");
  check(imp::mode_name(Mode::Alphanumeric) == std::string("alphanumeric"), "alphanumeric mode name");
  check(imp::mode_name(Mode::Unknown) == std::string("unknown"), "unknown mode name");

  Mode mode = Mode::Unknown;
  check(imp::parse_mode(mode, "native") && mode == Mode::Native, "parse native");
  check(imp::parse_mode(mode, "ALPHA") && mode == Mode::Alphanumeric, "parse alias");
  check(!imp::parse_mode(mode, "other"), "reject invalid mode");

  const auto backend = imp::create_backend();
  check(backend != nullptr, "create backend");

#if defined(__APPLE__)
  const x_error invalid_mode = backend->set_mode(Mode::Unknown);
  check(invalid_mode && invalid_mode.value() == EINVAL, "reject invalid mode before switching sources");

  check(imp::detail::k_supported.front().name == "wetype", "preserve built-in default pair");
  const auto initial_status = backend->status();
  if (!initial_status.error) {
    check(initial_status.mode == imp::detail::source_to_mode(imp::detail::k_supported.front(), initial_status.source_id),
      "new backend uses the first built-in pair");
  }
  check(!imp::detail::source_availability("test.source", true, true), "enabled selectable source is usable");
  const x_error disabled_source = imp::detail::source_availability("test.source", false, true);
  check(disabled_source && disabled_source.value() == EACCES
    && std::string(disabled_source.message()).find("test.source") != std::string::npos, "diagnose disabled source");
  const x_error unselectable_source = imp::detail::source_availability("test.source", true, false);
  check(unselectable_source && unselectable_source.value() == ENOTSUP, "diagnose unselectable source");
  const char* names[]{"wetype", "pinyin", "wubi"};
  const char* native_sources[]{
    "com.tencent.inputmethod.wetype.pinyin",
    "com.apple.inputmethod.SCIM.ITABC",
    "com.apple.inputmethod.SCIM.WBX",
  };
  for (size_t idx = 0; idx < 3; ++idx) {
    const auto* pair = imp::detail::find_pair(names[idx]);
    check(pair != nullptr, "find named pair");
    check(std::string(imp::detail::mode_to_source(*pair, Mode::Native)) == native_sources[idx],
      "map native mode to selected source");
    check(std::string(imp::detail::mode_to_source(*pair, Mode::Alphanumeric)) == "com.apple.keylayout.ABC",
      "map alphanumeric mode to ABC");
    check(imp::detail::mode_to_source(*pair, Mode::Unknown) == nullptr, "reject unknown target mode");
    check(imp::detail::source_to_mode(*pair, native_sources[idx]) == Mode::Native, "recognize selected native source");
    check(imp::detail::source_to_mode(*pair, "com.apple.keylayout.ABC") == Mode::Alphanumeric,
      "recognize shared ABC source without changing pair");
    check(imp::detail::source_to_mode(*pair, native_sources[(idx + 1) % 3]) == Mode::Unknown,
      "do not classify another pair's native source as selected");
    check(imp::detail::source_to_mode(*pair, "unrelated.source") == Mode::Unknown, "unmapped source");
    check(!backend->configure_pair(names[idx]), "configure a pair without switching sources");
    const imp::Status status = backend->status();
    if (!status.error) {
      check(status.mode == imp::detail::source_to_mode(*pair, status.source_id), "status uses selected pair");
    }
  }
  check(imp::detail::find_pair("missing") == nullptr, "unknown pair must not fall back");
  check(imp::detail::find_pair("") == nullptr, "empty pair is not a registered name");
  const x_error invalid_pair = backend->configure_pair("missing");
  check(invalid_pair && invalid_pair.value() == EINVAL, "reject unknown pair");
  const imp::Status status = backend->status();
  if (!status.error) {
    check(status.mode == imp::detail::source_to_mode(*imp::detail::find_pair("wubi"), status.source_id),
      "failed configuration preserves selected pair");
  }

  const x_error native_error(imp::detail::osstatus_domain, fnfErr);
  check(native_error && native_error.value() == fnfErr, "preserve original OSStatus");
  check(std::string(native_error.domain()) == "osstatus", "preserve OSStatus domain");
  check(std::string(native_error.message()) == GetMacOSStatusCommentString(fnfErr), "query native error message");
  check(!x_error(imp::detail::osstatus_domain, noErr), "OSStatus success");

  const int32_t unknown_code = std::numeric_limits<int32_t>::min();
  const x_error unknown_error(imp::detail::osstatus_domain, unknown_code);
  const std::string expected = "OSStatus " + std::to_string(unknown_code);
  check(unknown_error.message() == expected, "numeric fallback for unknown OSStatus");
  char buffer[1]{'x'};
  unknown_error.message(buffer, sizeof(buffer));
  check(buffer[0] == '\0', "terminate truncated OSStatus messages");
#else
  check(!backend->load_config(""), "automatic config is a no-op on an unsupported platform");
  const x_error config_error = backend->load_config("custom.conf");
  check(config_error && config_error.value() == ENOTSUP, "report unsupported explicit configuration");
  const x_error pair_error = backend->configure_pair("pinyin");
  check(pair_error && pair_error.value() == ENOTSUP, "report unsupported pair selection");
  const imp::Status status = backend->status();
  check(!status.backend.empty(), "preserve backend identity on query failure");
  check(status.error && status.error.value() == ENOSYS, "report unimplemented status query");
  const x_error error = backend->set_mode(Mode::Native);
  check(error && error.value() == ENOSYS, "report unimplemented setter");
#endif
}
