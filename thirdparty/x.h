/******************************************************************************
Copyright (c) 2023 Jihang Li
x.h is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details.
******************************************************************************/

/** @file
 * @brief Cross-platform C++ utilities.
 * @note On Linux, include this header before any system header so its default
 *       `_GNU_SOURCE` and `_FILE_OFFSET_BITS=64` definitions take effect.
 *       Alternatively, define these macros in the build configuration before
 *       any headers, including precompiled or forced includes.
 * @note On Cygwin and Linux, keep `_FILE_OFFSET_BITS` consistent across
 *       translation units that share file-system interfaces, and configure it
 *       before including system headers.
 */
#ifndef X_H
#define X_H x_version(0, 9, 2)


/// @brief Version and included modules; Core includes Error Handling.
#define X_H_VERBOSE "0.9.2+core"

/** @internal
 * Table of Contents
 *
 * Feature_Configuration
 * Architecture_Detection
 * Compiler_Detection
 * Operating_System_Detection
 * Platform_Detection
 *
 * Headers
 *
 * Symbol_Visibility
 * Miscellaneous
 *
 * DECL_Error_Handling
 *
 * IMPL_Error_Handling
 * @endinternal
 */

/// @brief Generate a version number.
/// @param major The major version number, ranges in [0, 99].
/// @param minor The minor version number, ranges in [0, 99].
/// @param patch The patch version number, ranges in [0, 99999].
/// @return The version number.
/// @remark A big range of patch allow the result version number specifically
///         work with [`_MSC_FULL_VER`](https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=msvc-170).
#define x_version(major, minor, patch) \
  (((major) % 100) * 10000000 + ((minor) % 100) * 100000 + ((patch) % 100000))

/******************************************************************************
 * @name Feature_Configuration{{{
 *****************************************************************************/
/// @brief Toggle the availability of CUDA driver API related functions.
#ifndef X_ENABLE_CUDA_DRIVER
#define X_ENABLE_CUDA_DRIVER (0)
#endif

/// @brief Toggle the availability of CUDA runtime API related functions.
#ifndef X_ENABLE_CUDA_RUNTIME
#define X_ENABLE_CUDA_RUNTIME (0)
#endif

/// @brief Toggle the availability of cuBLAS related functions.
#ifndef X_ENABLE_CUBLAS
#define X_ENABLE_CUBLAS (0)
#endif

/// @brief Toggle the built-in `"socket"` error domain.
/// @note Captures `WSAGetLastError()` on Windows and `errno` on POSIX. This
///       does not provide socket operations. Windows requires the Winsock
///       library.
#ifndef X_ENABLE_SOCKET
#define X_ENABLE_SOCKET (0)
#endif
// Feature_Configuration}}}

/******************************************************************************
 * @name Architecture_Detection{{{
 *****************************************************************************/
#if defined(_M_ARM64) || defined(__AARCH64EL__) \
  || defined(__aarch64__) || defined(__arm64)
#define X_ARM64 (1)
#else
#define X_ARM64 (0)
#endif

#if X_ARM64 || defined(_M_ARM) || defined(__ARM_ARCH) \
  || defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__) \
  || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5TEJ__) \
  || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6KZ__) \
  || defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6Z__) \
  || defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) \
  || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7R__) \
  || defined(__TARGET_ARCH_ARM) || defined(__TARGET_ARCH_THUMB) \
  || defined(__arm__) || defined(__thumb__)
#define X_ARM (1)
#else
#define X_ARM (0)
#endif

#if defined(_M_IX86) || defined(_X86_) \
  || defined(i386) || defined(__i386) || defined(__i386__) \
  || defined(__i486__) || defined(__i586__) || defined(__i686__) \
  || defined(__I86__) || defined(__INTEL__) || defined(__THW_INTEL__)
#define X_X86 (1)
#else
#define X_X86 (0)
#endif

#if defined(_M_AMD64) || defined(_M_X64) \
  || defined(__amd64) || defined(__amd64__) \
  || defined(__x86_64) || defined(__x86_64__)
#define X_X64 (1)
#else
#define X_X64 (0)
#endif
// Architecture_Detection}}}

/******************************************************************************
 * @name Compiler_Detection{{{
 *****************************************************************************/
#if defined(__clang__)
#define X_CLANG x_version(__clang_major__, __clang_minor__, __clang_patchlevel__)
#else
#define X_CLANG (0)
#endif

#if defined(__GNUC__) \
  && !defined(__ECC) && !defined(__ICC) && !defined(__ICL) \
  && !defined(__INTEL_COMPILER) && !defined(__INTEL_LLVM_COMPILER) \
  && !defined(__TINYC__) && !defined(__clang__)
#if defined(__GNUC_PATCHLEVEL__)
#define X_GCC x_version(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
#define X_GCC x_version(__GNUC__, __GNUC_MINOR__, 0)
#endif
#else
#define X_GCC (0)
#endif

