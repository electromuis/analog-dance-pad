#pragma once

#include <string>

namespace adp {

class ReporterBackend;
// ReporterBackend* ReporterBackendTcpCreate(std::string host, std::string port);
ReporterBackend* ReporterBackendWsCreate(std::string url);

};