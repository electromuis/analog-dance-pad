#pragma once

#include "hidapi.h"

namespace adp {

class DeviceServer {
public:
    DeviceServer(hid_device* device):device(device){}
    virtual ~DeviceServer(){};

protected:
    hid_device* device;
};

DeviceServer* DeviceServerCreate(hid_device* device);

};