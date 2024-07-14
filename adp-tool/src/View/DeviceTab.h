#pragma once

#include <Adp.h>

namespace adp {

class DeviceTab
{
public:
    void Render();
    void RenderFlash();
    void OpenFlashDialog(bool advanced);
};

}; // namespace adp.
