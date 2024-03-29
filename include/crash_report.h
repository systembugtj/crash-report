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

#ifndef _CRASHRPT_H_
#define _CRASHRPT_H_
#include <iostream>
#include <string>
#define NOGDI
#include <windows.h>
#include <dbghelp.h>
#include <assert.h>
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
#ifndef __out_ecount_z
#define __out_ecount_z(x)
#endif
#ifdef __cplusplus
#define CRASHRPT_EXTERNC extern "C"
#else
#define CRASHRPT_EXTERNC
#endif
#define CRASHRPTAPI(rettype) CRASHRPT_EXTERNC rettype WINAPI
//! Current CrashRpt version
#define CRASHRPT_VER 1208
typedef BOOL (CALLBACK *LPGETLOGFILE) (__reserved LPVOID lpvState);

enum SendMethod {
  CR_HTTP_MutilPart = 0,
  CR_HTTP_Base64 = 1,
  CR_TCP_Demo = 2,
  CR_UDT = 3,
  CR_FTP = 4,
};

#define CR_NEGATIVE_PRIORITY ((UINT)-1)
#define CR_CRASH_LOG_FILE "crash_log.dat"

// Flags for CR_INSTALL_INFO::falgs
#define CR_INST_STRUCTURED_EXCEPTION_HANDLER   0x1    //   Install SEH handler (deprecated name, use \ref CR_INST_SEH_EXCEPTION_HANDLER instead).
#define CR_INST_SEH_EXCEPTION_HANDLER          0x1    //   Install SEH handler.
#define CR_INST_TERMINATE_HANDLER              0x2    //   Install terminate handler.
#define CR_INST_UNEXPECTED_HANDLER             0x4    //   Install unexpected handler.
#define CR_INST_PURE_CALL_HANDLER              0x8    //   Install pure call handler (VS .NET and later).
#define CR_INST_NEW_OPERATOR_ERROR_HANDLER     0x10   //   Install new operator error handler (VS .NET and later).
#define CR_INST_SECURITY_ERROR_HANDLER         0x20   //   Install security errror handler (VS .NET and later).
#define CR_INST_INVALID_PARAMETER_HANDLER      0x40   //   Install invalid parameter handler (VS 2005 and later).
#define CR_INST_SIGABRT_HANDLER                0x80   //   Install SIGABRT signal handler.
#define CR_INST_SIGFPE_HANDLER                 0x100  //   Install SIGFPE signal handler.
#define CR_INST_SIGILL_HANDLER                 0x200  //   Install SIGILL signal handler.
#define CR_INST_SIGINT_HANDLER                 0x400  //   Install SIGINT signal handler.
#define CR_INST_SIGSEGV_HANDLER                0x800  //   Install SIGSEGV signal handler.
#define CR_INST_SIGTERM_HANDLER                0x1000 //   Install SIGTERM signal handler.
#define CR_INST_ALL_EXCEPTION_HANDLERS         0       //   Install all possible exception handlers.
#define CR_INST_CRT_EXCEPTION_HANDLERS         0x1FFE  //   Install exception handlers for the linked CRT module.
#define CR_INST_NO_GUI                         0x2000  //   Do not show GUI, send report silently (use for non-GUI apps only).
#define CR_INST_DONT_SEND_REPORT               0x8000  //   Don't send error report immediately, just save it locally.
#define CR_INST_APP_RESTART                    0x10000 //   Restart the application on crash.
#define CR_INST_NO_MINIDUMP                    0x20000 //   Do not include minidump file to crash report.
#define CR_INST_SEND_QUEUED_REPORTS            0x40000 //   CrashRpt should send error reports that are waiting to be delivered.
#define CR_INST_STORE_ZIP_ARCHIVES             0x80000 //   CrashRpt should store both uncompressed error report files and ZIP archives.

typedef struct tagCR_INSTALL_INFOW {
	WORD size;
	const wchar_t* application_name;
	const wchar_t* application_version;
	const wchar_t* crash_server_url;
  const wchar_t* crash_server_host;
  int crash_server_port;
	const wchar_t* sender_path;
	LPGETLOGFILE crash_callback;
	SendMethod send_method;
	DWORD flags;
	const wchar_t* privacy_policy_url;
	//   File name or folder of Debug help DLL.
	const wchar_t* debug_help_dll;
	MINIDUMP_TYPE minidump_type;
	const wchar_t* save_dir;
	const wchar_t* restart_cmd;
	const wchar_t* langpack_path;
	const wchar_t* custom_sender_icon;
} CR_INSTALL_INFOW;

typedef CR_INSTALL_INFOW *PCR_INSTALL_INFOW;

//  see CR_INSTALL_INFOW for details
typedef struct tagCR_INSTALL_INFOA {
	WORD size;
	const char* application_name;
	const char* application_version;
	const char* crash_server_url;
  const char* crash_server_host;
  int crash_server_port;
	const char* sender_path;
	LPGETLOGFILE crash_callback;
  SendMethod send_method;
	DWORD flags;
	const char* privacy_policy_url;
	const char* debug_help_dll;
	MINIDUMP_TYPE minidump_type;
	const char* save_dir;
	const char* restart_cmd;
	const char* langpack_path;
	const char* custom_sender_icon;
} CR_INSTALL_INFOA;

typedef CR_INSTALL_INFOA *PCR_INSTALL_INFOA;

