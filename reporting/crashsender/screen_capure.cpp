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

#include "stdafx.h"
#include "screen_capure.h"
#include "utility.h"

// Disable warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning(disable:4611)

// This function is used for monitor enumeration
BOOL CALLBACK EnumMonitorsProc(HMONITOR hMonitor, HDC /*hdcMonitor*/,
    LPRECT lprcMonitor, LPARAM dwData) {
  CScreenCapture* psc = (CScreenCapture*) dwData;

  MONITORINFOEX mi;
  HDC hDC = NULL;
  HDC hCompatDC = NULL;
  HBITMAP hBitmap = NULL;
  BITMAPINFO bmi;
  int nWidth = 0;
  int nHeight = 0;
  int nRowWidth = 0;
  LPBYTE pRowBits = NULL;
  CString sFileName;
  MonitorInfo monitor_info;

  // Get monitor rect size
  nWidth = lprcMonitor->right - lprcMonitor->left;
  nHeight = lprcMonitor->bottom - lprcMonitor->top;

  // Get monitor info
  mi.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hMonitor, &mi);

  // Get the device context for this monitor
  hDC = CreateDC(_T("DISPLAY"), mi.szDevice, NULL, NULL);
  if (hDC == NULL)
    goto cleanup;

  hCompatDC = CreateCompatibleDC(hDC);
  if (hCompatDC == NULL)
    goto cleanup;

  hBitmap = CreateCompatibleBitmap(hDC, nWidth, nHeight);
  if (hBitmap == NULL)
    goto cleanup;

  SelectObject(hCompatDC, hBitmap);

  int i;
  for (i = 0; i < (int) psc->capture_rect().size(); i++) {
    const CRect& rc = psc->capture_rect()[i];
    CRect rcIntersect;
    if (IntersectRect(&rcIntersect, lprcMonitor, &rc)) {
      BOOL bBitBlt = BitBlt(hCompatDC, rc.left - lprcMonitor->left, rc.top
          - lprcMonitor->top, rc.Width(), rc.Height(), hDC, rc.left
          - lprcMonitor->left, rc.top - lprcMonitor->top, SRCCOPY | CAPTUREBLT);
      if (!bBitBlt)
        goto cleanup;
    }
  }

  // Draw mouse cursor.
  if (PtInRect(lprcMonitor, psc->cursor_pos())) {
    if (psc->cursor_info().flags == CURSOR_SHOWING) {
      ICONINFO IconInfo;
      GetIconInfo((HICON) psc->cursor_info().hCursor, &IconInfo);
      int x = psc->cursor_pos().x - lprcMonitor->left - IconInfo.xHotspot;
      int y = psc->cursor_pos().y - lprcMonitor->top - IconInfo.yHotspot;
      DrawIcon(hCompatDC, x, y, (HICON) psc->cursor_info().hCursor);
      DeleteObject(IconInfo.hbmMask);
      DeleteObject(IconInfo.hbmColor);
    }
  }

  /* Write screenshot bitmap to an image file. */

  if (psc->format() == SCREENSHOT_FORMAT_PNG) {
    // Init PNG writer
    sFileName.Format(_T("%s\\screenshot%d.png"), psc->save_dir(),
        psc->start_id());
    psc->set_start_id(psc->start_id() + 1);
    BOOL bInit = psc->PngInit(nWidth, nHeight, psc->grayscale(), sFileName);
    if (!bInit)
      goto cleanup;
  } else if (psc->format() == SCREENSHOT_FORMAT_JPG) {
    // Init JPG writer
    sFileName.Format(_T("%s\\screenshot%d.jpg"), psc->save_dir(),
        psc->start_id());
        psc->set_start_id(psc->start_id() + 1);
    BOOL bInit = psc->JpegInit(nWidth, nHeight, psc->grayscale(),
        psc->jpeg_quality(), sFileName);
    if (!bInit)
      goto cleanup;
  } else if (psc->format() == SCREENSHOT_FORMAT_BMP) {
  }

  // We will get bitmap bits row by row
  nRowWidth = nWidth * 3 + 10;
  pRowBits = new BYTE[nRowWidth];
  if (pRowBits == NULL)
    goto cleanup;

  memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = nWidth;
  bmi.bmiHeader.biHeight = -nHeight;
  bmi.bmiHeader.biBitCount = 24;
  bmi.bmiHeader.biPlanes = 1;

  //int i;
  for (i = nHeight - 1; i >= 0; i--) {
    int nFetched = GetDIBits(hCompatDC, hBitmap, i, 1, pRowBits, &bmi,
        DIB_RGB_COLORS);
    if (nFetched != 1)
      break;

    if (psc->format() == SCREENSHOT_FORMAT_PNG) {
      BOOL bWrite = psc->PngWriteRow(pRowBits, nRowWidth, psc->grayscale());
      if (!bWrite)
        goto cleanup;
    } else if (psc->format() == SCREENSHOT_FORMAT_JPG) {
      BOOL bWrite = psc->JpegWriteRow(pRowBits, nRowWidth, psc->grayscale());
      if (!bWrite)
        goto cleanup;
    } else if (psc->format() == SCREENSHOT_FORMAT_BMP) {
    }
  }

  if (psc->format() == SCREENSHOT_FORMAT_PNG) {
    psc->PngFinalize();
  } else if (psc->format() == SCREENSHOT_FORMAT_JPG) {
    psc->JpegFinalize();
  } else if (psc->format() == SCREENSHOT_FORMAT_BMP) {
  } else {
    ATLASSERT(0); // Invalid format
    goto cleanup;
  }

  psc->AddOutFile(sFileName);

  monitor_info.m_rcMonitor = mi.rcMonitor;
  monitor_info.m_sDeviceID = mi.szDevice;
  monitor_info.m_sFileName = sFileName;
  psc->AddMonitorInfo(monitor_info);

  cleanup:

  // Clean up
  if (hDC)
    DeleteDC(hDC);

  if (hCompatDC)
    DeleteDC(hCompatDC);

  if (hBitmap)
    DeleteObject(hBitmap);

  if (pRowBits)
    delete[] pRowBits;

  // Next monitor
  return TRUE;
}

