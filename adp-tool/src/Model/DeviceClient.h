#pragma once

#include <string>
#include <cstdint>

namespace adp {

    enum WS_CMD {
    CMD_FAILED = 0x00,

    CMD_REPORT_SEND = 0x01,
    CMD_REPORT_GET  = 0x02,
    CMD_DATA_READ   = 0x03,

    CMD_REPORT_SEND_ACK = 0x04,
    CMD_REPORT_GET_ACK  = 0x05,
    CMD_DATA_READ_ACK   = 0x06
};

struct ReportHeader {
    uint8_t cmd;
    uint8_t reportId;
    uint16_t length;
};

class ReportPacketBase {
public:
    ReportPacketBase()
    {
        header.cmd = CMD_FAILED;
        header.reportId = 0;
        header.length = 0;
    }

    bool FromString(std::string s)
    {
        int leng = s.length();
        uint8_t* ptr = (uint8_t*)s.c_str();

        if (leng < sizeof(ReportHeader))
            return false;

        header = *(ReportHeader*)ptr;
        ptr += sizeof(ReportHeader);
        leng -= sizeof(ReportHeader);

        if (leng != header.length)
            return false;

        data = std::string((char*)ptr, leng);

        return true;
    }
    
    ReportHeader header;
    std::string data = "";
};

class ReporterBackend;
// ReporterBackend* ReporterBackendTcpCreate(std::string host, std::string port);
ReporterBackend* ReporterBackendWsCreate(std::string url);

};