/*! \brief Character set-independent mapping of CR_INSTALL_INFOW and CR_INSTALL_INFOA structures.
 *  \ingroup CrashRptStructs
 */
#ifdef UNICODE
typedef CR_INSTALL_INFOW CR_INSTALL_INFO;
typedef PCR_INSTALL_INFOW PCR_INSTALL_INFO;
#else
typedef CR_INSTALL_INFOA CR_INSTALL_INFO;
typedef PCR_INSTALL_INFOA PCR_INSTALL_INFO;
#endif // UNICODE
/*! \ingroup CrashRptAPI
 *  \brief  Installs exception handlers for the caller process.
 *
 *  \param[in] pInfo General information.
 *
 *  \remarks
 *    This function installs unhandled exception filter for the caller process.
 *    It also installs various CRT exception/error handlers that function for all threads of the caller process.
 *    For more information, see \ref exception_handling
 *
 *    Below is the list of installed handlers:
 *     - Top-level SEH exception filter [ \c SetUnhandledExceptionFilter() ]
 *     - C++ pure virtual call handler (Visual Studio .NET 2003 and later) [ \c _set_purecall_handler() ]
 *     - C++ invalid parameter handler (Visual Studio .NET 2005 and later) [ \c _set_invalid_parameter_handler() ]
 *     - C++ new operator error handler (Visual Studio .NET 2003 and later) [ \c _set_new_handler() ]
 *     - C++ buffer overrun handler (Visual Studio .NET 2003 only) [ \c _set_security_error_handler() ]
 *     - C++ abort handler [ \c signal(SIGABRT) ]
 *     - C++ illegal instruction handler [ \c signal(SIGINT) ]
 *     - C++ termination request [ \c signal(SIGTERM) ]
 *
 *    In a multithreaded program, additionally use crInstallToCurrentThread2() function for each execution
 *    thread, except the main one.
 *
 *    The \a pInfo parameter contains all required information needed to install CrashRpt.
 *
 *    This function fails when \a pInfo->sender_path doesn't contain valid path to crash_sender.exe
 *    or when \a pInfo->sender_path is equal to NULL, but \b crash_sender.exe is not located in the
 *    directory where \b crash_report.dll located.
 *
 *    On crash, the crash minidump file is created, which contains CPU information and
 *    stack trace information. Also XML file is created that contains additional
 *    information that may be helpful for crash analysis. These files along with several additional
 *    files added with crAddFile2() are packed to a single ZIP file.
 *
 *    When crash information is collected, another process, <b>crash_sender.exe</b>, is launched
 *    and the process where crash had occured is terminated. The CrashSender process is
 *    responsible for letting the user know about the crash and send the error report.
 *
 *    The error report can be sent over E-mail using address and subject passed to the
 *    function as \ref CR_INSTALL_INFO structure members. Another way of sending error report is an HTTP
 *    request using \a crash_server_url member of \ref CR_INSTALL_INFO.
 *
 *    This function may fail if an appropriate language file (\b crashrpt_lang.ini) is not found
 *    in the directory where the \b crash_sender.exe file is located.
 *
 *    If this function fails, use crGetLastErrorMsg() to retrieve the error message.
 *
 *    crInstallW() and crInstallA() are wide-character and multi-byte character versions of crInstall()
 *    function. The \ref crInstall macro defines character set independent mapping for these functions.
 *
 *    For code example, see \ref simple_example.
 *
 *  \sa crInstallW(), crInstallA(), crInstall(), CR_INSTALL_INFOW,
 *      CR_INSTALL_INFOA, CR_INSTALL_INFO, crUninstall(),
 *      CrAutoInstallHelper
 */

CRASHRPTAPI(int)
crInstallW(
		__in PCR_INSTALL_INFOW pInfo
);

/*! \ingroup CrashRptAPI
 *  \copydoc crInstallW()
 */

CRASHRPTAPI(int)
crInstallA(
		__in PCR_INSTALL_INFOA pInfo
);

/*! \brief Character set-independent mapping of crInstallW() and crInstallA() functions.
 * \ingroup CrashRptAPI
 */
#ifdef UNICODE
#define crInstall crInstallW
#else
#define crInstall crInstallA
#endif //UNICODE
/*! \ingroup CrashRptAPI
 *  \brief Unsinstalls exception handlers previously installed with crInstall().
 *
 *  \return
 *    This function returns zero if succeeded.
 *
 *  \remarks
 *
 *    Call this function on application exit to uninstall exception
 *    handlers previously installed with crInstall(). After function call, the exception handlers
 *    are restored to states they had before calling crInstall().
 *
 *    This function fails if crInstall() wasn't previously called in context of the
 *    caller process.
 *
 *    When this function fails, use crGetLastErrorMsg() to retrieve the error message.
 *
 *  \sa crInstallW(), crInstallA(), crInstall(), crUninstall(),
 *      CrAutoInstallHelper
 */

CRASHRPTAPI(int)
crUninstall();

