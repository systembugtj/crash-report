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

#define NOGDI
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>
#include "crash_report_probe.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "output.h"
#include "document_exporter.h"

using namespace std;
using namespace google;

// Character set independent string type
typedef std::basic_string<TCHAR> tstring;

DEFINE_string(input_file, "", "Required. input ZIP file name.");
DEFINE_string(input_md5_file, "", " Optional. Path to .md5 file containing MD5 hash"
		" for the --input_file or directory name where to"
		" search for the .md5 file.If this parameter is "
		"omitted, the .md5 file is searched in the directory"
		"where --input_file is located.");
DEFINE_string(output_file, "", "Optional. default to $input_file.txt or $input_file.html");
DEFINE_string(sym_search_path, "", "Optional. Symbol files search directory or list of directories"
		"separated with semicolon. If this parameter is omitted,"
		"symbol files are searched using the default search sequence.");
DEFINE_string(extract_path, "", "Optional. Specifies the directory where to"
					            " extract files contained in error report");
DEFINE_string(table_id, "", "Optional. Specifies the table ID, of the property to retrieve."
		"If this parameter specified, the property is written to the output file");
DEFINE_string(column_id, "", "Optional. Specifies the column ID, of the property to retrieve."
		"If this parameter specified, the property is written to the output file");
DEFINE_int32(row_id, 0, "Optional. Specifies the row ID, of the property to retrieve."
		"If this parameter specified, the property is written to the output file");
DEFINE_string(format, "html", "output format,now support text and html");

// Function prototypes
int process_report(string szInput, string szInputMD5, string szOutput,
		string szSymSearchPath, string szExtractPath, string szTableId,
		string szColumnId, int RowId);
int extract_files(CrpHandle hReport, const string& pszExtractPath);

tstring CrGetErrorMessage() {
	TCHAR buff[1024];
	crpGetLastErrorMsg(buff, 1024);
	return tstring(buff);
}
// We want to use secure version of _tfopen when possible
#if _MSC_VER<1400
#define _TFOPEN_S(_File, _Filename, _Mode) _File = _tfopen(_Filename, _Mode);
#else
#define _TFOPEN_S(_File, _Filename, _Mode) _tfopen_s(&(_File), _Filename, _Mode);
#endif

int _tmain(int argc, TCHAR** argv) {
	google::ParseCommandLineFlags(&argc, &argv, false);
	CHECK(!FLAGS_input_file.empty())<<"input file cannot be empty.pelase use --input_file";
	return process_report(FLAGS_input_file,
			FLAGS_input_md5_file,
			FLAGS_output_file,
			FLAGS_sym_search_path,
			FLAGS_extract_path,
			FLAGS_table_id,
			FLAGS_column_id,
			FLAGS_row_id);
}

