#include "imp/backend.h"

#include <iostream>
#include <vector>


#ifndef IMP_VERSION
#define IMP_VERSION "0.0.1"
#endif

namespace
{
enum class CommandKind
{
  Help,
  Set,
  Status,
  Version,
};

struct Command
{
  CommandKind kind{CommandKind::Help};
  imp::Mode mode{imp::Mode::Unknown};
  std::string pair;
  std::string config;
};

x_error parse_argument(int argc, char** argv, Command& command);
void print_error(const x_error& error);
void print_status(const imp::Status& status);
void print_usage(const char* program);
x_error run_command(const Command& command, const char* program);
x_error run_set(imp::Backend& backend, const imp::Mode mode);
x_error run_status(const imp::Backend& backend);
}  // namespace

int main(int argc, char** argv)
{
  Command cmd;
  x_error err = parse_argument(argc, argv, cmd);
  if (err) {
    print_error(err);
    return EXIT_FAILURE;
  }

  err = run_command(cmd, argv[0]);
  if (err) {
    print_error(err);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

namespace
{
x_error parse_argument(int argc, char** argv, Command& command)
{
  std::vector<const char*> arguments;
  for (int idx = 1; idx < argc; ++idx) {
    const std::string option = argv[idx];
    if (option == "--pair" || option == "--config") {
      std::string& value = option == "--pair" ? command.pair : command.config;
      if (!value.empty()) {
        print_usage(argv[0]);
        const std::string message = option + " must be specified only once";
        return x_error("posix", EINVAL, message.c_str());
      }
      if (idx + 1 >= argc || argv[idx + 1][0] == '\0' || argv[idx + 1][0] == '-') {
        print_usage(argv[0]);
        const std::string message = option + (option == "--pair" ? " requires a name" : " requires a path");
        return x_error("posix", EINVAL, message.c_str());
      }
      value = argv[++idx];
    } else {
      arguments.push_back(argv[idx]);
    }
  }

  if (arguments.empty()) {
    print_usage(argv[0]);
    return x_error("posix", EINVAL, "expected a command");
  }

  const std::string arg = arguments[0];
  if (arg == "--help" || arg == "-h") {
    command.kind = CommandKind::Help;
  } else if (arg == "--version" || arg == "-v") {
    command.kind = CommandKind::Version;
  } else if (arg == "status") {
    if (arguments.size() != 1) {
      print_usage(argv[0]);
      return x_error("posix", EINVAL, "status does not accept arguments");
    }
    command.kind = CommandKind::Status;
  } else if (arg == "set") {
    if (arguments.size() != 2) {
      print_usage(argv[0]);
      return x_error("posix", EINVAL, "set requires a mode");
    }
    if (!imp::parse_mode(command.mode, arguments[1])) {
      return x_error("posix", EINVAL, "mode must be 'native' or 'alphanumeric'");
    }
    command.kind = CommandKind::Set;
  } else {
    print_usage(argv[0]);
    const std::string msg = "unknown command: " + arg;
    return x_error("posix", EINVAL, msg.c_str());
  }

  return x_ok();
}

void print_error(const x_error& error)
{
  std::cerr
    << "error: [" << error.domain()
    << ':' << error.value()
    << "] " << error.message()
    << '\n';
}

void print_status(const imp::Status& status)
{
  std::cout
    << "backend=" << status.backend << '\n'
    << "mode=" << imp::mode_name(status.mode) << '\n';

  if (!status.source_id.empty()) {
    std::cout << "source_id=" << status.source_id << '\n';
  }

  if (!status.source_name.empty()) {
    std::cout << "source_name=" << status.source_name << '\n';
  }
}

void print_usage(const char* program)
{
  std::cout << "Usage: " << program << " [--config <path>] [--pair <name>] <command>\n"
    << "\n"
    << "Commands:\n"
    << "  status                  Show the current input-method status\n"
    << "  set native              Set the current method to native mode\n"
    << "  set alphanumeric        Set the current method to alphanumeric mode\n"
    << "\n"
    << "Options:\n"
    << "  --config <path>         Load macOS pairs (default: imp.conf beside the executable)\n"
    << "  --pair <name>           Select a pair (default: first configured pair)\n"
    << "  -h, --help              Show this help\n"
    << "  -v, --version           Show the version\n";
}

x_error run_command(const Command& command, const char* program)
{
  switch (command.kind) {
    case CommandKind::Help:
      print_usage(program);
      return x_ok();
    case CommandKind::Version:
      std::cout << "imp " << IMP_VERSION << '\n';
      return x_ok();
    case CommandKind::Status:
    case CommandKind::Set:
    {
      const auto backend = imp::create_backend();
      const x_error error = backend->load_config(command.config);
      if (error) {
        return error;
      }
      if (!command.pair.empty()) {
        const x_error error = backend->configure_pair(command.pair);
        if (error) {
          return error;
        }
      }
      return command.kind == CommandKind::Status
        ? run_status(*backend) : run_set(*backend, command.mode);
    }
  }

  return x_error("posix", EINVAL, "invalid command");
}

x_error run_set(imp::Backend& backend, const imp::Mode mode)
{
  const x_error error = backend.set_mode(mode);
  if (error) {
    return error;
  }

  std::cout << "mode=" << imp::mode_name(mode) << '\n';
  return x_ok();
}

x_error run_status(const imp::Backend& backend)
{
  const imp::Status status = backend.status();
  if (status.error) {
    return status.error;
  }

  print_status(status);

  return x_ok();
}
}  // namespace
