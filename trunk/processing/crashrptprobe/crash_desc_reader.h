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

#pragma once
#include "stdafx.h"
#include <map>
#include "tinyxml.h"
#include "strconv.h"

struct CCrashDescReader {
 public:
  CCrashDescReader();
  ~CCrashDescReader();

  int Load(CString file_name);

  bool loaded;
  DWORD crash_report_version;
  CString crash_guid;
  CString application_name;
  CString applicataion_version;
  CString image_name;
  CString os_version;
  BOOL is_64bit_system;
  CString system_time_UTC;
  CString geo_location;
  DWORD exception_type;
  DWORD exception_code;
  DWORD epe_subcode;
  CString inv_param_expression;
  CString inv_param_function;
  CString inv_param_file;
  DWORD inv_param_line;
  CString user_email;
  CString crash_description;
  //  in Kbytes
  CString memory_usage;
  CString gui_resource_count;
  CString open_handle_count;
  std::map<CString, CString> file_items;
  std::map<CString, CString> custom_props;

 private:
  void GetCrashReportVersion(TiXmlHandle & hRoot);
  void GetCrashGUID(TiXmlHandle & hRoot);
  void GetApplicationName(TiXmlHandle & hRoot);
  void GetApplicationVersion(TiXmlHandle & hRoot);
  void GetImageName(TiXmlHandle & hRoot);
  void GetOSInfomation(TiXmlHandle & hRoot);
  void GetLocation(TiXmlHandle & hRoot);
  void IsOS64Bit(TiXmlHandle & hRoot);
  void GetSystemTimeUTC(TiXmlHandle & hRoot);
  void GetExceptionType(TiXmlHandle & hRoot);
  void GetUserEmail(TiXmlHandle & hRoot);
  void GetCrashDescription(TiXmlHandle & hRoot);
  void ParseExceptionCode(TiXmlHandle & hRoot);
  void GetFPESubcode(TiXmlHandle & hRoot);
  void GetInfoForInvalidParameterException(TiXmlHandle & hRoot);
  void GetGUIResouceCount(TiXmlHandle & hRoot);
  void GetOpenHandleCount(TiXmlHandle & hRoot);
  void GetMemoryUsage(TiXmlHandle & hRoot);
  void GetFileList(TiXmlHandle & hRoot);
  void GetCustomProperty(TiXmlHandle hRoot);

  strconv_t strconv;
};

