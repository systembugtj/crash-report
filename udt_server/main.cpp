#define NOGDI
#include "udt_server.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <iostream>
#include "udt/udt.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

using namespace std;
using namespace google;

DEFINE_string(port, "8080", "port to bind for server");
DEFINE_string(save_dir, "", "dir to save the crash reports");

int main(int argc, char* argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, false);
  CrashReportUdtServer server(FLAGS_port, FLAGS_save_dir);
  server.Run();
  return 0;
}
