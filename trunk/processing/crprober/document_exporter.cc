#include "document_exporter.h"
#include <tchar.h>
#include <string>
#include <iostream>
#include "gflags/gflags.h"
#include "glog/logging.h"

using namespace std;

// Helper function that retrieves an error report property
int get_prop(CrpHandle hReport, const char* table_id,
    const char* column_id, string& str, int row_id) {
  const int BUFF_SIZE = 1024;
  char buffer[BUFF_SIZE];
  int result = crpGetProperty(hReport, table_id, column_id, row_id, buffer,
      BUFF_SIZE, NULL);
  if (result == 0)
    str = buffer;
  return result;
}

int get_table_row_count(CrpHandle hReport, LPCTSTR table_id) {
  return crpGetProperty(hReport, table_id, CRP_META_ROW_COUNT, 0, NULL, 0, NULL);
}

DocumentExporter::DocumentExporter(CrpHandle hReport, FILE* fp, string format) {
  hReport_ = hReport;
  if (format == "html") {
    outputer_ = new HtmlOutputter;
  } else if (format == "text") {
    outputer_ = new PlainTextOutputter;
  } else {
    CHECK(false)<< "cannot support output format:" << format;
  }
  outputer_->Init(fp);
}

DocumentExporter::~DocumentExporter() {
  //  do nothing now
  delete outputer_;
}

