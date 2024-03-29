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

#include "stdafx.h"
#include "Tests.h"
#include "crash_report.h"
#include "base/Utility.h"
#include "base/strconv.h"

std::map<std::string, std::string>* g_pTestSuiteList = NULL; // The list of test suites
std::string sCurTestSuite; // Current test suite
std::map<std::string, PFNTEST>* g_pTestList = NULL; // The list of test cases
std::vector<std::string>* g_pErrorList = NULL; // The list of errors
BOOL g_bRunningFromUNICODEFolder = FALSE; // Are we running from a UNICODE-named folder?

// Function forward declarations
std::vector<std::string> explode(std::string str, std::string separators = " \t");
void trim2(std::string& str, char* szTrim=" \t\n");
int fork();

int _tmain(int argc, TCHAR** argv)
{  
  if(argc==1)
  {  
    _tprintf(_T("\nDo you want to run tests from a folder cotaining Chineese characters to test UNICODE compatibility (y/n)?\n"));
    _tprintf(_T("Your choice > "));
    TCHAR szAnswer[1024]=_T("");
#if _MSC_VER>=1400
    _getts_s(szAnswer, 1024);  
#else
    _getts(szAnswer);  
#endif

    if(_tcscmp(szAnswer, _T("y"))==0 || 
       _tcscmp(szAnswer, _T("Y"))==0)
    {
      // Relaunch this process from another working directory containing UNICODE symbols in path.
      // This is needed to test all functionality on UNICODE compatibility.     
      g_bRunningFromUNICODEFolder = TRUE;  
    }    

    if(g_bRunningFromUNICODEFolder)
    {
      _tprintf(_T("Launching tests in another process:\n"));
      if(g_bRunningFromUNICODEFolder)
        _tprintf(_T(" - with working directory having UNICODE symbols in path\n"));
      return fork();    
    }
  }
  else
  {
    int argnum;
    for(argnum=1; argnum<argc; argnum++)
    {
      TCHAR* szArg = argv[argnum];
      if(_tcscmp(szArg, _T("/unicode"))==0)
        g_bRunningFromUNICODEFolder = TRUE;
    }    
  }

  printf("\n=== Automated tests for CrashRpt v.%d.%d.%d ===\n\n",
    CRASHRPT_VER/1000,
    (CRASHRPT_VER%1000)/100,
    (CRASHRPT_VER%1000)%100);

  printf("The list of avaliable test suites:\n");

  // Print the list of test suites
  std::map<std::string, std::string>::iterator siter;  
  for(siter=g_pTestSuiteList->begin(); siter!=g_pTestSuiteList->end(); siter++)
  {
    printf(" - %s : %s\n", siter->first.c_str(), siter->second.c_str());    
  }

  printf("\nEnter which test suites to run (separate names by space) or enter empty line to run all test suites.\n");
  printf("Your choice > ");
  char szSuiteList[1024]="";
#if _MSC_VER>=1400
  gets_s(szSuiteList, 1024);  
#else
  gets(szSuiteList);  
#endif

  // Create the list of test suites to run
  std::string sSuiteList = szSuiteList;
  std::vector<std::string> aTokens = explode(sSuiteList);
  std::set<std::string> aTestSuitesToRun;
  size_t i;
  for(i=0; i<aTokens.size(); i++) 
    aTestSuitesToRun.insert(aTokens[i]);
  
  // Determine how many tests to run
  size_t nTestsToRun = 0;
  if(aTestSuitesToRun.size()==0)
  {
    nTestsToRun = g_pTestList->size();
  }
  else
  {    
    std::map<std::string, PFNTEST>::iterator iter;
    for(iter=g_pTestList->begin(); iter!=g_pTestList->end(); iter++)
    {
      std::string sName = iter->first;
      size_t pos = sName.find(':');
      std::string sTestSuite = sName.substr(0, pos);    
      std::set<std::string>::iterator sit = 
        aTestSuitesToRun.find(sTestSuite);
      if(sit!=aTestSuitesToRun.end())
      {
        nTestsToRun++;        
      }      
    }
  }
  
  if(nTestsToRun==0)
  {
    printf("\nNo tests selected, exiting.\n");
    return 0;
  }

  printf("\nRunning tests...\n");

  // Walk through all registered test and run each one
  std::map<std::string, PFNTEST>::iterator iter;
  int n = 1;
  for(iter=g_pTestList->begin(); iter!=g_pTestList->end(); iter++)
  {
    std::string sName = iter->first;
    size_t pos = sName.find(':');
    std::string sTestSuite = sName.substr(0, pos);    
    std::set<std::string>::iterator sit = 
      aTestSuitesToRun.find(sTestSuite);
    if(aTestSuitesToRun.size()==0 || sit!=aTestSuitesToRun.end())
    {
      printf("- %d/%d: %s ...\n", n, nTestsToRun, iter->first.c_str());
      n++;
      iter->second();
    }          
  }

  printf("\n=== Summary ===\n\n");
  
  // Print all errors (if exist)
  if(g_pErrorList!=NULL)
  {
    size_t i;
    for(i=0; i<g_pErrorList->size(); i++)
    {
      printf("Error %d: %s\n", i, (*g_pErrorList)[i].c_str());
    }
  }

  printf("\n   Test count: %d\n", nTestsToRun);
  size_t nErrorCount = g_pErrorList!=NULL?g_pErrorList->size():0;
  printf(" Tests passed: %d\n", nTestsToRun-nErrorCount);
  printf(" Tests failed: %d\n", nErrorCount);

  // Wait for key press
  _getch();

  // Clean up
  if(g_pTestSuiteList!=NULL)
    delete g_pTestSuiteList;

  if(g_pTestList!=NULL)
    delete g_pTestList;

  if(g_pErrorList)
    delete g_pErrorList;

  // Return non-zero value if there were errors
  return nErrorCount==0?0:1;
}