CScreenCapture::CScreenCapture() {
  m_fp = NULL;
  png_ptr_ = NULL;
  info_ptr_ = NULL;
  start_id_ = 0;
}

CScreenCapture::~CScreenCapture() {
}

BOOL CScreenCapture::CaptureScreenRect(std::vector<CRect> arcCapture,
    CString sSaveDirName, int nIdStartFrom, SCREENSHOT_IMAGE_FORMAT fmt,
    int nJpegQuality, BOOL bGrayscale, std::vector<MonitorInfo>& monitor_list,
    std::vector<CString>& out_file_list) {
  // Init output variables
  monitor_list.clear();
  out_file_list.clear();

  // Set internal variables
  start_id_ = nIdStartFrom;
  save_dir_ = sSaveDirName;
  format_ = fmt;
  jpeg_quality_ = nJpegQuality;
  grayscale_ = bGrayscale;
  capture_rect_ = arcCapture;
  out_file_.clear();
  monitor_list_.clear();

  // Get cursor information
  GetCursorPos(&cursor_pos_);
  cursor_info_.cbSize = sizeof(CURSORINFO);
  GetCursorInfo(&cursor_info_);

  // Perform actual capture task inside of EnumMonitorsProc
  EnumDisplayMonitors(NULL, NULL, EnumMonitorsProc, (LPARAM) this);

  // Return
  out_file_list = out_file_;
  monitor_list = monitor_list_;
  return TRUE;
}

// Gets rectangle of the virtual screen
void CScreenCapture::GetScreenRect(LPRECT rcScreen) {
  int nWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  int nHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

  rcScreen->left = GetSystemMetrics(SM_XVIRTUALSCREEN);
  rcScreen->top = GetSystemMetrics(SM_YVIRTUALSCREEN);
  rcScreen->right = rcScreen->left + nWidth;
  rcScreen->bottom = rcScreen->top + nHeight;
}