bool DocumentExporter::ExportSummary(int& result) {

  outputer_->BeginSection(_T("Summary"));

  // Print CrashRpt version
  string sCrashRptVer;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_CRASHRPT_VERSION,
      sCrashRptVer);
  if (result == 0)
    outputer_->PutRecord(_T("Generator version"), sCrashRptVer.c_str());

  // Print CrashGUID
  string sCrashGUID;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_CRASH_GUID,
      sCrashGUID);
  if (result == 0)
    outputer_->PutRecord(_T("Crash GUID"), sCrashGUID.c_str());

  // Print AppName
  string sAppName;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_APP_NAME, sAppName);
  if (result == 0)
    outputer_->PutRecord(_T("Application name"), sAppName.c_str());

  // Print ImageName
  string sImageName;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_IMAGE_NAME,
      sImageName);
  if (result == 0)
    outputer_->PutRecord(_T("Executable image"), sImageName.c_str());

  // Print AppVersion
  string sAppVersion;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_APP_VERSION,
      sAppVersion);
  if (result == 0)
    outputer_->PutRecord(_T("Application version"), sAppVersion.c_str());

  // Print SystemTimeUTC
  string sSystemTimeUTC;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_SYSTEM_TIME_UTC,
      sSystemTimeUTC);
  if (result == 0)
    outputer_->PutRecord(_T("Date created (UTC)"), sSystemTimeUTC.c_str());

  // Print OperatingSystem
  string sOperatingSystem;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_OPERATING_SYSTEM,
      sOperatingSystem);
  if (result == 0)
    outputer_->PutRecord(_T("OS name (from user's registry)"),
        sOperatingSystem.c_str());

  string sOsVerMajor;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_OS_VER_MAJOR,
      sOsVerMajor);
  string sOsVerMinor;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_OS_VER_MINOR,
      sOsVerMinor);
  string sOsVerBuild;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_OS_VER_BUILD,
      sOsVerBuild);
  string sOsVerCSD;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_OS_VER_CSD, sOsVerCSD);

  string sOsVer;
  sOsVer += sOsVerMajor;
  sOsVer += _T(".");
  sOsVer += sOsVerMinor;
  sOsVer += _T(".");
  sOsVer += sOsVerBuild;
  sOsVer += _T(" ");
  sOsVer += sOsVerCSD;

  outputer_->PutRecord(_T("OS version (from minidump)"), sOsVer.c_str());

  // Print OSIs64Bit
  string sOSIs64Bit;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_OS_IS_64BIT,
      sOSIs64Bit);
  if (result == 0)
    outputer_->PutRecord(_T("OS is 64-bit"), sOSIs64Bit.c_str());

  // Print GeoLocation
  string sGeoLocation;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_GEO_LOCATION,
      sGeoLocation);
  if (result == 0)
    outputer_->PutRecord(_T("Geographic location"), sGeoLocation.c_str());

  // Print SystemType
  string sSystemType;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_PRODUCT_TYPE,
      sSystemType);
  if (result == 0)
    outputer_->PutRecord(_T("Product type"), sSystemType.c_str());

  // Print ProcessorArchitecture
  string sProcessorArchitecture;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_CPU_ARCHITECTURE,
      sProcessorArchitecture);
  if (result == 0)
    outputer_->PutRecord(_T("CPU architecture"), sProcessorArchitecture.c_str());

  // Print NumberOfProcessors
  string sCPUCount;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_CPU_COUNT, sCPUCount);
  if (result == 0)
    outputer_->PutRecord(_T("CPU count"), sCPUCount.c_str());

  // Print GUIResourceCount
  string sGUIResourceCount;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_GUI_RESOURCE_COUNT,
      sGUIResourceCount);
  if (result == 0)
    outputer_->PutRecord(_T("GUI resource count"), sGUIResourceCount.c_str());

  // Print OpenHandleCount
  string sOpenHandleCount;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_OPEN_HANDLE_COUNT,
      sOpenHandleCount);
  if (result == 0)
    outputer_->PutRecord(_T("Open handle count"), sOpenHandleCount.c_str());

  // Print MemoryUsageKbytes
  string sMemoryUsageKbytes;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC,
      CRP_COL_MEMORY_USAGE_KBYTES, sMemoryUsageKbytes);
  if (result == 0)
    outputer_->PutRecord(_T("Memory usage (Kbytes)"),
        sMemoryUsageKbytes.c_str());

  int nExceptionType = 0;
  string sExceptionType;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_EXCEPTION_TYPE,
      sExceptionType);
  if (result == 0) {
    nExceptionType = _ttoi(sExceptionType.c_str());
    outputer_->PutRecord(_T("Exception type"), sExceptionType.c_str());
  }

  string sExceptionAddress;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC, CRP_COL_EXCEPTION_ADDRESS,
      sExceptionAddress);
  if (result == 0)
    outputer_->PutRecord(_T("Exception address"), sExceptionAddress.c_str());

  string sExceptionCode;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC,
      CRP_COL_EXCPTRS_EXCEPTION_CODE, sExceptionCode);
  if (result == 0)
    outputer_->PutRecord(_T("SEH exception code (from minidump)"),
        sExceptionCode.c_str());

  string sExceptionThreadRowID;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC,
      CRP_COL_EXCEPTION_THREAD_ROWID, sExceptionThreadRowID);
  int uExceptionThreadRowID = _ttoi(sExceptionThreadRowID.c_str());
  if (result == 0) {
    string sExceptionThreadId;
    result = get_prop(hReport_, CRP_TBL_MDMP_THREADS, CRP_COL_THREAD_ID,
        sExceptionThreadId, uExceptionThreadRowID);
    if (result == 0)
      outputer_->PutRecord(_T("Exception thread ID"),
          sExceptionThreadId.c_str());
  }

  string sExceptionModuleRowID;
  result = get_prop(hReport_, CRP_TBL_MDMP_MISC,
      CRP_COL_EXCEPTION_MODULE_ROWID, sExceptionModuleRowID);
  int uExceptionModuleRowID = _ttoi(sExceptionModuleRowID.c_str());
  if (result == 0) {
    string sExceptionModuleName;
    result = get_prop(hReport_, CRP_TBL_MDMP_MODULES, CRP_COL_MODULE_NAME,
        sExceptionModuleName, uExceptionModuleRowID);
    if (result == 0)
      outputer_->PutRecord(_T("Exception module name"),
          sExceptionModuleName.c_str());
  }

  // Print UserEmail
  string sUserEmail;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC, CRP_COL_USER_EMAIL,
      sUserEmail);
  if (result == 0)
    outputer_->PutRecord(_T("User email"), sUserEmail.c_str());

  // Print ProblemDescription
  string sProblemDescription;
  result = get_prop(hReport_, CRP_TBL_XMLDESC_MISC,
      CRP_COL_PROBLEM_DESCRIPTION, sProblemDescription);
  if (result == 0)
    outputer_->PutRecord(_T("Problem description"), sProblemDescription.c_str());

  outputer_->EndSection();
  return true;
}

bool DocumentExporter::ExportUserDefineInfo(int& /*result*/) {
	outputer_->BeginSection(_T("Application-defined properties"));

	int nPropCount =
			get_table_row_count(hReport_, CRP_TBL_XMLDESC_CUSTOM_PROPS);
	if (nPropCount > 0) {
		// Print custom property list
		outputer_->BeginTableCell(1);
		outputer_->PutTableColumnName(_T("id"), 2, false);
		outputer_->PutTableColumnName(_T("Name"), 16, false);
		outputer_->PutTableColumnName(_T("Value"), 32, true);
		outputer_->EndTableCell();

		int i;
		for (i = 0; i < nPropCount; i++) {
			outputer_->BeginTableCell(1);
			char szBuffer[10];
			_snprintf_s(szBuffer, 10, _T("%d"), i + 1);
			outputer_->PutTableCell(szBuffer, 2, false);
			string sPropName;
			get_prop(hReport_, CRP_TBL_XMLDESC_CUSTOM_PROPS,
					CRP_COL_PROPERTY_NAME, sPropName, i);
			outputer_->PutTableCell(sPropName.c_str(), 16, false);
			string sPropValue;
			get_prop(hReport_, CRP_TBL_XMLDESC_CUSTOM_PROPS,
					CRP_COL_PROPERTY_VALUE, sPropValue, i);
			outputer_->PutTableCell(sPropValue.c_str(), 32, true);
			outputer_->EndTableCell();
		}
	}
	outputer_->EndSection();
	return true;
}