/*! \ingroup DeprecatedAPI
 *  \brief Installs exception handlers to the current thread.
 *
 *  \return This function returns zero if succeeded.
 *
 *  \remarks
 *
 *   This function sets exception handlers for the caller thread. If you have
 *   several execution threads, you ought to call the function for each thread,
 *   except the main one.
 *
 *   The list of C++ exception/error handlers installed with this function:
 *    - terminate handler [ \c set_terminate() ]
 *    - unexpected handler [ \c set_unexpected() ]
 *    - floating point error handler [ \c signal(SIGFPE) ]
 *    - illegal instruction handler [ \c signal(SIGILL) ]
 *    - illegal storage access handler [ \c signal(SIGSEGV) ]
 *
 *   The crInstall() function automatically installs C++ exception handlers for the
 *   main thread, so no need to call crInstallToCurrentThread() for the main thread.
 *
 *   This function fails if calling it twice for the same thread.
 *   When this function fails, use crGetLastErrorMsg() to retrieve the error message.
 *
 *   Call crUninstallFromCurrentThread() to uninstall C++ exception handlers from
 *   current thread.
 *
 *   This function is deprecatd. The crInstallToCurrentThread2() function gives
 *   better control of what exception handlers to install.
 *
 *   \sa crInstallToCurrentThread2(),
 *       crUninstallFromCurrentThread(), CrThreadAutoInstallHelper
 */

CRASHRPTAPI(int)
crInstallToCurrentThread();

/*! \ingroup CrashRptAPI
 *  \brief Installs exception handlers to the caller thread.
 *  \return This function returns zero if succeeded.
 *  \param[in] dwFlags Flags.
 *
 *  \remarks
 *
 *  This function is available <b>since v.1.1.2</b>.
 *
 *  The function sets exception handlers for the caller thread. If you have
 *  several execution threads, you ought to call the function for each thread,
 *  except the main one.
 *
 *  The function works the same way as obsolete crInstallToCurrentThread(), but provides
 *  an ability to select what exception handlers to install.
 *
 *  \a dwFlags defines what exception handlers to install. Use zero value
 *  to install all possible exception handlers. Or use a combination of the following constants:
 *
 *      - \ref CR_INST_TERMINATE_HANDLER              Install terminate handler
 *      - \ref CR_INST_UNEXPECTED_HANDLER             Install unexpected handler
 *      - \ref CR_INST_SIGFPE_HANDLER                 Install SIGFPE signal handler
 *      - \ref CR_INST_SIGILL_HANDLER                 Install SIGILL signal handler
 *      - \ref CR_INST_SIGSEGV_HANDLER                Install SIGSEGV signal handler
 *
 *  Example:
 *
 *   \code
 *   DWORD WINAPI ThreadProc(LPVOID lpParam)
 *   {
 *     // Install exception handlers
 *     crInstallToCurrentThread2(0);
 *
 *     // Your code...
 *
 *     // Uninstall exception handlers
 *     crUninstallFromCurrentThread();
 *
 *     return 0;
 *   }
 *   \endcode
 *
 *  \sa
 *    crInstallToCurrentThread()
 */

CRASHRPTAPI(int)
crInstallToCurrentThread2(DWORD dwFlags);

/*! \ingroup CrashRptAPI
 *  \brief Uninstalls C++ exception handlers from the current thread.
 *  \return This function returns zero if succeeded.
 *
 *  \remarks
 *
 *    This function unsets exception handlers from the caller thread. If you have
 *    several execution threads, you ought to call the function for each thread.
 *    After calling this function, the exception handlers for current thread are
 *    replaced with the handlers that were before call of crInstallToCurrentThread2().
 *
 *    This function fails if crInstallToCurrentThread2() wasn't called for current thread.
 *
 *    When this function fails, use crGetLastErrorMsg() to retrieve the error message.
 *
 *    No need to call this function for the main execution thread. The crUninstall()
 *    will automatically uninstall C++ exception handlers for the main thread.
 *
 *   \sa crInstallToCurrentThread(), crInstallToCurrentThread2(),
 *       crUninstallFromCurrentThread(), CrThreadAutoInstallHelper
 */

CRASHRPTAPI(int)
crUninstallFromCurrentThread();

/*! \ingroup DeprecatedAPI
 *  \brief Adds a file to crash report.
 *
 *  \return This function returns zero if succeeded.
 *
 *  \param[in] pszFile Absolute path to the file to add.
 *  \param[in] pszDesc File description (used in Error Report Details dialog).
 *
 *  \note
 *    This function is deprecated and will be removed in one of the future releases. It is recommended to use crAddFile2() function instead.
 *
 *    This function can be called anytime after crInstall() to add one or more
 *    files to the generated crash report.
 *
 *    When this function is called, the file is marked to be added to the error report,
 *    then the function returns control to the caller.
 *    When crash occurs, all marked files are added to the report by the \b crash_sender.exe process.
 *    If a file is locked by someone for exclusive access, the file won't be included. Inside of \ref LPGETLOGFILE crash callback,
 *    ensure files to be included are acessible for reading.
 *
 *    \a pszFile should be a valid absolute path of a file to add to crash report.
 *
 *    \a pszDesc is a description of a file. It can be NULL.
 *
 *    This function fails if \a pszFile doesn't exist at the moment of function call.
 *
 *    The crAddFileW() and crAddFileA() are wide-character and multibyte-character
 *    versions of crAddFile() function. The crAddFile() macro defines character set
 *    independent mapping.
 *
 *  \sa crAddFileW(), crAddFileA(), crAddFile()
 */

CRASHRPTAPI(int)
crAddFileW(
		const wchar_t* pszFile,
		const wchar_t* pszDesc
);

/*! \ingroup DeprecatedAPI
 *  \copydoc crAddFileW()
 */

