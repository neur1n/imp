#include "imp/backend.h"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <vector>


int imp_test_main(int argc, char** argv);

namespace
{
enum class Scenario
{
  Success,
  UnknownMode,
  QueryFailure,
  QueryUnsupported,
  QueryUnimplemented,
  ForeignQueryError,
  SetFailure,
  SetUnsupported,
  SetUnimplemented,
  ForeignSetError,
  NativeSetError,
  InformationalSuccess,
};

Scenario scenario{Scenario::Success};
int query_calls{0};
int set_calls{0};

const x_error_domain native_domain{nullptr, nullptr, nullptr, "test-native"};
const x_error_domain informational_domain{
  nullptr,
  [](const int32_t value) { return value < 0; },
  nullptr,
  "test-hresult",
};

class TestBackend final : public imp::Backend
{
public:
  imp::Status status() const override
  {
    ++query_calls;
    imp::Status result;
    result.backend = "test-backend";
    if (scenario == Scenario::UnknownMode) {
      return result;
    }

    result.mode = imp::Mode::Native;
    result.source_id = "test.native";
    result.source_name = "Test Native";
    switch (scenario) {
      case Scenario::QueryFailure:
        result.error.set("posix", EIO, "status query failed");
        break;
      case Scenario::QueryUnsupported:
        result.error.set("posix", ENOTSUP, "status query unsupported");
        break;
      case Scenario::QueryUnimplemented:
        result.error.set("posix", ENOSYS, "status query unimplemented");
        break;
      case Scenario::ForeignQueryError:
        result.error.set("generic", ENOSYS, "foreign query failure");
        break;
      case Scenario::InformationalSuccess:
        result.error.set(informational_domain, 1);
        break;
      default:
        break;
    }
    return result;
  }

  x_error set_mode(imp::Mode) override
  {
    ++set_calls;
    switch (scenario) {
      case Scenario::SetFailure:
        return x_error("posix", EINVAL, "backend rejected mode");
      case Scenario::SetUnsupported:
        return x_error("posix", ENOTSUP, "mode setting unsupported");
      case Scenario::SetUnimplemented:
        return x_error("posix", ENOSYS, "mode setting unimplemented");
      case Scenario::ForeignSetError:
        return x_error("generic", ENOTSUP, "foreign set failure");
      case Scenario::NativeSetError:
        return x_error(native_domain, -43);
      case Scenario::InformationalSuccess:
        return x_error(informational_domain, 1);
      default:
        return x_ok();
    }
  }
};

struct Result
{
  int exit_code;
  std::string output;
  std::string error;
};

Result run(std::initializer_list<const char*> arguments, Scenario selected = Scenario::Success)
{
  scenario = selected;
  query_calls = 0;
  set_calls = 0;

  std::vector<std::string> storage{"imp"};
  for (const char* argument : arguments) {
    storage.emplace_back(argument);
  }
  std::vector<char*> argv;
  for (std::string& argument : storage) {
    argv.push_back(argument.data());
  }
  argv.push_back(nullptr);

  std::ostringstream output;
  std::ostringstream error;
  auto* old_output = std::cout.rdbuf(output.rdbuf());
  auto* old_error = std::cerr.rdbuf(error.rdbuf());
  const int exit_code = imp_test_main(static_cast<int>(storage.size()), argv.data());
  std::cout.rdbuf(old_output);
  std::cerr.rdbuf(old_error);

  return {exit_code, output.str(), error.str()};
}

void check(bool condition, const char* message)
{
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(EXIT_FAILURE);
  }
}

std::string diagnostic(const char* domain, int value, const char* message)
{
  return "error: [" + std::string(domain) + ':' + std::to_string(value) + "] " + message + '\n';
}

void expect(const char* name, const Result& result, int exit_code,
  const std::string& output, const std::string& error)
{
  if (result.exit_code != exit_code || result.output != output || result.error != error) {
    std::cerr << name << ": unexpected CLI result\n"
      << "exit=" << result.exit_code << " (expected " << exit_code << ")\n"
      << "stdout: " << result.output << "\nstderr: " << result.error << '\n';
    std::exit(EXIT_FAILURE);
  }
}
}  // namespace