#if defined(_MSC_VER) \
  && !X_CLANG && !defined(__ECC) && !defined(__ICC) && !defined(__ICL) \
  && !defined(__INTEL_COMPILER) && !defined(__INTEL_LLVM_COMPILER)
#ifdef _MSC_FULL_VER
#define X_MSVC _MSC_FULL_VER
#else
#define X_MSVC x_version((_MSC_VER / 100) % 100, _MSC_VER % 100, 0)
#endif
#else
#define X_MSVC (0)
#endif

#if defined(__NVCC__)
#if defined(__CUDACC_VER_BUILD__) && defined(__CUDACC_VER_MAJOR__) \
  && defined(__CUDACC_VER_MINOR__)
#define X_NVCC x_version(__CUDACC_VER_MAJOR__, __CUDACC_VER_MINOR__, __CUDACC_VER_BUILD__)
#else
#define X_NVCC (1)
#endif
#else
#define X_NVCC (0)
#endif
// Compiler_Detection}}}

/******************************************************************************
 * @name Operating_System_Detection{{{
 *****************************************************************************/
#if defined(__CYGWIN__)
#include <cygwin/version.h>
/// @note It needs a special handling because `CYGWIN_VERSION_API_MINOR` is
///       greater than 99.
/// @see [Cygwin](https://github.com/cygwin/cygwin/blob/main/winsup/cygwin/include/cygwin/version.h)
#define X_CYGWIN x_version( \
    CYGWIN_VERSION_API_MAJOR + CYGWIN_VERSION_API_MINOR / 100, \
    CYGWIN_VERSION_API_MINOR % 100, 0)
#else
#define X_CYGWIN (0)
#endif

#if defined(__APPLE__) && defined(__MACH__) \
  && defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
#define X_IOS (__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__ * 1000)
#else
#define X_IOS (0)
#endif

#if defined(__gnu_linux__) || defined(__linux) \
  || defined(__linux__) || defined(linux)
#define X_LINUX (1)
#else
#define X_LINUX (0)
#endif

#if defined(Macintosh) || defined(macintosh)
#define X_MACOS x_version(9, 0, 0)
#elif !X_IOS && defined(__APPLE__) && defined(__MACH__)
#define X_MACOS x_version(10, 0, 0)
#else
#define X_MACOS (0)
#endif

#if !X_CYGWIN && (defined(_WIN32) || defined(_WIN64) \
  || defined(__TOS_WIN__) || defined(__TOS_WIX__) \
  || defined(__WIN32__) || defined(__WINDOWS__))
#define X_WINDOWS (1)
#else
#define X_WINDOWS (0)
#endif
// Operating_System_Detection}}}

/******************************************************************************
 * @name Platform_Detection{{{
 *****************************************************************************/
#if defined(__ANDROID__)
#define X_ANDROID (1)
#else
#define X_ANDROID (0)
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
#include <_mingw.h>
#define X_MINGW (1)
#else
#define X_MINGW (0)
#endif

#if defined(__MINGW32__)
#if defined(__MINGW32_VERSION_MAJOR) && defined(__MINGW32_VERSION_MINOR)
#define X_MINGW32 x_version(__MINGW32_VERSION_MAJOR, __MINGW32_VERSION_MINOR, 0)
#else
#define X_MINGW32 (1)
#endif
#else
#define X_MINGW32 (0)
#endif

#if defined(__MINGW64__)
#if defined(__MINGW64_VERSION_MAJOR) && defined(__MINGW64_VERSION_MINOR)
#define X_MINGW64 x_version(__MINGW64_VERSION_MAJOR, __MINGW64_VERSION_MINOR, 0)
#else
#define X_MINGW64 (1)
#endif
#else
#define X_MINGW64 (0)
#endif
// Platform_Detection}}}

// ********************************************************** Configurations{{{
// Feature-test macros must be defined before including any system header.
#if X_LINUX
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#if X_CYGWIN || X_LINUX
#ifndef _FILE_OFFSET_BITS
// Enable large-file interfaces on supported POSIX targets.
#define _FILE_OFFSET_BITS (64)
#endif
#endif

// Architecture width detection requires <cstdint>.
#include <cstdint>
#if INTPTR_MAX == INT64_MAX
#define X_32BIT (0)
#define X_64BIT (1)
#elif INTPTR_MAX == INT32_MAX
#define X_32BIT (1)
#define X_64BIT (0)
#else
#error "Only 32-bit or 64-bit architecture is supported."
#endif

#if !X_MSVC && defined(_MSC_VER)
#define _X_MSVC_COMPAT (1)
#else
#define _X_MSVC_COMPAT (0)
#endif
// Configurations}}}

// ***************************************************************** Headers{{{
#include <cctype>
#include <cerrno>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <type_traits>
#include <utility>

#if X_WINDOWS
#if X_ENABLE_SOCKET
#if defined(_MSC_VER)
#pragma comment(lib, "Ws2_32")
#endif
#include <winsock2.h>
#endif
#include <windows.h>
#include <winternl.h>
#else
#if X_LINUX
#include <sys/sysinfo.h>
#elif X_IOS || X_MACOS
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif
#include <unistd.h>
#endif

