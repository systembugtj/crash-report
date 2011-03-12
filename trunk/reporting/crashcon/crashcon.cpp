/*************************************************************************************
 This file is a part of CrashRpt library.

 Copyright (c) 2003, Michael Carruth
 All rights reserved.

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * Neither the name of the author nor the names of its contributors
 may be used to endorse or promote products derived from this software without
 specific prior written permission.


 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ***************************************************************************************/

// crashcon.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <assert.h>
#include <process.h>
#include "crash_report.h"

LPVOID lpvState = NULL;

/*
class TestClass {
public:
  //  will crash in constructor
  TestClass() {
    pt = NULL;
    *pt = 5;
  }
  ~TestClass() {

  }
  void Print() {
    printf("%d\n", *pt);
  }
  int* pt;
};


TestClass global_object;
*/

//  为了要在C++运行时构造全局C++对象之前就注册全局未处理异常的hander
//  可能的一个方法就是，如果C++运行时在对C++对象进行初始化之前能够提供一个callback registry的机会。
//  或者是实现自己的一个C++运行时库。这个相对比较困难，工作量非常大。
//  还有一个可能可以的方法是，生成一个全局的对象，保证该对象在所有的C++对象之前被创建。而install部分
//  放在该对象的构造函数中。
int main(int argc, char* argv[]) {
  // Install crash reporting
  CR_INSTALL_INFO info;
  memset(&info, 0, sizeof(CR_INSTALL_INFO));
  info.size = sizeof(CR_INSTALL_INFO);
  info.application_name = _T("CrashRpt Console Test");
  info.application_version = _T("1.0.0");
  info.email_subject = _T("CrashRpt Console Test 1.0.0 Error Report");
  info.email_address = _T("test@hotmail.com");

  CrAutoInstallHelper helper(&info);

  //int nInstResult = crInstall(&info);
  assert(helper.result() == 0);

  if (helper.result() != 0) {
    TCHAR buff[256];
    crGetLastErrorMsg(buff, 256);
    _tprintf(_T("%s\n"), buff);
    return FALSE;
  }

  printf(
      "Press Enter to simulate a null pointer exception or any other key to exit...\n");
  int n = _getch();
  if (n == 13) {
    int *p = 0;
    *p = 0;
  }

  //int nUninstRes = crUninstall();
  //assert(nUninstRes == 0);
  return 0;
}