bool DocumentExporter::ExportThreadStack(int& result) {
	int nThreadCount = get_table_row_count(hReport_, CRP_TBL_MDMP_THREADS);
	for (int i = 0; i < nThreadCount; i++) {
		string sThreadId;
		result = get_prop(hReport_, CRP_TBL_MDMP_THREADS, CRP_COL_THREAD_ID,
				sThreadId, i);
		if (result != 0) {
			continue;
		}
		string str = _T("Stack trace for thread ");
		str += sThreadId;
		outputer_->BeginSection(str.c_str());
		outputer_->BeginTableCell(1);
		outputer_->PutTableCell(_T("Frame"), 32, true);
		outputer_->EndTableCell();

		string sStackTableId;
		get_prop(hReport_, CRP_TBL_MDMP_THREADS, CRP_COL_THREAD_STACK_TABLEID,
				sStackTableId, i);

		BOOL bMissingFrames = FALSE;
		int nFrameCount = get_table_row_count(hReport_, sStackTableId.c_str());
		int j;
		for (j = 0; j < nFrameCount; j++) {
			outputer_->BeginTableCell(1);
			string sModuleName;
			string sAddrPCOffset;
			string sSymbolName;
			string sOffsInSymbol;
			string sSourceFile;
			string sSourceLine;
			string sModuleRowId;
			result = get_prop(hReport_, sStackTableId.c_str(),
					CRP_COL_STACK_MODULE_ROWID, sModuleRowId, j);
			if (result == 0) {
				int nModuleRowId = _ttoi(sModuleRowId.c_str());
				if (nModuleRowId == -1) {
					if (!bMissingFrames) {
						outputer_->BeginTableCell(1);
						outputer_->PutTableCell(_T("[Frames below may be incorrect and/or missing]"),
												32, true);
						outputer_->EndTableCell();
					}
					bMissingFrames = TRUE;
				}
				get_prop(hReport_, CRP_TBL_MDMP_MODULES, CRP_COL_MODULE_NAME,
						sModuleName, nModuleRowId);
			}

			get_prop(hReport_, sStackTableId.c_str(),
					CRP_COL_STACK_ADDR_PC_OFFSET, sAddrPCOffset, j);
			get_prop(hReport_, sStackTableId.c_str(),
					CRP_COL_STACK_SYMBOL_NAME, sSymbolName, j);
			get_prop(hReport_, sStackTableId.c_str(),
					CRP_COL_STACK_OFFSET_IN_SYMBOL, sOffsInSymbol, j);
			get_prop(hReport_, sStackTableId.c_str(),
					CRP_COL_STACK_SOURCE_FILE, sSourceFile, j);
			get_prop(hReport_, sStackTableId.c_str(),
					CRP_COL_STACK_SOURCE_LINE, sSourceLine, j);

			string str;
			str = sModuleName;
			if (!str.empty())
				str += _T("!");

			if (sSymbolName.empty())
				str += sAddrPCOffset;
			else {
				str += sSymbolName + _T("+") + sOffsInSymbol;
			}

			if (!sSourceFile.empty()) {
				size_t pos = sSourceFile.rfind('\\');
				if (pos >= 0)
					sSourceFile = sSourceFile.substr(pos + 1);
				str += _T(" [ ") + sSourceFile +  _T(": ") + sSourceLine + _T(" ] ");
			}
			outputer_->PutTableCell(str.c_str(), 32, true);
			outputer_->EndTableCell();
		}
		outputer_->EndSection();
	}
	return true;
}

