// COutputter
// This class is used for generating the content of the resulting file.
// Currently text format is supported.
// TODO(yesp) :这里应该实现一个基础类，
//  使得各种输出都有一个统一的接口。并且这里应该实现一个输出格式为Html的类。
#include "output.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <assert.h>

using namespace std;

void AbstractOutputter::Init(FILE* f) {
  assert(f != NULL);
  fout_ = f;
}

void PlainTextOutputter::BeginDocument(const char* pszTitle) {
  fprintf(fout_, "==== %s ==== \n\n", pszTitle);
}

void PlainTextOutputter::EndDocument() {
  fprintf(fout_, "\n== END ==\n");
}

void PlainTextOutputter::BeginSection(const char* pszTitle) {
  fprintf(fout_, "=====================     %s     =====================\n\n", pszTitle);
}

void PlainTextOutputter::EndSection() {
  fprintf(fout_, "\n\n");
}

void PlainTextOutputter::PutRecord(const char* pszName, const char* pszValue) {
  fprintf(fout_, "%s = %s\n", pszName, pszValue);
}

void PlainTextOutputter::PutTableCell(const char* pszValue, int width, bool bLastInRow) {
  char szFormat[32];
   _snprintf_s(szFormat, 32, "%%-%ds%s", width, bLastInRow ? "\n" :" ");
  fprintf(fout_, szFormat, pszValue);
}

PlainTextOutputter::~PlainTextOutputter() {
  //  do nothing now
}


void HtmlOutputter::BeginDocument(const char* pszTitle) {
  fprintf(fout_, "<!doctype html><html><head><meta http-equiv=\"content-type\""
                  "content=\"text/html; charset=UTF-8\"><title>%s</title></head>"
                  "<body>", pszTitle);
}

void HtmlOutputter::EndDocument() {
  fprintf(fout_, "</body></html>");
  }

void HtmlOutputter::BeginSection(const char* pszTitle) {
  fprintf(fout_, "<hr><h1><center>%s</center></h1>", pszTitle);
  fprintf(fout_, "<table border=\"1\">", pszTitle);
  }

void HtmlOutputter::EndSection() {
  fprintf(fout_, "</table>");
  }

void HtmlOutputter::PutRecord(const char* pszName, const char* pszValue) {
  fprintf(fout_, "<tr><td>%s</td><td>%s</td></tr>", pszName, pszValue);
  }

void HtmlOutputter::PutTableCell(const char* pszValue, int /* width */, bool /*bLastInRow*/) {
  fprintf(fout_, "<td>%s</td>", pszValue);
  }

HtmlOutputter::~HtmlOutputter() {
	//  do nothing now
}
void HtmlOutputter::PutTableColumnName(const char* pszValue, int /* width */, bool /*bLastInRow*/) {
	fprintf(fout_, "<th>%s</th>", pszValue);
}
void HtmlOutputter::BeginTableCell(int /*border*/) {
	fprintf(fout_, "<tr>");
}
void HtmlOutputter::EndTableCell() {
	fprintf(fout_, "</tr>");
}
