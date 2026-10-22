#ifndef IMP_DETAIL_MACOS_ERROR_H
#define IMP_DETAIL_MACOS_ERROR_H


#include "x.h"

#include <CoreServices/CoreServices.h>


namespace imp
{
namespace detail
{
X_INL char* osstatus_message(
    char* buffer, const size_t size, const int32_t value)
{
  if (buffer == nullptr || size == 0) {
    return buffer;
  }

  const char* msg = GetMacOSStatusCommentString(static_cast<OSStatus>(value));
  if (msg == nullptr || msg[0] == '\0') {
    msg = GetMacOSStatusErrorString(static_cast<OSStatus>(value));
  }

  if (msg == nullptr || msg[0] == '\0') {
    std::snprintf(buffer, size, "OSStatus %d", static_cast<int>(value));
  } else {
    std::snprintf(buffer, size, "%s", msg);
  }

  return buffer;
}

// Referenced directly by x_error; no registry initialization is needed.
X_INL const x_error_domain osstatus_domain{
  nullptr,
  nullptr,
  osstatus_message,
  "osstatus",
};
}  // namespace detail
}  // namespace imp


#endif  // IMP_DETAIL_MACOS_ERROR_H
