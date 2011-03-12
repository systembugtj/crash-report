//  demo how to use crash_report in a application

#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <assert.h>
#include <process.h>
#include "crash_report.h"

int main(int argc, char* argv[]) {
	CrAutoInstallHelper helper;
	helper.set_application_name(_T("Test Application"));
	helper.set_application_version(_T("0.1.1"));
	helper.set_email_address(_T("CrashRpt Console Test"));
	helper.set_email_address(_T("yeshunping@gmail.com"));
	helper.set_crash_server_url(_T("http://localhost:8080/crashrpt.php"));
  if(helper.Install() != 0) {
    printf("fail to install crash_report for this application");
    return -1;
  }

	printf("Press Enter to simulate a null pointer exception or any other key to exit...\n");
	int n = _getch();
	if (n == 13) {
		int *p = 0;
		*p = 0;
	}
	return 0;
}

