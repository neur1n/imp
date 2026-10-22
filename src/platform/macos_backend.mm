#include "imp/backend.h"
#include "detail/macos_error.h"

#include <Carbon/Carbon.h>


namespace imp
{
namespace
{
struct InputSourcePair
{
  const char* alphanumeric;
  const char* native;
};

constexpr InputSourcePair k_supported[]{
  {"com.apple.keylayout.ABC", "com.tencent.inputmethod.wetype.pinyin"},
};

std::string copy_cf_string(CFTypeRef value)
{
  if (value == nullptr || CFGetTypeID(value) != CFStringGetTypeID()) {
    return {};
  }

  const auto str = static_cast<CFStringRef>(value);
  const CFIndex capacity = CFStringGetMaximumSizeForEncoding(
      CFStringGetLength(str), kCFStringEncodingUTF8) + 1;
  std::string res(static_cast<size_t>(capacity), '\0');
  if (!CFStringGetCString(str, res.data(), capacity, kCFStringEncodingUTF8)) {
    return {};
  }

  res.resize(std::strlen(res.c_str()));

  return res;
}

std::string current_source_id()
{
  TISInputSourceRef src{TISCopyCurrentKeyboardInputSource()};
  if (src == nullptr) {
    return {};
  }

  const std::string id = copy_cf_string(
      TISGetInputSourceProperty(src, kTISPropertyInputSourceID));

  CFRelease(src);

  return id;
}

const InputSourcePair* find_pair(const std::string& source_id)
{
  for (const InputSourcePair& pair : k_supported) {
    if (source_id == pair.alphanumeric || source_id == pair.native) {
      return &pair;
    }
  }

  return nullptr;
}

const char* mode_to_source(const InputSourcePair& pair, const Mode mode)
{
  switch (mode) {
    case Mode::Native:
      return pair.native;
    case Mode::Alphanumeric:
      return pair.alphanumeric;
    case Mode::Unknown:
      return nullptr;
  }

  return nullptr;
}

x_error select_source(const char* source_id)
{
  x_error err;

  CFArrayRef sources = TISCreateInputSourceList(nullptr, false);
  if (sources == nullptr) {
    return err.set("posix", EIO, "macOS did not return available input sources");
  }

  err.set("posix", ENOENT, "The requested input source was not found");

  const CFIndex cnt = CFArrayGetCount(sources);
  for (CFIndex idx = 0; idx < cnt; ++idx) {
    TISInputSourceRef src = static_cast<TISInputSourceRef>(
        const_cast<void*>(CFArrayGetValueAtIndex(sources, idx)));
    const std::string current = copy_cf_string(
       TISGetInputSourceProperty(src, kTISPropertyInputSourceID));

    if (current == source_id) {
      err.set(detail::osstatus_domain, TISSelectInputSource(src));
      break;
    }
  }

  CFRelease(sources);

  return err;
}

Mode source_to_mode(const std::string& source_id)
{
  const InputSourcePair* pair = find_pair(source_id);
  if (pair == nullptr) {
    return Mode::Unknown;
  }

  if (source_id == pair->native) {
    return Mode::Native;
  }

  return Mode::Alphanumeric;
}

class MacOSBackend final : public Backend
{
public:
  x_error set_mode(const Mode mode) override
  {
    if (mode != Mode::Native && mode != Mode::Alphanumeric) {
      return x_error("posix", EINVAL, "mode must be 'native' or 'alphanumeric'");
    }

    const InputSourcePair* pair = find_pair(current_source_id());
    if (pair == nullptr) {
      pair = &k_supported[0];
    }

    const char* source_id = mode_to_source(*pair, mode);
    return select_source(source_id);
  }

  Status status() const override
  {
    Status res;
    res.backend = "macos-tis";

    TISInputSourceRef src{TISCopyCurrentKeyboardInputSource()};
    if (src == nullptr) {
      res.error.set("posix", EIO, "macOS did not return a current input source");
      return res;
    }

    res.source_id = copy_cf_string(
        TISGetInputSourceProperty(src, kTISPropertyInputSourceID));
    res.source_name = copy_cf_string(
        TISGetInputSourceProperty(src, kTISPropertyLocalizedName));

    CFRelease(src);

    if (res.source_id.empty()) {
      res.error.set("posix", EIO, "macOS did not return a valid input source ID");
    } else {
      res.mode = source_to_mode(res.source_id);
    }

    return res;
  }
};  // class MacOSBackend
}  // namespace

std::unique_ptr<Backend> create_backend()
{
  return std::make_unique<MacOSBackend>();
}
}  // namespace imp
