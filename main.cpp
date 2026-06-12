#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cwctype>

/*
 * Definisi makro untuk menjaga kompatibilitas kompilasi pada compiler MinGW versi lama (seperti GCC 4.4.1)
 * yang berkas header bawaannya belum mendefinisikan konstanta era Windows Vista / 7 ke atas.
 */
#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif

#ifndef IDI_SHIELD
#define IDI_SHIELD MAKEINTRESOURCE(32518)
#endif

// Dimensi dasar Start Menu
const int WINDOW_WIDTH = 420;
const int WINDOW_HEIGHT = 600;

// Parameter pembagian tata letak antarmuka (UI Layout)
const int HEADER_HEIGHT = 80;
const int SEARCH_BOX_HEIGHT = 50;
const int FOOTER_HEIGHT = 60;
const int MAIN_HEIGHT = WINDOW_HEIGHT - HEADER_HEIGHT - SEARCH_BOX_HEIGHT - FOOTER_HEIGHT; // Tinggi viewport list aplikasi
const int ITEM_HEIGHT = 50;
const int ITEM_MARGIN = 10;

struct AppItem {
    std::wstring name;
    std::wstring path;
    HICON icon;
    bool isHovered;
};

// State penampung data aplikasi global
std::vector<AppItem> g_appItems;        // Master data yang di-load dari apps.txt
std::vector<AppItem> g_filteredItems;   // Data yang aktif dirender (hasil filter search)
std::wstring g_searchQuery = L"";       
bool g_caretVisible = true;             
int g_scrollOffset = 0;
bool g_powerHovered = false;
bool g_trackingMouse = false;           

HFONT g_hFontMain = NULL;
HFONT g_hFontBold = NULL;
HFONT g_hFontSmall = NULL;
HICON g_hPowerIcon = NULL;

/*
 * Konversi string multi-byte (ASCII/UTF-8) ke wide-string (UTF-16) diperlukan karena Windows API
 * menggunakan wchar_t untuk rendering teks berkualitas tinggi (DrawTextW) dan shell process (ShellExecuteW).
 */
std::wstring ConvertToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

/*
 * Memperoleh path direktori aktif tempat executable dijalankan.
 * Menjamin pembacaan file konfigurasi apps.txt selalu relatif terhadap letak aplikasi.
 */
std::wstring GetExeDirectory() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    std::wstring path(buffer);
    size_t pos = path.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? L"" : path.substr(0, pos);
}

/*
 * Algoritma pencarian substring tanpa memperdulikan huruf besar/kecil (case-insensitive).
 * Mengubah karakter ke bentuk huruf kecil sebelum perbandingan untuk mempermudah pencarian.
 */
