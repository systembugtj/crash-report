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

// CrashRptProbe.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "crash_report_probe.h"
#include "crash_report.h"
#include <map>
#include "crash_desc_reader.h"
#include "minidump_reader.h"
#include "base/md5.h"
#include "base/utility.h"
#include "base/strconv.h"
#include "unzip.h"

CComAutoCriticalSection g_crp_cs; // Critical section for thread-safe accessing error messages
std::map<DWORD, CString> g_crp_sErrorMsg; // Last error messages for each calling thread.

// Funtion prototype
int crpSetErrorMsg(PTSTR pszErrorMsg);

TCHAR* exctypes[13] = { _T("SEH exception"), _T("terminate call"), _T(
    "unexpected call"), _T("pure virtual call"), _T("new operator fault"), _T(
    "buffer overrun"), _T("invalid parameter"), _T("SIGABRT signal"), _T(
    "SIGFPE signal"), _T("SIGILL signal"), _T("SIGINT signal"), _T(
    "SIGSEGV signal"), _T("SIGTERM signal"), };

// CrpReportData
// This structure is used internally for storing report data
struct CrpReportData {
  CrpReportData() {
    m_hZip = 0;
    m_pDescReader = NULL;
    m_pDmpReader = NULL;
  }

  unzFile m_hZip; // Handle to the ZIP archive
  CCrashDescReader* m_pDescReader; // Pointer to the crash description reader object
  MiniDumpReader* m_pDmpReader; // Pointer to the minidump reader object
  CString m_sMiniDumpTempName; // The name of the tmp file to store extracted minidump in
  CString m_sSymSearchPath; // Symbol files search path
  std::vector<CString> m_ContainedFiles;
};

// The list of opened handles
std::map<int, CrpReportData> g_OpenedHandles;

// CalcFileMD5Hash
// Calculates the MD5 hash for the given file
int CalcFileMD5Hash(CString sFileName, CString& sMD5Hash) {
  crpSetErrorMsg(_T("Unspecified error."));

  BYTE buff[512];
  MD5 md5;
  MD5_CTX md5_ctx;
  unsigned char md5_hash[16];
  FILE* f = NULL;

#if _MSC_VER<1400
  f = _tfopen(sFileName, _T("rb"));
#else
  _tfopen_s(&f, sFileName, _T("rb"));
#endif

  if (f == NULL) {
    crpSetErrorMsg(_T("Couldn't open ZIP file."));
    return -1; // Couldn't open ZIP file
  }

  md5.MD5Init(&md5_ctx);

  while (!feof(f)) {
    size_t count = fread(buff, 1, 512, f);
    if (count > 0) {
      md5.MD5Update(&md5_ctx, buff, (unsigned int) count);
    }
  }

  fclose(f);
  md5.MD5Final(md5_hash, &md5_ctx);

  int i;
  for (i = 0; i < 16; i++) {
    CString number;
    number.Format(_T("%02x"), md5_hash[i]);
    sMD5Hash += number;
  }

  crpSetErrorMsg(_T("Success."));
  return 0;
}

int UnzipFile(unzFile hZip, const char* szFileName, const TCHAR* szOutFileName) {
  int status = -1;
  int zr = 0;
  int open_file_res = 0;
  FILE* f = NULL;
  BYTE buff[1024];
  int read_len = 0;

  zr = unzLocateFile(hZip, szFileName, 1);
  if (zr != UNZ_OK)
    return -1;

  open_file_res = unzOpenCurrentFile(hZip);
  if (open_file_res != UNZ_OK)
    goto cleanup;

#if _MSC_VER>=1400
  _tfopen_s(&f, szOutFileName, _T("wb"));
#else
  f = _tfopen(szOutFileName, _T("wb"));
#endif

  if (f == NULL)
    goto cleanup;

  for (;;) {
    read_len = unzReadCurrentFile(hZip, buff, 1024);

    if (read_len < 0)
      goto cleanup;

    if (read_len == 0)
      break;

    size_t written = fwrite(buff, read_len, 1, f);
    if (written != 1)
      goto cleanup;
  }

  status = 0;

  cleanup:

  if (open_file_res == UNZ_OK)
    unzCloseCurrentFile(hZip);

  if (f != NULL)
    fclose(f);

  return status;
}