#if X_ENABLE_CUDA_DRIVER
#include <cuda.h>
#endif
#if X_ENABLE_CUDA_RUNTIME
#include <cuda_runtime.h>
#endif
#if X_ENABLE_CUBLAS
#include <cublas_v2.h>
#endif
// Headers}}}

/******************************************************************************
 * @name Symbol_Visibility{{{
 * @see [Microsoft Docs](https://docs.microsoft.com/en-us/cpp/cpp/dllexport-dllimport?view=msvc-170)
 *      and [GCC Wiki](https://gcc.gnu.org/wiki/Visibility).
 *****************************************************************************/
#if X_CYGWIN || X_WINDOWS
/// @brief Hidden-visibility annotation is not applied to Cygwin/Windows DLLs.
#define X_HID

/// @brief Mark a symbol exported by the current Cygwin or Windows DLL.
#define X_EXP __declspec(dllexport)

/// @brief Mark a symbol imported from another Cygwin or Windows DLL.
#define X_IMP __declspec(dllimport)
#else
/// @brief Mark a symbol as internal to the current shared library.
#define X_HID __attribute__ ((visibility("hidden")))

/// @brief Mark a symbol exported by the current shared library.
#define X_EXP __attribute__ ((visibility("default")))

/// @brief Mark a symbol imported from another shared library.
/// @note ELF imports use the exported visibility; hidden visibility is for
///       internals.
#define X_IMP __attribute__ ((visibility("default")))
#endif
// Symbol_Visibility}}}

/******************************************************************************
 * @name Miscellaneous{{{
 *****************************************************************************/
/// @brief Unify the function signature macro between compilers.
#if X_MSVC || _X_MSVC_COMPAT
#define X_PRETTY_FUNCTION __FUNCSIG__
#else
#define X_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

#ifdef _X_MSVC_COMPAT
#undef _X_MSVC_COMPAT
#endif

#define X_INL inline

X_INL const char* _x_filename(const char* file)
{
  const char* sep = std::strrchr(file, '/');

#if X_WINDOWS
  const char* alt = std::strrchr(file, '\\');
  if (alt != nullptr && (sep == nullptr || alt > sep)) {
    sep = alt;
  }
#endif

  return sep == nullptr ? file : sep + 1;
}

/// @brief Get the base name from a full path.
/// @note Windows recognizes both `'/'` and `'\\'`; POSIX recognizes only
///       `'/'`.
#define X_FILENAME _x_filename(__FILE__)

