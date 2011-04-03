#include "init_socket.h"
#include <stdio.h>
#include <string>

class CrashReportServer {
public:
  CrashReportServer();
  ~CrashReportServer();
  void Run();
private:
  bool CreateListenSocket();
  void ReceiveCrashReport(SOCKET client);
  bool WriteStringToFile(const std::string& content,
                         const std::string& filename);
  SOCKET listen_socket_;
};