CRASHRPTAPI(int)
crAddFileA(
		const char* pszFile,
		const char* pszDesc
);

/*! \brief Character set-independent mapping of crAddFileW() and crAddFileA() functions.
 *  \ingroup DeprecatedAPI
 */
#ifdef UNICODE
#define crAddFile crAddFileW
#else
#define crAddFile crAddFileA
#endif //UNICODE
// Flags for crAddFile2() function.

#define CR_AF_TAKE_ORIGINAL_FILE  0 //   Take the original file (do not copy it to the error report folder).
#define CR_AF_MAKE_FILE_COPY      1 //   Copy the file to the error report folder.
#define CR_AF_FILE_MUST_EXIST     0 //   Function will fail if file doesn't exist at the moment of function call.
#define CR_AF_MISSING_FILE_OK     2 //   Do not fail if file is missing (assume it will be created later).
/*! \ingroup CrashRptAPI
 *  \brief Adds a file to crash report.
 *
 *  \return This function returns zero if succeeded.
 *
 *  \param[in] pszFile     Absolute path to the file to add, required.
 *  \param[in] pszDestFile Destination file name, optional.
 *  \param[in] pszDesc     File description (used in Error Report Details dialog), optional.
 *  \param[in] dwFlags     Flags, optional.
 *
 *    This function can be called anytime after crInstall() to add one or more
 *    files to the generated crash report.
 *
 *    When this function is called, the file is marked to be added to the error report,
 *    then the function returns control to the caller.
 *    When crash occurs, all marked files are added to the report by the \b crash_sender.exe process.
 *    If a file is locked by someone for exclusive access, the file won't be included. Inside of \ref LPGETLOGFILE crash callback,
 *    close open file handles and ensure files to be included are acessible for reading.
 *
 *    \a pszFile should be a valid absolute path of a file to add to crash report.
 *
 *    \a pszDestFile should be the name of destination file. This parameter can be used
 *    to specify different file name for the file in ZIP archive. If this parameter is NULL, the pszFile
 *    file name is used as destination file name.
 *
 *    \a pszDesc is a literal description of a file. It can be NULL.
 *
 *    \a dwFlags parameter defines the behavior of the function. This can be a combination of the following flags:
 *       - \ref CR_AF_TAKE_ORIGINAL_FILE  On crash, the \b crash_sender.exe will try to locate the file from its original location. This behavior is the default one.
 *       - \ref CR_AF_MAKE_FILE_COPY      On crash, the \b crash_sender.exe will make a copy of the file and save it to the error report folder.
 *
 *       - \ref CR_AF_FILE_MUST_EXIST     The function will fail if file doesn't exist at the moment of function call (the default behavior).
 *       - \ref CR_AF_MISSING_FILE_OK     Do not fail if file is missing (assume it will be created later).
 *
 *    If you use postponed error report delivery (if you specify \ref CR_INST_SEND_QUEUED_REPORTS flag for \ref CR_INSTALL_INFO::dwFlags structure member)
 *    you must also specify the \ref CR_AF_MAKE_FILE_COPY as \a dwFlags parameter value. This will
 *    guarantee that a snapshot of your file at the moment of crash is taken and saved to the error report folder.
 *
 *    This function fails if \a pszFile doesn't exist at the moment of function call,
 *    unless you specify \ref CR_AF_MISSING_FILE_OK flag.
 *
 *    The crAddFile2W() and crAddFile2A() are wide-character and multibyte-character
 *    versions of crAddFile2() function. The crAddFile2() macro defines character set
 *    independent mapping.
 *
 *    This function is available <b>since v.1.2.1</b>. This function replaces the crAddFile() function.
 *
 *  \sa crAddFile2W(), crAddFile2A(), crAddFile2()
 */

CRASHRPTAPI(int)
crAddFile2W(
		const wchar_t* pszFile,
		const wchar_t* pszDestFile,
		const wchar_t* pszDesc,
		DWORD dwFlags
);

/*! \ingroup CrashRptAPI
 *  \copydoc crAddFile2W()
 */

CRASHRPTAPI(int)
crAddFile2A(
		const char* pszFile,
		const char* pszDestFile,
		const char* pszDesc,
		DWORD dwFlags
);

/*! \brief Character set-independent mapping of crAddFile2W() and crAddFile2A() functions.
 *  \ingroup CrashRptAPI
 */