// Shared Core helper for Logging and Date and Time.
X_INL const char* _x_timestamp(char* const buffer, const size_t size)
{
  if (buffer == nullptr || size == 0) {
    return "";
  }

  buffer[0] = '\0';
  if (size < 26) {
    return buffer;
  }

  const time_t now = std::time(nullptr);
  if (now == static_cast<time_t>(-1)) {
    return buffer;
  }

#if X_WINDOWS
  if (ctime_s(buffer, size, &now) != 0) {
#else
  if (ctime_r(&now, buffer) == nullptr) {
#endif
    buffer[0] = '\0';
    return buffer;
  }

  const size_t len = std::strlen(buffer);
  if (len > 0 && buffer[len - 1] == '\n') {
    buffer[len - 1] = '\0';
  }

  return buffer;
}
// Miscellaneous}}}

/******************************************************************************
 * @name DECL_Error_Handling{{{
 * @brief A collection of error handling utilities.
 *****************************************************************************/
/// @brief Assert an expression and abort on failure.
/// @param ... The required expression followed by an optional C string,
///            `nullptr`, or an empty string to omit the message.
/// @note The expression is evaluated once; the message is evaluated once only
///       when the assertion fails. Diagnostic text reflects macro expansion.
#define x_assert(...) \
  _X_ASSERT_SELECT(__VA_ARGS__, _X_ASSERT_TWO, _X_ASSERT_ONE, 0)(__VA_ARGS__)

/// @brief Evaluate a domain status-code or @ref x_error expression and report
///        failure.
/// @param domain The nonempty registered domain name for status codes;
///               ignored for @ref x_error results.
/// @param error A writable @ref x_error lvalue receiving the result.
/// @param expression A complete expression yielding a code fitting `int32_t`
///                   or @ref x_error.
/// @note All three arguments are evaluated once. Failures print to `stderr`
///       independently of Logging; Debug diagnostics include source context
///       and the expression text.
///       Status-code failures do not abort.
/// @attention Does not capture `errno`/`GetLastError()`. APIs returning
///            sentinel values require a failure-checking wrapper. Parenthesize
///            expressions containing macro-visible commas, including template
///            commas.
#define x_check(domain, error, expression) do { \
  (error) = _x_check_impl( \
      (domain), [&]() { return (expression); }, \
      X_FILENAME, #expression, static_cast<long long>(__LINE__)); \
} while (false)

/// @brief Total registry capacity, including built-in domains.
/// @note Use the same capacity and feature configuration in every translation
///       unit.
#ifndef X_ERROR_DOMAIN_LIMIT
#define X_ERROR_DOMAIN_LIMIT (16)
#endif

#if X_ERROR_DOMAIN_LIMIT \
  < (2 + !!(X_IOS | X_MACOS) + 3 * !!X_WINDOWS  + !!X_ENABLE_SOCKET \
    + !!X_ENABLE_CUDA_DRIVER + !!X_ENABLE_CUDA_RUNTIME + !!X_ENABLE_CUBLAS)
#error "X_ERROR_DOMAIN_LIMIT must accommodate all enabled built-in domains."
#endif

/// @brief Error interpretation and optional capture callbacks for a domain.
/// @note A `nullptr` `failed` callback uses `value != 0`; `nullptr` `capture`
///       means unsupported. `message` must terminate `buffer` when
///       `buffer != nullptr` and `size > 0`.
/// @attention Keep the domain, `name`, and callbacks valid and unchanged while
///            registered or referenced by an @ref x_error.
struct x_error_domain
{
  int32_t (*capture)();
  bool (*failed)(const int32_t value);
  char* (*message)(char* buffer, const size_t size, const int32_t value);

  const char* name;
};

/// @brief An error value that evaluates to `true` on failure.
/// @note All @ref x_error::set overloads return `*this` and clear the previous
///       message override unless a new one is supplied. See
///       @ref x_error_domain for lifetime requirements.
class x_error
{
public:
  /// @brief Construct a successful error value in the `posix` domain.
  X_INL x_error();

  /// @brief Capture the current error value for a registered domain name.
  /// @note See @ref x_error_builtin_domains for built-in domains and capture
  ///       behavior.
  /// @attention The name and capture requirements of `set(name)` apply.
  X_INL explicit x_error(const char* name);

  /// @brief Construct an error from a registered domain name and value.
  /// @note See @ref x_error_builtin_domains for built-in domain names.
  /// @attention `nullptr`/empty/unknown names print a diagnostic and abort.
  X_INL x_error(const char* name, const int32_t value);

  /// @brief Construct an error with a copied, user-supplied message.
  /// @param message A C string to override domain formatting; `nullptr`
  ///                selects domain formatting; an empty string is retained.
  /// @note See @ref x_error_builtin_domains for built-in domain names.
  /// @attention The same name requirements as `x_error(name, value)` apply.
  X_INL x_error(const char* name, const int32_t value, const char* message);

  /// @brief Construct an error referring directly to a domain and value.
  /// @note The domain need not be registered; its lifetime rules still apply.
  X_INL x_error(const x_error_domain& domain, const int32_t value);

  /// @brief Capture the current error value for a registered domain name.
  /// @anchor x_error_builtin_domains
  /// @note Built-in domains and capture availability:
  ///       - `"generic"`: always available; capture unsupported.
  ///       - `"posix"`: always available; captures `errno`.
  ///       - `"mach"`: iOS/macOS; capture unsupported.
  ///       - `"win32"`: Windows; captures `GetLastError()`.
  ///       - `"wincom"` (HRESULT): Windows; capture unsupported.
  ///       - `"winnt"` (NTSTATUS): Windows; capture unsupported.
  ///       - `"socket"`: requires `X_ENABLE_SOCKET`; captures
  ///         `WSAGetLastError()` on Windows and `errno` otherwise.
  ///       - `"cudadrv"`: requires `X_ENABLE_CUDA_DRIVER`; capture unsupported.
  ///       - `"cudart"`: requires `X_ENABLE_CUDA_RUNTIME`; captures
  ///         `cudaGetLastError()`, reading and clearing last-error.
  ///       - `"cublas"`: requires `X_ENABLE_CUBLAS`; capture unsupported.
  /// @note Capture `errno`/Win32/socket last-error state only after API
  ///       failure; it may be stale after success. CUDA Runtime capture reads
  ///       the current host thread's last-error in that Runtime instance and
  ///       resets it to `cudaSuccess`, without synchronizing GPU work.
  /// @attention `nullptr`/empty/unknown names or unsupported capture requests
  ///            print a diagnostic and abort.
  X_INL x_error& set(const char* name);

  /// @brief Set a registered domain name and value.
  /// @note See @ref x_error_builtin_domains for built-in domain names.
  /// @attention The same name requirements as `x_error(name, value)` apply.
  X_INL x_error& set(const char* name, const int32_t value);

  /// @brief Set a registered domain name, value, and copied message override.
  /// @note The name/message rules of the three-argument constructor apply.
  ///       See @ref x_error_builtin_domains for built-in domain names.
  X_INL x_error& set(const char* name, const int32_t value, const char* message);

  /// @brief Set a domain directly and a value.
  /// @note The domain need not be registered; its lifetime rules still apply.
  X_INL x_error& set(const x_error_domain& domain, const int32_t value);

  /// @brief Get the domain name, or `"generic"` when the domain has no name.
  X_INL const char* domain() const;

  /// @brief Get an owned message string.
  /// @note A message override takes precedence and is not truncated.
  ///       Otherwise, domain messages use 256 bytes including the terminator
  ///       and may truncate. Text remains valid until mutation or the next
  ///       no-argument @ref x_error::message() call. Concurrent no-argument
  ///       calls on this object are not safe.
  X_INL const char* message() const;

  /// @brief Format an error message into caller-provided storage.
  /// @param buffer Writable destination buffer.
  /// @param size Capacity in bytes, including the terminator.
  /// @return `buffer`. A `nullptr` `buffer` or zero `size` performs no write.
  /// @note Messages may be truncated to fit the buffer. Uses caller capacity,
  ///       not the no-argument limit, and does not mutate cached text. A
  ///       copied message override takes precedence over domain formatting.
  ///       Without an override, `"generic"` formats the numeric code without
  ///       a system message lookup.
  X_INL const char* message(char* buffer, const size_t size) const;

  /// @brief Get the domain status code.
  X_INL int32_t value() const;

  /// @brief Return whether this error value indicates failure.
  /// @note Uses the domain's predicate; a `nullptr` predicate uses
  ///       `value != 0`.
  X_INL operator bool() const;

  /// @brief Register a named user-defined error domain.
  /// @return 0 on success; `EINVAL` for a `nullptr` domain or `nullptr`/empty
  ///         name; `EEXIST` for a duplicate name (including built-ins);
  ///         `ENOSPC` when full.
  /// @attention Register before concurrent access; registration is not
  ///            synchronized. See @ref x_error_domain for lifetime
  ///            requirements.
  X_INL static int32_t register_domain(const x_error_domain* domain);

private:
  const x_error_domain* m_domain{nullptr};
  int32_t m_value{0};
  bool m_has_message{false};
  mutable std::string m_message;
};

/// @brief Construct a successful error value.
X_INL x_error x_ok();

/// @brief Return whether an error value indicates failure.
/// @note Uses the domain predicate; a missing predicate uses `value != 0`.
/// @note `"wincom"` uses `FAILED()` (so `S_FALSE` is successful); `"winnt"`
///       uses `!NT_SUCCESS()` (warnings count as failure); `"mach"` uses
///       `value != KERN_SUCCESS`. Other built-in domains use nonzero failure.
X_INL bool x_fail(const x_error& error);

/// @brief Return whether an error value indicates success.
X_INL bool x_succ(const x_error& error);
// DECL_Error_Handling}}}

// ***************************************************** IMPL_Error_Handling{{{
// x_error_common{{{
X_INL char* _x_error_copy(char* buffer, const size_t size, const char* text)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  const size_t len = std::strlen(text);
  const size_t cnt = len < size - 1 ? len : size - 1;
  if (cnt > 0) {
    std::memmove(buffer, text, cnt);
  }

  buffer[cnt] = '\0';

  return buffer;
}