CRASHRPTPROBE_API(int)
crpOpenErrorReportW(
    const wchar_t* pszFileName,
    const wchar_t* pszMd5Hash,
    const wchar_t* pszSymSearchPath,
    DWORD dwFlags,
    CrpHandle* pHandle) {
  UNREFERENCED_PARAMETER(dwFlags);

  int status = -1;
  int nNewHandle = 0;
  CrpReportData report_data;
  int zr = 0;
  int xml_find_res = UNZ_END_OF_LIST_OF_FILE;
  int dmp_find_res = UNZ_END_OF_LIST_OF_FILE;
  char szXmlFileName[1024]="";
  char szDmpFileName[1024]="";
  char szFileName[1024]="";
  CString sCalculatedMD5Hash;
  CString sAppName;
  strconv_t strconv;

  crpSetErrorMsg(_T("Unspecified error."));
  *pHandle = 0;

  report_data.m_sSymSearchPath = pszSymSearchPath;
  report_data.m_pDescReader = new CCrashDescReader;
  report_data.m_pDmpReader = new MiniDumpReader;

  // Check dbghelp.dll version
  if(!report_data.m_pDmpReader->CheckDbgHelpApiVersion())
  {
    crpSetErrorMsg(_T("Invalid dbghelp.dll version (v6.11 expected)."));
    goto exit; // Invalid hash
  }

  // Check ZIP integrity
  if(pszMd5Hash!=NULL) {
    int result = CalcFileMD5Hash(pszFileName, sCalculatedMD5Hash);
    if(result!=0)
    goto exit;

    if(sCalculatedMD5Hash.CompareNoCase(pszMd5Hash)!=0) {
      crpSetErrorMsg(_T("File might be corrupted, because MD5 hash is wrong."));
      goto exit; // Invalid hash
    }
  }

  // Open ZIP archive
  report_data.m_hZip = unzOpen((const char*)pszFileName);
  if(report_data.m_hZip==NULL) {
    crpSetErrorMsg(_T("Error opening ZIP archive."));
    goto exit;
  }

  // Look for v1.1 crash description XML
  xml_find_res = unzLocateFile(report_data.m_hZip, (const char*)"crashrpt.xml", 1);
  zr = unzGetCurrentFileInfo(report_data.m_hZip, NULL, szXmlFileName, 1024, NULL, 0, NULL, 0);

  // Look for v1.1 crash dump 
  dmp_find_res = unzLocateFile(report_data.m_hZip, (const char*)"crashdump.dmp", 1);
  zr = unzGetCurrentFileInfo(report_data.m_hZip, NULL, szDmpFileName, 1024, NULL, 0, NULL, 0);

  // Check that both xml and dmp found
  if(xml_find_res!=UNZ_OK || dmp_find_res!=UNZ_OK)
  {
    crpSetErrorMsg(_T("File is not a valid crash report (XML or DMP missing)."));
    goto exit; // XML or DMP not found 
  }

  // Load crash description data
  if(xml_find_res==UNZ_OK)
  {
    CString sTempFile = Utility::getTempFileName();
    zr = UnzipFile(report_data.m_hZip, szXmlFileName, sTempFile);
    if(zr!=0)
    {
      crpSetErrorMsg(_T("Error extracting ZIP item."));
      Utility::RecycleFile(sTempFile, TRUE);
      goto exit; // Can't unzip ZIP element
    }

    int result = report_data.m_pDescReader->Load(sTempFile);
    DeleteFile(sTempFile);
    if(result!=0)
    {
      crpSetErrorMsg(_T("Crash description file is not a valid XML file."));
      goto exit; // Corrupted XML
    }
  }

  // Extract minidump file
  if(dmp_find_res==UNZ_OK)
  {
    CString sTempFile = Utility::getTempFileName();
    zr = UnzipFile(report_data.m_hZip, szDmpFileName, sTempFile);
    if(zr!=0)
    {
      Utility::RecycleFile(sTempFile, TRUE);
      crpSetErrorMsg(_T("Error extracting ZIP item."));
      goto exit; // Can't unzip ZIP element
    }

    report_data.m_sMiniDumpTempName = sTempFile;
  }

  // Enumerate contained files
  zr = unzGoToFirstFile(report_data.m_hZip);
  if(zr==UNZ_OK)
  {
    for(;;)
    {
      zr = unzGetCurrentFileInfo(report_data.m_hZip,
          NULL, szFileName, 1024, NULL, 0, NULL, 0);
      if(zr!=UNZ_OK)
      break;

      CString sFileName = szFileName;
      report_data.m_ContainedFiles.push_back(sFileName);

      zr=unzGoToNextFile(report_data.m_hZip);
      if(zr!=UNZ_OK)
      break;
    }
  }

  // Add handle to the list of opened handles
  nNewHandle = (int)g_OpenedHandles.size()+1;
  g_OpenedHandles[nNewHandle] = report_data;
  *pHandle = nNewHandle;

  crpSetErrorMsg(_T("Success."));
  status = 0;

  exit:

  if(status!=0)
  {
    delete report_data.m_pDescReader;
    delete report_data.m_pDmpReader;
    Utility::RecycleFile(report_data.m_sMiniDumpTempName, TRUE);

    if(report_data.m_hZip!=0)
    unzClose(report_data.m_hZip);
  }

  return status;
}