#ifdef UNICODE
#define crAddFile2 crAddFile2W
#else
#define crAddFile2 crAddFile2A
#endif //UNICODE
// Flags for crAddScreenshot function.
#define CR_AS_VIRTUAL_SCREEN  0  //   Take a screenshot of the virtual screen.
#define CR_AS_MAIN_WINDOW     1  //   Take a screenshot of application's main window.
#define CR_AS_PROCESS_WINDOWS 2  //   Take a screenshot of all visible process windows.
#define CR_AS_GRAYSCALE_IMAGE 4  //   Make a grayscale image instead of a full-color one.
#define CR_AS_USE_JPEG_FORMAT 8  //   Store screenshots as JPG files.
/*! \ingroup CrashRptAPI
 *  \brief Adds a screenshot to the crash report.
 *
 *  \return This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
 *
 *  \param[in] dwFlags Flags, optional.
 *
 *  \remarks
 *
 *  This function can be used to take a screenshot at the moment of crash and add it to the error report.
 *  Screenshot information may help the developer to better understand the state of the application
 *  at the moment of crash and reproduce the error.
 *
 *  When this function is called, screenshot flags are saved,
 *  then the function returns control to the caller.
 *  When crash occurs, screenshot is made by the \b crash_sender.exe process and added to the report.
 *
 *  \b dwFlags
 *
 *    Use one of the following constants to specify what part of virtual screen to capture:
 *    - \ref CR_AS_VIRTUAL_SCREEN  Use this to take a screenshot of the whole desktop (virtual screen).
 *    - \ref CR_AS_MAIN_WINDOW     Use this to take a screenshot of the application's main window.
 *    - \ref CR_AS_PROCESS_WINDOWS Use this to take a screenshot of all visible windows that belong to the process.
 *
 *  The main application window is a window that has a caption (\b WS_CAPTION), system menu (\b WS_SYSMENU) and
 *  the \b WS_EX_APPWINDOW extended style. If CrashRpt doesn't find such window, it considers the first found process window as
 *  the main window.
 *
 *  Screenshots are added in form of PNG files by default. You can specify the \ref CR_AS_USE_JPEG_FORMAT flag to save
 *  screenshots as JPEG files instead.
 *
 *  In addition, you can specify the \ref CR_AS_GRAYSCALE_IMAGE flag to make a grayscale screenshot
 *  (by default color image is made). Grayscale image gives smaller file size.
 *
 *  If you use JPEG image format, you may better use the \ref crAddScreenshot2() function, that allows to
 *  define JPEG image quality.
 *
 *  When capturing entire desktop consisting of several monitors,
 *  one screenshot file is added per each monitor.
 *
 *  You should be careful when using this feature, because screenshots may contain user-identifying
 *  or private information. Always specify purposes you will use collected
 *  information for in your Privacy Policy.
 *
 *  \sa
 *   crAddFile2()
 */

CRASHRPTAPI(int)
crAddScreenshot(
		DWORD dwFlags
);

/*! \ingroup CrashRptAPI
 *  \brief Adds a screenshot to the crash report.
 *
 *  \return This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
 *
 *  \param[in] dwFlags Flags, optional.
 *  \param[in] nJpegQuality Defines the JPEG image quality, optional.
 *
 *  \remarks
 *
 *  This function can be used to take a screenshot at the moment of crash and add it to the error report.
 *  Screenshot information may help the developer to better understand state of the application
 *  at the moment of crash and reproduce the error.
 *
 *  When this function is called, screenshot flags are saved, then the function returns control to the caller.
 *  When crash occurs, screenshot is made by the \b crash_sender.exe process and added to the report.
 *
 *  \b dwFlags
 *
 *    Use one of the following constants to specify what part of virtual screen to capture:
 *    - \ref CR_AS_VIRTUAL_SCREEN  Use this to take a screenshot of the whole desktop (virtual screen).
 *    - \ref CR_AS_MAIN_WINDOW     Use this to take a screenshot of the main application main window.
 *    - \ref CR_AS_PROCESS_WINDOWS Use this to take a screenshot of all visible windows that belong to the process.
 *
 *  The main application window is a window that has a caption (\b WS_CAPTION), system menu (\b WS_SYSMENU) and
 *  the \b WS_EX_APPWINDOW extended style. If CrashRpt doesn't find such window, it considers the first found process window as
 *  the main window.
 *
 *  Screenshots are added in form of PNG files by default. You can specify the \ref CR_AS_USE_JPEG_FORMAT flag to save
 *  screenshots as JPEG files instead.
 *
 *  If you use JPEG format, you can use the \a nJpegQuality parameter to define the JPEG image quality.
 *  This should be the number between 0 and 100, inclusively. The bigger the number, the better the quality and the bigger the JPEG file size.
 *  If you use PNG file format, this parameter is ignored.
 *
 *  In addition, you can specify the \ref CR_AS_GRAYSCALE_IMAGE flag to make a grayscale screenshot
 *  (by default color image is made). Grayscale image gives smaller file size.
 *
 *  When capturing entire desktop consisting of several monitors,
 *  one screenshot file is added per each monitor.
 *
 *  You should be careful when using this feature, because screenshots may contain user-identifying
 *  or private information. Always specify purposes you will use collected
 *  information for in your Privacy Policy.
 *
 *  \sa
 *   crAddFile2()
 */

CRASHRPTAPI(int)
crAddScreenshot2(
		DWORD dwFlags,
		int nJpegQuality
);

/*! \ingroup CrashRptAPI
 *  \brief Adds a string property to the crash report.
 *
 *  \return This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
 *
 *  \param[in] pszPropName   Name of the property, required.
 *  \param[in] pszPropValue  Value of the property, required.
 *
 *  \remarks
 *
 *  Use this function to add a string property to the crash description XML file.
 *  User-added properties are listed under \<CustomProps\> tag of the XML file.
 *
 *  The following example shows how to add information about the amount of free disk space to the crash
 *  description XML file:
 *  \code
 *  // It is assumed that you already calculated the amount of free disk space, converted it to text
 *  // and store it as szFreeSpace string.
 *  LPCTSTR szFreeSpace = _T("0 Kb");
 *  crAddProperty(_T("FreeDiskSpace"), szFreeSpace);
 *
 *  \endcode
 *
 *  \sa
 *   crAddFile2(), crAddScreenshot()
 */