X_INL char* _x_error_number(
    char* buffer, const size_t size, const char* prefix, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  // Format numeric fallbacks in sufficiently large storage, then truncate
  // explicitly. Small caller buffers are valid, including under `-Werror`.
  // Longest built-in prefix + space + INT32_MIN + terminator needs 30 bytes.
  char msg[32]{};
  std::snprintf(msg, sizeof(msg), "%s %d", prefix, value);

  return _x_error_copy(buffer, size, msg);
}

X_INL bool _x_error_failed_nonzero(const int32_t value)
{
  return value != 0;
}
// x_error_common}}}

// x_error_generic{{{
X_INL char* _x_error_message_generic(
    char* buffer, const size_t size, const int32_t value)
{
  return _x_error_number(buffer, size, "Generic error", value);
}

inline const x_error_domain error_domain_generic{
  nullptr,
  _x_error_failed_nonzero,
  _x_error_message_generic,
  "generic"
};
// x_error_generic}}}

// x_error_posix{{{
X_INL int32_t _x_error_capture_posix()
{
  return static_cast<int32_t>(errno);
}

X_INL char* _x_error_message_posix(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

#if X_WINDOWS
  char msg[256]{};
  if (strerror_s(msg, sizeof(msg), value) != 0 || msg[0] == '\0') {
    _x_error_number(buffer, size, "Unknown error", value);
  } else {
    _x_error_copy(buffer, size, msg);
  }
#else
  buffer[0] = '\0';
#if defined(_GNU_SOURCE) && (defined(__GLIBC__) || X_CYGWIN)
  // GNU strerror_r may return immutable text instead of using buffer.
  const char* msg = strerror_r(value, buffer, size);
  if (msg == nullptr || msg[0] == '\0') {
    _x_error_number(buffer, size, "Unknown error", value);
  } else if (msg != buffer) {
    _x_error_copy(buffer, size, msg);
  }
#else
  int perr = strerror_r(value, buffer, size);
  // An XSI ERANGE result need not leave a terminated buffer.
  buffer[size - 1] = '\0';
  if ((perr != 0 && perr != ERANGE) || buffer[0] == '\0') {
    _x_error_number(buffer, size, "Unknown error", value);
  }
#endif
#endif

  return buffer;
}