CRASHRPTPROBE_API(int)
crpOpenErrorReportA(
    const char* pszFileName,
    const char* pszMd5Hash,
    const char* pszSymSearchPath,
    DWORD dwFlags,
    CrpHandle* pHandle)
{
  strconv_t strconv;
  return crpOpenErrorReportW(
      strconv.a2w(pszFileName),
      strconv.a2w(pszMd5Hash),
      strconv.a2w(pszSymSearchPath),
      dwFlags,
      pHandle);
}

CRASHRPTPROBE_API(int)
crpCloseErrorReport(
    CrpHandle handle)
{
  crpSetErrorMsg(_T("Unspecified error."));

  // Look for such handle
  std::map<int, CrpReportData>::iterator it = g_OpenedHandles.find(handle);
  if(it==g_OpenedHandles.end())
  {
    crpSetErrorMsg(_T("Invalid handle specified."));
    return 1;
  }

  delete it->second.m_pDescReader;
  delete it->second.m_pDmpReader;
  Utility::RecycleFile(it->second.m_sMiniDumpTempName, TRUE);

  if(it->second.m_hZip)
  unzClose(it->second.m_hZip);

  // Remove from the list of opened handles
  g_OpenedHandles.erase(it);

  // OK.
  crpSetErrorMsg(_T("Success."));
  return 0;
}

int ParseDynTableId(CString sTableId, int& index) {
  if (sTableId.Left(5) == "STACK") {
    CString sIndex = sTableId.Mid(5);
    index = _ttoi(sIndex.GetBuffer(0));
    return 0;
  }

  return -1;
}