CRASHRPTAPI(int)
crAddPropertyW(
		const wchar_t* pszPropName,
		const wchar_t* pszPropValue
);

/*! \ingroup CrashRptAPI
 *  \copydoc crAddPropertyW()
 */

CRASHRPTAPI(int)
crAddPropertyA(
		const char* pszPropName,
		const char* pszPropValue
);

/*! \brief Character set-independent mapping of crAddPropertyW() and crAddPropertyA() functions.
 *  \ingroup CrashRptAPI
 */
#ifdef UNICODE
#define crAddProperty crAddPropertyW
#else
#define crAddProperty crAddPropertyA
#endif //UNICODE
/*! \ingroup CrashRptAPI
 *  \brief Adds a registry key dump to the crash report.
 *
 *  \return This function returns zero if succeeded. Use crGetLastErrorMsg() to retrieve the error message on fail.
 *
 *  \param[in] pszRegKey        Registry key to dump, required.
 *  \param[in] pszDstFileName   Name of the destination file, required.
 *  \param[in] dwFlags          Flags, reserved.
 *
 *  \remarks
 *
 *  Use this function to add a dump of a Windows registry key into the crash report. This function
 *  is available since v.1.2.6.
 *
 *  The \a pszRegKey parameter must be the name of the registry key. The key name should begin with "HKEY_CURRENT_USER"
 *  or "HKEY_LOCAL_MACHINE". Other root keys are not supported.
 *
 *  The content of the key specified by the \a pszRegKey parameter will be stored in a human-readable XML
 *  format and included into the error report as \a pszDstFileName destination file. You can dump multiple registry keys
 *  to the same destination file.
 *
 *  The \a dwFlags parameter is reserved for future use and should be set to zero.
 *
 *  The following example shows how to dump two registry keys to \b regkey.xml file:
 *
 *  \code
 *
 *  crAddRegKey(_T("HKEY_CURRENT_USER\\Software\\MyApp"), _T("regkey.xml"), 0);
 *  crAddRegKey(_T("HKEY_LOCAL_MACHINE\\Software\\MyApp"), _T("regkey.xml"), 0);
 *
 *  \endcode
 *
 *  \sa
 *   crAddFile2(), crAddScreenshot(), crAddProperty()
 */

CRASHRPTAPI(int)
crAddRegKeyW(
		const wchar_t* pszRegKey,
		const wchar_t* pszDstFileName,
		DWORD dwFlags
);

/*! \ingroup CrashRptAPI
 *  \copydoc crAddRegKeyW()
 */

CRASHRPTAPI(int)
crAddRegKeyA(
		const char* pszRegKey,
		const char* pszDstFileName,
		DWORD dwFlags
);

/*! \brief Character set-independent mapping of crAddRegKeyW() and crAddRegKeyA() functions.
 *  \ingroup CrashRptAPI
 */
#ifdef UNICODE
#define crAddRegKey crAddRegKeyW
#else
#define crAddRegKey crAddRegKeyA
#endif //UNICODE
// Exception types
#define CR_WIN32_STRUCTURED_EXCEPTION   0    //   SEH exception (deprecated name, use \ref CR_SEH_EXCEPTION instead).
#define CR_SEH_EXCEPTION                0    //   SEH exception.
#define CR_CPP_TERMINATE_CALL           1    //   C++ terminate() call.
#define CR_CPP_UNEXPECTED_CALL          2    //   C++ unexpected() call.
#define CR_CPP_PURE_CALL                3    //   C++ pure virtual function call (VS .NET and later).
#define CR_CPP_NEW_OPERATOR_ERROR       4    //   C++ new operator fault (VS .NET and later).
#define CR_CPP_SECURITY_ERROR           5    //   Buffer overrun error (VS .NET only).
#define CR_CPP_INVALID_PARAMETER        6    //   Invalid parameter exception (VS 2005 and later).
#define CR_CPP_SIGABRT                  7    //   C++ SIGABRT signal (abort).
#define CR_CPP_SIGFPE                   8    //   C++ SIGFPE signal (flotating point exception).
#define CR_CPP_SIGILL                   9    //   C++ SIGILL signal (illegal instruction).
#define CR_CPP_SIGINT                   10   //   C++ SIGINT signal (CTRL+C).
#define CR_CPP_SIGSEGV                  11   //   C++ SIGSEGV signal (invalid storage access).
#define CR_CPP_SIGTERM                  12   //   C++ SIGTERM signal (termination request).
/*! \ingroup CrashRptStructs
 *
 *  This structure defines the information needed to generate crash minidump file and
 *  provide the developer with other information about the error. This structure is used by
 *  the crGenerateErrorReport() function.
 *
 *  \b cb [in]
 *
 *  This must contain the size of this structure in bytes.
 *
 *  \b pexcptrs [in, optional]
 *
 *    Should contain the exception pointers. If this parameter is NULL,
 *    the current CPU state is used to generate exception pointers.
 *
 *  \b exctype [in]
 *
 *    The type of exception. This parameter may be one of the following:
 *     - \ref CR_SEH_EXCEPTION             SEH (Structured Exception Handling) exception
 *     - \ref CR_CPP_TERMINATE_CALL        C++ terminate() function call
 *     - \ref CR_CPP_UNEXPECTED_CALL       C++ unexpected() function call
 *     - \ref CR_CPP_PURE_CALL             Pure virtual method call (Visual Studio .NET 2003 and later)
 *     - \ref CR_CPP_NEW_OPERATOR_ERROR    C++ 'new' operator error (Visual Studio .NET 2003 and later)
 *     - \ref CR_CPP_SECURITY_ERROR        Buffer overrun (Visual Studio .NET 2003 only)
 *     - \ref CR_CPP_INVALID_PARAMETER     Invalid parameter error (Visual Studio 2005 and later)
 *     - \ref CR_CPP_SIGABRT               C++ SIGABRT signal
 *     - \ref CR_CPP_SIGFPE                C++ floating point exception
 *     - \ref CR_CPP_SIGILL                C++ illegal instruction
 *     - \ref CR_CPP_SIGINT                C++ SIGINT signal
 *     - \ref CR_CPP_SIGSEGV               C++ invalid storage access
 *     - \ref CR_CPP_SIGTERM               C++ termination request
 *
 *   \b code [in, optional]
 *
 *      Used if \a exctype is \ref CR_SEH_EXCEPTION and represents the SEH exception code.
 *      If \a pexptrs is NULL, this value is used when generating exception information for initializing
 *      \c pexptrs->ExceptionRecord->ExceptionCode member, otherwise it is ignored.
 *
 *   \b fpe_subcode [in, optional]
 *
 *      Used if \a exctype is equal to \ref CR_CPP_SIGFPE. It defines the floating point
 *      exception subcode (see \c signal() function ducumentation in MSDN).
 *
 *   \b expression, \b function, \b file and \b line [in, optional]
 *
 *     These parameters are used when \a exctype is \ref CR_CPP_INVALID_PARAMETER.
 *     These members are typically non-zero when using debug version of CRT.
 *
 *  \b bManual [in]
 *
 *     Since v.1.2.4, \a bManual parameter should be equal to TRUE if the report is generated manually.
 *     The value of \a bManual parameter affects the automatic application restart behavior. If the application
 *     restart is requested by the \ref CR_INST_APP_RESTART flag of CR_INSTALL_INFO::dwFlags structure member,
 *     and if \a bManual is FALSE, the application will be
 *     restarted after error report generation. If \a bManual is TRUE, the application won't be restarted.
 *
 *  \b hSenderProcess [out]
 *
 *     As of v.1.2.8, \a hSenderProcess parameter will contain the handle to the <b>crash_sender.exe</b> process when
 *     \ref crGenerateErrorReport function returns. The caller may use this handle to wait until <b>crash_sender.exe</b>
 *     process exits and check the exit code.
 */