inline const x_error_domain error_domain_posix{
  _x_error_capture_posix,
  _x_error_failed_nonzero,
  _x_error_message_posix,
  "posix"
};
// x_error_posix}}}

// x_error_mach{{{
#if X_IOS || X_MACOS
X_INL bool _x_error_failed_mach(const int32_t value)
{
  return static_cast<kern_return_t>(value) != KERN_SUCCESS;
}

X_INL char* _x_error_message_mach(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  const char* msg = mach_error_string(static_cast<mach_error_t>(value));
  if (msg == nullptr || msg[0] == '\0') {
    return _x_error_number(buffer, size, "Mach error", value);
  }

  return _x_error_copy(buffer, size, msg);
}

inline const x_error_domain error_domain_mach{
  nullptr,
  _x_error_failed_mach,
  _x_error_message_mach,
  "mach"
};
#endif  // X_IOS || X_MACOS
// x_error_mach}}}

// x_error_windows{{{
#if X_WINDOWS
X_INL DWORD _x_error_message_windows(
    char* buffer, const size_t size, const DWORD flags, const void* source,
    const DWORD value)
{
  DWORD len = FormatMessageA(
      flags | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
      source, value, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), buffer,
      static_cast<DWORD>(size), nullptr);
  while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
    buffer[--len] = '\0';
  }

  return len;
}

X_INL int32_t _x_error_capture_win32()
{
  return static_cast<int32_t>(GetLastError());
}

X_INL char* _x_error_message_win32(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  DWORD len = _x_error_message_windows(
      buffer, size, FORMAT_MESSAGE_FROM_SYSTEM, nullptr,
      static_cast<DWORD>(value));
  if (len == 0) {
    _x_error_number(buffer, size, "Win32 error", value);
  }

  return buffer;
}

inline const x_error_domain error_domain_win32{
  _x_error_capture_win32,
  _x_error_failed_nonzero,
  _x_error_message_win32,
  "win32"
};

X_INL bool _x_error_failed_wincom(const int32_t value)
{
  return FAILED(static_cast<HRESULT>(value));
}

X_INL char* _x_error_message_wincom(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  DWORD len = _x_error_message_windows(
      buffer, size, FORMAT_MESSAGE_FROM_SYSTEM, nullptr,
      static_cast<DWORD>(value));
  if (len == 0
      && HRESULT_FACILITY(static_cast<HRESULT>(value)) == FACILITY_WIN32) {
    len = _x_error_message_windows(
        buffer, size, FORMAT_MESSAGE_FROM_SYSTEM, nullptr,
        static_cast<DWORD>(HRESULT_CODE(static_cast<HRESULT>(value))));
  }
  if (len == 0) {
    char msg[32]{};
    std::snprintf(
        msg, sizeof(msg), "HRESULT 0x%08x",
        static_cast<unsigned int>(static_cast<uint32_t>(value)));
    _x_error_copy(buffer, size, msg);
  }

  return buffer;
}

inline const x_error_domain error_domain_wincom{
  nullptr,
  _x_error_failed_wincom,
  _x_error_message_wincom,
  "wincom"
};

X_INL bool _x_error_failed_winnt(const int32_t value)
{
  return !NT_SUCCESS(static_cast<NTSTATUS>(value));
}

X_INL char* _x_error_message_winnt(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  HMODULE module = GetModuleHandleA("ntdll.dll");
  if (module == nullptr
      || _x_error_message_windows(
        buffer, size, FORMAT_MESSAGE_FROM_HMODULE, module,
        static_cast<DWORD>(value)) == 0) {
    char msg[32]{};
    std::snprintf(
        msg, sizeof(msg), "NTSTATUS 0x%08x",
        static_cast<unsigned int>(static_cast<uint32_t>(value)));
    _x_error_copy(buffer, size, msg);
  }

  return buffer;
}

inline const x_error_domain error_domain_winnt{
  nullptr,
  _x_error_failed_winnt,
  _x_error_message_winnt,
  "winnt"
};
#endif  // X_WINDOWS
// x_error_windows}}}

// x_error_socket{{{
#if X_ENABLE_SOCKET
X_INL int32_t _x_error_capture_socket()
{
#if X_WINDOWS
  return static_cast<int32_t>(WSAGetLastError());
#else
  return static_cast<int32_t>(errno);
#endif
}

X_INL char* _x_error_message_socket(
    char* buffer, const size_t size, const int32_t value)
{
#if X_WINDOWS
  return _x_error_message_win32(buffer, size, value);
#else
  return _x_error_message_posix(buffer, size, value);
#endif
}

inline const x_error_domain error_domain_socket{
  _x_error_capture_socket,
  _x_error_failed_nonzero,
  _x_error_message_socket,
  "socket"
};
#endif  // X_ENABLE_SOCKET
// x_error_socket}}}

// x_error_cuda_driver{{{
#if X_ENABLE_CUDA_DRIVER
X_INL bool _x_error_failed_cuda_driver(const int32_t value)
{
  return static_cast<CUresult>(value) != CUDA_SUCCESS;
}

