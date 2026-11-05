#ifndef IMP_BACKEND_H
#define IMP_BACKEND_H


#include "x.h"

#include <memory>


namespace imp
{
/// @var imp::Mode imp::Mode::Native
/// @brief Native-language input state.
///
/// @var imp::Mode imp::Mode::Alphanumeric
/// @brief Alphanumeric input state.
///
/// @var imp::Mode imp::Mode::Unknown
/// @brief The current state could not be determined or mapped.
/// @note This is not a setting target and does not itself indicate failure.
///
/// @enum imp::Mode
/// @brief An abstract input state interpreted by each platform backend.
/// @note A backend may implement this mapping by controlling an input
///       method's mode or by selecting a documented input source.
enum class Mode
{
  Native,
  Alphanumeric,
  Unknown,
};

/// @var imp::Mode imp::Status::mode
/// @brief Observed mode; may be @ref Mode::Unknown even on query success.
///
/// @var std::string imp::Status::backend
/// @brief Project-defined backend identifier, also available on failure.
///
/// @var std::string imp::Status::source_id
/// @brief Platform-specific current source identifier; empty if unavailable.
///
/// @var std::string imp::Status::source_name
/// @brief Current source's display name; empty if unavailable.
///
/// @var x_error imp::Status::error
/// @brief Query result; evaluates to `true` on failure.
///
/// @struct imp::Status
/// @brief Observed input-method state and the result of querying it.
/// @note Check @ref Status::error before using the observed state. On failure,
///       @ref Status::backend still identifies the backend; other observations
///       may be incomplete.
struct Status
{
  Mode mode{Mode::Unknown};
  std::string backend;
  std::string source_id;
  std::string source_name;
  x_error error;
};

/// @brief Platform-specific input-method querying and control interface.
/// @note Operational failures are reported through @ref x_error values.
class Backend
{
public:
  /// @brief Destroy the backend and release its resources.
  virtual ~Backend() = default;

  /// @brief Configure the named input-source pair used by subsequent operations.
  /// @param name Pair name from the loaded configuration or built-in pairs.
  /// @return An error for an unknown pair or POSIX ENOTSUP if pair selection is unsupported.
  /// @note Does not switch the input source. A failed configuration preserves the previous pair.
  virtual x_error configure_pair(const std::string& name);

  /// @brief Load input-source pairs without querying or switching sources.
  /// @param path Configuration file; empty selects automatic discovery.
  /// @return An error for invalid/unreadable configuration or unsupported explicit configuration.
  /// @note On macOS, an absent automatic imp.conf uses built-in pairs; an explicit path must exist.
  ///       Loading selects the first pair. Failure preserves the previous configuration.
  virtual x_error load_config(const std::string& path);

  /// @brief Request a target input state using the backend's control mechanism.
  /// @param mode Requested @ref Mode::Native or @ref Mode::Alphanumeric state.
  /// @return An error value whose domain determines success or failure.
  /// @note A successful @ref status call is not required before setting a mode.
  ///       Check the returned value with @ref x_fail or its `bool` conversion.
  virtual x_error set_mode(const Mode mode) = 0;

  /// @brief Query the current input-method state.
  /// @return A snapshot with query failures in @ref Status::error and the
  ///         backend identifier populated even on failure.
  virtual Status status() const = 0;
};  // class Backend

/// @brief Construct the platform backend selected by the build configuration.
/// @return An owning pointer to the backend, including a stub on platforms
///         whose operations are not implemented.
std::unique_ptr<Backend> create_backend();

/// @brief Get the stable lowercase name of a mode.
/// @param mode Mode to describe.
/// @return A static string: `"native"`, `"alphanumeric"`, or `"unknown"`.
/// @note Unrecognized enumeration values also produce `"unknown"`.
const char* mode_name(const Mode mode);

/// @brief Parse a case-insensitive input-state name.
/// @param[out] mode Receives the parsed mode on success; unchanged on failure.
/// @param value Name to parse: `"native"`, `"alphanumeric"`, or `"alpha"`.
/// @pre `value` points to a valid null-terminated string.
/// @return `true` if the name was recognized; `false` otherwise.
bool parse_mode(Mode& mode, const char* value);
}  // namespace imp


#endif  // IMP_BACKEND_H
