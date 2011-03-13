#ifndef DOCUMENT_EXPORTER_H_
#define DOCUMENT_EXPORTER_H_

#define NOGDI
#include <windows.h>
#include <iostream>
#include <string>
#include "output.h"
#include "crash_report_probe.h"

// Return codes
enum ReturnCode {
  SUCCESS = 0, // OK
  UNEXPECTED = 1, // Unexpected error
  INVALIDARG = 2, // Invalid argument
  INVALIDMD5 = 3, // Integrity check failed
  EXTRACTERR = 4
// File extraction error
};

int get_prop(CrpHandle hReport, const char* table_id, const char* column_id,
    std::string& str, int row_id = 0);
int get_table_row_count(CrpHandle hReport, LPCTSTR table_id);

class DocumentExporter {
public:
  DocumentExporter(CrpHandle hReport, FILE* f, std::string format);
  ~DocumentExporter();
  ReturnCode Export();

private:
  bool ExportSummary(int& result);
  bool ExportUserDefineInfo(int& result);
  bool ExportFileList(int& result);
  bool ExportThreadStack(int& result);
  bool ExportModuleList(int& result);
  bool ExportMinidumpLoadLog(int& result);

  AbstractOutputter* outputer_;
  CrpHandle hReport_;
};

#endif //  DOCUMENT_EXPORTER_H_