X_INL char* _x_error_message_cuda_driver(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  const char* msg{nullptr};
  CUresult cres = cuGetErrorString(static_cast<CUresult>(value), &msg);
  if (cres != CUDA_SUCCESS || msg == nullptr) {
    _x_error_number(buffer, size, "CUDA driver error", value);
    return buffer;
  }

  _x_error_copy(buffer, size, msg);

  return buffer;
}

inline const x_error_domain error_domain_cuda_driver{
  nullptr,
  _x_error_failed_cuda_driver,
  _x_error_message_cuda_driver,
  "cudadrv"
};
#endif  // X_ENABLE_CUDA_DRIVER
// x_error_cuda_driver}}}

// x_error_cuda_runtime{{{
#if X_ENABLE_CUDA_RUNTIME
X_INL int32_t _x_error_capture_cuda_runtime()
{
  return static_cast<int32_t>(cudaGetLastError());
}

X_INL bool _x_error_failed_cuda_runtime(const int32_t value)
{
  return static_cast<cudaError_t>(value) != cudaSuccess;
}

X_INL char* _x_error_message_cuda_runtime(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  const char* msg = cudaGetErrorString(static_cast<cudaError_t>(value));
  _x_error_copy(buffer, size, msg == nullptr ? "CUDA runtime error" : msg);

  return buffer;
}

inline const x_error_domain error_domain_cuda_runtime{
  _x_error_capture_cuda_runtime,
  _x_error_failed_cuda_runtime,
  _x_error_message_cuda_runtime,
  "cudart"
};
#endif  // X_ENABLE_CUDA_RUNTIME
// x_error_cuda_runtime}}}

// x_error_cublas{{{
#if X_ENABLE_CUBLAS
X_INL bool _x_error_failed_cublas(const int32_t value)
{
  return static_cast<cublasStatus_t>(value) != CUBLAS_STATUS_SUCCESS;
}

X_INL char* _x_error_message_cublas(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  const char* msg = cublasGetStatusString(static_cast<cublasStatus_t>(value));
  _x_error_copy(buffer, size, msg == nullptr ? "cuBLAS error" : msg);

  return buffer;
}

inline const x_error_domain error_domain_cublas{
  nullptr,
  _x_error_failed_cublas,
  _x_error_message_cublas,
  "cublas"
};
#endif  // X_ENABLE_CUBLAS
// x_error_cublas}}}

struct x_error_domain_registry
{
  const x_error_domain* list[X_ERROR_DOMAIN_LIMIT];
  size_t count;
};

inline x_error_domain_registry error_domain_registry{
  {
    &error_domain_generic,
    &error_domain_posix,
#if X_IOS || X_MACOS
    &error_domain_mach,
#endif
#if X_WINDOWS
    &error_domain_win32,
    &error_domain_wincom,
    &error_domain_winnt,
#endif
#if X_ENABLE_SOCKET
    &error_domain_socket,
#endif
#if X_ENABLE_CUDA_DRIVER
    &error_domain_cuda_driver,
#endif
#if X_ENABLE_CUDA_RUNTIME
    &error_domain_cuda_runtime,
#endif
#if X_ENABLE_CUBLAS
    &error_domain_cublas,
#endif
  },
  static_cast<size_t>(
      2 + !!(X_IOS | X_MACOS) + 3 * !!X_WINDOWS + !!X_ENABLE_SOCKET
      + !!X_ENABLE_CUDA_DRIVER + !!X_ENABLE_CUDA_RUNTIME + !!X_ENABLE_CUBLAS)
};

X_INL x_error x_ok()
{
  return x_error{};
}

X_INL bool x_fail(const x_error& error)
{
  return static_cast<bool>(error);
}

X_INL bool x_succ(const x_error& error)
{
  return !x_fail(error);
}

// x_error_domain{{{
X_INL const x_error_domain* _x_error_domain_lookup(const char* name)
{
  if (name == nullptr || name[0] == '\0') {
    return nullptr;
  }

  const x_error_domain* domain{nullptr};
  for (size_t i{0}; i < error_domain_registry.count; ++i) {
    domain = error_domain_registry.list[i];
    if (domain != nullptr && domain->name != nullptr
        && std::strcmp(name, domain->name) == 0) {
      return domain;
    }
  }

  return nullptr;
}

X_INL const x_error_domain* _x_error_domain_require(const char* name)
{
  const auto* domain = _x_error_domain_lookup(name);
  if (domain == nullptr) {
    std::fprintf(
        stderr, "Unknown error domain: \"%s\".\n",
        name == nullptr ? "(null)" : name);
    std::abort();
  }

  return domain;
}

X_INL char* _x_error_domain_message(
    char* buffer, const size_t size, const int32_t value,
    const x_error_domain* domain)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  if (domain == nullptr || domain->message == nullptr) {
    return _x_error_number(buffer, size, "Unknown error", value);
  }

  return domain->message(buffer, size, value);
}
// x_error_domain}}}

