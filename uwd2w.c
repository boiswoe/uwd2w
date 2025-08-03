#include <windows.h>

DWORD getExplorerPID() {
    HWND hwnd = GetShellWindow();
    DWORD pid = 0;
    if (hwnd) GetWindowThreadProcessId(hwnd, &pid);
    return pid;
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
    ShellExecute(NULL, "open", "uwd2.exe", NULL, NULL, SW_HIDE);
    while (1) {
        DWORD pid = getExplorerPID();
        if (pid == 0) {
            continue;
        }
        HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (!hProc) {
            continue;
        }
        WaitForSingleObject(hProc, INFINITE);
        CloseHandle(hProc);
        ShellExecute(NULL, "open", "uwd2.exe", NULL, NULL, SW_HIDE); // patch ulang
    }
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
	HANDLE hMutex = CreateMutexA(NULL, FALSE, "uwd2w_single_instance");
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		return 0;
	}

	if (IsAdmin()) {
		ensure_run_key_global();  // ke LOCAL_MACHINE
	} else {
		ensure_run_key_user();    // ke CURRENT_USER
	}

	runWatcher();
    return 0;
}