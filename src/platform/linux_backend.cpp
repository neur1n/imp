#include "imp/backend.h"


namespace imp
{
namespace
{
class LinuxBackend final : public Backend
{
public:
  x_error set_mode(const Mode) override
  {
    return x_error("posix", ENOSYS, "Direct IBus/Fcitx5 mode control is not implemented yet");
  }

  Status status() const override
  {
    Status result;
    result.backend = "linux-dbus";
    result.error.set("posix", ENOSYS, "Linux input-method status queries are not implemented yet");
    return result;
  }
};
}  // namespace

std::unique_ptr<Backend> create_backend()
{
  return std::make_unique<LinuxBackend>();
}
}  // namespace imp
