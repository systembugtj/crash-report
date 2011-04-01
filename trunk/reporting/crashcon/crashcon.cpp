//  Copyright 2011.
//  Author: yeshunping@gmail.com (Shunping Ye)

//  demo how to use crash_report in a application
//  FIXME(yesp) : debug 版本Install总是失败，而Release版本却可以。
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <tchar.h>
#include <assert.h>
#include <process.h>
#include <string>
#include "crash_report_helper.h"
#include "glog/logging.h"

// crt_atexit.c
#include <stdlib.h>
#include <stdio.h>

//  if last time has crash ,then print a warning.
void SimpleCrashCallback(vector<pair<time_t, bool>>& logdata,
                         int type) {
  LOG(WARNING) <<"some errors happend last time ,software should check for its"
                 " healthy here...";
  if (type & crash_report::kCrashLastestTime) {
    LOG(INFO) << "reason : " << "software crash last time";
  } 
  if (type & crash_report::kCrashTooOfen) {
    LOG(INFO) << "reason : " << "software crash too ofen these days";
  }
  //LOG(INFO) << "below is the running history:";
  //for (int i = 0; i < logdata.size(); ++i) {
  //  LOG(INFO) << "time:" << ctime(&logdata[i].first) << ", "
  //            << logdata[i].second << endl;
  //}
}

int main(int argc, char* argv[]) {
  if (argc == 2 && std::string(argv[1]) == "/restart") {
    LOG(INFO) << "restart!";
    return -1;
  }
  //  not ,helper object must create in main function.
  crash_report::CrAutoInstallHelper helper;
  helper.CheckHealthyBeforeStart(SimpleCrashCallback);
  helper.set_application_name(_T("Test Application"));
  helper.set_application_version(_T("0.1.1"));
  helper.set_flags(CR_INST_ALL_EXCEPTION_HANDLERS
      | CR_INST_SEND_QUEUED_REPORTS
      | CR_INST_APP_RESTART);
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
  LOG(INFO) << "Press Enter to simulate a null pointer exception or"
               " any other key to exit...";
  int n = _getch();
  if (n == 13) {
    int *p = 0;
    *p = 0;
  }
  return 0;
}
