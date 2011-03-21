//  Copyright 2011.
//  Author: yeshunping@gmail.com (Shunping Ye)

//  demo how to use crash_report in a application
//  FIXME(yesp) : debug 版本Install总是失败，而Release版本却可以。
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <assert.h>
#include <process.h>
#include <string>
#include "crash_report.h"
#include "glog/logging.h"

int main(int argc, char* argv[]) {
  if (argc == 2 && std::string(argv[1]) == "/restart") {
    printf("restart!\n");
    return -1;
  }

  CrAutoInstallHelper helper;
  helper.set_application_name(_T("Test Application"));
  helper.set_application_version(_T("0.1.1"));
  helper.set_flags(CR_INST_ALL_EXCEPTION_HANDLERS | CR_INST_HTTP_BINARY_ENCODING |
      CR_INST_SEND_QUEUED_REPORTS | CR_INST_APP_RESTART);
  helper.set_crash_server_url(_T("http://localhost:8080/crashrpt.php"));
  helper.set_restart_cmd(_T("/restart"));

  if (helper.Install() != 0) {
    LOG(ERROR) << "fail to install crash_report for this application";
    char reason[1000];
    crGetLastErrorMsgA(reason, 1000);
    LOG(ERROR) << "reason:" << reason;
    return -1;
  }
  LOG(INFO) << "install crash_report successfully";
  printf("Press Enter to simulate a null pointer exception or any other key to exit...\n");
  int n = _getch();
  if (n == 13) {
    int *p = 0;
    *p = 0;
  }
  return 0;
}
