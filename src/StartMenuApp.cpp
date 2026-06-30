#include "StartMenuApp.h"
#include "UIRenderer.h"
#include <windowsx.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <cwctype>

// =========================================================
// 1. INISIALISASI KELAS (CONSTRUCTOR)
// =========================================================
StartMenuApp::StartMenuApp(HINSTANCE hInstance) 
    : m_hInstance(hInstance), m_hWnd(NULL), 
      m_caretVisible(true), m_scrollOffset(0), 
      m_powerHovered(false), m_trackingMouse(false), 
      m_isShowingDialog(false), m_hFontMain(NULL), 
      m_hFontBold(NULL), m_hFontSmall(NULL), 
      m_hPowerIcon(NULL) {}

// =========================================================
// 2. PEMROSESAN DATA & MEMORI (DATA PARSER & HELPER)
// Berisi fungsi penerjemah teks, path, dan pembaca file (.bmp / .txt)
// =========================================================
std::wstring StartMenuApp::ConvertToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::wstring StartMenuApp::GetExeDirectory() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring path(buffer);
    size_t pos = path.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? L"" : path.substr(0, pos);
}

// =========================================================
// 3. LOGIKA PENCARIAN (SEARCH LOGIC)
// =========================================================
bool StartMenuApp::ContainsSubstringCaseInsensitive(const std::wstring& str, const std::wstring& sub) {
    if (sub.empty()) return true;
    if (str.size() < sub.size()) return false;
    
    std::wstring strLower = L"";
    for (size_t i = 0; i < str.size(); ++i) {
        strLower += (wchar_t)towlower(str[i]);
    }
    
    std::wstring subLower = L"";
    for (size_t i = 0; i < sub.size(); ++i) {
        subLower += (wchar_t)towlower(sub[i]);
    }
    
    return strLower.find(subLower) != std::wstring::npos;
}

void StartMenuApp::UpdateFilter() {
    m_filteredItems.clear();
    for (size_t i = 0; i < m_appItems.size(); ++i) {
        if (ContainsSubstringCaseInsensitive(m_appItems[i].name, m_searchQuery)) {
            m_filteredItems.push_back(m_appItems[i]);
        }
    }
    m_scrollOffset = 0;
    if (m_hWnd) InvalidateRect(m_hWnd, NULL, FALSE);
}

HBITMAP LoadBitmapManually(const std::wstring& filepath) {
    HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return NULL;

    // 1. Baca seluruh ukuran file
    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == 0 || fileSize == INVALID_FILE_SIZE) {
        CloseHandle(hFile);
        return NULL;
    }

    // 2. Siapkan memori dan baca SELURUH isi file BMP sekaligus
    BYTE* fileData = new BYTE[fileSize];
    DWORD bytesRead;
    if (!ReadFile(hFile, fileData, fileSize, &bytesRead, NULL) || bytesRead != fileSize) {
        delete[] fileData;
        CloseHandle(hFile);
        return NULL;
    }
    CloseHandle(hFile);

    // 3. Parsing Header BMP secara manual menggunakan casting pointer
    BITMAPFILEHEADER* bmf = (BITMAPFILEHEADER*)fileData;
    
    // Cek apakah 2 byte pertama adalah "BM" (Bitmap)
    if (bmf->bfType != 0x4D42) { 
        delete[] fileData;
        return NULL;
    }

    // Ambil informasi header (lebar, tinggi, bit warna, dll)
    BITMAPINFO* bmi = (BITMAPINFO*)(fileData + sizeof(BITMAPFILEHEADER));
    
    // Ambil posisi persis di mana data piksel warna mulai
    BYTE* pixels = fileData + bmf->bfOffBits;

    // 4. Buat objek HBITMAP dari data mentah tersebut
    HDC hdc = GetDC(NULL);
    HBITMAP hBitmap = CreateDIBitmap(hdc, &bmi->bmiHeader, CBM_INIT, pixels, bmi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    delete[] fileData;
    return hBitmap;
}

void StartMenuApp::LoadApps() {
    std::wstring configPath = GetExeDirectory() + L"\\apps.txt";
    std::string narrowPath(configPath.begin(), configPath.end());
    std::ifstream file(narrowPath.c_str());
    
    if (!file.is_open()) {
        AppItem item;
        item.name = L"Notepad";
        item.path = L"C:\\Windows\\System32\\notepad.exe";
        item.isHovered = false;
        
        std::wstring defaultBmp = GetExeDirectory() + L"\\assets\\notepad.bmp";
        item.hBitmap = LoadBitmapManually(defaultBmp);
        
        m_appItems.push_back(item);
        m_filteredItems = m_appItems;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        size_t separator = line.find('|');
        if (separator == std::string::npos) continue;
        
        std::string name = line.substr(0, separator);
        std::string path = line.substr(separator + 1);
        
        AppItem item;
        item.name = ConvertToWide(name);
        item.path = ConvertToWide(path);
        item.isHovered = false;
        
        std::wstring bmpPath = GetExeDirectory() + L"\\assets\\" + item.name + L".bmp";
        item.hBitmap = LoadBitmapManually(bmpPath);
        
        m_appItems.push_back(item);
    }
    file.close();
    m_filteredItems = m_appItems;
}

// =========================================================
// 4. LOGIKA INTERAKSI (HOVER & COLLISION DETECTION)
// =========================================================
void StartMenuApp::UpdateHoverState(int mouseX, int mouseY) {
    bool stateChanged = false;
    int totalItems = (int)m_filteredItems.size();
    int listStart = HEADER_HEIGHT + SEARCH_BOX_HEIGHT;
    
    for (int i = 0; i < totalItems; ++i) {
        int itemY = listStart + i * ITEM_HEIGHT - m_scrollOffset;
        bool currentlyHovered = false;
        
        if (mouseY >= listStart && mouseY < listStart + MAIN_HEIGHT) {
            if (mouseX >= ITEM_MARGIN && mouseX < WINDOW_WIDTH - ITEM_MARGIN &&
                mouseY >= itemY && mouseY < itemY + ITEM_HEIGHT) {
                currentlyHovered = true;
            }
        }
        
        if (m_filteredItems[i].isHovered != currentlyHovered) {
            m_filteredItems[i].isHovered = currentlyHovered;
            stateChanged = true;
        }
    }
    
    bool powerHover = (mouseX >= WINDOW_WIDTH - 56 && mouseX < WINDOW_WIDTH - 12 &&
                       mouseY >= 546 && mouseY < 586);
    if (m_powerHovered != powerHover) {
        m_powerHovered = powerHover;
        stateChanged = true;
    }
    
    if (stateChanged && m_hWnd) {
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
}

// =========================================================
// 5. EVENT HANDLERS (MOUSE & KEYBOARD)
// Menangani semua input fisik dari pengguna
// =========================================================
LRESULT StartMenuApp::OnCreate() {
    m_hFontMain = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                              DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontBold = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                              OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                              DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    m_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                               DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    
    ExtractIconExW(L"shell32.dll", 27, NULL, &m_hPowerIcon, 1);
    if (!m_hPowerIcon) {
        m_hPowerIcon = LoadIconW(NULL, (LPCWSTR)IDI_SHIELD);
    }
    
    SetTimer(m_hWnd, 1, 500, NULL);
    LoadApps();
    return 0;
}

LRESULT StartMenuApp::OnMouseMove(int x, int y) {
    if (!m_trackingMouse) {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&tme);
        m_trackingMouse = true;
    }
    UpdateHoverState(x, y);
    return 0;
}

LRESULT StartMenuApp::OnMouseLeave() {
    m_trackingMouse = false;
    for (size_t i = 0; i < m_filteredItems.size(); ++i) {
        m_filteredItems[i].isHovered = false;
    }
    m_powerHovered = false;
    InvalidateRect(m_hWnd, NULL, FALSE);
    return 0;
}

