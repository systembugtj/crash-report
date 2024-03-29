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

/*! \file   CrashRptProbe.h
 *  \brief  Defines the interface for the crash_report_reader.dll.
 *  \date   2009-2011
 *  \author zexspectrum
 */

#ifndef __CRASHRPT_PROBE_H__
#define __CRASHRPT_PROBE_H__

#ifdef __cplusplus
#define CRASHRPTPROBE_EXTERNC extern "C"
#else
#define CRASHRPTPROBE_EXTERNC
#endif

// Define SAL macros to be empty if some old Visual Studio used
#ifndef __reserved
  #define __reserved
#endif
#ifndef __in
  #define __in
#endif
#ifndef __in_opt
  #define __in_opt
#endif
#ifndef __out
  #define __out
#endif
#ifndef __out_ecount
  #define __out_ecount(x)
#endif
#ifndef __out_ecount_z
  #define __out_ecount_z(x)
#endif

#define CRASHRPTPROBE_API(rettype) CRASHRPTPROBE_EXTERNC rettype WINAPI

//! Handle to an opened error report.
typedef int CrpHandle;

/*! \defgroup CrashRptProbeAPI CrashRptProbe Functions*/

/*! \ingroup CrashRptProbeAPI
 *  \brief Opens a zipped crash report file.
 *
 *  \return This function returns zero on success. 
 *
 *  \param[in] pszFileName Zipped report file name.
 *  \param[in] pszMd5Hash String containing MD5 hash for the ZIP file data.
 *  \param[in] pszSymSearchPath Symbol files (PDB) search path.
 *  \param[in] dwFlags Flags, reserved for future use.
 *  \param[out] phReport Handle to the opened crash report.
 *
 *  \remarks
 *
 *  Use this function to open a ZIP archive containing an error report. The error report typically contains
 *  several compressed files, such as XML crash description file, crash minidump file, and (optionally) 
 *  application-defined files.
 *
 *  \a pszFileName should be the name of the error report (ZIP file) to open. Absolute or relative path accepted.
 *  This parameter is required.
 *
 *  \a pszMd5Hash is a string containing the MD5 hash calculated for \a pszFileName. The MD5
 *  hash is a sequence of 16 characters being used for integrity checks. 
 *  If this parameter is NULL, integrity check is not performed.
 *
 *  If the error report is delivered by HTTP, the MD5 hash can be extracted by server-side script from the
 *  'md5' parameter. When the error report is delivered by email, the MD5 hash is attached to the mail message.
 *  The integrity check can be performed to ensure the error report was not corrupted during delivery.
 *  For more information, see \ref sending_error_reports.
 *
 *  \a pszSymSearchPath parameter defines where to look for symbols files (*.PDB). You can specify the list of 
 *  semicolon-separated directories to search in. If this parameter is NULL, the default search sequence is used.
 *  For the default search sequence, see the documentation for \b SymInitialize() function in MSDN.
 *
 *  Symbol files are required for crash report processing. They contain various information used by the debugger.
 *  For more information about saving symbol files, see \ref preparing_to_software_release.
 *
 *  \a dwFlags is currently not used, should be zero.
 *
 *  \a phReport parameter receives the handle to the opened crash report. If the function fails,
 *  this parameter becomes zero. 
 *
 *  This function does the following when opening report file:
 *    - It performs integrity checks for the error report being opened, if MD5 hash is specified.
 *    - It searches for crashrpt.xml and crashdump.dml files inside of ZIP archive, if such files
 *      do not present, it assumes the report was generated by CrashRpt v1.0. In such case it searches for
 *      any file having *.dmp or *.xml extension and assumes these are valid XML and DMP file.
 *    - It extracts and loads the XML file and checks its structure.
 *    - It extracts the minidump file to the temporary location.
 *
 *  On failure, use crpGetLastErrorMsg() function to get the last error message.
 *
 *  Use the crpCloseErrorReport() function to close the opened error report.
 *
 *  \note
 *
 *  The crpOpenErrorReportW() and crpOpenErrorReportA() are wide character and multibyte 
 *  character versions of crpOpenErrorReport(). 
 * 
 *
 *  \sa 
 *    crpCloseErrorReport()
 */

