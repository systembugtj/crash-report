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

// File: AssyncNotification.h
// Description: Provides a way to communicate between sender thread and the main thread.
// Authors: zexspectrum
// Date: 2009

#pragma once
#include "stdafx.h"

struct AssyncNotification {
  /* Constructor */
  AssyncNotification();

  void Reset();
  void SetProgress(CString sStatusMsg,
                   int percentCompleted, bool bRelative =true);
  void SetProgress(int percentCompleted, bool bRelative = true);
  void GetProgress(int& nProgressPct, std::vector<CString>& msg_log);
  void SetCompleted(int nCompletionStatus);
  int WaitForCompletion();
  // Cancels the assynchronious operation
  void Cancel();
  bool IsCancelled();
  // Waits until the feedback is received
  void WaitForFeedback(int &code);
  // Notifies about feedback is ready to be received
  void FeedbackReady(int code);

private:

  CComAutoCriticalSection m_cs; // Protects internal state
  int m_nCompletionStatus;
  HANDLE m_hCompletionEvent;
  HANDLE m_hCancelEvent;
  HANDLE m_hFeedbackEvent;
  int m_nPercentCompleted;
  std::vector<CString> m_statusLog; // Status log
};
