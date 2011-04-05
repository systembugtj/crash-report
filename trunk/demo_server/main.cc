//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)

#include "server.h"
#include "init_socket.h"
#include <string>

CInitSock initSock;
using namespace std;

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, false);
  CrashReportServer server;
  server.Run();
  return 0;
}