BOOL CScreenCapture::PngInit(int nWidth, int nHeight, BOOL bGrayscale,
    CString sFileName) {
  m_fp = NULL;
  png_ptr_ = NULL;
  info_ptr_ = NULL;

  out_file_.push_back(sFileName);

#if _MSC_VER>=1400
  _tfopen_s(&m_fp, sFileName.GetBuffer(0), _T("wb"));
#else
  m_fp = _tfopen(sFileName.GetBuffer(0), _T("wb"));
#endif

  if (!m_fp) {
    return FALSE;
  }

  png_ptr_ = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp) NULL,
      NULL, NULL);
  if (!png_ptr_)
    return FALSE;

  info_ptr_ = png_create_info_struct(png_ptr_);
  if (!info_ptr_) {
    png_destroy_write_struct(&png_ptr_, (png_infopp) NULL);
    return FALSE;
  }

  /* Error handler*/
  if (setjmp(png_jmpbuf(png_ptr_))) {
    png_destroy_write_struct(&png_ptr_, &info_ptr_);
    fclose(m_fp);
    return FALSE;
  }

  png_init_io(png_ptr_, m_fp);

  /* set the zlib compression level */
  png_set_compression_level(png_ptr_, Z_BEST_COMPRESSION);

  /* set other zlib parameters */
  png_set_compression_mem_level(png_ptr_, 8);
  png_set_compression_strategy(png_ptr_, Z_DEFAULT_STRATEGY);
  png_set_compression_window_bits(png_ptr_, 15);
  png_set_compression_method(png_ptr_, 8);
  png_set_compression_buffer_size(png_ptr_, 8192);

  png_set_IHDR(png_ptr_, info_ptr_, nWidth, //width,
      nHeight, //height,
      8, // bit_depth
      bGrayscale ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB, // color_type
      PNG_INTERLACE_NONE, // interlace_type
      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_set_bgr(png_ptr_);

  /* write the file information */
  png_write_info(png_ptr_, info_ptr_);

  return TRUE;
}

BOOL CScreenCapture::PngWriteRow(LPBYTE pRow, int nRowLen, BOOL bGrayscale) {
  // Convert RGB to BGR
  LPBYTE pRow2 = NULL;

  if (bGrayscale) {
    int i;
    pRow2 = new BYTE[nRowLen / 3];
    for (i = 0; i < nRowLen / 3; i++)
      pRow2[i] = (pRow[i * 3 + 0] + pRow[i * 3 + 1] + pRow[i * 3 + 2]) / 3;
  } else {
    pRow2 = pRow;
  }

  png_bytep rows[1] = { pRow2 };
  png_write_rows(png_ptr_, (png_bytepp) &rows, 1);

  if (bGrayscale)
    delete[] pRow2;
  return TRUE;
}

BOOL CScreenCapture::PngFinalize() {
  /* end write */
  png_write_end(png_ptr_, info_ptr_);

  /* clean up */
  png_destroy_write_struct(&png_ptr_, (png_infopp) &info_ptr_);

  if (m_fp)
    fclose(m_fp);

  return TRUE;
}

BOOL CScreenCapture::JpegInit(int nWidth, int nHeight, BOOL bGrayscale,
    int nQuality, CString sFileName) {
  /* Step 1: allocate and initialize JPEG compression object */

  jpeg_compress_.err = jpeg_std_error(&jpeg_error_mgr_);
  jpeg_create_compress(&jpeg_compress_);

  /* Step 2: specify data destination (eg, a file) */

#if _MSC_VER < 1400
  m_fp = _tfopen(sFileName, _T("wb"));
#else
  _tfopen_s(&m_fp, sFileName, _T("wb"));
#endif
  if (m_fp == NULL)
    return FALSE;

  jpeg_stdio_dest(&jpeg_compress_, m_fp);

  /* Step 3: set parameters for compression */

  jpeg_compress_.image_width = nWidth; /* image width and height, in pixels */
  jpeg_compress_.image_height = nHeight;
  jpeg_compress_.input_components = bGrayscale ? 1 : 3; /* # of color components per pixel */
  jpeg_compress_.in_color_space = bGrayscale ? JCS_GRAYSCALE : JCS_RGB; /* colorspace of input image */
  jpeg_set_defaults(&jpeg_compress_);
  jpeg_set_quality(&jpeg_compress_, nQuality, TRUE /* limit to baseline-JPEG values */);

  /* Step 4: Start compressor */

  jpeg_start_compress(&jpeg_compress_, TRUE);

  return TRUE;
}