bool ContainsSubstringCaseInsensitive(const std::wstring& str, const std::wstring& sub) {
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

/*
 * Menyaring aplikasi yang cocok dengan kata kunci secara real-time.
 * Scroll offset direset ke 0 agar daftar kembali ke atas setiap kali filter berubah.
 */
void UpdateFilter(HWND hWnd) {
    g_filteredItems.clear();
    for (size_t i = 0; i < g_appItems.size(); ++i) {
        if (ContainsSubstringCaseInsensitive(g_appItems[i].name, g_searchQuery)) {
            g_filteredItems.push_back(g_appItems[i]);
        }
    }
    g_scrollOffset = 0;
    InvalidateRect(hWnd, NULL, FALSE);
}

/*
 * Membaca data apps.txt secara dinamis.
 * Mengekstrak icon kecil (16x16) dari target path menggunakan fungsi shell Windows.
 * Dilengkapi fallback icon default sistem jika proses pembacaan file/icon gagal.
 */
void LoadApps() {
    std::wstring configPath = GetExeDirectory() + L"\\apps.txt";
    std::string narrowPath(configPath.begin(), configPath.end());
    std::ifstream file(narrowPath.c_str());
    
    if (!file.is_open()) {
        // Fallback default jika file konfigurasi apps.txt tidak ditemukan
        AppItem item;
        item.name = L"Notepad";
        item.path = L"C:\\Windows\\System32\\notepad.exe";
        item.isHovered = false;
        
        SHFILEINFOW sfi = {0};
        SHGetFileInfoW(item.path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON);
        item.icon = sfi.hIcon ? sfi.hIcon : LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
        
        g_appItems.push_back(item);
        g_filteredItems = g_appItems;
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
        
        // SHGetFileInfoW bekerja secara dinamis membaca icon dari target path, file, ataupun ekstensi file
        SHFILEINFOW sfi = {0};
        DWORD_PTR hr = SHGetFileInfoW(item.path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON);
        if (hr != 0 && sfi.hIcon != NULL) {
            item.icon = sfi.hIcon;
        } else {
            // Fallback dengan simulasi tipe file atribut jika target program sedang tidak ada secara fisik
            hr = SHGetFileInfoW(item.path.c_str(), FILE_ATTRIBUTE_NORMAL, &sfi, sizeof(sfi), 
                                SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
            if (hr != 0 && sfi.hIcon != NULL) {
                item.icon = sfi.hIcon;
            } else {
                item.icon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
            }
        }
        
        g_appItems.push_back(item);
    }
    file.close();
    g_filteredItems = g_appItems;
}

/*
 * Menguji titik koordinat mouse terhadap area tombol list aplikasi (viewport-relative) dan tombol Power.
 * Pemanggilan InvalidateRect hanya dilakukan ketika ada perubahan status hover untuk optimalisasi performa CPU.
 */
void UpdateHoverState(HWND hWnd, int mouseX, int mouseY) {
    bool stateChanged = false;
    int totalItems = (int)g_filteredItems.size();
    int listStart = HEADER_HEIGHT + SEARCH_BOX_HEIGHT;
    
    for (int i = 0; i < totalItems; ++i) {
        int itemY = listStart + i * ITEM_HEIGHT - g_scrollOffset;
        bool currentlyHovered = false;
        
        if (mouseY >= listStart && mouseY < listStart + MAIN_HEIGHT) {
            if (mouseX >= ITEM_MARGIN && mouseX < WINDOW_WIDTH - ITEM_MARGIN &&
                mouseY >= itemY && mouseY < itemY + ITEM_HEIGHT) {
                currentlyHovered = true;
            }
        }
        
        if (g_filteredItems[i].isHovered != currentlyHovered) {
            g_filteredItems[i].isHovered = currentlyHovered;
            stateChanged = true;
        }
    }
    
    bool powerHover = (mouseX >= WINDOW_WIDTH - 56 && mouseX < WINDOW_WIDTH - 12 &&
                       mouseY >= 546 && mouseY < 586);
    if (g_powerHovered != powerHover) {
        g_powerHovered = powerHover;
        stateChanged = true;
    }
    
    if (stateChanged) {
        InvalidateRect(hWnd, NULL, FALSE);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            // Menginisialisasi visual font sistem dengan parameter antialiasing CLEARTYPE
            g_hFontMain = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                                      DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontBold = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                      OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                                      DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            g_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, 
                                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, 
                                       DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            // Mengambil icon Power standard Windows dari perpustakaan shell32.dll
            ExtractIconExW(L"shell32.dll", 27, NULL, &g_hPowerIcon, 1);
            if (!g_hPowerIcon) {
                g_hPowerIcon = LoadIconW(NULL, (LPCWSTR)IDI_SHIELD);
            }
            
            // Timer berdenyut 500ms digunakan untuk memperbarui status kedip kursor ketik (caret)
            SetTimer(hWnd, 1, 500, NULL);
            LoadApps();
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            
            // TrackMouseEvent dipasang secara dinamis untuk menangkap saat mouse meninggalkan jendela
            if (!g_trackingMouse) {
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                tme.dwHoverTime = HOVER_DEFAULT;
                TrackMouseEvent(&tme);
                g_trackingMouse = true;
            }
            
            UpdateHoverState(hWnd, x, y);
            return 0;
        }
        
        case WM_MOUSELEAVE: {
            // Ketika kursor meninggalkan window, paksa hilangkan seluruh efek highlight UI
            g_trackingMouse = false;
            for (size_t i = 0; i < g_filteredItems.size(); ++i) {
                g_filteredItems[i].isHovered = false;
            }
            g_powerHovered = false;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
        
        case WM_MOUSEWHEEL: {
            short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            int totalItems = (int)g_filteredItems.size();
            int contentHeight = totalItems * ITEM_HEIGHT;
            
            if (contentHeight > MAIN_HEIGHT) {
                int maxScroll = contentHeight - MAIN_HEIGHT;
                g_scrollOffset -= (zDelta / WHEEL_DELTA) * 30; // Kecepatan scroll 30px per notch
                
                if (g_scrollOffset < 0) g_scrollOffset = 0;
                if (g_scrollOffset > maxScroll) g_scrollOffset = maxScroll;
                
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd, &pt);
                UpdateHoverState(hWnd, pt.x, pt.y);
                
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_CHAR: {
            wchar_t ch = (wchar_t)wParam;
            if (ch == VK_BACK) {
                if (!g_searchQuery.empty()) {
                    g_searchQuery.erase(g_searchQuery.size() - 1);
                    UpdateFilter(hWnd);
                }
            } else if (ch == VK_ESCAPE) {
                if (!g_searchQuery.empty()) {
                    g_searchQuery = L"";
                    UpdateFilter(hWnd);
                } else {
                    DestroyWindow(hWnd);
                }
            } else if (ch == VK_RETURN) {
                // Shortcut instan: Menekan Enter langsung membuka hasil pencarian paling atas
                if (!g_filteredItems.empty()) {
                    HINSTANCE res = ShellExecuteW(NULL, L"open", g_filteredItems[0].path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                    if ((INT_PTR)res > 32) {
                        DestroyWindow(hWnd);
                    } else {
                        MessageBoxW(hWnd, L"Gagal membuka aplikasi!", L"Error", MB_OK | MB_ICONERROR);
                    }
                }
            } else if (ch >= 32) {
                g_searchQuery += ch;
                UpdateFilter(hWnd);
            }
            
            g_caretVisible = true;
            return 0;
        }
        
        case WM_TIMER: {
            if (wParam == 1) {
                g_caretVisible = !g_caretVisible;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);
            int listStart = HEADER_HEIGHT + SEARCH_BOX_HEIGHT;
            
            int totalItems = (int)g_filteredItems.size();
            for (int i = 0; i < totalItems; ++i) {
                int itemY = listStart + i * ITEM_HEIGHT - g_scrollOffset;
                if (mouseY >= listStart && mouseY < listStart + MAIN_HEIGHT) {
                    if (mouseX >= ITEM_MARGIN && mouseX < WINDOW_WIDTH - ITEM_MARGIN &&
                        mouseY >= itemY && mouseY < itemY + ITEM_HEIGHT) {
                        
                        HINSTANCE res = ShellExecuteW(NULL, L"open", g_filteredItems[i].path.c_str(), NULL, NULL, SW_SHOWNORMAL);
                        if ((INT_PTR)res <= 32) {
                            MessageBoxW(hWnd, L"Gagal membuka aplikasi! Pastikan path pada apps.txt benar.", L"Error", MB_OK | MB_ICONERROR);
                        }
                        
                        DestroyWindow(hWnd);
                        return 0;
                    }
                }
            }
            
            if (mouseX >= WINDOW_WIDTH - 56 && mouseX < WINDOW_WIDTH - 12 &&
                mouseY >= 546 && mouseY < 586) {
                
                int result = MessageBoxW(hWnd, 
                    L"Apakah Anda ingin keluar dari aplikasi Start Menu Clone?", 
                    L"Start Menu Clone", 
                    MB_YESNO | MB_ICONQUESTION | MB_TOPMOST);
                if (result == IDYES) {
                    DestroyWindow(hWnd);
                }
                return 0;
            }
            return 0;
        }
        
        case WM_KILLFOCUS: {
            // Apabila user mengklik aplikasi eksternal atau di luar jendela Start Menu, hancurkan jendela ini
            DestroyWindow(hWnd);
            return 0;
        }
        
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            /*
             * Double Buffering: Seluruh komponen digambar di memori DC (hdcMem) terlebih dahulu
             * baru kemudian dipindahkan ke layar asli (BitBlt). Mengurangi artifact flickering visual.
             */
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
            HGDIOBJ hOldBmp = SelectObject(hdcMem, hbmMem);
            
            // --- 1. Menggambar Background Utama ---
            RECT clientRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
            HBRUSH hBgBrush = CreateSolidBrush(RGB(31, 31, 31));
            FillRect(hdcMem, &clientRect, hBgBrush);
            DeleteObject(hBgBrush);
            
            int listStart = HEADER_HEIGHT + SEARCH_BOX_HEIGHT;
            RECT footerRect = { 0, listStart + MAIN_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT };
            HBRUSH hFooterBrush = CreateSolidBrush(RGB(24, 24, 24));
            FillRect(hdcMem, &footerRect, hFooterBrush);
            DeleteObject(hFooterBrush);
            
            // Garis pembatas tipis horizontal footer
            HPEN hDividerPen = CreatePen(PS_SOLID, 1, RGB(48, 48, 48));
            HGDIOBJ hOldPen = SelectObject(hdcMem, hDividerPen);
            MoveToEx(hdcMem, 0, listStart + MAIN_HEIGHT, NULL);
            LineTo(hdcMem, WINDOW_WIDTH, listStart + MAIN_HEIGHT);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hDividerPen);
            
            // --- 2. Menggambar Header Profil Pengguna ---
            HBRUSH hAvatarBrush = CreateSolidBrush(RGB(0, 120, 215));
            HGDIOBJ hOldBrush = SelectObject(hdcMem, hAvatarBrush);
            HPEN hNullPen = CreatePen(PS_NULL, 0, 0);
            hOldPen = SelectObject(hdcMem, hNullPen);
            
            Ellipse(hdcMem, 25, 16, 25 + 48, 16 + 48);
            
            SelectObject(hdcMem, hOldBrush);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hAvatarBrush);
            DeleteObject(hNullPen);
            
            SetBkMode(hdcMem, TRANSPARENT);
            SetTextColor(hdcMem, RGB(255, 255, 255));
            
            HGDIOBJ hOldFont = SelectObject(hdcMem, g_hFontBold);
            RECT avatarTextRect = { 25, 16, 25 + 48, 16 + 48 };
            DrawTextW(hdcMem, L"NM", -1, &avatarTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            RECT nameRect = { 85, 20, WINDOW_WIDTH - 20, 44 };
            DrawTextW(hdcMem, L"Naufal Muammar", -1, &nameRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
            
            SelectObject(hdcMem, g_hFontSmall);
            SetTextColor(hdcMem, RGB(160, 160, 160));
            RECT subtitleRect = { 85, 44, WINDOW_WIDTH - 20, 68 };
            DrawTextW(hdcMem, L"Administrator", -1, &subtitleRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
            
            // --- 3. Menggambar Box Pencarian (Search Bar) ---
            RECT searchBoxRect = { 15, 90, WINDOW_WIDTH - 15, 120 };
            HBRUSH hSearchBoxBrush = CreateSolidBrush(RGB(45, 45, 45));
            HPEN hSearchBorderPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 70));
            
            HGDIOBJ hOldSearchBrush = SelectObject(hdcMem, hSearchBoxBrush);
            HGDIOBJ hOldSearchPen = SelectObject(hdcMem, hSearchBorderPen);
            
            RoundRect(hdcMem, searchBoxRect.left, searchBoxRect.top, searchBoxRect.right, searchBoxRect.bottom, 6, 6);
            
            SelectObject(hdcMem, hOldSearchBrush);
            SelectObject(hdcMem, hOldSearchPen);
            DeleteObject(hSearchBoxBrush);
            DeleteObject(hSearchBorderPen);
            
            SelectObject(hdcMem, g_hFontMain);
            RECT textSearchRect = { 25, 90, WINDOW_WIDTH - 25, 120 };
            
            if (g_searchQuery.empty()) {
                SetTextColor(hdcMem, RGB(140, 140, 140));
                DrawTextW(hdcMem, L"Ketik untuk mencari...", -1, &textSearchRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            } else {
                SetTextColor(hdcMem, RGB(255, 255, 255));
                DrawTextW(hdcMem, g_searchQuery.c_str(), -1, &textSearchRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }
            
            // Hitung lebar string dinamis (GetTextExtentPoint32W) untuk memposisikan kursor ketik secara presisi
            if (g_caretVisible) {
                SIZE sizeText;
                GetTextExtentPoint32W(hdcMem, g_searchQuery.c_str(), (int)g_searchQuery.size(), &sizeText);
                int caretX = 25 + sizeText.cx;
                
                HPEN hCaretPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
                HGDIOBJ hOldP = SelectObject(hdcMem, hCaretPen);
                MoveToEx(hdcMem, caretX, 97, NULL);
                LineTo(hdcMem, caretX, 113);
                SelectObject(hdcMem, hOldP);
                DeleteObject(hCaretPen);
            }
            
            // --- 4. Menggambar List Aplikasi (Clipped) ---
            // Memasang clipping region (HRGN) agar list aplikasi tidak menggambar menembus batas header/footer saat di-scroll
            HRGN hClipRgn = CreateRectRgn(0, listStart, WINDOW_WIDTH, listStart + MAIN_HEIGHT);
            SelectClipRgn(hdcMem, hClipRgn);
            
            int totalItems = (int)g_filteredItems.size();
            for (int i = 0; i < totalItems; ++i) {
                int itemY = listStart + i * ITEM_HEIGHT - g_scrollOffset;
                
                RECT hoverRect = { ITEM_MARGIN, itemY + 2, WINDOW_WIDTH - ITEM_MARGIN, itemY + ITEM_HEIGHT - 2 };
                
                if (g_filteredItems[i].isHovered) {
                    HBRUSH hHoverBrush = CreateSolidBrush(RGB(48, 48, 48));
                    HPEN hItemNullPen = CreatePen(PS_NULL, 0, 0);
                    HGDIOBJ hOldB = SelectObject(hdcMem, hHoverBrush);
                    HGDIOBJ hOldP = SelectObject(hdcMem, hItemNullPen);
                    
                    RoundRect(hdcMem, hoverRect.left, hoverRect.top, hoverRect.right, hoverRect.bottom, 8, 8);
                    
                    SelectObject(hdcMem, hOldB);
                    SelectObject(hdcMem, hOldP);
                    DeleteObject(hHoverBrush);
                    DeleteObject(hItemNullPen);
                }
                
                int iconSize = 24;
                int iconX = ITEM_MARGIN + 15;
                int iconY = itemY + (ITEM_HEIGHT - iconSize) / 2;
                if (g_filteredItems[i].icon) {
                    DrawIconEx(hdcMem, iconX, iconY, g_filteredItems[i].icon, iconSize, iconSize, 0, NULL, DI_NORMAL);
                }
                
                SelectObject(hdcMem, g_hFontMain);
                SetTextColor(hdcMem, RGB(255, 255, 255));
                RECT textRect = { ITEM_MARGIN + 54, itemY, WINDOW_WIDTH - ITEM_MARGIN - 15, itemY + ITEM_HEIGHT };
                DrawTextW(hdcMem, g_filteredItems[i].name.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }
            
            if (totalItems == 0) {
                SelectObject(hdcMem, g_hFontMain);
                SetTextColor(hdcMem, RGB(160, 160, 160));
                RECT emptyTextRect = { 20, listStart + 20, WINDOW_WIDTH - 20, listStart + 80 };
                DrawTextW(hdcMem, L"Tidak ada aplikasi yang cocok.", -1, &emptyTextRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
            }
            
            SelectClipRgn(hdcMem, NULL);
            DeleteObject(hClipRgn);
            
            // --- 5. Menggambar Footer (Windows Logo & Power Button) ---
            HBRUSH hLogoBrush = CreateSolidBrush(RGB(0, 120, 215));
            hOldBrush = SelectObject(hdcMem, hLogoBrush);
            HPEN hLogoNullPen = CreatePen(PS_NULL, 0, 0);
            hOldPen = SelectObject(hdcMem, hLogoNullPen);
            
            // Gambar logo datar Windows 11 (terdiri dari 4 kotak biru kecil)
            RECT rTL = { 20, 560, 29, 569 };
            FillRect(hdcMem, &rTL, hLogoBrush);
            RECT rTR = { 31, 560, 40, 569 };
            FillRect(hdcMem, &rTR, hLogoBrush);
            RECT rBL = { 20, 571, 29, 580 };
            FillRect(hdcMem, &rBL, hLogoBrush);
            RECT rBR = { 31, 571, 40, 580 };
            FillRect(hdcMem, &rBR, hLogoBrush);
            
            SelectObject(hdcMem, hOldBrush);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hLogoBrush);
            DeleteObject(hLogoNullPen);
            
            SelectObject(hdcMem, g_hFontMain);
            SetTextColor(hdcMem, RGB(255, 255, 255));
            RECT logoTextRect = { 48, 540, 150, 600 };
            DrawTextW(hdcMem, L"Windows", -1, &logoTextRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            
            if (g_powerHovered) {
                HBRUSH hPowerHoverBrush = CreateSolidBrush(RGB(48, 48, 48));
                HPEN hPowerNullPen = CreatePen(PS_NULL, 0, 0);
                HGDIOBJ hOldB = SelectObject(hdcMem, hPowerHoverBrush);
                HGDIOBJ hOldP = SelectObject(hdcMem, hPowerNullPen);
                
                RoundRect(hdcMem, WINDOW_WIDTH - 56, 546, WINDOW_WIDTH - 12, 586, 8, 8);
                
                SelectObject(hdcMem, hOldB);
                SelectObject(hdcMem, hOldP);
                DeleteObject(hPowerHoverBrush);
                DeleteObject(hPowerNullPen);
            }
            
            if (g_hPowerIcon) {
                DrawIconEx(hdcMem, WINDOW_WIDTH - 50, 550, g_hPowerIcon, 32, 32, 0, NULL, DI_NORMAL);
            }
            
            // --- 6. Menggambar Border Luar Window ---
            HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 70));
            hOldPen = SelectObject(hdcMem, hBorderPen);
            HGDIOBJ hOldBr = SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
            
            RoundRect(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 12, 12);
            
            SelectObject(hdcMem, hOldPen);
            SelectObject(hdcMem, hOldBr);
            DeleteObject(hBorderPen);
            
            BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, hdcMem, 0, 0, SRCCOPY);
            
            SelectObject(hdcMem, hOldBmp);
            DeleteObject(hbmMem);
            DeleteDC(hdcMem);
            
            SelectObject(hdcMem, hOldFont);
            EndPaint(hWnd, &ps);
            return 0;
        }
        
        case WM_DESTROY: {
            // Bersihkan seluruh GDI resource untuk mencegah memory leak
            KillTimer(hWnd, 1);
            if (g_hFontMain) DeleteObject(g_hFontMain);
            if (g_hFontBold) DeleteObject(g_hFontBold);
            if (g_hFontSmall) DeleteObject(g_hFontSmall);
            if (g_hPowerIcon) DestroyIcon(g_hPowerIcon);
            
            for (size_t i = 0; i < g_appItems.size(); ++i) {
                if (g_appItems[i].icon) {
                    DestroyIcon(g_appItems[i].icon);
                }
            }
            
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"StartMenuCloneClass";
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Registrasi Window Class Gagal!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Mendapatkan koordinat layar bersih (tanpa taskbar) untuk menempatkan window melayang di kiri bawah
    RECT rectWorkArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rectWorkArea, 0);
    
    int x = rectWorkArea.left + 12;
    int y = rectWorkArea.bottom - WINDOW_HEIGHT - 12;
    
    // WS_POPUP (Borderless) digabungkan WS_EX_TOOLWINDOW (tidak tampil di taskbar utama/Alt-Tab)
    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        L"Start Menu Clone",
        WS_POPUP,
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );
    
    if (!hWnd) {
        MessageBoxW(NULL, L"Pembuatan Window Gagal!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // Menerapkan window region agar sudut jendela melengkung (rounded corners)
    HRGN hRgn = CreateRoundRectRgn(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 12, 12);
    SetWindowRgn(hWnd, hRgn, TRUE);
    
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