typedef struct tagCR_EXCEPTION_INFO {
	WORD cb; //   Size of this structure in bytes; should be initialized before using.
	PEXCEPTION_POINTERS pexcptrs; //   Exception pointers.
	int exctype; //   Exception type.
	DWORD code; //   Code of SEH exception.
	unsigned int fpe_subcode; //   Floating point exception subcode.
	const wchar_t* expression; //   Assertion expression.
	const wchar_t* function; //   Function in which assertion happened.
	const wchar_t* file; //   File in which assertion happened.
	unsigned int line; //   Line number.
	BOOL bManual; //   Flag telling if the error report is generated manually or not.
	HANDLE hSenderProcess; //   Handle to the crash_sender.exe process.
} CR_EXCEPTION_INFO;

typedef CR_EXCEPTION_INFO *PCR_EXCEPTION_INFO;

/*! \ingroup CrashRptAPI
 *  \brief Manually generates an errror report.
 *
 *  \return This function returns zero if succeeded. When failed, it returns a non-zero value.
 *     Use crGetLastErrorMsg() to retrieve the error message.
 *
 *  \param[in] pExceptionInfo Exception information.
 *
 *  \remarks
 *
 *    Call this function to manually generate a crash report. When crash information is collected,
 *    control is returned to the caller. The crGenerateErrorReport() doesn't terminate the caller process.
 *
 *    The crash report contains crash minidump, crash description in XML format and
 *    additional custom files added with crAddFile2().
 *
 *    The exception information should be passed using \ref CR_EXCEPTION_INFO structure.
 *
 *    The following example shows how to use crGenerateErrorReport() function.
 *
 *    \code
 *    CR_EXCEPTION_INFO ei;
 *    memset(&ei, 0, sizeof(CR_EXCEPTION_INFO));
 *    ei.size = sizeof(CR_EXCEPTION_INFO);
 *    ei.exctype = CR_SEH_EXCEPTION;
 *    ei.code = 1234;
 *    ei.pexcptrs = NULL;
 *
 *    int result = crGenerateErrorReport(&ei);
 *
 *    if(result!=0)
 *    {
 *      // If goes here, crGenerateErrorReport() has failed
 *      // Get the last error message
 *      TCHAR szErrorMsg[256];
 *      crGetLastErrorMsg(szErrorMsg, 256);
 *    }
 *
 *    // Manually terminate program
 *    ExitProcess(0);
 *
 *    \endcode
 */

CRASHRPTAPI(int)
crGenerateErrorReport(
		__in_opt CR_EXCEPTION_INFO* pExceptionInfo
);

