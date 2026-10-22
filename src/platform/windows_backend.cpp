#include "imp/backend.h"


namespace imp
{
namespace
{
class WindowsBackend final : public Backend
{
public:
  x_error set_mode(const Mode) override
  {
    return x_error("posix", ENOSYS, "The target-thread TSF backend is not implemented yet");
  }

  Status status() const override
  {
    Status result;
    result.backend = "windows-tsf";
    result.error.set("posix", ENOSYS, "Windows input-method status queries are not implemented yet");
    return result;
  }
};  // class WindowsBackend
}  // namespace

std::unique_ptr<Backend> create_backend()
{
  return std::make_unique<WindowsBackend>();
}
}  // namespace imp