// Helper function that removes spaces from the beginning and end of the string
void trim2(std::string& str, char* szTrim)
{
  std::string::size_type pos = str.find_last_not_of(szTrim);
  if(pos != std::string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(szTrim);
    if(pos != std::string::npos) str.erase(0, pos);
  }
  else str.erase(str.begin(), str.end());
}

// Helper function that splits a string into list of tokens
std::vector<std::string> explode(std::string str, std::string separators)
{
  std::vector<std::string> aTokens;

  size_t pos = 0;
  for(;;)
  {
    pos = str.find_first_of(separators, 0);
    
    std::string sToken = str.substr(0, pos);
    if(pos!=std::string::npos)
      str = str.substr(pos+1);
    
    trim2(sToken);
    if(sToken.length()>0)
      aTokens.push_back(sToken);

    if(pos==std::string::npos)
      break;    
  }

  return aTokens;
}


// Launches tests as another process
int fork()
{
  DWORD dwExitCode = 1;
  CString sWorkingFolder;
  CString sCmdLine;
  CString sExeFolder;        
    
  sWorkingFolder = Utility::GetModulePath(NULL);
  sExeFolder = Utility::GetModulePath(NULL);

  if(g_bRunningFromUNICODEFolder)
  {
    CString sAppDataFolder;
    
    Utility::GetSpecialFolder(CSIDL_LOCAL_APPDATA, sAppDataFolder);
    sWorkingFolder = sAppDataFolder+_T("\\CrashRpt UNICODE 应用程序名称");
    BOOL bCreate = Utility::CreateFolder(sWorkingFolder);
    if(!bCreate)
      return 1;
    
    /* Copy all required files to temporary directory. */
    
    

  #ifdef _DEBUG
    BOOL bCopy = CopyFile(sExeFolder+_T("\\crash_reportd.dll"), sWorkingFolder+_T("\\crash_reportd.dll"), TRUE);
    if(!bCopy)
      goto cleanup;
    BOOL bCopy5 = CopyFile(sExeFolder+_T("\\crash_report_readerd.dll"), sWorkingFolder+_T("\\crash_report_readerd.dll"), TRUE);
    if(!bCopy5)
      goto cleanup;  
    BOOL bCopy2 = CopyFile(sExeFolder+_T("\\crash_senderd.exe"), sWorkingFolder+_T("\\crash_senderd.exe"), TRUE);
    if(!bCopy2)
      goto cleanup;
    BOOL bCopy4 = CopyFile(sExeFolder+_T("\\Testsd.exe"), sWorkingFolder+_T("\\Testsd.exe"), TRUE);
    if(!bCopy4)
      goto cleanup;  
  #else
  #ifndef CRASHRPT_LIB
    BOOL bCopy = CopyFile(sExeFolder+_T("\\crash_report.dll"), sWorkingFolder+_T("\\crash_report.dll"), TRUE);
    if(!bCopy)
      goto cleanup;
    BOOL bCopy5 = CopyFile(sExeFolder+_T("\\crash_report_reader.dll"), sWorkingFolder+_T("\\crash_report_reader.dll"), TRUE);
    if(!bCopy5)
      goto cleanup;
  #endif //!CRASHRPT_LIB
    BOOL bCopy2 = CopyFile(sExeFolder+_T("\\crash_sender.exe"), sWorkingFolder+_T("\\crash_sender.exe"), TRUE);
    if(!bCopy2)
      goto cleanup;
    BOOL bCopy4 = CopyFile(sExeFolder+_T("\\Tests.exe"), sWorkingFolder+_T("\\Tests.exe"), TRUE);
    if(!bCopy4)
      goto cleanup;  
  #endif

    BOOL bCopy3 = CopyFile(sExeFolder+_T("\\crashrpt_lang.ini"), sWorkingFolder+_T("\\crashrpt_lang.ini"), TRUE);
    if(!bCopy3)
      goto cleanup;

    BOOL bCopy6 = CopyFile(sExeFolder+_T("\\dummy.ini"), sWorkingFolder+_T("\\dummy.ini"), TRUE);
    if(!bCopy6)
      goto cleanup;

    BOOL bCopy7 = CopyFile(sExeFolder+_T("\\dummy.log"), sWorkingFolder+_T("\\dummy.log"), TRUE);
    if(!bCopy7)
      goto cleanup;

    BOOL bCopy8 = CopyFile(sExeFolder+_T("\\dbghelp.dll"), sWorkingFolder+_T("\\dbghelp.dll"), TRUE);
    if(!bCopy8)
      goto cleanup;
  }

  /* Create new process */

#ifdef _DEBUG
  sCmdLine = _T("\"") + sWorkingFolder+_T("\\Testsd.exe") + _T("\"");
#else
  sCmdLine = _T("\"") + sWorkingFolder+_T("\\Tests.exe") + _T("\"");
#endif    

  if(g_bRunningFromUNICODEFolder)
    sCmdLine += _T(" /unicode");

  HANDLE hProcess = NULL;

  STARTUPINFO si;
  memset(&si, 0, sizeof(STARTUPINFO));
  si.cb = sizeof(STARTUPINFO);

  PROCESS_INFORMATION pi;
  memset(&pi, 0, sizeof(PROCESS_INFORMATION));    

  BOOL bCreateProcess = CreateProcess(NULL, sCmdLine.GetBuffer(0),
      NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
  if(!bCreateProcess)
  {    
    _tprintf(_T("Error creating process! Press any key to exit.\n."));  
	_getch();
    goto cleanup;
  }

  hProcess = pi.hProcess;

  // Wait until process exits.
  WaitForSingleObject(hProcess, INFINITE);
  
  GetExitCodeProcess(hProcess, &dwExitCode);

cleanup:

  // Clean up  
  if(g_bRunningFromUNICODEFolder)
    Utility::RecycleFile(sWorkingFolder, TRUE);

  return dwExitCode;
}

