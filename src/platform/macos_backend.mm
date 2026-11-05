#include "imp/backend.h"
#include "detail/macos_error.h"
#include "detail/macos_source.h"

#include <Carbon/Carbon.h>
#include <mach-o/dyld.h>

#include <filesystem>
#include <utility>


namespace imp
{
namespace
{
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

x_error default_config_path(std::string& path)
{
  uint32_t size = 1024;
  std::vector<char> buffer(size);
  if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
    buffer.resize(size);
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
      return x_error("posix", EIO, "Could not locate the executable for configuration discovery");
    }
  }
  std::error_code error;
  const auto executable = std::filesystem::canonical(buffer.data(), error);
  if (error) {
    return x_error("posix", error.value(), "Could not resolve the executable for configuration discovery");
  }
  path = (executable.parent_path() / "imp.conf").string();
  return x_ok();
}

x_error select_source(const char* source_id)
{
  const CFStringRef id = CFStringCreateWithCString(nullptr, source_id, kCFStringEncodingUTF8);
  if (id == nullptr) {
    return x_error("posix", EINVAL, "Input source ID is not valid UTF-8");
  }
  const void* keys[]{kTISPropertyInputSourceID};
  const void* values[]{id};
  CFDictionaryRef filter = CFDictionaryCreate(nullptr, keys, values, 1,
    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  CFRelease(id);
  if (filter == nullptr) {
    return x_error("posix", ENOMEM, "Could not create an input-source filter");
  }

  // Query enabled sources first; enumerate installed sources only for this target.
  CFArrayRef sources = TISCreateInputSourceList(filter, false);
  if (sources != nullptr && CFArrayGetCount(sources) == 0) {
    CFRelease(sources);
    sources = TISCreateInputSourceList(filter, true);
  }
  CFRelease(filter);
  if (sources == nullptr) {
    return x_error("posix", EIO, "macOS did not return available input sources");
  }

  x_error err;
  if (CFArrayGetCount(sources) == 0) {
    const std::string message = std::string("Input source was not found among installed sources: ") + source_id;
    err.set("posix", ENOENT, message.c_str());
  } else {
    TISInputSourceRef src = static_cast<TISInputSourceRef>(
        const_cast<void*>(CFArrayGetValueAtIndex(sources, 0)));
    err = detail::source_availability(source_id,
      TISGetInputSourceProperty(src, kTISPropertyInputSourceIsEnabled) == kCFBooleanTrue,
      TISGetInputSourceProperty(src, kTISPropertyInputSourceIsSelectCapable) == kCFBooleanTrue);
    if (!err) {
      err.set(detail::osstatus_domain, TISSelectInputSource(src));
    }
  }

  CFRelease(sources);

  return err;
}

class MacOSBackend final : public Backend
{
public:
  x_error load_config(const std::string& path) override
  {
    std::string resolved = path;
    if (resolved.empty()) {
      const x_error error = default_config_path(resolved);
      if (error) {
        return error;
      }
    }
    std::vector<InputSourcePair> pairs;
    const x_error error = load_pairs(resolved, pairs);
    if (error) {
      if (!path.empty() || error.value() != ENOENT) {
        return error;
      }
      pairs = detail::k_supported;
    }
    this->m_pairs = std::move(pairs);
    this->m_pair = &this->m_pairs.front();
    return x_ok();
  }

  x_error configure_pair(const std::string& name) override
  {
    const auto* pair = find_pair(this->m_pairs, name);
    if (pair == nullptr) {
      const std::string message = "Unknown macOS input-source pair: " + name;
      return x_error("posix", EINVAL, message.c_str());
    }
    this->m_pair = pair;
    return x_ok();
  }

  x_error set_mode(const Mode mode) override
  {
    if (mode != Mode::Native && mode != Mode::Alphanumeric) {
      return x_error("posix", EINVAL, "mode must be 'native' or 'alphanumeric'");
    }

    const char* source_id = detail::mode_to_source(*this->m_pair, mode);
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
      res.mode = detail::source_to_mode(*this->m_pair, res.source_id);
    }

    return res;
  }

private:
  std::vector<InputSourcePair> m_pairs{detail::k_supported};
  const InputSourcePair* m_pair{&this->m_pairs.front()};
};  // class MacOSBackend
}  // namespace

std::unique_ptr<Backend> create_backend()
{
  return std::make_unique<MacOSBackend>();
}
}  // namespace imp
