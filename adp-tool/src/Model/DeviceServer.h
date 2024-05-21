#pragma once

#include "Reporter.h"

namespace adp {

class DeviceServer {
public:
    DeviceServer(ReporterBackend& device):device(device){}
    virtual ~DeviceServer(){};

protected:
    ReporterBackend& device;
};

DeviceServer* DeviceServerCreate(ReporterBackend& device);

};