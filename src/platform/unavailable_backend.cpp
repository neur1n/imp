#include "imp/backend.h"


namespace imp
{
namespace
{
class UnavailableBackend final : public Backend
{
public:
  x_error set_mode(const Mode) override
  {
    return x_error("posix", ENOSYS, "This platform has no backend yet.");
  }

  Status status() const override
  {
    Status result;
    result.backend = "unavailable";
    result.error.set("posix", ENOSYS, "This platform has no backend yet.");
    return result;
  }
};
}  // namespace

std::unique_ptr<Backend> create_backend()
{
  return std::make_unique<UnavailableBackend>();
}
}  // namespace imp
