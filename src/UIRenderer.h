#pragma once

#include "Globals.h"
#include <vector>

class UIRenderer {
public:
    static void DrawBackground(HDC hdcMem);
    static void DrawHeader(HDC hdcMem, HFONT hFontBold, HFONT hFontSmall);
    static void DrawSearchBox(HDC hdcMem, HFONT hFontMain, const std::wstring& searchQuery, bool caretVisible);
    static void DrawAppList(HDC hdcMem, HFONT hFontMain, const std::vector<AppItem>& filteredItems, int scrollOffset);
    static void DrawFooter(HDC hdcMem, HFONT hFontMain, HICON hPowerIcon, bool powerHovered);
    static void DrawWindowBorder(HDC hdcMem);
};
