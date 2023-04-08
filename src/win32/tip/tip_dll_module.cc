// Copyright 2010-2021, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "win32/tip/tip_dll_module.h"

#include <windows.h>

#include <atomic>

#include "base/crash_report_handler.h"
#include "absl/base/call_once.h"

namespace mozc::win32::tsf {
namespace {

void TipShutdownCrashReportHandler() {
  if (CrashReportHandler::IsInitialized()) {
    // Uninitialize the breakpad.
    CrashReportHandler::Uninitialize();
  }
}

}  // namespace

volatile LONG TipDllModule::ref_count_ = 0;
HMODULE TipDllModule::module_handle_ = nullptr;
bool TipDllModule::unloaded_ = false;
absl::once_flag TipDllModule::uninitialize_once_;

LONG TipDllModule::Release() noexcept {
  if (::InterlockedDecrement(&ref_count_) == 0) {
    // |ref_count_| is now decremented to be 0. So our DLL is likely to be
    // unloaded soon. Here is the good point to release global resources
    // that should not be unloaded in DllMain due to the loader lock.
    // However, it should also be noted that there is a chance that
    // AddRef() is called again and the application continues to use Mozc
    // client DLL. Actually we can observe this situation inside
    // "Visual Studio 2012 Remote Debugging Monitor" running on Windows 8.
    // Thus we must not shut down libraries that cannot be designed to be
    // re-initializable. For instance, we must not call following
    // functions here.
    // - SingletonFinalizer::Finalize()            - b/10233768
    // - mozc::protobuf::ShutdownProtobufLibrary() - b/2126375
    absl::call_once(uninitialize_once_, &TipShutdownCrashReportHandler);
  }
  return ref_count_;
}

void TipDllModule::InitForUnitTest() {
  absl::call_once(uninitialize_once_, []() {});
}

}  // namespace mozc::win32::tsf
