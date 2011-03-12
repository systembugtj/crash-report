#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <stdio.h>
#include <stdlib.h>

class AbstractOutputter {
public:
  AbstractOutputter(){}
  virtual ~AbstractOutputter(){}
  virtual void Init(FILE* f);
  virtual void BeginDocument(const char* pszTitle) = 0 ;
  virtual void EndDocument() = 0 ;
  virtual void BeginSection(const char* pszTitle) = 0 ;
  virtual void EndSection() = 0 ;
  virtual void PutRecord(const char* pszName, const char* pszValue) = 0 ;
  virtual void PutTableCell(const char* pszValue, int width, bool bLastInRow) = 0 ;
  virtual void PutTableColumnName(const char* pszValue, int width, bool bLastInRow) = 0 ;
  virtual void BeginTableCell( int border) = 0 ;
  virtual void EndTableCell() = 0 ;
protected:
  FILE* fout_;
  };

class PlainTextOutputter : public AbstractOutputter {
public:
  PlainTextOutputter(){}
  virtual ~PlainTextOutputter();
  virtual void BeginDocument(const char* pszTitle);
  virtual void EndDocument();
  virtual void BeginSection(const char* pszTitle);
  virtual void EndSection();
  virtual void PutRecord(const char* pszName, const char* pszValue);
  virtual void PutTableCell(const char* pszValue, int width, bool bLastInRow);
  virtual void PutTableColumnName(const char* pszValue, int width, bool bLastInRow){
	  PutTableCell(pszValue, width, bLastInRow);
  }
  virtual void BeginTableCell(int border){}
  virtual void EndTableCell(){}
};

class HtmlOutputter : public AbstractOutputter {
public:
  HtmlOutputter(){}
  virtual ~HtmlOutputter();
  virtual void BeginDocument(const char* pszTitle);
  virtual void EndDocument();
  virtual void BeginSection(const char* pszTitle);
  virtual void EndSection();
  virtual void PutRecord(const char* pszName, const char* pszValue);
  virtual void PutTableCell(const char* pszValue, int width, bool bLastInRow);
  virtual void PutTableColumnName(const char* pszValue, int width, bool bLastInRow);
  virtual void BeginTableCell(int border);
  virtual void EndTableCell();
  };
#endif // OUTPUT_H_

