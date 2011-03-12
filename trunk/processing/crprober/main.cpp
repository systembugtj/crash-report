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
#define NOGDI
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include "CrashRptProbe.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "output.h"
#include "document_exporter.h"

using namespace std;
using namespace google;

// Character set independent string type
typedef std::basic_string<TCHAR> tstring;

DEFINE_string(input_file, "", "Required. Absolute or relative path to input ZIP"
                              " file name.");
DEFINE_string(input_md5, "", " Optional. Path to .md5 file containing MD5 hash"
                             " for the --input_file or directory name where to"
                             " search for the .md5 file.If this parameter is "
                             "omitted, the .md5 file is searched in the directory"
                             "where --input_file is located.");
DEFINE_string(output_file, "", "Optional. Output file name or directory name."
                               " Or use empty name to direct output to terminal.");
DEFINE_string(sym_search_path, "", "Optional. Symbol files search directory or list of directories"
                                   "separated with semicolon. If this parameter is omitted,"
                                   "symbol files are searched using the default search sequence.");
DEFINE_string(extract_path, "", "Optional. Specifies the directory where to extract files contained in error report");
DEFINE_string(table_id, "", "Optional. Specifies the table ID, of the property to retrieve."
                            "If this parameter specified, the property is written to the output file");
DEFINE_string(column_id, "",  "Optional. Specifies the column ID, of the property to retrieve."
                               "If this parameter specified, the property is written to the output file");
DEFINE_int32(row_id, 0, "Optional. Specifies the row ID, of the property to retrieve."
                          "If this parameter specified, the property is written to the output file");
DEFINE_string(format, "html", "output format,now support text and html");

// Function prototypes
int process_report(string szInput, string szInputMD5, string szOutput,
                   string szSymSearchPath, string szExtractPath,
                   string szTableId, string szColumnId, int RowId);
int extract_files(CrpHandle hReport, LPCTSTR pszExtractPath);

// We want to use secure version of _tfopen when possible
#if _MSC_VER<1400
#define _TFOPEN_S(_File, _Filename, _Mode) _File = _tfopen(_Filename, _Mode);
#else
#define _TFOPEN_S(_File, _Filename, _Mode) _tfopen_s(&(_File), _Filename, _Mode);
#endif

int _tmain(int argc, TCHAR** argv) {
  google::ParseCommandLineFlags(&argc, &argv, false);
  CHECK(!FLAGS_input_file.empty())
         <<"input file cannot be empty.pelase use --input_file";

  return process_report(FLAGS_input_file,
      FLAGS_input_md5,
      FLAGS_output_file,
      FLAGS_sym_search_path,
      FLAGS_extract_path,
      FLAGS_table_id,
      FLAGS_column_id,
      FLAGS_row_id);
}