CRASHRPTPROBE_API(int)
crpGetPropertyW(
    CrpHandle hReport,
    const wchar_t* lpszTableId,
    const wchar_t* lpszColumnId,
    INT nRowIndex,
    LPWSTR lpszBuffer,
    ULONG cchBuffSize,
    PULONG pcchCount)
{
  crpSetErrorMsg(_T("Unspecified error."));

  // Set default output values
  if(lpszBuffer!=NULL && cchBuffSize>=1)
  lpszBuffer[0] = 0; // Empty buffer
  if(pcchCount!=NULL)
  *pcchCount = 0;

  const wchar_t* pszPropVal = NULL;
  const int BUFF_SIZE = 4096;
  TCHAR szBuff[BUFF_SIZE]; // Internal buffer to store property value
  strconv_t strconv; // String convertor object

  // Validate input parameters
  if( lpszTableId==NULL ||
      lpszColumnId==NULL ||
      nRowIndex<0 || // Check we have non-negative row index
      (lpszBuffer==NULL && cchBuffSize!=0) || // Check that we have a valid buffer
      (lpszBuffer!=NULL && cchBuffSize==0)
  )
  {
    crpSetErrorMsg(_T("Invalid argument specified."));
    return -1;
  }

  std::map<int, CrpReportData>::iterator it = g_OpenedHandles.find(hReport);
  if(it==g_OpenedHandles.end())
  {
    crpSetErrorMsg(_T("Invalid handle specified."));
    return -1;
  }

  CCrashDescReader* pDescReader = it->second.m_pDescReader;
  MiniDumpReader* pDmpReader = it->second.m_pDmpReader;

  CString sTableId = lpszTableId;
  CString sColumnId = lpszColumnId;
  int nDynTableIndex = -1;
  // nDynTable will be equal to 0 if a stack trace table is queired
  int nDynTable = ParseDynTableId(sTableId, nDynTableIndex);

  // Check if we need to load minidump file to be able to get the property
  if(sTableId.Compare(CRP_TBL_MDMP_MISC)==0 ||
      sTableId.Compare(CRP_TBL_MDMP_MODULES)==0 ||
      sTableId.Compare(CRP_TBL_MDMP_THREADS)==0 ||
      sTableId.Compare(CRP_TBL_MDMP_LOAD_LOG)==0 ||
      nDynTable==0 ||
      (pDescReader->crash_report_version==1000 && sTableId.Compare(CRP_TBL_XMLDESC_MISC)==0) )
  {
    // Load the minidump
    pDmpReader->Open(it->second.m_sMiniDumpTempName, it->second.m_sSymSearchPath);

    // Walk the stack if this is needed to get the property
    if(nDynTable==0)
    {
      pDmpReader->StackWalk(pDmpReader->dump_data().threads_list[nDynTableIndex].thread_id);
    }
  }

  if(sTableId.Compare(CRP_TBL_XMLDESC_MISC)==0)
  {
    // This table contains single row.
    if(nRowIndex!=0)
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return 1; // return row count in this table
    }
    else if(sColumnId.Compare(CRP_COL_CRASHRPT_VERSION)==0)
    {
      _ULTOT_S(pDescReader->crash_report_version, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_CRASH_GUID)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->crash_guid);
    }
    else if(sColumnId.Compare(CRP_COL_APP_NAME)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->application_name);
    }
    else if(sColumnId.Compare(CRP_COL_APP_VERSION)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->applicataion_version);
    }
    else if(sColumnId.Compare(CRP_COL_IMAGE_NAME)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->image_name);
    }
    else if(sColumnId.Compare(CRP_COL_OPERATING_SYSTEM)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->os_version);
    }
    else if(sColumnId.Compare(CRP_COL_SYSTEM_TIME_UTC)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->system_time_UTC);
    }
    else if(sColumnId.Compare(CRP_COL_INVPARAM_FUNCTION)==0)
    {
      if(pDescReader->exception_type!=CR_CPP_INVALID_PARAMETER)
      {
        crpSetErrorMsg(_T("This property is supported for invalid parameter errors only."));
        return -3;
      }
      pszPropVal = strconv.t2w(pDescReader->inv_param_function);
    }
    else if(sColumnId.Compare(CRP_COL_INVPARAM_EXPRESSION)==0)
    {
      if(pDescReader->exception_type!=CR_CPP_INVALID_PARAMETER)
      {
        crpSetErrorMsg(_T("This property is supported for invalid parameter errors only."));
        return -3;
      }
      pszPropVal = strconv.t2w(pDescReader->inv_param_expression);
    }
    else if(sColumnId.Compare(CRP_COL_INVPARAM_FILE)==0)
    {
      if(pDescReader->exception_type!=CR_CPP_INVALID_PARAMETER)
      {
        crpSetErrorMsg(_T("This property is supported for invalid parameter errors only."));
        return -3;
      }
      pszPropVal = strconv.t2w(pDescReader->inv_param_file);
    }
    else if(sColumnId.Compare(CRP_COL_INVPARAM_LINE)==0)
    {
      if(pDescReader->exception_type!=CR_CPP_INVALID_PARAMETER)
      {
        crpSetErrorMsg(_T("This property is supported for invalid parameter errors only."));
        return -3;
      }
      _ULTOT_S(pDescReader->inv_param_line, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_EXCEPTION_TYPE)==0)
    {
      _ULTOT_S(pDescReader->exception_type, szBuff, BUFF_SIZE, 10);
      _TCSCAT_S(szBuff, BUFF_SIZE, _T(" "));
      _TCSCAT_S(szBuff, BUFF_SIZE, exctypes[pDescReader->exception_type]);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_EXCEPTION_CODE)==0)
    {
      _ULTOT_S(pDescReader->exception_code, szBuff, BUFF_SIZE, 16);
      _TCSCAT_S(szBuff, BUFF_SIZE, _T(" "));
      CString msg = Utility::FormatErrorMsg(pDescReader->exception_code);
      _TCSCAT_S(szBuff, BUFF_SIZE, msg);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_FPE_SUBCODE)==0)
    {
      _ULTOT_S(pDescReader->epe_subcode, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_USER_EMAIL)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->user_email);
    }
    else if(sColumnId.Compare(CRP_COL_PROBLEM_DESCRIPTION)==0)
    {
      pszPropVal = strconv.t2w(pDescReader->crash_description);
    }
    else if(sColumnId.Compare(CRP_COL_GUI_RESOURCE_COUNT)==0)
    {
      // We do not support this property for older versions of CrashRpt
      if(pDescReader->crash_report_version<1201)
      {
        crpSetErrorMsg(_T("Invalid column ID is specified."));
        return -3;
      }
      pszPropVal = strconv.t2w(pDescReader->gui_resource_count);
    }
    else if(sColumnId.Compare(CRP_COL_OPEN_HANDLE_COUNT)==0)
    {
      // We do not support this property for older versions of CrashRpt
      if(pDescReader->crash_report_version<1201)
      {
        crpSetErrorMsg(_T("Invalid column ID is specified."));
        return -3;
      }
      pszPropVal = strconv.t2w(pDescReader->open_handle_count);
    }
    else if(sColumnId.Compare(CRP_COL_MEMORY_USAGE_KBYTES)==0)
    {
      // We do not support this property for older versions of CrashRpt
      if(pDescReader->crash_report_version<1201)
      {
        crpSetErrorMsg(_T("Invalid column ID is specified."));
        return -3;
      }
      pszPropVal = strconv.t2w(pDescReader->memory_usage);
    }
    else if(sColumnId.Compare(CRP_COL_OS_IS_64BIT)==0)
    {
      // We do not support this property for older versions of CrashRpt
      if(pDescReader->crash_report_version<1207)
      {
        crpSetErrorMsg(_T("Invalid column ID is specified."));
        return -3;
      }
      _STPRINTF_S(szBuff, BUFF_SIZE, L"%d", pDescReader->is_64bit_system);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_GEO_LOCATION)==0)
    {
      // We do not support this property for older versions of CrashRpt
      if(pDescReader->crash_report_version<1207)
      {
        crpSetErrorMsg(_T("Invalid column ID is specified."));
        return -3;
      }
      pszPropVal = strconv.t2w(pDescReader->geo_location);
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(sTableId.Compare(CRP_TBL_XMLDESC_FILE_ITEMS)==0)
  {
    if(nRowIndex>=(int)pDescReader->file_items.size())
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return (int)pDescReader->file_items.size();
    }
    else if( sColumnId.Compare(CRP_COL_FILE_ITEM_NAME)==0 ||
        sColumnId.Compare(CRP_COL_FILE_ITEM_DESCRIPTION)==0 )
    {
      std::map<CString, CString>::iterator it = pDescReader->file_items.begin();
      int i;
      for(i=0; i<nRowIndex; i++) it++;

      if(sColumnId.Compare(CRP_COL_FILE_ITEM_NAME)==0)
      pszPropVal = strconv.t2w(it->first);
      else
      pszPropVal = strconv.t2w(it->second);
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(sTableId.Compare(CRP_TBL_XMLDESC_CUSTOM_PROPS)==0)
  {
    if(pDescReader->crash_report_version<1201)
    {
      crpSetErrorMsg(_T("Invalid table ID specified."));
      return -3;
    }

    if(nRowIndex>=(int)pDescReader->custom_props.size())
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return (int)pDescReader->custom_props.size();
    }
    else if( sColumnId.Compare(CRP_COL_PROPERTY_NAME)==0 ||
        sColumnId.Compare(CRP_COL_PROPERTY_VALUE)==0 )
    {
      std::map<CString, CString>::iterator it = pDescReader->custom_props.begin();
      int i;
      for(i=0; i<nRowIndex; i++) it++;

      if(sColumnId.Compare(CRP_COL_PROPERTY_NAME)==0)
      pszPropVal = strconv.t2w(it->first);
      else
      pszPropVal = strconv.t2w(it->second);
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(sTableId.Compare(CRP_TBL_MDMP_MISC)==0)
  {
    if(nRowIndex!=0)
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return 1; // there is 1 row in this table
    }
    else if(sColumnId.Compare(CRP_COL_CPU_ARCHITECTURE)==0)
    {
      _ULTOT_S(pDmpReader->dump_data().processor_architecture, szBuff, BUFF_SIZE, 10);
      _TCSCAT_S(szBuff, BUFF_SIZE, _T(" "));

      TCHAR* szDescription = _T("unknown processor type");
      if(pDmpReader->dump_data().processor_architecture==PROCESSOR_ARCHITECTURE_AMD64)
      szDescription = _T("x64 (AMD or Intel)");
      if(pDmpReader->dump_data().processor_architecture==PROCESSOR_ARCHITECTURE_IA32_ON_WIN64)
      szDescription = _T("WOW");
      if(pDmpReader->dump_data().processor_architecture==PROCESSOR_ARCHITECTURE_IA64)
      szDescription = _T("Intel Itanium Processor Family (IPF)");
      if(pDmpReader->dump_data().processor_architecture==PROCESSOR_ARCHITECTURE_INTEL)
      szDescription = _T("x86");

      _TCSCAT_S(szBuff, BUFF_SIZE, szDescription);

      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_CPU_COUNT)==0)
    {
      _ULTOT_S(pDmpReader->dump_data().processors_number, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_PRODUCT_TYPE)==0)
    {
      _ULTOT_S(pDmpReader->dump_data().product_type, szBuff, BUFF_SIZE, 10);
      _TCSCAT_S(szBuff, BUFF_SIZE, _T(" "));

      TCHAR* szDescription = _T("unknown product type");
      if(pDmpReader->dump_data().product_type==VER_NT_DOMAIN_CONTROLLER)
      szDescription = _T("domain controller");
      if(pDmpReader->dump_data().product_type==VER_NT_SERVER)
      szDescription = _T("server");
      if(pDmpReader->dump_data().product_type==VER_NT_WORKSTATION)
      szDescription = _T("workstation");

      _TCSCAT_S(szBuff, BUFF_SIZE, szDescription);

      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_OS_VER_MAJOR)==0)
    {
      _ULTOT_S(pDmpReader->dump_data().major_version, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_OS_VER_MINOR)==0)
    {
      _ULTOT_S(pDmpReader->dump_data().minor_version, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_OS_VER_BUILD)==0)
    {
      _ULTOT_S(pDmpReader->dump_data().build_number, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_OS_VER_CSD)==0)
    {
      pszPropVal = strconv.t2w(pDmpReader->dump_data().lastest_service_pack);
    }
    else if(sColumnId.Compare(CRP_COL_EXCPTRS_EXCEPTION_CODE)==0)
    {
      if(!pDmpReader->read_exception_stream())
      {
        crpSetErrorMsg(_T("There is no exception information in minidump file."));
        return -3;
      }
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("0x%x"), pDmpReader->dump_data().exception_code);
      _TCSCAT_S(szBuff, BUFF_SIZE, _T(" "));
      CString msg = Utility::FormatErrorMsg(pDmpReader->dump_data().exception_code);
      _TCSCAT_S(szBuff, BUFF_SIZE, msg);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_EXCEPTION_ADDRESS)==0)
    {
      if(!pDmpReader->read_exception_stream())
      {
        crpSetErrorMsg(_T("There is no exception information in minidump file."));
        return -3;
      }
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->dump_data().exception_address);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_EXCEPTION_THREAD_ROWID)==0)
    {
      if(!pDmpReader->read_exception_stream())
      {
        crpSetErrorMsg(_T("There is no exception information in minidump file."));
        return -3;
      }
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("%d"), pDmpReader->GetThreadRowIdByThreadId(pDmpReader->dump_data().exception_thread_id));
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_EXCEPTION_MODULE_ROWID)==0)
    {
      if(!pDmpReader->read_exception_stream())
      {
        crpSetErrorMsg(_T("There is no exception information in minidump file."));
        return -3;
      }
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("%d"), pDmpReader->GetModuleRowIdByAddress(pDmpReader->dump_data().exception_address));
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_EXCEPTION_THREAD_STACK_MD5)==0)
    {
      if(!pDmpReader->read_exception_stream())
      {
        crpSetErrorMsg(_T("There is no exception information in minidump file."));
        return -3;
      }
      int nThreadROWID = pDmpReader->GetThreadRowIdByThreadId(pDmpReader->dump_data().exception_thread_id);
      if(nThreadROWID>=0)
      {
        pDmpReader->StackWalk(pDmpReader->dump_data().threads_list[nThreadROWID].thread_id);
        CString sMD5 = pDmpReader->dump_data().threads_list[nThreadROWID].stack_trace_md5;
        _STPRINTF_S(szBuff, BUFF_SIZE, _T("%s"), sMD5.GetBuffer(0));
      }
      pszPropVal = szBuff;
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(sTableId.Compare(CRP_TBL_MDMP_MODULES)==0)
  {
    if(nRowIndex>=(int)pDmpReader->dump_data().modules_list.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return (int)pDmpReader->dump_data().modules_list.size();
    }
    else if(sColumnId.Compare(CRP_COL_MODULE_NAME)==0)
    {
      pszPropVal = strconv.t2w(pDmpReader->dump_data().modules_list[nRowIndex].module_name);
    }
    else if(sColumnId.Compare(CRP_COL_MODULE_IMAGE_NAME)==0)
    {
      pszPropVal = strconv.t2w(pDmpReader->dump_data().modules_list[nRowIndex].image_name);
    }
    else if(sColumnId.Compare(CRP_COL_MODULE_BASE_ADDRESS)==0)
    {
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->dump_data().modules_list[nRowIndex].base_address);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_MODULE_SIZE)==0)
    {
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("%I64u"), pDmpReader->dump_data().modules_list[nRowIndex].image_size);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_MODULE_LOADED_PDB_NAME)==0)
    {
      pszPropVal = strconv.t2w(pDmpReader->dump_data().modules_list[nRowIndex].loaded_pdb_name);
    }
    else if(sColumnId.Compare(CRP_COL_MODULE_LOADED_IMAGE_NAME)==0)
    {
      pszPropVal = strconv.t2w(pDmpReader->dump_data().modules_list[nRowIndex].loaded_image_name);
    }
    else if(sColumnId.Compare(CRP_COL_MODULE_SYM_LOAD_STATUS)==0)
    {
      CString sSymLoadStatus;
      MinidumpModule m = pDmpReader->dump_data().modules_list[nRowIndex];
      if(m.image_unmatched)
      sSymLoadStatus = _T("No matching binary found.");
      else if(m.pdb_unmatched)
      sSymLoadStatus = _T("No matching PDB file found.");
      else
      {
        if(m.no_symbol_info)
        sSymLoadStatus = _T("No symbols loaded.");
        else
        sSymLoadStatus = _T("Symbols loaded.");
      }

#if _MSC_VER < 1400
      _tcscpy(szBuff, sSymLoadStatus.GetBuffer(0));
#else
      _tcscpy_s(szBuff, BUFF_SIZE, sSymLoadStatus.GetBuffer(0));
#endif

      pszPropVal = strconv.t2w(szBuff);
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else if(sTableId.Compare(CRP_TBL_MDMP_THREADS)==0)
  {
    if(nRowIndex>=(int)pDmpReader->dump_data().threads_list.size())
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return (int)pDmpReader->dump_data().threads_list.size();
    }
    else if(sColumnId.Compare(CRP_COL_THREAD_ID)==0)
    {
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("0x%x"), pDmpReader->dump_data().threads_list[nRowIndex].thread_id);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_THREAD_STACK_TABLEID)==0)
    {
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("STACK%d"), nRowIndex);
      pszPropVal = szBuff;
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }

  }
  else if(sTableId.Compare(CRP_TBL_MDMP_LOAD_LOG)==0)
  {
    if(nRowIndex>=(int)pDmpReader->dump_data().load_log.size())
    {
      crpSetErrorMsg(_T("Invalid row index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return (int)pDmpReader->dump_data().load_log.size();
    }
    else if(sColumnId.Compare(CRP_COL_LOAD_LOG_ENTRY)==0)
    {
      _TCSCPY_S(szBuff, BUFF_SIZE, pDmpReader->dump_data().load_log[nRowIndex]);
      pszPropVal = szBuff;
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }

  }
  else if(nDynTable==0)
  {
    int nEntryIndex = nDynTableIndex;

    // Ensure we walked the stack for this thread
    assert(pDmpReader->dump_data().threads_list[nEntryIndex].has_stack_walk);

    if(nRowIndex>=(int)pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace.size())
    {
      crpSetErrorMsg(_T("Invalid index specified."));
      return -4;
    }

    if(sColumnId.Compare(CRP_META_ROW_COUNT)==0)
    {
      return (int)pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace.size();
    }
    else if(sColumnId.Compare(CRP_COL_STACK_OFFSET_IN_SYMBOL)==0)
    {
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace[nRowIndex].symbol_offset);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_STACK_ADDR_PC_OFFSET)==0)
    {
      _STPRINTF_S(szBuff, BUFF_SIZE, _T("0x%I64x"), pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace[nRowIndex].address_PC_offset);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_STACK_SOURCE_LINE)==0)
    {
      _ULTOT_S(pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace[nRowIndex].source_line_number, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_STACK_MODULE_ROWID)==0)
    {
      _LTOT_S(pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace[nRowIndex].module_row_id, szBuff, BUFF_SIZE, 10);
      pszPropVal = szBuff;
    }
    else if(sColumnId.Compare(CRP_COL_STACK_SYMBOL_NAME)==0)
    {
      pszPropVal = strconv.t2w(pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace[nRowIndex].symbol_name);
    }
    else if(sColumnId.Compare(CRP_COL_STACK_SOURCE_FILE)==0)
    {
      pszPropVal = strconv.t2w(pDmpReader->dump_data().threads_list[nEntryIndex].stack_trace[nRowIndex].source_file_name);
    }
    else
    {
      crpSetErrorMsg(_T("Invalid column ID specified."));
      return -2;
    }
  }
  else
  {
    crpSetErrorMsg(_T("Invalid table ID specified."));
    return -3;
  }

  // Check the provided buffer size
  if(lpszBuffer==NULL || cchBuffSize==0)
  {
    // User wants us to get the required length of the buffer
    if(pcchCount!=NULL)
    {
      *pcchCount = pszPropVal!=NULL?(ULONG)wcslen(pszPropVal):0;
    }
  }
  else
  {
    // User wants us to return the property value 
    ULONG uRequiredLen = pszPropVal!=NULL?(ULONG)wcslen(pszPropVal):0;
    if(uRequiredLen>(cchBuffSize))
    {
      crpSetErrorMsg(_T("Buffer is too small."));
      return uRequiredLen;
    }

    // Copy the property to the buffer
    if(pszPropVal!=NULL)
    WCSCPY_S(lpszBuffer, cchBuffSize, pszPropVal);

    if(pcchCount!=NULL)
    {
      *pcchCount = uRequiredLen;
    }
  }

  // Done.
  crpSetErrorMsg(_T("Success."));
  return 0;
}

