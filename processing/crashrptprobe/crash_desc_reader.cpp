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
//  rewrited by Shunping Ye(yeshunping@gmail.com)
#include "stdafx.h"
#include "crash_report.h"
#include "crash_desc_reader.h"
#include "tinyxml.h"
#include "base/utility.h"

CCrashDescReader::CCrashDescReader() {
  loaded = false;
  exception_type = 0;
  epe_subcode = 0;
  exception_code = 0;
  inv_param_line = 0;
}

CCrashDescReader::~CCrashDescReader() {
}

static const char* GetText(TiXmlHandle& hRoot, const char* tagname) {
  TiXmlHandle node = hRoot.ToElement()->FirstChild(tagname);
  if (node.ToElement()) {
    TiXmlText *text_node = node.FirstChild().Text();
    if (text_node) {
      return text_node->Value();
    } else {
      return NULL;
    }
  } else {
    return NULL;
  }
}

void CCrashDescReader::GetCrashReportVersion(TiXmlHandle & hRoot) {
  // Get generator version
  const char *szCrashRptVersion = hRoot.ToElement()->Attribute("version");
  if (szCrashRptVersion != NULL) {
    crash_report_version = atoi(szCrashRptVersion);
  }
}

void CCrashDescReader::GetCrashGUID(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "CrashGUID");
  if (text) {
    crash_guid = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetApplicationName(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "AppName");
  if (text) {
    application_name = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetApplicationVersion(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "AppVersion");
  if (text) {
    applicataion_version = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetImageName(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "ImageName");
  if (text) {
    image_name = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetOSInfomation(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "OperatingSystem");
  if (text) {
    os_version = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetLocation(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "GeoLocation");
  if (text) {
    geo_location = strconv.utf82t(text);
  }
}

void CCrashDescReader::IsOS64Bit(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "OSIs64Bit");
  is_64bit_system = FALSE;
  if (text) {
    is_64bit_system = atoi(text);
  }
}

void CCrashDescReader::GetSystemTimeUTC(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "SystemTimeUTC");
  if (text) {
    system_time_UTC = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetExceptionType(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "ExceptionType");
  if (text) {
    exception_type = atoi(text);
  }
}

void CCrashDescReader::GetUserEmail(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "UserEmail");
  if (text) {
    user_email = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetCrashDescription(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "ProblemDescription");
  if (text) {
    crash_description = strconv.utf82t(text);
  }
}

void CCrashDescReader::ParseExceptionCode(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "ExceptionCode");
  if (text) {
    exception_code = atoi(text);
  }
}

void CCrashDescReader::GetFPESubcode(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "FPESubcode");
  if (text) {
    epe_subcode = atoi(text);
  }
}

// Get InvParamExpression, InvParamFunction, InvParamFile, InvParamLine
// (for invalid parameter exceptions only)
void CCrashDescReader::GetInfoForInvalidParameterException(TiXmlHandle & hRoot) {
  if (exception_type != CR_CPP_INVALID_PARAMETER) {
    return;
  }
  const char *text = GetText(hRoot, "InvParamExpression");
  if (text) {
    inv_param_expression = strconv.utf82t(text);
  }
  text = GetText(hRoot, "InvParamFunction");
  if (text) {
    inv_param_function = strconv.utf82t(text);
  }
  text = GetText(hRoot, "InvParamFile");
  if (text) {
    inv_param_file = strconv.utf82t(text);
  }
  text = GetText(hRoot, "InvParamLine");
  if (text) {
    inv_param_line = atoi(text);
  }
}

void CCrashDescReader::GetGUIResouceCount(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "GUIResourceCount");
  if (text) {
    gui_resource_count = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetOpenHandleCount(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "OpenHandleCount");
  if (text) {
    open_handle_count = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetMemoryUsage(TiXmlHandle & hRoot) {
  const char *text = GetText(hRoot, "MemoryUsageKbytes");
  if (text) {
    memory_usage = strconv.utf82t(text);
  }
}

void CCrashDescReader::GetFileList(TiXmlHandle & hRoot) {
  TiXmlHandle hFileList = hRoot.ToElement()->FirstChild("FileList");
  if (hFileList.ToElement()) {
    TiXmlHandle hFileItem = hFileList.ToElement()->FirstChild("FileItem");
    while (hFileItem.ToElement()) {
      const char *szFileName = hFileItem.ToElement()->Attribute("name");
      const char *szFileDescription = hFileItem.ToElement()->Attribute(
          "description");
      CString sFileName, sFileDescription;
      if (szFileName != NULL)
        sFileName = strconv.utf82t(szFileName);
      if (szFileName != NULL)
        sFileDescription = strconv.utf82t(szFileDescription);
      file_items[sFileName] = sFileDescription;
      hFileItem = hFileItem.ToElement()->NextSibling();
    }
  }
}

void CCrashDescReader::GetCustomProperty(TiXmlHandle hRoot) {
  // Get custom property list
  TiXmlHandle hCustomProps = hRoot.ToElement()->FirstChild("CustomProps");
  if (hCustomProps.ToElement()) {
    TiXmlHandle hProp = hCustomProps.ToElement()->FirstChild("Prop");
    while (hProp.ToElement()) {
      const char *szName = hProp.ToElement()->Attribute("name");
      const char *szValue = hProp.ToElement()->Attribute("value");
      CString sName, sValue;
      if (szName != NULL) {
        sName = strconv.utf82t(szName);
      }
      if (szValue != NULL) {
        sValue = strconv.utf82t(szValue);
      }
      custom_props[sName] = sValue;
      hProp = hProp.ToElement()->NextSibling();
    }
  }
}

int CCrashDescReader::Load(CString sFileName) {
  TiXmlDocument doc;
  FILE* fp = NULL;
  if (loaded) {
    // already loaded
    return 1;
  }
  fp = _tfopen(sFileName, _T("rb"));
  if (fp == NULL) {
    return -1;
  }
  if (!doc.LoadFile(fp)) {
    fclose(fp);
    // XML is corrupted
    return -2;
  }
  TiXmlHandle hDoc(&doc);
  TiXmlHandle hRoot = hDoc.FirstChild("CrashRpt").ToElement();
  if (hRoot.ToElement() == NULL) {
    return -3; // Invalid XML structure
  }

  //  parse all data in xml file
  GetCrashReportVersion(hRoot);
  GetCrashGUID(hRoot);
  GetApplicationName(hRoot);
  GetApplicationVersion(hRoot);
  GetImageName(hRoot);
  GetOSInfomation(hRoot);
  GetLocation(hRoot);
  IsOS64Bit(hRoot);
  GetSystemTimeUTC(hRoot);
  GetExceptionType(hRoot);
  GetUserEmail(hRoot);
  GetCrashDescription(hRoot);
  ParseExceptionCode(hRoot);
  GetFPESubcode(hRoot);
  GetInfoForInvalidParameterException(hRoot);
  GetGUIResouceCount(hRoot);
  GetOpenHandleCount(hRoot);
  GetMemoryUsage(hRoot);
  GetFileList(hRoot);
  GetCustomProperty(hRoot);

  fclose(fp);
  // OK  
  loaded = true;
  return 0;
}