// Processes a crash report file.
int process_report(string szInput, string szInputMD5, string szOutput,
                   string szSymSearchPath, string szExtractPath,
                   string szTableId, string szColumnId, int RowId) {
  int result = UNEXPECTED; // Status
  CrpHandle hReport = 0; // Handle to the error report
  string sInDirName;
  string sInFileName;
  string sOutDirName;
  string sMD5DirName;
  string sMD5FileName;
  string sExtactDirName;
  // Did user specified file name for .MD5 file or directory name for search?
  BOOL bInputMD5FromDir = FALSE;
  // Do we save resulting files to directory or save single resulting file?
  BOOL bOutputToDir = FALSE;
  DWORD dwFileAttrs = 0;
  char szMD5Buffer[64] = _T("");
  char* szMD5Hash = NULL;
  FILE* f = NULL;

  // Decide input dir and file name
  sInDirName = szInput;
  size_t pos = sInDirName.rfind('\\');
  if (pos < 0) // There is no back slash in path
  {
    sInDirName = _T("");
    sInFileName = szInput;
  } else {
    sInFileName = sInDirName.substr(pos + 1);
    sInDirName = sInDirName.substr(0, pos);
  }

  if (!szExtractPath.empty()) {
    // Determine if user has specified a valid dir name for file extraction
    dwFileAttrs = GetFileAttributes(szExtractPath.c_str());
    if (dwFileAttrs == INVALID_FILE_ATTRIBUTES || !(dwFileAttrs
        & FILE_ATTRIBUTE_DIRECTORY)) {
      result = INVALIDARG;
      _tprintf(_T("Invalid directory name for file extraction.\n"));
      goto done;
    }
  }

  if (!szInputMD5.empty()) {
    // Determine if user wants us to search for .MD5 files in a directory
    // or if he specifies the .MD5 file name.
    dwFileAttrs = GetFileAttributes(szInputMD5.c_str());
    if (dwFileAttrs != INVALID_FILE_ATTRIBUTES && (dwFileAttrs
        & FILE_ATTRIBUTE_DIRECTORY))
      bInputMD5FromDir = TRUE;

    // Append the last back slash to the MD5 dir name if needed
    if (bInputMD5FromDir) {
      sMD5DirName = szInputMD5;
      size_t pos = sMD5DirName.rfind('\\');
      if (pos < 0) // There is no back slash in path
        sMD5DirName = _T("");
      else if (pos != sMD5DirName.length() - 1) // Append the back slash to dir name
        sMD5DirName = sMD5DirName.substr(0, pos + 1);
    }
  } else {
    // Assume .md5 files are in the same dir as input file
    sMD5DirName = sInDirName;
  }

  // Determine if user wants us to save resulting file in directory using its respective
  // file name or if he specifies the file name for the saved file
  dwFileAttrs = GetFileAttributes(szOutput.c_str());
  if (dwFileAttrs != INVALID_FILE_ATTRIBUTES && (dwFileAttrs
      & FILE_ATTRIBUTE_DIRECTORY)) {
    bOutputToDir = TRUE;
  }

  // Decide MD5 file name
  if (szInputMD5.empty()) {
    // If /md5 cmdline argument is omitted, search for md5 files in the same dir
    sMD5FileName = szInput;
    sMD5FileName += _T(".md5");
  } else {
    if (bInputMD5FromDir) {
      // Look for .md5 files in the special directory
      sMD5FileName = sMD5DirName + sInFileName;
      sMD5FileName += _T(".md5");
    } else {
      // Look for MF5 hash in the specified file
      sMD5FileName = szInputMD5;
    }
  }

  // Get MD5 hash from .md5 file
  _TFOPEN_S(f, sMD5FileName.c_str(), _T("rt"));
  if (f != NULL) {
    szMD5Hash = _fgetts(szMD5Buffer, 64, f);
    fclose(f);
  }

  // Open the error report file
  int res = crpOpenErrorReport(szInput.c_str(), szMD5Hash,
      szSymSearchPath.c_str(), 0, &hReport);
  if (res != 0) {
    result = UNEXPECTED;
    TCHAR buff[1024];
    crpGetLastErrorMsg(buff, 1024);
    _tprintf(_T("Error '%s' while processing file '%s'\n"), buff,
        sInFileName.c_str());
    goto done;
  } else {
    // Output results
    string sOutFileName;
    if (!szOutput.empty()) {
      if (bOutputToDir) {
        // Write output to directory
        sOutFileName = tstring(szOutput);
        if (sOutFileName[sOutFileName.length() - 1] != '\\')
          sOutFileName += _T("\\");
        sOutFileName += sInFileName + _T(".txt");
      } else {
        // Write output to single file
        sOutFileName = szOutput;
      }

      // Open resulting file
      _TFOPEN_S(f, sOutFileName.c_str(), _T("wt"));
      if (f == NULL) {
        result = UNEXPECTED;
        _tprintf(_T("Error: couldn't open output file '%s'.\n"),
            sOutFileName.c_str());
        goto done;
      }
    } else {
      f = stdout; // Write output to terminal
    }

    if (f == NULL) {
      result = UNEXPECTED;
      _tprintf(_T("Error: couldn't open output file.\n"));
      goto done;
    }

    if (!szTableId.empty()) {
      CHECK(!szColumnId.empty())<< "column is empty, please use --columm_id=";
      // Get single property
      string sProp;
      int get = get_prop(hReport, szTableId.c_str(), szColumnId.c_str(), sProp, RowId);
      if (szColumnId == CRP_META_ROW_COUNT) {
        if (get < 0) {
          result = UNEXPECTED;
          TCHAR szErr[1024];
          crpGetLastErrorMsg(szErr, 1024);
          _tprintf(_T("%s\n"), szErr);
          goto done;
        } else {
          // Print row count in the specified table
          _ftprintf(f, _T("%d\n"), get);
        }
      } else if (get != 0) {
        result = UNEXPECTED;
        TCHAR szErr[1024];
        crpGetLastErrorMsg(szErr, 1024);
        _tprintf(_T("%s\n"), szErr);
        goto done;
      } else {
        _ftprintf(f, _T("%s\n"), sProp.c_str());
      }
    } else if (!szOutput.empty()) {
      // Write error report properties to the resulting file
      //  result = output_document(hReport, f);
      DocumentExporter exporter(hReport, f, FLAGS_format);
      result = exporter.Export();
      if (result != 0)
      goto done;
    }

    if (!szExtractPath.empty()) {
      // Extract files from error report
      result = extract_files(hReport, szExtractPath.c_str());
      if (result != 0)
      goto done;
    }
  }
  // Success.
  result = SUCCESS;

  done:
  if (f != NULL && f != stdout)
  fclose(f);
  if (hReport != 0)
  crpCloseErrorReport(hReport);
  return result;
}

int extract_files(CrpHandle hReport, const char* pszExtractPath) {
  string sExtractPath = pszExtractPath;
  if (sExtractPath.at(sExtractPath.length() - 1) != '\\')
    sExtractPath += _T("\\"); // Add the last slash if needed

  // Get count of files to extract.
  int nFileCount = get_table_row_count(hReport, CRP_TBL_XMLDESC_FILE_ITEMS);
  if (nFileCount < 0) {
    return UNEXPECTED; // Error getting file count.
  }

  int i;
  for (i = 0; i < nFileCount; i++) {
    string sFileName;
    int nResult = get_prop(hReport, CRP_TBL_XMLDESC_FILE_ITEMS,
        CRP_COL_FILE_ITEM_NAME, sFileName, i);
    if (nResult != 0) {
      return UNEXPECTED; // Error getting file name
    }

    string sFileSaveAs = sExtractPath + sFileName;
    nResult = crpExtractFile(hReport, sFileName.c_str(), sFileSaveAs.c_str(),
        TRUE);
    if (nResult != 0) {
      return EXTRACTERR; // Error extracting file
    }
  }

  // Success.
  return SUCCESS;
}

