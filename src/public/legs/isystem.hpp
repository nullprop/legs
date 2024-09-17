#pragma once

namespace legs
{
class ISystem
{
  public:
    ISystem()          = default;
    virtual ~ISystem() = default;

    ISystem(const ISystem&)            = delete;
    ISystem(ISystem&&)                 = delete;
    ISystem& operator=(const ISystem&) = delete;
    ISystem& operator=(ISystem&&)      = delete;

    virtual void OnLevelLoad() {};
    virtual void OnFrame() {};
    virtual void OnTick() {};
};
}; // namespace legs
