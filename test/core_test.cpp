#include "imp/backend.h"

#if defined(__APPLE__)
#include "detail/macos_error.h"
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
  const imp::Status status = backend->status();
  check(!status.backend.empty(), "preserve backend identity on query failure");
  check(status.error && status.error.value() == ENOSYS, "report unimplemented status query");
  const x_error error = backend->set_mode(Mode::Native);
  check(error && error.value() == ENOSYS, "report unimplemented setter");
#endif
}