BOOL CScreenCapture::JpegWriteRow(LPBYTE pRow, int nRowLen, BOOL bGrayscale) {
  // Convert RGB to BGR
  LPBYTE pRow2 = NULL;
  int i;
  if (bGrayscale) {
    pRow2 = new BYTE[nRowLen / 3];
    for (i = 0; i < nRowLen / 3; i++)
      pRow2[i] = (pRow[i * 3 + 0] + pRow[i * 3 + 1] + pRow[i * 3 + 2]) / 3;
  } else {
    pRow2 = new BYTE[nRowLen];
    for (i = 0; i < nRowLen / 3; i++) {
      pRow2[i * 3 + 0] = pRow[i * 3 + 2];
      pRow2[i * 3 + 1] = pRow[i * 3 + 1];
      pRow2[i * 3 + 2] = pRow[i * 3 + 0];
    }
  }

  JSAMPROW row_pointer[1];
  row_pointer[0] = (JSAMPROW) pRow2;
  (void) jpeg_write_scanlines(&jpeg_compress_, row_pointer, 1);

  delete[] pRow2;

  return TRUE;
}

BOOL CScreenCapture::JpegFinalize() {
  /* Step 6: Finish compression */
  jpeg_finish_compress(&jpeg_compress_);

  /* After finish_compress, we can close the output file. */
  if (m_fp != NULL)
    fclose(m_fp);
  m_fp = NULL;

  /* Step 7: release JPEG compression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_compress(&jpeg_compress_);

  return TRUE;
}

void CScreenCapture::AddOutFile(const CString& file) {
  out_file_.push_back(file);
}
void CScreenCapture::AddMonitorInfo(const MonitorInfo& monitor) {
  monitor_list_.push_back(monitor);
}

struct FindWindowData {
  HANDLE hProcess; // Handle to the process
  BOOL bAllProcessWindows; // If TRUE, finds all process windows, else only the main one
  std::vector<WindowInfo>* paWindows; // Output array of window handles
};

BOOL CALLBACK EnumWndProc(HWND hWnd, LPARAM lParam) {
  FindWindowData* pFWD = (FindWindowData*) lParam;

  // Get process ID
  DWORD dwMyProcessId = GetProcessId(pFWD->hProcess);

  if (IsWindowVisible(hWnd)) // Get only wisible windows
  {
    // Determine the process ID of the current window
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    // Compare window process ID to our process ID
    if (dwProcessId == dwMyProcessId) {
      DWORD dwStyle = GetWindowLong(hWnd, GWL_STYLE);
      if ((dwStyle & WS_CHILD) == 0) // Get only non-child windows
      {
        DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
        WindowInfo wi;
        TCHAR szTitle[1024];
        GetWindowText(hWnd, szTitle, 1024);
        wi.m_sTitle = szTitle;
        GetWindowRect(hWnd, &wi.m_rcWnd);
        wi.dwStyle = dwStyle;
        wi.dwExStyle = dwExStyle;
        pFWD->paWindows->push_back(wi);

        return TRUE;
      }
    }
  }

  return TRUE;
}

BOOL CScreenCapture::FindWindows(HANDLE hProcess, BOOL bAllProcessWindows,
    std::vector<WindowInfo>* paWindows) {
  FindWindowData fwd;
  fwd.bAllProcessWindows = bAllProcessWindows;
  fwd.hProcess = hProcess;
  fwd.paWindows = paWindows;
  EnumWindows(EnumWndProc, (LPARAM) & fwd);

  if (!bAllProcessWindows) {
    // Find only the main window
    // The main window should have caption, system menu and WS_EX_APPWINDOW style.  
    WindowInfo MainWnd;
    BOOL bMainWndFound = FALSE;

    size_t i;
    for (i = 0; i < paWindows->size(); i++) {
      if (((*paWindows)[i].dwExStyle & WS_EX_APPWINDOW)
          && ((*paWindows)[i].dwStyle & WS_CAPTION) && ((*paWindows)[i].dwStyle
          & WS_SYSMENU)) {
        // Match!
        bMainWndFound = TRUE;
        MainWnd = (*paWindows)[i];
        break;
      }
    }

    if (!bMainWndFound && paWindows->size() > 0) {
      MainWnd = (*paWindows)[0];
      bMainWndFound = TRUE;
    }

    if (bMainWndFound) {
      paWindows->clear();
      paWindows->push_back(MainWnd);
    }
  }

  return TRUE;
}

