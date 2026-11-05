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
  PairUnsupported,
  ConfigFailure,
  ConfigUnsupported,
};

Scenario scenario{Scenario::Success};
int query_calls{0};
int set_calls{0};
int pair_calls{0};
std::string selected_pair;
int config_calls{0};
std::string selected_config;

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
  x_error load_config(const std::string& path) override
  {
    ++config_calls;
    selected_config = path;
    if (scenario == Scenario::ConfigUnsupported) {
      return Backend::load_config(path);
    }
    if (scenario == Scenario::ConfigFailure) {
      return x_error("posix", EINVAL, "bad.conf:2: malformed record");
    }
    return x_ok();
  }

  x_error configure_pair(const std::string& name) override
  {
    if (config_calls != 1) {
      return x_error("posix", EINVAL, "pair must be selected after loading configuration");
    }
    ++pair_calls;
    if (scenario == Scenario::PairUnsupported) {
      return Backend::configure_pair(name);
    }
    if (name != "wetype" && name != "pinyin" && name != "wubi") {
      return x_error("posix", EINVAL, "unknown pair");
    }
    selected_pair = name;
    return x_ok();
  }

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
  pair_calls = 0;
  selected_pair.clear();
  config_calls = 0;
  selected_config.clear();

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
  check(pair_calls == 0, "unspecified pair uses backend default");
  check(config_calls == 1 && selected_config.empty(), "CLI requests automatic config discovery");
  expect("explicit config", run({"--config", "path with spaces.conf", "status"}), EXIT_SUCCESS, status_output, "");
  check(selected_config == "path with spaces.conf" && pair_calls == 0, "explicit file keeps its own default pair");
  expect("config and pair", run({"set", "native", "--pair", "pinyin", "--config", "custom.conf"}),
    EXIT_SUCCESS, "mode=native\n", "");
  check(selected_config == "custom.conf" && selected_pair == "pinyin", "explicit pair overrides file default");
  expect("bad config", run({"--config", "bad.conf", "--pair", "pinyin", "set", "native"}, Scenario::ConfigFailure),
    EXIT_FAILURE, "", diagnostic("posix", EINVAL, "bad.conf:2: malformed record"));
  check(pair_calls == 0 && query_calls == 0 && set_calls == 0, "invalid config must not select a pair or run an operation");
  expect("unsupported config", run({"--config", "custom.conf", "status"}, Scenario::ConfigUnsupported), EXIT_FAILURE, "",
    diagnostic("posix", ENOTSUP, "Input-source configuration is not supported by this backend"));
  check(query_calls == 0 && set_calls == 0, "unsupported config is not ignored");
  for (const char* pair : {"wetype", "pinyin", "wubi"}) {
    expect("named pair status", run({"--pair", pair, "status"}), EXIT_SUCCESS, status_output, "");
    check(pair_calls == 1 && selected_pair == pair && query_calls == 1,
      "configure selected pair before querying");
    expect("named pair set", run({"--pair", pair, "set", "native"}), EXIT_SUCCESS, "mode=native\n", "");
    check(pair_calls == 1 && selected_pair == pair && set_calls == 1 && query_calls == 0,
      "configure selected pair without querying before setting");
    expect("named pair alphanumeric", run({"set", "alphanumeric", "--pair", pair}),
      EXIT_SUCCESS, "mode=alphanumeric\n", "");
    check(selected_pair == pair, "pair option may follow the command");
  }
  expect("unknown pair", run({"--pair", "missing", "set", "native"}), EXIT_FAILURE, "",
    diagnostic("posix", EINVAL, "unknown pair"));
  check(pair_calls == 1 && query_calls == 0 && set_calls == 0, "invalid pair must not run an operation");
  expect("unsupported pair", run({"--pair", "pinyin", "status"}, Scenario::PairUnsupported), EXIT_FAILURE, "",
    diagnostic("posix", ENOTSUP, "Input-source pair selection is not supported by this backend"));
  check(query_calls == 0 && set_calls == 0, "unsupported pair must not be ignored");
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
  check(help.output.find("Usage: imp [--config <path>] [--pair <name>] <command>\n") == 0, "help usage");
  expect("short help", run({"-h"}), EXIT_SUCCESS, help.output, "");
  const Result version = run({"--version"});
  check(version.exit_code == EXIT_SUCCESS && version.error.empty(), "version succeeds");
  check(version.output.find("imp ") == 0, "version output");
  expect("short version", run({"-v"}), EXIT_SUCCESS, version.output, "");
  expect("help does not configure pair", run({"--pair", "pinyin", "--help"}), EXIT_SUCCESS, help.output, "");
  check(pair_calls == 0 && query_calls == 0 && set_calls == 0, "help does not invoke backend operations");
  expect("help does not load config", run({"--config", "missing.conf", "--help"}), EXIT_SUCCESS, help.output, "");
  check(config_calls == 0, "help bypasses configuration loading");
  expect("missing config path", run({"--config"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--config requires a path"));
  expect("empty config path", run({"--config", "", "status"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--config requires a path"));
  expect("option instead of config path", run({"--config", "--help"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--config requires a path"));
  expect("duplicate config", run({"--config", "a.conf", "--config", "b.conf", "status"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--config must be specified only once"));
  check(config_calls == 0, "config syntax failures must not invoke the backend");

  expect("missing pair name", run({"--pair"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--pair requires a name"));
  expect("empty pair name", run({"--pair", "", "status"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--pair requires a name"));
  expect("option instead of pair name", run({"--pair", "--help"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--pair requires a name"));
  expect("duplicate pair", run({"--pair", "pinyin", "--pair", "wubi", "status"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "--pair must be specified only once"));
  expect("pair without command", run({"--pair", "pinyin"}), EXIT_FAILURE, help.output,
    diagnostic("posix", EINVAL, "expected a command"));
  check(pair_calls == 0 && query_calls == 0 && set_calls == 0, "pair parsing failures do not invoke backend operations");

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
