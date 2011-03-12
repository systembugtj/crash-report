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

#pragma once
#include "stdafx.h"

typedef void (__cdecl *PFNTEST)();

extern std::map<std::string, std::string>* g_pTestSuiteList; // Test suite list
extern std::string sCurTestSuite; // Current test suite
extern std::map<std::string, PFNTEST>* g_pTestList; // Test case list
extern std::vector<std::string>* g_pErrorList; // The list of errors
extern BOOL g_bRunningFromUNICODEFolder; // Are we running from a UNICODE-named folder?

// Helper class used to register a test suite
class CTestSuiteRegistrator
{
public:
  CTestSuiteRegistrator(const char* szTestSuiteName, const char* szDesc)
  {
    if(g_pTestSuiteList==NULL)
    {
      g_pTestSuiteList = new std::map<std::string, std::string>;
    }
    std::string sSuiteName = std::string(szTestSuiteName);
    (*g_pTestSuiteList)[sSuiteName] = szDesc;

    sCurTestSuite = szTestSuiteName;
  }
};

#define REGISTER_TEST_SUITE(szSuite, szDesc)\
  CTestSuiteRegistrator __testSuite##szSuite ( #szSuite , szDesc );

// Helper class used to register a test case
class CTestRegistrator
{
public:
  CTestRegistrator(const char* szTestName, PFNTEST pfnTest)
  {
    if(g_pTestList==NULL)
    {
      g_pTestList = new std::map<std::string, PFNTEST>;
    }
    std::string sName = sCurTestSuite;
    sName += "::";
    sName += std::string(szTestName);
    (*g_pTestList)[sName] = pfnTest;
  }
};

#define REGISTER_TEST(pfnTest)\
  void pfnTest();\
  CTestRegistrator __test##pfnTest ( #pfnTest , pfnTest );


#define TEST_ASSERT(expr)\
if(!(expr)) { printf("!!!Error in test: "__FUNCTION__ " Expr: " #expr "\n"); \
std::string assertion = "In test: "__FUNCTION__ " Expr: " #expr;\
  if(g_pErrorList==NULL) g_pErrorList = new std::vector<std::string>;\
g_pErrorList->push_back(assertion);\
goto test_cleanup; }

#define __TEST_CLEANUP__ test_cleanup:
