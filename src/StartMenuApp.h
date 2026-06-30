#pragma once

#include "Globals.h"
#include <vector>
#include <string>

class StartMenuApp {
private:
    HWND m_hWnd;
    HINSTANCE m_hInstance;
    
    std::vector<AppItem> m_appItems;
    std::vector<AppItem> m_filteredItems;
    std::wstring m_searchQuery;
    
    bool m_caretVisible;
    int m_scrollOffset;
    bool m_powerHovered;
    bool m_trackingMouse;
    bool m_isShowingDialog;
    
    HFONT m_hFontMain;
    HFONT m_hFontBold;
    HFONT m_hFontSmall;
    HICON m_hPowerIcon;

    std::wstring ConvertToWide(const std::string& str);
    std::wstring GetExeDirectory();
    bool ContainsSubstringCaseInsensitive(const std::wstring& str, const std::wstring& sub);
    void UpdateFilter();
    void LoadApps();
    void UpdateHoverState(int mouseX, int mouseY);

    LRESULT OnCreate();
    LRESULT OnMouseMove(int x, int y);
    LRESULT OnMouseLeave();
    LRESULT OnMouseWheel(short zDelta);
    LRESULT OnChar(wchar_t ch);
    LRESULT OnTimer(WPARAM timerId);
    LRESULT OnLButtonDown(int mouseX, int mouseY);
    LRESULT OnKillFocus();
    LRESULT OnPaint();
    LRESULT OnDestroy();

public:
    StartMenuApp(HINSTANCE hInstance);
    
    bool Initialize(int nCmdShow);
    void RunMessageLoop();

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};
