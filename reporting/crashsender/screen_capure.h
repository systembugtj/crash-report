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

#ifndef SCREEN_CAPURE_H_
#define SCREEN_CAPURE_H_

#include "stdafx.h"

extern "C" {
#include "png.h"
}
#include "jpeglib.h"

// Window information
struct WindowInfo {
  CString m_sTitle; // Window title
  CRect m_rcWnd; // Window rect
  DWORD dwStyle;
  DWORD dwExStyle;
};

// Monitor info
struct MonitorInfo {
  CString m_sDeviceID; // Device ID
  CRect m_rcMonitor; // Monitor rectangle in screen coordinates
  CString m_sFileName; // Image file name corresponding to this monitor
};

// Desktop screen shot info
struct ScreenshotInfo {
  ScreenshotInfo() {
    m_bValid = FALSE;
  }

  BOOL m_bValid;
  CRect m_rcVirtualScreen;
  std::vector<MonitorInfo> m_aMonitors; // The list of monitors.
  std::vector<WindowInfo> m_aWindows; // The list of windows.
};

// What format to use when saving screenshots
enum SCREENSHOT_IMAGE_FORMAT {
  SCREENSHOT_FORMAT_PNG = 0, // Use PNG format
  SCREENSHOT_FORMAT_JPG = 1, // Use JPG format
  SCREENSHOT_FORMAT_BMP = 2
// Use BMP format
};

// Desktop screenshot capture
class CScreenCapture {
public:
  // Constructor
  CScreenCapture();
  ~CScreenCapture();

  // Returns virtual screen rectangle
  void GetScreenRect(LPRECT rcScreen);

  // Returns an array of visible windows for the specified process or 
  // the main window of the process.
  BOOL FindWindows(HANDLE hProcess, BOOL bAllProcessWindows, std::vector<
      WindowInfo>* paWindows);

  // Captures the specified screen area and returns the list of image files
  BOOL CaptureScreenRect(std::vector<CRect> arcCapture, CString sSaveDirName,
      int nIdStartFrom, SCREENSHOT_IMAGE_FORMAT fmt, int nJpegQuality,
      BOOL bGrayscale, std::vector<MonitorInfo>& monitor_list, std::vector<
          CString>& out_file_list);

  /* PNG management functions */

  // Initializes PNG file header
  BOOL PngInit(int nWidth, int nHeight, BOOL bGrayscale, CString sFileName);
  // Writes a scan line to the PNG file
  BOOL PngWriteRow(LPBYTE pRow, int nRowLen, BOOL bGrayscale);
  // Closes PNG file
  BOOL PngFinalize();

  /* JPEG management functions */

  BOOL JpegInit(int nWidth, int nHeight, BOOL bGrayscale, int nQuality,
      CString sFileName);
  BOOL JpegWriteRow(LPBYTE pRow, int nRowLen, BOOL bGrayscale);
  BOOL JpegFinalize();

  void AddOutFile(const CString& file);
  void AddMonitorInfo(const MonitorInfo& monitor);

  //  setter and getter
  BOOL grayscale() {
    return grayscale_;
  }
  int start_id() {
    return start_id_;
  }
  int jpeg_quality() {
    return jpeg_quality_;
  }
  CPoint cursor_pos() {
    return cursor_pos_;
  }
  CString save_dir() {
    return save_dir_;
  }
  CURSORINFO cursor_info() {
    return cursor_info_;
  }
  SCREENSHOT_IMAGE_FORMAT format() {
    return format_;
  }
  const std::vector<CRect>& capture_rect() {
    return capture_rect_;
  }
  void set_start_id(int id) {
    start_id_ = id;
  }

private:
  // Array of capture rectangles
  std::vector<CRect> capture_rect_;
  // Create grayscale image or not
  BOOL grayscale_;
  CURSORINFO cursor_info_;
  SCREENSHOT_IMAGE_FORMAT format_;
  // An ID for the current screenshot image
  int start_id_;
  int jpeg_quality_;
  std::vector<MonitorInfo> monitor_list_;
  // The list of output image files
  std::vector<CString> out_file_;
  // Current mouse cursor pos
  CPoint cursor_pos_;
  CString save_dir_;
  FILE* m_fp;
  png_structp png_ptr_;
  png_infop info_ptr_;
  struct jpeg_compress_struct jpeg_compress_;
  struct jpeg_error_mgr jpeg_error_mgr_;
};

#endif //SCREEN_CAPURE_H_
