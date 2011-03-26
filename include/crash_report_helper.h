#ifndef CRASH_REPORT_HELPER_H_
#define CRASH_REPORT_HELPER_H_

#include <ctime>

#include "crash_report.h"

namespace crash_report {

 static void CallBackAtNormalExit() {
    FILE* fp = fopen(CR_CRASH_LOG_FILE, "a+");
    if ( fp!= NULL) {
      // 1 stands for exit normally 
      fprintf(fp, "%d:1\n", time(NULL));
    } else {
      //  printf("fail to open file crash.log");
      MessageBoxA(NULL, "fail to open file crash.log", "error", 0);
    }
    fclose(fp);
  }


  //  typedef std::basic_string<TCHAR> tstring;
  class CrAutoInstallHelper {
  public:
    CrAutoInstallHelper() {
      memset(&info_, 0, sizeof(CR_INSTALL_INFO));
      info_.size = sizeof(CR_INSTALL_INFO);
      //  set some default value for CR_INSTALL_INFO
      // only support HTTP now.delete SMTP and SMAPI
      info_.priorities[CR_HTTP] = 3;
      atexit(CallBackAtNormalExit);
    }
    ~CrAutoInstallHelper() {
      crUninstall();
    }
    //  ����Ҫһ�������������÷���Э�顣��������޸�ΪЭ�����ƣ�
    //  �����Ƿ���Э������ȼ���
    void set_application_name( TCHAR* name) {
      info_.application_name = name;
    }
    void set_application_version( TCHAR* version) {
      info_.application_version = version;
    }
    void set_crash_server_url( TCHAR* crash_server_url) {
      info_.crash_server_url = crash_server_url;
    }
    void set_sender_path( TCHAR* sender_path) {
      info_.sender_path = sender_path;
    }
    void set_crash_callback(LPGETLOGFILE crash_callback) {
      info_.crash_callback = crash_callback;
    }
    void set_flags(DWORD flags) {
      info_.flags = flags;
    }
    void set_privacy_policy_url( TCHAR* privacy_policy_url) {
      info_.privacy_policy_url = privacy_policy_url;
    }
    void set_debug_help_dll( TCHAR* debug_help_dll) {
      info_.debug_help_dll = debug_help_dll;
    }
    void set_minidump_type(MINIDUMP_TYPE minidump_type) {
      info_.minidump_type = minidump_type;
    }
    void set_save_dir( TCHAR* save_dir) {
      info_.save_dir = save_dir;
    }
    void set_restart_cmd( TCHAR* restart_cmd) {
      info_.restart_cmd = restart_cmd;
    }
    void set_langpack_path( TCHAR* langpack_path) {
      info_.langpack_path = langpack_path;
    }
    void set_custom_sender_icon( TCHAR* custom_sender_icon) {
      info_.custom_sender_icon = custom_sender_icon;
    }

    //  new function to check healthy before run application.
    // TODO(yesp) : ʵ�ָú������ڳ�����������ǰ��
    //  �ȼ���ϴ��Ƿ�����ȷ�˳��ģ�������ǵĻ�����Ӧ�ò鿴�Ƿ��ܹ��ָ���
    //  ����ĳ��ģ�鲻�ܼ���֮�࣬���߷����ϴα��������ı��������ļ��ȡ�
    //  �����������ʹ���û��Ļص����������û��Լ�ȥ��ȥ�ϴ��쳣�˳��������
    //  �����ڻص�������������ǿ�ƻ��߽����û������ȡ�
    bool CrAutoInstallHelper::CheckHealthyBeforeStart() {
      return true;
    }


    bool AddFile(const TCHAR* pszFile,
      const TCHAR* pszDestFile, const TCHAR* pszDesc, DWORD dwFlags) {
        crAddFile2(pszFile, pszDestFile, pszDesc, dwFlags);
        return true;
    }
    bool AddFile(const TCHAR* pszFile, const TCHAR* pszDesc) {
      crAddFile(pszFile, pszDesc);
      return true;

    }
    bool AddScreenshot(DWORD dwFlags) {
      crAddScreenshot(dwFlags);
      return true;

    }
    bool AddScreenshot(DWORD dwFlags, int nJpegQuality) {
      crAddScreenshot2(dwFlags, nJpegQuality);
      return true;
    }
    bool AddProperty(const TCHAR* key, const TCHAR* value) {
      crAddProperty(key, value);
      return true;
    }
    bool AddRegKey(const TCHAR* pszRegKey,
      const TCHAR* pszDstFileName, DWORD dwFlags) {
        crAddRegKey(pszRegKey, pszDstFileName, dwFlags);
        return true;
    }

    int Install() {
      return crInstall(&info_);
    }
  private:
    CR_INSTALL_INFO info_;
  };

  class CrThreadAutoInstallHelper {
  public:
    CrThreadAutoInstallHelper(DWORD falgs = 0) {
      result_ = crInstallToCurrentThread2(falgs);
      assert(result_ == 0);
    }
    ~CrThreadAutoInstallHelper() {
      result_ = crUninstallFromCurrentThread();
      assert(result_ == 0);
    }
    int result() { return result_; }
  private:
    int result_;
  };

}  // namespace crash_report

#endif //  CRASH_REPORT_HELPER_H_