namespace imp
{
std::unique_ptr<Backend> create_backend()
{
  return std::make_unique<TestBackend>();
}
}  // namespace imp

int main()
{
  const std::string status_output =
    "backend=test-backend\nmode=native\nsource_id=test.native\nsource_name=Test Native\n";
  expect("status", run({"status"}), EXIT_SUCCESS, status_output, "");
  expect("unknown mode", run({"status"}, Scenario::UnknownMode), EXIT_SUCCESS,
    "backend=test-backend\nmode=unknown\n", "");
  expect("query failure", run({"status"}, Scenario::QueryFailure), EXIT_FAILURE, "",
    diagnostic("posix", EIO, "status query failed"));
  expect("unsupported query", run({"status"}, Scenario::QueryUnsupported), EXIT_FAILURE, "",
    diagnostic("posix", ENOTSUP, "status query unsupported"));
  expect("unimplemented query", run({"status"}, Scenario::QueryUnimplemented), EXIT_FAILURE, "",
    diagnostic("posix", ENOSYS, "status query unimplemented"));
  expect("foreign query code", run({"status"}, Scenario::ForeignQueryError), EXIT_FAILURE, "",
    diagnostic("generic", ENOSYS, "foreign query failure"));
  expect("informational query success", run({"status"}, Scenario::InformationalSuccess),
    EXIT_SUCCESS, status_output, "");

  expect("native", run({"set", "native"}), EXIT_SUCCESS, "mode=native\n", "");
  expect("alphanumeric", run({"set", "alphanumeric"}), EXIT_SUCCESS, "mode=alphanumeric\n", "");
  expect("mode alias", run({"set", "ALPHA"}), EXIT_SUCCESS, "mode=alphanumeric\n", "");
  expect("set without querying", run({"set", "native"}, Scenario::QueryFailure),
    EXIT_SUCCESS, "mode=native\n", "");
  check(query_calls == 0 && set_calls == 1, "setter must not depend on status query");
  expect("backend EINVAL", run({"set", "native"}, Scenario::SetFailure), EXIT_FAILURE, "",
    diagnostic("posix", EINVAL, "backend rejected mode"));
  expect("unsupported set", run({"set", "native"}, Scenario::SetUnsupported), EXIT_FAILURE, "",
    diagnostic("posix", ENOTSUP, "mode setting unsupported"));
  expect("unimplemented set", run({"set", "native"}, Scenario::SetUnimplemented), EXIT_FAILURE, "",
    diagnostic("posix", ENOSYS, "mode setting unimplemented"));
  expect("foreign set code", run({"set", "native"}, Scenario::ForeignSetError), EXIT_FAILURE, "",
    diagnostic("generic", ENOTSUP, "foreign set failure"));
  expect("native set code", run({"set", "native"}, Scenario::NativeSetError), EXIT_FAILURE, "",
    diagnostic("test-native", -43, "Unknown error -43"));
  expect("informational set success", run({"set", "native"}, Scenario::InformationalSuccess),
    EXIT_SUCCESS, "mode=native\n", "");

  const Result help = run({"--help"});
  check(help.exit_code == EXIT_SUCCESS && help.error.empty(), "help succeeds");
  check(help.output.find("Usage: imp <command>\n") == 0, "help usage");
  expect("short help", run({"-h"}), EXIT_SUCCESS, help.output, "");
  const Result version = run({"--version"});
  check(version.exit_code == EXIT_SUCCESS && version.error.empty(), "version succeeds");
  check(version.output.find("imp ") == 0, "version output");
  expect("short version", run({"-v"}), EXIT_SUCCESS, version.output, "");

  expect("missing command", run({}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "expected a command"));
  expect("unknown command", run({"invalid"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "unknown command: invalid"));
  expect("status arguments", run({"status", "extra"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "status does not accept arguments"));
  expect("missing mode", run({"set"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "set requires a mode"));
  expect("extra set argument", run({"set", "native", "extra"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "set requires a mode"));
  expect("invalid mode", run({"set", "invalid"}), EXIT_FAILURE, "",
    diagnostic("posix", EINVAL, "mode must be 'native' or 'alphanumeric'"));
  check(query_calls == 0 && set_calls == 0, "invalid arguments must not invoke backend operations");
}