// x_error_set{{{
X_INL x_error::x_error()
  :m_domain{&error_domain_posix}
{
}

X_INL x_error::x_error(const char* name)
{
  this->set(name);
}

X_INL x_error::x_error(const char* name, const int32_t value)
{
  this->set(name, value);
}

X_INL x_error::x_error(const x_error_domain& domain, const int32_t value)
{
  this->set(domain, value);
}

X_INL x_error::x_error(
    const char* name, const int32_t value, const char* message)
{
  this->set(name, value, message);
}

X_INL x_error& x_error::set(const char* name)
{
  const auto* domain = _x_error_domain_require(name);
  if (domain->capture == nullptr) {
    std::fprintf(
        stderr, "Error domain \"%s\" does not support error capturing.\n",
        domain->name);
    std::abort();
  }

  // Capture before touching string storage or doing any further library work.
  return this->set(*domain, domain->capture());
}

X_INL x_error& x_error::set(const char* name, const int32_t value)
{
  return this->set(*_x_error_domain_require(name), value);
}

X_INL x_error& x_error::set(const x_error_domain& domain, const int32_t value)
{
  this->m_domain = &domain;
  this->m_value = value;
  this->m_has_message = false;
  this->m_message.clear();

  return *this;
}

X_INL x_error& x_error::set(
    const char* name, const int32_t value, const char* message)
{
  // Copy first: message may refer to this value's previous message() text.
  std::string msg = message == nullptr ? "" : message;
  this->set(name, value);
  this->m_message = std::move(msg);
  this->m_has_message = message != nullptr;

  return *this;
}
// x_error_set}}}

X_INL const char* x_error::domain() const
{
  if (this->m_domain != nullptr && this->m_domain->name != nullptr) {
    return this->m_domain->name;
  } else {
    return "generic";
  }
}

X_INL const char* x_error::message() const
{
  if (!this->m_has_message) {
    char buf[256]{};
    this->message(buf, sizeof(buf));
    this->m_message = buf;
  }

  return this->m_message.c_str();
}

X_INL const char* x_error::message(char* buffer, const size_t size) const
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  if (this->m_has_message) {
    _x_error_copy(buffer, size, this->m_message.c_str());
  } else {
    const auto* domain =
      this->m_domain == nullptr ? &error_domain_generic : this->m_domain;
    _x_error_domain_message(buffer, size, this->m_value, domain);
  }

  return buffer;
}

X_INL int32_t x_error::value() const
{
  return this->m_value;
}

X_INL x_error::operator bool() const
{
  if (this->m_domain != nullptr && this->m_domain->failed != nullptr) {
    return this->m_domain->failed(this->m_value);
  } else {
    return this->m_value != 0;
  }
}

X_INL int32_t x_error::register_domain(const x_error_domain* domain)
{
  if (domain == nullptr || domain->name == nullptr || domain->name[0] == '\0') {
    return EINVAL;
  }

  if (_x_error_domain_lookup(domain->name) != nullptr) {
    return EEXIST;
  }

  if (error_domain_registry.count >= X_ERROR_DOMAIN_LIMIT) {
    return ENOSPC;
  }

  error_domain_registry.list[error_domain_registry.count++] = domain;

  return 0;
}

// Support optional assertion messages in strict C++17. The trailing dummy
// argument in x_assert keeps the selector's variadic tail nonempty.
#define _X_ASSERT_SELECT(_1, _2, function, ...) function
#define _X_ASSERT_ONE(expression) _X_ASSERT_TWO(expression, nullptr)
#define _X_ASSERT_TWO(expression, message) do { \
  if (!(expression)) { \
    _x_assert_fail( \
        #expression, (message), X_FILENAME, static_cast<long long>(__LINE__), \
        X_PRETTY_FUNCTION); \
  } \
} while (false)

X_INL void _x_assert_fail(
    const char* expression, const char* message,
    const char* file, const long long line, const char* function)
{
  std::fprintf(stderr, "Assertion failed: %s\n", expression);

  if (message != nullptr && message[0] != '\0') {
    std::fprintf(stderr, "Message: %s\n", message);
  }

  std::fprintf(stderr, "Position: %s:%lld: %s\n", file, line, function);

  std::abort();
}

template<typename Callable>
X_INL x_error _x_check_impl(
    const char* domain, Callable&& evaluate,
    const char* file, const char* expression, const long long line)
{
  x_error err;
  if constexpr (std::is_same_v<std::decay_t<decltype(evaluate())>, x_error>) {
    err = evaluate();
  } else {
    err.set(domain, static_cast<int32_t>(evaluate()));
  }

  if (x_fail(err)) {
#ifdef NDEBUG
    (void)file;
    (void)expression;
    (void)line;
    std::fprintf(stderr, "Check failed: %s\n", err.message());
#else
    std::fprintf(
        stderr, "Check failed: %s\nPosition: %s:%lld: %s\n",
        err.message(), file, line, expression);
#endif
  }

  return err;
}
// IMPL_Error_Handling}}}


#endif  // X_H
