//  Copyright 2011 . All Rights Reserved.
//  Author: yeshunping@gmail.com (Shunping Ye)
#include "reporting/crashsender/sender/tcp_simple_sender.h"

#include <string>
#include <stdio.h>
#include <winsock2.h>

#include "base/strconv.h"
	// 链接到WS2_32.lib
#pragma comment(lib, "WS2_32")

using namespace std;

SimpleTcpSender::SimpleTcpSender(SendData* data, AssyncNotification* assync) :
  AbstractSender(data, assync) {
    status_ = false;
}

SimpleTcpSender::~SimpleTcpSender() {
  // 关闭套节字
  closesocket(sock_);
  WSACleanup();
}

bool SimpleTcpSender::ConnectToServer() {
  strconv_t convter;

  // 初始化WS2_32.dll
  WSADATA wsaData;
  WORD sockVersion = MAKEWORD(2, 2);
  if (WSAStartup(sockVersion, &wsaData) != 0) {
    assync_->SetProgress(_T("call WSAStartup failed"), 100, false);
    return false;
  }

  sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sock_ == INVALID_SOCKET) {
    assync_->SetProgress(_T("fail to create socket"), 100, false);
    return false;
  }
  sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;

  string ascii_url = convter.t2a(data_->url);
  int index = ascii_url.find(":");
  string host;
  string port_str;
  if (index == string::npos)
    return false;

  host = ascii_url.substr(0, index);
  port_str = ascii_url.substr(index + 1);
  MessageBoxA(NULL, host.c_str(), "host", 0);
  MessageBoxA(NULL, port_str.c_str(), "port", 0);
  int port = atoi(port_str.c_str());

  servAddr.sin_port = htons(port);
  // TODO(yesp) :use host to get ip address
  servAddr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
  if (connect(sock_, (sockaddr*) &servAddr, sizeof(servAddr)) == -1) {
    assync_->SetProgress(_T("fail to connect to server"), 100, false);
    return false;
  }
  return true;
}

bool SimpleTcpSender::SendKeyValue(string key, string value) {
  key = key + ":";
  int size = key.length() + value.size();
  send(sock_, (const char*)&size, sizeof(int), 0);
  send(sock_, key.c_str(), key.length(), 0);
  send(sock_, value.data(), value.size(), 0);
  return true;
}

bool WriteStringToFile(const string& content, const string& filename) {
  // TODO(yesp) : get dir path.
  //  get the abs file path
  FILE* fp = fopen(filename.c_str(), "wb");
  if (fp != NULL) {
    fwrite(content.data(), 1, content.size(), fp);
    fclose(fp);
    return true;
  } else {
    return false;
  }
}
bool ReadFileToString(string file, string* out) {
  FILE* fp = fopen(file.c_str(), "rb");
  if (fp == NULL) {
    return false;
  }
  int len;
  //  get file size
  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  //  read file
  char *buf = new char[len];
  int rsize = fread(buf, len, 1, fp);
  out->assign(buf, len);
  fclose(fp);
  delete buf;
  return true;
}

// TODO(yesp) : 实现该函数
bool SimpleTcpSender::SendCrashReport() {
  //  把相关数据一个个发送过去。
 //  使用MagicData|size+data|size+data|...|0的形式发送。
  //  开始的标志是一个签名数据，结束的size大小为0
  int size = strlen("CrashReport");
  send(sock_, (const char*)&size, sizeof(int), 0);
  send(sock_, "CrashReport", strlen("CrashReport"), 0);

  SendKeyValue("appname", data_->appname);
  SendKeyValue("appversion", data_->appversion);
  SendKeyValue("crashguid", data_->crashguid);
  SendKeyValue("description", data_->description);
  SendKeyValue("md5", data_->md5);
  strconv_t convter;
  string filename = convter.t2a(data_->filename);
  string content;
  if(!ReadFileToString(filename, &content)) {
    assync_->SetProgress(_T("ReadFileToString failed"), 100, false);
  }
  //  send file data
  SendKeyValue("file", content);
  //  last block size = 0 to tell server to end receiving data.
  size = 0;
  send(sock_, (const char*)&size, sizeof(int), 0);
  return true;
}

void SimpleTcpSender::ReceiveAskData() {
  char buff[256];
  int nRecv = recv(sock_, buff, 256, 0);
  if (nRecv > 0) {
    buff[nRecv] = '\0';
    if (strcmp(buff, "success") == 0) {
      status_ = true;
    }
  }
}

bool SimpleTcpSender::Send() {
  if (!ConnectToServer())
    return status_;
  if (!SendCrashReport())
    return status_;
  ReceiveAskData();
  assync_->SetCompleted(status_ ? 0 : 1);
  return status_;
}