CRASHRPTPROBE_API(int)
crpOpenErrorReportW(
  __in const wchar_t* pszFileName,
  __in_opt const wchar_t* pszMd5Hash,
  __in_opt const wchar_t* pszSymSearchPath,
  __reserved DWORD dwFlags,
  __out CrpHandle* phReport
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpOpenErrorReportW()
 *
 */

CRASHRPTPROBE_API(int)
crpOpenErrorReportA(
  __in const char* pszFileName,
  __in_opt const char* pszMd5Hash,
  __in_opt const char* pszSymSearchPath,  
  __reserved DWORD dwFlags,
  __out CrpHandle* phReport
);

/*! \brief Character set-independent mapping of crpOpenErrorReportW() and crpOpenErrorReportA() functions. 
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpOpenErrorReport crpOpenErrorReportW
#else
#define crpOpenErrorReport crpOpenErrorReportA
#endif //UNICODE

/*! \ingroup CrashRptProbeAPI
 *  \brief Closes the crash report.
 *  \return This function returns zero on success.
 *  \param[in] hReport Handle to the opened error report.
 *
 *  \remarks
 *
 *  Use this function to close the error report previously opened with crpOpenErrorReport()
 *  function.
 *
 *  If this function fails, use crpGetLastErrorMsg() function to get the error message.
 *
 *  \sa
 *    crpOpenErrorReport(), crpOpenErrorReportW(), crpOpenErrorReportA(), crpGetLastErrorMsg()
 */

CRASHRPTPROBE_API(int) 
crpCloseErrorReport(
  CrpHandle hReport  
);

/* Table names passed to crpGetProperty() function. */

#define CRP_TBL_XMLDESC_MISC _T("XmlDescMisc")                //!< Table: Miscellaneous info contained in crash description XML file. 
#define CRP_TBL_XMLDESC_FILE_ITEMS _T("XmlDescFileItems")     //!< Table: The list of file items contained in error report.
#define CRP_TBL_XMLDESC_CUSTOM_PROPS _T("XmlDescCustomProps") //!< Table: The list of application-defined properties (available since v.1.2.1).
#define CRP_TBL_MDMP_MISC    _T("MdmpMisc")    //!< Table: Miscellaneous info contained in crash minidump file.  
#define CRP_TBL_MDMP_MODULES _T("MdmpModules") //!< Table: The list of loaded modules.
#define CRP_TBL_MDMP_THREADS _T("MdmpThreads") //!< Table: The list of threads.
#define CRP_TBL_MDMP_LOAD_LOG _T("MdmpLoadLog") //!< Table: Minidump loading log.

/* Meta information */

#define CRP_META_ROW_COUNT _T("RowCount") //!< Row count in the table.  

/* Column names passed to crpGetProperty() function. */

// Columns IDs of the CRP_XMLDESC_MISC table
#define CRP_COL_CRASHRPT_VERSION _T("CrashRptVersion") //!< Column: Version of CrashRpt library that generated the report.
#define CRP_COL_CRASH_GUID       _T("CrashGUID")       //!< Column: Globally unique identifier (GUID) of the error report.
#define CRP_COL_APP_NAME         _T("AppName")         //!< Column: Application name.
#define CRP_COL_APP_VERSION      _T("AppVersion")      //!< Column: Application version.
#define CRP_COL_IMAGE_NAME       _T("ImageName")       //!< Column: Path to the executable file.
#define CRP_COL_OPERATING_SYSTEM _T("OperatingSystem") //!< Column: Opration system name, including build number and service pack.
#define CRP_COL_SYSTEM_TIME_UTC  _T("SystemTimeUTC")   //!< Column: Time (UTC) when the crash occured.
#define CRP_COL_EXCEPTION_TYPE   _T("ExceptionType")   //!< Column: Code of exception handler that cought the exception.
#define CRP_COL_EXCEPTION_CODE   _T("ExceptionCode")   //!< Column: Exception code; for the structured exceptions only.
#define CRP_COL_INVPARAM_FUNCTION _T("InvParamFunction") //!< Column: Function name; for invalid parameter errors only.
#define CRP_COL_INVPARAM_EXPRESSION _T("InvParamExpression") //!< Column: Expression; for invalid parameter errors only.
#define CRP_COL_INVPARAM_FILE    _T("InvParamFile")    //!< Column: Source file name; for invalid parameter errors only.
#define CRP_COL_INVPARAM_LINE    _T("InvParamLine")    //!< Column: Source line; for invalid parameter errors only.
#define CRP_COL_FPE_SUBCODE      _T("FPESubcode")      //!< Column: Subcode of floating point exception; for FPE exceptions only.
#define CRP_COL_USER_EMAIL       _T("UserEmail")       //!< Column: Email of the user who sent this report.
#define CRP_COL_PROBLEM_DESCRIPTION _T("ProblemDescription") //!< Column: User-provided problem description.
#define CRP_COL_MEMORY_USAGE_KBYTES _T("MemoryUsageKbytes")  //!<  Column: Memory usage at the moment of crash (in KB).
#define CRP_COL_GUI_RESOURCE_COUNT _T("GUIResourceCount")    //!<  Column: Count of used GUI resources at the moment of crash.
#define CRP_COL_OPEN_HANDLE_COUNT  _T("OpenHandleCount")     //!<  Column: Count of open handles at the moment of crash.
#define CRP_COL_OS_IS_64BIT  _T("OSIs64Bit")                 //!<  Column: Operating system is 64-bit.
#define CRP_COL_GEO_LOCATION _T("GeoLocation")               //!<  Column: Geographic location of the error report sender.

// Column IDs of the CRP_XMLDESC_FILE_ITEMS table
#define CRP_COL_FILE_ITEM_NAME   _T("FileItemName")    //!< Column: File list: Name of the file contained in the report.
#define CRP_COL_FILE_ITEM_DESCRIPTION _T("FileItemDescription") //!< Column: File list: Description of the file contained in the report.

// Column IDs of the CRP_XMLDESC_CUSTOM_PROPS table
#define CRP_COL_PROPERTY_NAME   _T("PropertyName")     //!< Column: Name of the application-defined property.
#define CRP_COL_PROPERTY_VALUE  _T("PropertyValue")    //!< Column: Value of the application-defined property.

// Column IDs of the CRP_MDMP_MISC table
#define CRP_COL_CPU_ARCHITECTURE _T("CPUArchitecture") //!< Column: Processor architecture.
#define CRP_COL_CPU_COUNT        _T("CPUCount")        //!< Column: Number of processors.
#define CRP_COL_PRODUCT_TYPE     _T("ProductType")     //!< Column: Type of system (server or workstation).
#define CRP_COL_OS_VER_MAJOR     _T("OSVerMajor")      //!< Column: OS major version.
#define CRP_COL_OS_VER_MINOR     _T("OSVerMinor")      //!< Column: OS minor version.
#define CRP_COL_OS_VER_BUILD     _T("OSVerBuild")      //!< Column: OS build number.
#define CRP_COL_OS_VER_CSD       _T("OSVerCSD")        //!< Column: The latest service pack installed.
#define CRP_COL_EXCPTRS_EXCEPTION_CODE _T("ExptrsExceptionCode")  //!< Column: Code of the structured exception.
#define CRP_COL_EXCEPTION_ADDRESS _T("ExceptionAddress")          //!< Column: Exception address.
#define CRP_COL_EXCEPTION_THREAD_ROWID _T("ExceptionThreadROWID") //!< Column: ROWID in \ref CRP_TBL_MDMP_THREADS of the thread in which exception occurred.
#define CRP_COL_EXCEPTION_THREAD_STACK_MD5  _T("ExceptionThreadStackMD5") //!< Column: MD5 hash of the stack trace of the thread where exception occurred.
#define CRP_COL_EXCEPTION_MODULE_ROWID _T("ExceptionModuleROWID") //!< Column: ROWID in \ref CRP_TBL_MDMP_MODULES of the module in which exception occurred.

// Column IDs of the CRP_MDMP_MODULES table
#define CRP_COL_MODULE_NAME      _T("ModuleName")           //!< Column: Module name.
#define CRP_COL_MODULE_IMAGE_NAME _T("ModuleImageName")     //!< Column: Image name containing full path.  
#define CRP_COL_MODULE_BASE_ADDRESS _T("ModuleBaseAddress") //!< Column: Module base load address.
#define CRP_COL_MODULE_SIZE      _T("ModuleSize")           //!< Column: Module size.
#define CRP_COL_MODULE_LOADED_PDB_NAME _T("LoadedPDBName")  //!< Column: The full path and file name of the .pdb file. 
#define CRP_COL_MODULE_LOADED_IMAGE_NAME _T("LoadedImageName")  //!< Column: The full path and file name of executable file.
#define CRP_COL_MODULE_SYM_LOAD_STATUS _T("ModuleSymLoadStatus") //!< Column: Symbol load status for the module.

// Column IDs of the CRP_MDMP_THREADS table
#define CRP_COL_THREAD_ID            _T("ThdeadID")           //!< Column: Thread ID.
#define CRP_COL_THREAD_STACK_TABLEID _T("ThreadStackTABLEID") //!< Column: The table ID of the table containing stack trace for this thread.
  
// Column IDs of a stack trace table
#define CRP_COL_STACK_MODULE_ROWID     _T("StackModuleROWID")    //!< Column: Stack trace: ROWID of the module in the CRP_TBL_MODULES table.
#define CRP_COL_STACK_SYMBOL_NAME      _T("StackSymbolName")     //!< Column: Stack trace: symbol name.
#define CRP_COL_STACK_OFFSET_IN_SYMBOL _T("StackOffsetInSymbol") //!< Column: Stack trace: offset in symbol, hexadecimal.
#define CRP_COL_STACK_SOURCE_FILE      _T("StackSourceFile")     //!< Column: Stack trace: source file name.
#define CRP_COL_STACK_SOURCE_LINE      _T("StackSourceLine")     //!< Column: Stack trace: source file line number.
#define CRP_COL_STACK_ADDR_PC_OFFSET   _T("StackAddrPCOffset")   //!< Column: Stack trace: AddrPC offset.

// Column IDs of the CRP_MDMP_LOAD_LOG table
#define CRP_COL_LOAD_LOG_ENTRY _T("LoadLogEntry")   //!< Column: A entry of the minidump loading log.

/*! \ingroup CrashRptProbeAPI
 *  \brief Retrieves a string property from crash report.
 *  \return This function returns zero on success, with one exception (see Remarks for more information).
 *
 *  \param[in]  hReport Handle to the previously opened crash report.
 *  \param[in]  lpszTableId Table ID.
 *  \param[in]  lpszColumnId Column ID.
 *  \param[in]  nRowIndex Index of the row in the table.
 *  \param[out] lpszBuffer Output buffer.
 *  \param[in]  cchBuffSize Size of the output buffer in characters.
 *  \param[out] pcchCount Count of characters written to the buffer.
 *
 *  \remarks
 *
 *  Use this function to retrieve data from the crash report that was previously opened with the
 *  crpOpenErrorReport() function.
 *
 *  Properties are organized into tables having rows and columns. For the list of available tables,
 *  see \ref using_crashrptprobe_api.
 *
 *  To get the number of rows in a table, pass the constant \ref CRP_META_ROW_COUNT as column ID. In this case the
 *  function returns number of rows as its return value, or negative value on failure. 
 *
 *  In all other cases the function returns zero on success.
 *
 *  Some properties are loaded from crash description XML file, while others are loaded from crash minidump file.
 *  The minidump is loaded once when you retrive a property from it. This reduces the overall processing time.
 *
 *  \a hReport should be the handle to the opened error report.
 *
 *  \a lpszTableId represente the ID of the table.
 *
 *  \a lpszColumnId represents the ID of the column in the table. 
 *
 *  \a nRowIndex defines the zero-based index of the row in the table.
 *  
 *  \a lpszBuffer defines the buffer where retrieved property value will be placed. If this parameter
 *  is NULL, it is ignored and \a pcchCount is set with the required size in characters of the buffer.
 *
 *  \a cchBuffSize defines the buffer size in characters. To calculate required buffer size, set \a lpszBuffer with NULL, 
 *  the function will set \a pcchCount with the number of characters required.
 *
 *  \a pcchCount is set with the actual count of characters copied to the \a lpszBuffer. If this parameter is NULL,
 *  it is ignored.
 *
 *  If this function fails, use crpGetLastErrorMsg() function to get the error message.
 *
 *  For code examples of using this function, see \ref crashrptprobe_api_examples.
 *
 *  \note
 *  The crpGetPropertyW() and crpGetPropertyA() are wide character and multibyte 
 *  character versions of crpGetProperty(). 
 *
 *  \sa
 *    crpGetPropertyW(), crpGetPropertyA(), crpOpenErrorReport(), crpGetLastErrorMsg()
 */ 

CRASHRPTPROBE_API(int) 
crpGetPropertyW(
  CrpHandle hReport,
  const wchar_t* lpszTableId,
  const wchar_t* lpszColumnId,
  INT nRowIndex,
  __out_ecount_z(pcchBuffSize) LPWSTR lpszBuffer,
  ULONG cchBuffSize,
  __out PULONG pcchCount
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpGetPropertyW()
 *
 */

CRASHRPTPROBE_API(int) 
crpGetPropertyA(
  CrpHandle hReport,
  const char* lpszTableId,
  const char* lpszColumnId,
  INT nRowIndex,
  __out_ecount_z(pcchBuffSize) LPSTR lpszBuffer,
  ULONG cchBuffSize,
  __out PULONG pcchCount
);

/*! \brief Character set-independent mapping of crpGetPropertyW() and crpGetPropertyA() functions. 
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpGetProperty crpGetPropertyW
#else
#define crpGetProperty crpGetPropertyA
#endif //UNICODE

/*! \ingroup CrashRptProbeAPI
 *  \brief Extracts a file from the opened error report.
 *  \return This function returns zero if succeeded.
 *
 *  \param[in] hReport Handle to the opened error report.
 *  \param[in] lpszFileName The name of the file to extract.
 *  \param[in] lpszFileSaveAs The resulting name of the extracted file.
 *  \param[in] bOverwriteExisting Overwrite the destination file if it already exists?
 *
 *  \remarks
 *
 *  Use this function to extract a compressed file from the error report (ZIP) file.
 *
 *  \a lpszFileName parameter should be the name of the file to extract. For more information
 *  about enumerating file names, see \ref crashrptprobe_api_examples.
 *
 *  \a lpszFileSaveAs defines the name of the file to extract to. 
 *
 *  \a bOverwriteExisting flag defines the behavior when the destination file already exists.
 *  If this parameter is TRUE, the file is overwritten, otherwise the function fails.
 *
 *  If this function fails, use crpGetLastErrorMsg() to retrieve the error message.
 *
 *  \note
 *    The crpExtractFileW() and crpExtractFileA() are wide character and multibyte 
 *    character versions of crpExtractFile(). 
 *
 *  \sa
 *    crpExtractFileA(), crpExtractFileW(), crpExtractFile()
 */

CRASHRPTPROBE_API(int) 
crpExtractFileW(
  CrpHandle hReport,
  const wchar_t* lpszFileName,
  const wchar_t* lpszFileSaveAs,
  BOOL bOverwriteExisting
);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpExtractFileW() 
 */

CRASHRPTPROBE_API(int) 
crpExtractFileA(
  CrpHandle hReport,
  const char* lpszFileName,
  const char* lpszFileSaveAs,
  BOOL bOverwriteExisting
);

/*! \brief Character set-independent mapping of crpExtractFileW() and crpExtractFileA() functions. 
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpExtractFile crpExtractFileW
#else
#define crpExtractFile crpExtractFileA
#endif //UNICODE

/*! \ingroup CrashRptProbeAPI 
 *  \brief Gets the last CrashRptProbe error message.
 *
 *  \return This function returns length of error message in characters.
 *
 *  \param[out] pszBuffer Pointer to the buffer.
 *  \param[in]  cchBuffSize Size of buffer in characters.
 *
 *  \remarks
 *
 *  This function gets the last CrashRptProbe error message. You can use this function
 *  to retrieve the text status of the last called CrashRptProbe function.
 *
 *  If buffer is too small for the error message, the message is truncated.
 *
 *  \note 
 *    crpGetLastErrorMsgW() and crpGetLastErrorMsgA() are wide-character and multi-byte character versions
 *    of crpGetLastErrorMsg(). The crpGetLastErrorMsg() macro defines character set independent mapping.
 *
 *  The following example shows how to use crpGetLastErrorMsg() function.
 *
 *  \code
 *  
 *  // .. call some CrashRptProbe function
 *
 *  // Get the status message
 *  TCHAR szErrorMsg[256];
 *  crpGetLastErrorMsg(szErrorMsg, 256);
 *  \endcode
 *
 *  \sa crpGetLastErrorMsgA(), crpGetLastErrorMsgW(), crpGetLastErrorMsg()
 */

CRASHRPTPROBE_API(int)
crpGetLastErrorMsgW(
  __out_ecount(cchBuffSize) LPWSTR pszBuffer, 
  __in UINT cchBuffSize);

/*! \ingroup CrashRptProbeAPI
 *  \copydoc crpGetLastErrorMsgW()
 *
 */

CRASHRPTPROBE_API(int)
crpGetLastErrorMsgA(
  __out_ecount(cchBuffSize) LPSTR pszBuffer, 
  __in UINT cchBuffSize);

/*! \brief Defines character set-independent mapping for crpGetLastErrorMsgW() and crpGetLastErrorMsgA().
 *  \ingroup CrashRptProbeAPI
 */

#ifdef UNICODE
#define crpGetLastErrorMsg crpGetLastErrorMsgW
#else
#define crpGetLastErrorMsg crpGetLastErrorMsgA
#endif //UNICODE


#endif __CRASHRPT_PROBE_H__