bool DocumentExporter::ExportFileList(int& /*result*/) {
	outputer_->BeginSection(_T("File list"));

	// Print file list
	outputer_->BeginTableCell(1);
	outputer_->PutTableColumnName(_T("id"), 2, false);
	outputer_->PutTableColumnName(_T("Name"), 16, false);
	outputer_->PutTableColumnName(_T("Description"), 32, true);
	outputer_->EndTableCell();

	int nItemCount = get_table_row_count(hReport_, CRP_TBL_XMLDESC_FILE_ITEMS);
	int i;
	for (i = 0; i < nItemCount; i++) {
		outputer_->BeginTableCell(1);
		char szBuffer[10];
		_snprintf_s(szBuffer, 10, _T("%d"), i + 1);
		outputer_->PutTableCell(szBuffer, 2, false);
		string sFileName;
		get_prop(hReport_, CRP_TBL_XMLDESC_FILE_ITEMS, CRP_COL_FILE_ITEM_NAME,
				sFileName, i);
		outputer_->PutTableCell(sFileName.c_str(), 16, false);
		string sDesc;
		get_prop(hReport_, CRP_TBL_XMLDESC_FILE_ITEMS,
				CRP_COL_FILE_ITEM_DESCRIPTION, sDesc, i);
		outputer_->PutTableCell(sDesc.c_str(), 32, true);
		outputer_->EndTableCell();
	}

	outputer_->EndSection();
	return true;
}
bool DocumentExporter::ExportModuleList(int& result) {
  // Print module list
  outputer_->BeginSection(_T("Module List"));

  outputer_->BeginTableCell(1);
  outputer_->PutTableColumnName(_T("id"), 2, false);
  outputer_->PutTableColumnName(_T("Name"), 32, false);
  outputer_->PutTableColumnName(_T("SymLoadStatus"), 32, false);
  outputer_->PutTableColumnName(_T("LoadedPDBName"), 48, false);
  outputer_->PutTableColumnName(_T("LoadedImageName"), 48, true);
  outputer_->EndTableCell();

  // Get module count
  int nItemCount = get_table_row_count(hReport_, CRP_TBL_MDMP_MODULES);
  for (int i = 0; i < nItemCount; i++) {
	  outputer_->BeginTableCell(1);
    char szBuffer[10];
    _snprintf_s(szBuffer, 10, _T("%d"), i + 1);
    outputer_->PutTableCell(szBuffer, 2, false);

    string sModuleName;
    result = get_prop(hReport_, CRP_TBL_MDMP_MODULES, CRP_COL_MODULE_NAME,
        sModuleName, i);
    outputer_->PutTableCell(sModuleName.c_str(), 32, false);

    string sSymLoadStatus;
    result = get_prop(hReport_, CRP_TBL_MDMP_MODULES,
        CRP_COL_MODULE_SYM_LOAD_STATUS, sSymLoadStatus, i);
    outputer_->PutTableCell(sSymLoadStatus.c_str(), 32, false);

    string sLoadedPDBName;
    result = get_prop(hReport_, CRP_TBL_MDMP_MODULES,
        CRP_COL_MODULE_LOADED_PDB_NAME, sLoadedPDBName, i);
    outputer_->PutTableCell(sLoadedPDBName.c_str(), 48, false);

    string sLoadedImageName;
    result = get_prop(hReport_, CRP_TBL_MDMP_MODULES,
        CRP_COL_MODULE_LOADED_IMAGE_NAME, sLoadedImageName, i);
    outputer_->PutTableCell(sLoadedImageName.c_str(), 48, true);
    outputer_->EndTableCell();
  }
  outputer_->EndSection();
  return true;
}

bool DocumentExporter::ExportMinidumpLoadLog(int& result) {
  outputer_->BeginSection(_T("Minidump Load Log"));
  int nItemCount = get_table_row_count(hReport_, CRP_TBL_MDMP_LOAD_LOG);
  for (int i = 0; i < nItemCount; i++) {
	  outputer_->BeginTableCell(1);
    char szBuffer[10];
    _snprintf_s(szBuffer, 10, _T("%d"), i + 1);
    outputer_->PutTableCell(szBuffer, 2, false);

    string sEntry;
    result = get_prop(hReport_, CRP_TBL_MDMP_LOAD_LOG, CRP_COL_LOAD_LOG_ENTRY,
        sEntry, i);
    outputer_->PutTableCell(sEntry.c_str(), 64, true);
    outputer_->EndTableCell();
  }
  outputer_->EndSection();
  return true;
}

// Writes all error report properties to the file
ReturnCode DocumentExporter::Export() {
  int result = UNEXPECTED;
  outputer_->BeginDocument(_T("Error Report"));
  ExportSummary(result);
  ExportUserDefineInfo(result);
  ExportFileList(result);
  ExportThreadStack(result);
  ExportModuleList(result);
  ExportMinidumpLoadLog(result);

  outputer_->EndDocument();
  return SUCCESS;
}