CRASHRPTPROBE_API(int)
crpGetPropertyA(
    CrpHandle hReport,
    const char* lpszTableId,
    const char* lpszColumnId,
    INT nRowIndex,
    LPSTR lpszBuffer,
    ULONG cchBuffSize,
    PULONG pcchCount)
{
  crpSetErrorMsg(_T("Unspecified error."));

  WCHAR* szBuffer = NULL;
  strconv_t strconv;

  if(lpszBuffer!=NULL && cchBuffSize>0)
  szBuffer = new WCHAR[cchBuffSize];

  int result = crpGetPropertyW(
      hReport,
      strconv.a2w(lpszTableId),
      strconv.a2w(lpszColumnId),
      nRowIndex,
      szBuffer,
      cchBuffSize,
      pcchCount);

  if(szBuffer!=NULL)
  {
    const char* aszResult = strconv.w2a(szBuffer);
    delete [] szBuffer;
    STRCPY_S(lpszBuffer, cchBuffSize, aszResult);
  }

  return result;
}

CRASHRPTPROBE_API(int)
crpExtractFileW(
    CrpHandle hReport,
    const wchar_t* lpszFileName,
    const wchar_t* lpszFileSaveAs,
    BOOL bOverwriteExisting)
{
  crpSetErrorMsg(_T("Unspecified error."));

  strconv_t strconv;
  int zr;
  unzFile hZip = 0;

  std::map<int, CrpReportData>::iterator it = g_OpenedHandles.find(hReport);
  if(it==g_OpenedHandles.end())
  {
    crpSetErrorMsg(_T("Invalid handle specified."));
    return -1;
  }

  hZip = it->second.m_hZip;

  zr = unzLocateFile(hZip, strconv.w2a(lpszFileName), 1);
  if(zr!=UNZ_OK)
  {
    crpSetErrorMsg(_T("Couldn't find the specified zip item."));
    return -2;
  }

  if(!bOverwriteExisting)
  {
    // Check if such file already exists
    DWORD dwFileAttrs = GetFileAttributes(lpszFileSaveAs);
    if(dwFileAttrs!=INVALID_FILE_ATTRIBUTES && // such object exists
        dwFileAttrs!=FILE_ATTRIBUTE_DIRECTORY) // and it is not a directory
    {
      crpSetErrorMsg(_T("Such file already exists."));
      return -3;
    }
  }

  zr = UnzipFile(hZip, strconv.w2a(lpszFileName), strconv.w2t(lpszFileSaveAs));
  if(zr!=UNZ_OK)
  {
    crpSetErrorMsg(_T("Error extracting the specified zip item."));
    return -4;
  }

  crpSetErrorMsg(_T("Success."));
  return 0;
}