// Processes a crash report file.
int process_report(string in_file, string input_md5_file, string out_file,
		string symbol_path, string extract_path, string table_id,
		string column_id, int row_id) {
	CrpHandle hReport = 0; // Handle to the error report
	string input_dir;
	string input_file;
	string output_dir;
	string md5_dir;
	string md5_file;
	string extract_dir;
  string save_file;
	// Did user specified file name for .MD5 file or directory name for search?
	bool input_md5_from_dir = false;
	// Do we save resulting files to directory or save single resulting file?
	bool output_to_dir = false;
	DWORD dwFileAttrs = 0;
	char md5[64] = _T("");
	char* md5_hash = NULL;
	FILE* fp = NULL;

	// Decide input dir and file name
	input_dir = in_file;
	size_t pos = input_dir.rfind('\\');
	if (pos < 0) // There is no back slash in path
	{
		input_dir = _T("");
		input_file = in_file;
	} else {
		input_file = input_dir.substr(pos + 1);
		input_dir = input_dir.substr(0, pos);
	}

	if (!extract_path.empty()) {
		// Determine if user has specified a valid dir name for file extraction
		dwFileAttrs = GetFileAttributes(extract_path.c_str());
		if (dwFileAttrs == INVALID_FILE_ATTRIBUTES ||
      !(dwFileAttrs & FILE_ATTRIBUTE_DIRECTORY)) {
			LOG(ERROR) << "Invalid directory name for file extraction";
			goto done;
		}
	}

	// Determine if user wants us to save resulting file in directory using its respective
	// file name or if he specifies the file name for the saved file
	dwFileAttrs = GetFileAttributes(out_file.c_str());
	if (dwFileAttrs != INVALID_FILE_ATTRIBUTES && (dwFileAttrs
			& FILE_ATTRIBUTE_DIRECTORY)) {
		output_to_dir = TRUE;
	}

	// Decide MD5 file name ,and get md5 value from file
	if (input_md5_file.empty()) {
		md5_file = in_file + ".md5";
	} else {
		md5_file = input_md5_file;
	}
	_TFOPEN_S(fp, md5_file.c_str(), _T("rt"));
	if (fp != NULL) {
		md5_hash = _fgetts(md5, 64, fp);
		fclose(fp);
	}

	// Open the error report file
	int res = crpOpenErrorReport(in_file.c_str(), md5_hash,
			symbol_path.c_str(), 0, &hReport);
	if (res != 0) {
		CHECK(false)<< "Error while processing file:"
		            << input_file << "\nreason:" << CrGetErrorMessage();
		return -1;
	}

	if (!out_file.empty()) {
		//  output_file is a directory
		if (output_to_dir) {
			save_file = tstring(out_file);
			if (save_file[save_file.length() - 1] != '\\') {
				save_file += _T("\\");
			}
			save_file += input_file;
		} else {
			// Write output to single file
			save_file = out_file;
		}
	} else {
		//  default output file name
		save_file += input_file;
	}

	if (FLAGS_format == "html") {
		save_file += _T(".html");
	} else if (FLAGS_format == "text") {
		save_file +=  _T(".txt");
	} else {
		LOG(ERROR) << "fromat :" << FLAGS_format << " cannot be supported now";
		return -1;
	}
	// Open resulting file
	_TFOPEN_S(fp, save_file.c_str(), _T("wt"));
	CHECK(fp != NULL) << "couldn't open output file:" << save_file;

	CHECK(fp != NULL) << "fail to open output file";
	//  output some table or whole file
	if (!table_id.empty()) {
		CHECK(!column_id.empty())<<"column is empty, please use --columm_id=";
		// Get single property
		string sProp;
		int get = get_prop(hReport, table_id.c_str(), column_id.c_str(), sProp, row_id);
		if (get != 0) {
			LOG(ERROR) << "error:" << CrGetErrorMessage();
			goto done;
		}
		//  save to output file
		if (column_id == CRP_META_ROW_COUNT) {
			_ftprintf(fp, _T("%d\n"), get);
		} else {
			_ftprintf(fp, _T("%s\n"), sProp.c_str());
		}
	} else {
		DocumentExporter exporter(hReport, fp, FLAGS_format);
    if (exporter.Export() != 0) {
			LOG(ERROR) << "fail to export info from error report";
			goto done;
		}
	}
	//  extract files
	if (!extract_path.empty()) {
		if(extract_files(hReport, extract_path) != 0) {
			LOG(ERROR) << "fail to extract zip file";
		}
	}

	done:
	if (fp != NULL) {
		fclose(fp);
	}
	if (hReport != 0) {
		crpCloseErrorReport(hReport);
	}
	return 0;
}

int extract_files(CrpHandle hReport, const string& extract_path) {
	string path = extract_path;
	if (path.at(path.length() - 1) != '\\')
		path += _T("\\"); // Add the last slash if needed
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
			return UNEXPECTED;
		}

		string sFileSaveAs = path + sFileName;
		nResult = crpExtractFile(hReport, sFileName.c_str(),
				sFileSaveAs.c_str(), TRUE);
		if (nResult != 0) {
			return EXTRACTERR; // Error extracting file
		}
	}
	return SUCCESS;
}

