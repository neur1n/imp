#ifndef IMP_DETAIL_MACOS_SOURCE_H
#define IMP_DETAIL_MACOS_SOURCE_H

#include "imp/backend.h"
#include "imp/config.h"

#include <string_view>


namespace imp
{
namespace detail
{
inline const std::vector<InputSourcePair> k_supported{
  {"wetype", "com.apple.keylayout.ABC", "com.tencent.inputmethod.wetype.pinyin"},
  {"pinyin", "com.apple.keylayout.ABC", "com.apple.inputmethod.SCIM.ITABC"},
  {"wubi", "com.apple.keylayout.ABC", "com.apple.inputmethod.SCIM.WBX"},
};

inline const InputSourcePair* find_pair(const std::string_view name)
{
  return imp::find_pair(k_supported, name);
}

inline const char* mode_to_source(const InputSourcePair& pair, const Mode mode)
{
  switch (mode) {
    case Mode::Native:
      return pair.native.c_str();
    case Mode::Alphanumeric:
      return pair.alphanumeric.c_str();
    case Mode::Unknown:
      return nullptr;
  }

  return nullptr;
}

inline Mode source_to_mode(const InputSourcePair& pair, const std::string& source_id)
{
  if (source_id == pair.native) {
    return Mode::Native;
  }

  if (source_id == pair.alphanumeric) {
    return Mode::Alphanumeric;
  }

  return Mode::Unknown;
}

inline x_error source_availability(const std::string& source_id, bool enabled, bool selectable)
{
  if (!enabled) {
    const std::string message = "Input source is installed but not enabled: " + source_id;
    return x_error("posix", EACCES, message.c_str());
  }
  if (!selectable) {
    const std::string message = "Input source is not selectable: " + source_id;
    return x_error("posix", ENOTSUP, message.c_str());
  }
  return x_ok();
}
}  // namespace detail
}  // namespace imp


#endif  // IMP_DETAIL_MACOS_SOURCE_H
