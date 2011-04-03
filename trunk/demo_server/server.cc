//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)
#define NOGDI  //  to use glog
#include "server.h"

#include <map>

#include "gflags/gflags.h"
#include "glog/logging.h"
#include <stdio.h>

using namespace std;
using namespace google;

DEFINE_int32(port, 8080, "the port to bind");
DEFINE_string(save_dir, "", "dir path to save the received crash report");

CrashReportServer::CrashReportServer() {
}
CrashReportServer::~CrashReportServer() {
}

bool CrashReportServer::CreateListenSocket() {
  LOG(INFO) << "server starting....";
  listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  CHECK(listen_socket_ != INVALID_SOCKET)<< "fail to create a socket";
  sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons(FLAGS_port);
  sin.sin_addr.S_un.S_addr = INADDR_ANY;
  if (bind(listen_socket_, (LPSOCKADDR) & sin, sizeof(sin)) == SOCKET_ERROR) {
    LOG(ERROR) << "fail to bind to port:" << FLAGS_port;
    return false;
  }
  LOG(ERROR) << "bind to port:" << FLAGS_port;
  LOG(ERROR) << "begin to accept request:" << FLAGS_port;

  if (listen(listen_socket_, 2) == SOCKET_ERROR) {
    LOG(INFO) << "fail to listen";
    return false;
  }
  return true;
}

bool CrashReportServer::WriteStringToFile(const string& content,
                                          const string& filename) {
  // TODO(yesp) : get dir path.
  //  get the abs file path
  FILE* fp = fopen(filename.c_str(), "wb");
  if (fp != NULL) {
    fwrite(content.data(), 1, content.size(),fp);
    fclose(fp);
    return true;
  }else {
  return false;
  }
}

void CrashReportServer::ReceiveCrashReport(SOCKET client) {
  //  如果接收成功，则发送相应消息确认。
  const char* success_message = "success";
  int nRecv = 0;
  map<string, string> data;
  vector<string> kv_data;
  bool magic_word_received = false;
  bool is_key = true;
  while (true) {
    int size = 0;
    nRecv = recv(client, (char*)&size, sizeof(int), 0);
    if (nRecv <= 0) {
      break;
    }
    LOG(INFO) << "size = " << size;
    if(size ==0) {
      LOG(INFO) << "data end, last block size == 0";
      break;
    }
    char* buff = new char[size];
    nRecv = recv(client, buff, size, 0);
    if (nRecv <= 0) {
      break;
    }
    //  buff[size] = '\0';
    if (!magic_word_received) {
      magic_word_received = true;
    } else {
      string str;
      str.assign(buff, size);
      kv_data.push_back(str);
      //  LOG(INFO) << str;
    }
    delete buff;
    //  TODO(yesp) : 根据Key存储不同的数据。如果Key是file,则存储该文件。
  }
  LOG(INFO) << "key_value size = " << kv_data.size();
  // TODO(yesp) : parse key_value data;
  for (int i =0; i < kv_data.size(); ++i) {
    int index = kv_data[i].find(':');
    //  冒号不要。
    string key = kv_data[i].substr(0, index);
    string value = kv_data[i].substr(index + 1);
    data[key] = value;
  }
  if (!data["crashguid"].empty()) {
    LOG(INFO) << "write data to file using crashguid as file name";
    WriteStringToFile( data["file"], data["crashguid"] + ".zip");
  } else {
    LOG(INFO) << "no crashguid, don't save the file";
  }
  send(client, success_message, strlen(success_message), 0);
  closesocket(client);
}

void CrashReportServer::Run() {
  if (!CreateListenSocket()) {
    return;
  }
  sockaddr_in peer_addr;
  int nAddrLen = sizeof(peer_addr);
  SOCKET client;
  while (TRUE) {
    client = accept(listen_socket_, (SOCKADDR*) &peer_addr, &nAddrLen);
    if (client == INVALID_SOCKET) {
      LOG(WARNING) << "fail to call accept";
      continue;
    }
    // TODO(yesp) : use a thread to receive the data
    LOG(INFO) << "receive a request from :" << inet_ntoa(peer_addr.sin_addr);
    ReceiveCrashReport(client);
  }

  closesocket(listen_socket_);
}