/*! \ingroup DeprecatedAPI
 *  \brief Can be used as a SEH exception filter.
 *
 *  \return This function returns \c EXCEPTION_EXECUTE_HANDLER if succeeds, else \c EXCEPTION_CONTINUE_SEARCH.
 *
 *  \param[in] code Exception code.
 *  \param[in] ep   Exception pointers.
 *
 *  \remarks
 *
 *     As of v.1.2.8, this function is declared deprecated. It may be removed in one of the future releases.
 *
 *     This function can be called instead of a SEH exception filter
 *     inside of __try{}__except(Expression){} statement. The function generates a error report
 *     and returns control to the exception handler block.
 *
 *     The exception code is usually retrieved with \b GetExceptionCode() intrinsic function
 *     and the exception pointers are retrieved with \b GetExceptionInformation() intrinsic
 *     function.
 *
 *     If an error occurs, this function returns \c EXCEPTION_CONTINUE_SEARCH.
 *     Use crGetLastErrorMsg() to retrieve the error message on fail.
 *
 *     The following example shows how to use crExceptionFilter().
 *
 *     \code
 *     int* p = NULL;   // pointer to NULL
 *     __try
 *     {
 *        *p = 13; // causes an access violation exception;
 *     }
 *     __except(crExceptionFilter(GetExceptionCode(), GetExceptionInformation()))
 *     {
 *       // Terminate program
 *       ExitProcess(1);
 *     }
 *
 *     \endcode
 */

CRASHRPTAPI(int)
crExceptionFilter(
		unsigned int code,
		__in_opt struct _EXCEPTION_POINTERS* ep);

#define CR_NONCONTINUABLE_EXCEPTION  32  //   Non continuable sofware exception.
#define CR_THROW                     33  //   Throw C++ typed exception.
/*! \ingroup CrashRptAPI
 *  \brief Emulates a predefined crash situation.
 *
 *  \return This function doesn't return if succeded. If failed, returns non-zero value. Call crGetLastErrorMsg()
 *   to get the last error message.
 *
 *  \param[in] ExceptionType Type of crash.
 *
 *  \remarks
 *
 *    This function uses some a priori incorrect or vulnerable code or raises a C++ signal or raises an uncontinuable
 *    software exception to cause crash.
 *
 *    This function can be used to test if CrashRpt handles a crash situation correctly.
 *
 *    CrashRpt will intercept an error or exception if crInstall() and/or crInstallToCurrentThread2()
 *    were previously called. crInstall() installs exception handlers that function on per-process basis.
 *    crInstallToCurrentThread2() installs exception handlers that function on per-thread basis.
 *
 *  \a ExceptionType can be one of the following constants:
 *    - \ref CR_SEH_EXCEPTION  This will generate a null pointer exception.
 *    - \ref CR_CPP_TERMINATE_CALL This results in call of terminate() C++ function.
 *    - \ref CR_CPP_UNEXPECTED_CALL This results in call of unexpected() C++ function.
 *    - \ref CR_CPP_PURE_CALL This emulates a call of pure virtual method call of a C++ class instance (Visual Studio .NET 2003 and later).
 *    - \ref CR_CPP_NEW_OPERATOR_ERROR This emulates C++ new operator failure (Visual Studio .NET 2003 and later).
 *    - \ref CR_CPP_SECURITY_ERROR This emulates copy of large amount of data to a small buffer (Visual Studio .NET 2003 only).
 *    - \ref CR_CPP_INVALID_PARAMETER This emulates an invalid parameter C++ exception (Visual Studio 2005 and later).
 *    - \ref CR_CPP_SIGABRT This raises SIGABRT signal (abnormal program termination).
 *    - \ref CR_CPP_SIGFPE This causes floating point exception.
 *    - \ref CR_CPP_SIGILL This raises SIGILL signal (illegal instruction signal).
 *    - \ref CR_CPP_SIGINT This raises SIGINT signal.
 *    - \ref CR_CPP_SIGSEGV This raises SIGSEGV signal.
 *    - \ref CR_CPP_SIGTERM This raises SIGTERM signal (program termination request).
 *    - \ref CR_NONCONTINUABLE_EXCEPTION This raises a noncontinuable software exception (expected result
 *         is the same as in \ref CR_SEH_EXCEPTION).
 *    - \ref CR_THROW This throws a C++ typed exception (expected result is the same as in \ref CR_CPP_TERMINATE_CALL).
 *
 *  The \ref CR_SEH_EXCEPTION uses null pointer write operation to cause the access violation.
 *
 *  The \ref CR_NONCONTINUABLE_EXCEPTION has the same effect as \ref CR_SEH_EXCEPTION, but it uses
 *  \b RaiseException() WinAPI function to raise noncontinuable software exception.
 *
 *  The following example shows how to use crEmulateCrash() function.
 *
 *  \code
 *  // emulate null pointer exception (access violation)
 *  crEmulateCrash(CR_SEH_EXCEPTION);
 *  \endcode
 *
 */

CRASHRPTAPI(int)
crEmulateCrash(
		unsigned ExceptionType) throw (...);

CRASHRPTAPI(int)
crGetLastErrorMsgW(
		__out_ecount_z(uBuffSize) LPWSTR pszBuffer,
		UINT uBuffSize);

CRASHRPTAPI(int)
crGetLastErrorMsgA(
		__out_ecount_z(uBuffSize) LPSTR pszBuffer,
		UINT uBuffSize);

#ifdef UNICODE
#define crGetLastErrorMsg crGetLastErrorMsgW
#else
#define crGetLastErrorMsg crGetLastErrorMsgA
#endif //UNICODE
//// Helper wrapper classes

#endif //  _CRASHRPT_H_