LRESULT StartMenuApp::OnMouseWheel(short zDelta) {
    int totalItems = (int)m_filteredItems.size();
    int contentHeight = totalItems * ITEM_HEIGHT;
    
    if (contentHeight > MAIN_HEIGHT) {
        int maxScroll = contentHeight - MAIN_HEIGHT;
        m_scrollOffset -= (zDelta / WHEEL_DELTA) * 30;
        
        if (m_scrollOffset < 0) m_scrollOffset = 0;
        if (m_scrollOffset > maxScroll) m_scrollOffset = maxScroll;
        
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(m_hWnd, &pt);
        UpdateHoverState(pt.x, pt.y);
        
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
    return 0;
}

LRESULT StartMenuApp::OnChar(wchar_t ch) {
    if (ch == VK_BACK) {
        if (!m_searchQuery.empty()) {
            m_searchQuery.erase(m_searchQuery.size() - 1);
            UpdateFilter();
        }
    } else if (ch == VK_ESCAPE) {
        if (!m_searchQuery.empty()) {
            m_searchQuery = L"";
            UpdateFilter();
        } else {
            DestroyWindow(m_hWnd);
        }
    } else if (ch == VK_RETURN) {
        if (!m_filteredItems.empty()) {
            HINSTANCE res = ShellExecuteW(NULL, L"open", m_filteredItems[0].path.c_str(), NULL, NULL, SW_SHOWNORMAL);
            if ((INT_PTR)res > 32) {
                DestroyWindow(m_hWnd);
            } else {
                MessageBoxW(m_hWnd, L"Gagal membuka aplikasi!", L"Error", MB_OK | MB_ICONERROR);
            }
        }
    } else if (ch >= 32) {
        m_searchQuery += ch;
        UpdateFilter();
    }
    
    m_caretVisible = true;
    return 0;
}

LRESULT StartMenuApp::OnTimer(WPARAM timerId) {
    if (timerId == 1) {
        m_caretVisible = !m_caretVisible;
        InvalidateRect(m_hWnd, NULL, FALSE);
    }
    return 0;
}

LRESULT StartMenuApp::OnLButtonDown(int mouseX, int mouseY) {
    int listStart = HEADER_HEIGHT + SEARCH_BOX_HEIGHT;
    int totalItems = (int)m_filteredItems.size();
    
    for (int i = 0; i < totalItems; ++i) {
        int itemY = listStart + i * ITEM_HEIGHT - m_scrollOffset;
        if (mouseY >= listStart && mouseY < listStart + MAIN_HEIGHT) {
            if (mouseX >= ITEM_MARGIN && mouseX < WINDOW_WIDTH - ITEM_MARGIN &&
                mouseY >= itemY && mouseY < itemY + ITEM_HEIGHT) {
                
                HINSTANCE res = ShellExecuteW(NULL, L"open", m_filteredItems[i].path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                if ((INT_PTR)res <= 32) {
                    MessageBoxW(m_hWnd, L"Gagal membuka aplikasi! Pastikan path pada apps.txt benar.", L"Error", MB_OK | MB_ICONERROR);
                }
                
                DestroyWindow(m_hWnd);
                return 0;
            }
        }
    }
    
    // Cek klik pada tombol Power
    if (mouseX >= WINDOW_WIDTH - 56 && mouseX < WINDOW_WIDTH - 12 &&
        mouseY >= 546 && mouseY < 586) {
        DestroyWindow(m_hWnd);
        return 0;
    }
    
    return 0;
}

LRESULT StartMenuApp::OnKillFocus() {
    if (!m_isShowingDialog) {
        DestroyWindow(m_hWnd);
    }
    return 0;
}

// =========================================================
// 6. PENJEMBATAN VISUAL (RENDERING BRIDGE)
// Mendelegasikan tugas menggambar ke UIRenderer
// =========================================================
LRESULT StartMenuApp::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hWnd, &ps);
    
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
    HGDIOBJ hOldBmp = SelectObject(hdcMem, hbmMem);
    
    UIRenderer::DrawBackground(hdcMem);
    UIRenderer::DrawHeader(hdcMem, m_hFontBold, m_hFontSmall);
    UIRenderer::DrawSearchBox(hdcMem, m_hFontMain, m_searchQuery, m_caretVisible);
    UIRenderer::DrawAppList(hdcMem, m_hFontMain, m_filteredItems, m_scrollOffset);
    UIRenderer::DrawFooter(hdcMem, m_hFontMain, m_hPowerIcon, m_powerHovered);
    UIRenderer::DrawWindowBorder(hdcMem);
    
    BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcMem, 0, 0, SRCCOPY);
    
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    
    EndPaint(m_hWnd, &ps);
    return 0;
}

// =========================================================
// 7. SIKLUS HIDUP JENDELA & MESSAGE ROUTING
// Pondasi Win32 API untuk membuat, menghancurkan, dan menjalankan jendela
// =========================================================
LRESULT StartMenuApp::OnDestroy() {
    KillTimer(m_hWnd, 1);
    if (m_hFontMain) DeleteObject(m_hFontMain);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontSmall) DeleteObject(m_hFontSmall);
    if (m_hPowerIcon) DestroyIcon(m_hPowerIcon);
    
    for (size_t i = 0; i < m_appItems.size(); ++i) {
        if (m_appItems[i].hBitmap) {
            DeleteObject(m_appItems[i].hBitmap);
        }
    }
    
    PostQuitMessage(0);
    return 0;
}

bool StartMenuApp::Initialize(int nCmdShow) {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StartMenuApp::StaticWndProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"StartMenuCloneClassOOP";
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Registrasi Window Class Gagal!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    RECT rectWorkArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
    
    int x = rectWorkArea.left + 12;
    int y = rectWorkArea.bottom - WINDOW_HEIGHT - 12;
    
    m_hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Start Menu Clone",
        WS_POPUP,
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, m_hInstance, this
    );
    
    if (!m_hWnd) {
        MessageBoxW(NULL, L"Pembuatan Window Gagal!", L"Error", MB_OK | MB_ICONERROR);
        return false;
    }
    
    HRGN hRgn = CreateRoundRectRgn(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 12, 12);
    SetWindowRgn(m_hWnd, hRgn, TRUE);
    
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);
    
    return true;
}

void StartMenuApp::RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK StartMenuApp::StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    StartMenuApp* pThis = NULL;

    if (message == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (StartMenuApp*)pCreate->lpCreateParams;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hWnd = hWnd;
    } else {
        pThis = (StartMenuApp*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->WindowProc(hWnd, message, wParam, lParam);
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT StartMenuApp::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE:
            return OnCreate();
            
        case WM_MOUSEMOVE:
            return OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            
        case WM_MOUSELEAVE:
            return OnMouseLeave();
            
        case WM_MOUSEWHEEL:
            return OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
            
        case WM_CHAR:
            return OnChar((wchar_t)wParam);
            
        case WM_TIMER:
            return OnTimer(wParam);
            
        case WM_LBUTTONDOWN:
            return OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            
        case WM_KILLFOCUS:
            return OnKillFocus();
            
        case WM_PAINT:
            return OnPaint();
            
        case WM_DESTROY:
            return OnDestroy();
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