CRASHRPTPROBE_API(int)
crpExtractFileA(
    CrpHandle hReport,
    const char* lpszFileName,
    const char* lpszFileSaveAs,
    BOOL bOverwriteExisting)
{
  strconv_t strconv;
  const wchar_t* pwszFileName = strconv.a2w(lpszFileName);
  const wchar_t* pwszFileSaveAs = strconv.a2w(lpszFileSaveAs);

  return crpExtractFileW(hReport, pwszFileName, pwszFileSaveAs, bOverwriteExisting);
}

CRASHRPTPROBE_API(int)
crpGetLastErrorMsgW(
    LPWSTR pszBuffer,
    UINT uBuffSize)
{
  if(pszBuffer==NULL || uBuffSize==0)
  return -1; // Null pointer to buffer

  strconv_t strconv;

  g_crp_cs.Lock();

  DWORD dwThreadId = GetCurrentThreadId();
  std::map<DWORD, CString>::iterator it = g_crp_sErrorMsg.find(dwThreadId);

  if(it==g_crp_sErrorMsg.end())
  {
    // No error message for current thread.
    CString sErrorMsg = _T("No error.");
    const wchar_t* pwszErrorMsg = strconv.t2w(sErrorMsg.GetBuffer(0));
    int size = min((int)wcslen(pwszErrorMsg), (int)uBuffSize-1);
    WCSNCPY_S(pszBuffer, uBuffSize, pwszErrorMsg, sErrorMsg.GetLength());
    pszBuffer[uBuffSize-1] = 0;
    g_crp_cs.Unlock();
    return size;
  }

  const wchar_t* pwszErrorMsg = strconv.t2w(it->second.GetBuffer(0));
  int size = min((int)wcslen(pwszErrorMsg), (int)uBuffSize-1);
  WCSNCPY_S(pszBuffer, uBuffSize, pwszErrorMsg, size);
  pszBuffer[uBuffSize-1] = 0;
  g_crp_cs.Unlock();
  return size;
}

CRASHRPTPROBE_API(int)
crpGetLastErrorMsgA(
    LPSTR pszBuffer,
    UINT uBuffSize)
{
  if(pszBuffer==NULL || uBuffSize==0)
  return -1;

  strconv_t strconv;

  WCHAR* pwszBuffer = new WCHAR[uBuffSize];

  int res = crpGetLastErrorMsgW(pwszBuffer, uBuffSize);

  const char* paszBuffer = strconv.w2a(pwszBuffer);

  STRCPY_S(pszBuffer, uBuffSize, paszBuffer);

  delete [] pwszBuffer;

  return res;
}

int crpSetErrorMsg(PTSTR pszErrorMsg) {
  g_crp_cs.Lock();
  DWORD dwThreadId = GetCurrentThreadId();
  g_crp_sErrorMsg[dwThreadId] = pszErrorMsg;
  g_crp_cs.Unlock();
  return 0;
}
