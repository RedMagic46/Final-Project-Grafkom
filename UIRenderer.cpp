#include "UIRenderer.h"

void UIRenderer::DrawBackground(HDC hdcMem) {
    RECT clientRect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    HBRUSH hBgBrush = CreateSolidBrush(RGB(31, 31, 31));
    FillRect(hdcMem, &clientRect, hBgBrush);
    DeleteObject(hBgBrush);
    
    int listStart = HEADER_HEIGHT + SEARCH_BOX_HEIGHT;
    RECT footerRect = { 0, listStart + MAIN_HEIGHT, WINDOW_WIDTH, WINDOW_HEIGHT };
    HBRUSH hFooterBrush = CreateSolidBrush(RGB(24, 24, 24));
    FillRect(hdcMem, &footerRect, hFooterBrush);
    DeleteObject(hFooterBrush);
    
    HPEN hDividerPen = CreatePen(PS_SOLID, 1, RGB(48, 48, 48));
    HGDIOBJ hOldPen = SelectObject(hdcMem, hDividerPen);
    MoveToEx(hdcMem, 0, listStart + MAIN_HEIGHT, NULL);
    LineTo(hdcMem, WINDOW_WIDTH, listStart + MAIN_HEIGHT);
    SelectObject(hdcMem, hOldPen);
    DeleteObject(hDividerPen);
}

void UIRenderer::DrawHeader(HDC hdcMem, HFONT hFontBold, HFONT hFontSmall) {
    HBRUSH hAvatarBrush = CreateSolidBrush(RGB(0, 120, 215));
    HGDIOBJ hOldBrush = SelectObject(hdcMem, hAvatarBrush);
    HPEN hNullPen = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ hOldPen = SelectObject(hdcMem, hNullPen);
    
    Ellipse(hdcMem, 25, 16, 25 + 48, 16 + 48);
    
    SelectObject(hdcMem, hOldBrush);
    SelectObject(hdcMem, hOldPen);
    DeleteObject(hAvatarBrush);
    DeleteObject(hNullPen);
    
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(255, 255, 255));
    
    HGDIOBJ hOldFont = SelectObject(hdcMem, hFontBold);
    RECT avatarTextRect = { 25, 16, 25 + 48, 16 + 48 };
    DrawTextW(hdcMem, L"NM", -1, &avatarTextRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    RECT nameRect = { 85, 20, WINDOW_WIDTH - 20, 44 };
    DrawTextW(hdcMem, L"Naufal Muammar", -1, &nameRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
    
    SelectObject(hdcMem, hFontSmall);
    SetTextColor(hdcMem, RGB(160, 160, 160));
    RECT subtitleRect = { 85, 44, WINDOW_WIDTH - 20, 68 };
    DrawTextW(hdcMem, L"Administrator", -1, &subtitleRect, DT_LEFT | DT_TOP | DT_SINGLELINE);
    
    SelectObject(hdcMem, hOldFont);
}

void UIRenderer::DrawSearchBox(HDC hdcMem, HFONT hFontMain, const std::wstring& searchQuery, bool caretVisible) {
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
    
    HGDIOBJ hOldFont = SelectObject(hdcMem, hFontMain);
    RECT textSearchRect = { 25, 90, WINDOW_WIDTH - 25, 120 };
    
    SetBkMode(hdcMem, TRANSPARENT);
    if (searchQuery.empty()) {
        SetTextColor(hdcMem, RGB(140, 140, 140));
        DrawTextW(hdcMem, L"Ketik untuk mencari...", -1, &textSearchRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
        SetTextColor(hdcMem, RGB(255, 255, 255));
        DrawTextW(hdcMem, searchQuery.c_str(), -1, &textSearchRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    
    if (caretVisible) {
        SIZE sizeText;
        GetTextExtentPoint32W(hdcMem, searchQuery.c_str(), (int)searchQuery.size(), &sizeText);
        int caretX = 25 + sizeText.cx;
        
        HPEN hCaretPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HGDIOBJ hOldP = SelectObject(hdcMem, hCaretPen);
        MoveToEx(hdcMem, caretX, 97, NULL);
        LineTo(hdcMem, caretX, 113);
        SelectObject(hdcMem, hOldP);
        DeleteObject(hCaretPen);
    }
    
    SelectObject(hdcMem, hOldFont);
}

void UIRenderer::DrawAppList(HDC hdcMem, HFONT hFontMain, const std::vector<AppItem>& filteredItems, int scrollOffset) {
    int listStart = HEADER_HEIGHT + SEARCH_BOX_HEIGHT;
    HRGN hClipRgn = CreateRectRgn(0, listStart, WINDOW_WIDTH, listStart + MAIN_HEIGHT);
    SelectClipRgn(hdcMem, hClipRgn);
    
    HGDIOBJ hOldFont = SelectObject(hdcMem, hFontMain);
    SetBkMode(hdcMem, TRANSPARENT);
    
    int totalItems = (int)filteredItems.size();
    for (int i = 0; i < totalItems; ++i) {
        int itemY = listStart + i * ITEM_HEIGHT - scrollOffset;
        
        RECT hoverRect = { ITEM_MARGIN, itemY + 2, WINDOW_WIDTH - ITEM_MARGIN, itemY + ITEM_HEIGHT - 2 };
        
        if (filteredItems[i].isHovered) {
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
        if (filteredItems[i].hBitmap) {
            HDC hdcBmp = CreateCompatibleDC(hdcMem);
            HGDIOBJ hOldBmp = SelectObject(hdcBmp, filteredItems[i].hBitmap);
            
            BITMAP bm;
            GetObject(filteredItems[i].hBitmap, sizeof(bm), &bm);
            
            // [SEBELUM DIPERBAIKI - GAMBAR RUSAK & GEPENG]
            // Kode di bawah ini akan memaksa gambar masuk ke kotak 24x24 tanpa anti-aliasing
            StretchBlt(hdcMem, iconX, iconY, iconSize, iconSize, hdcBmp, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
            
            /* 
            // [SESUDAH DIPERBAIKI - GAMBAR MULUS & PROPORSIONAL]
            // Hapus komentar blok ini dan hapus StretchBlt di atas untuk presentasi
            
            int oldMode = SetStretchBltMode(hdcMem, HALFTONE);
            SetBrushOrgEx(hdcMem, 0, 0, NULL);
            
            int destWidth = iconSize;
            int destHeight = iconSize;
            int offsetX = 0;
            int offsetY = 0;

            if (bm.bmWidth > bm.bmHeight) {
                destHeight = (iconSize * bm.bmHeight) / bm.bmWidth;
                offsetY = (iconSize - destHeight) / 2;
            } else if (bm.bmHeight > bm.bmWidth) {
                destWidth = (iconSize * bm.bmWidth) / bm.bmHeight;
                offsetX = (iconSize - destWidth) / 2;
            }

            StretchBlt(hdcMem, iconX + offsetX, iconY + offsetY, destWidth, destHeight, 
                       hdcBmp, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                       
            SetStretchBltMode(hdcMem, oldMode);
            */
                       
            SelectObject(hdcBmp, hOldBmp);
            DeleteDC(hdcBmp);
        }
        
        SetTextColor(hdcMem, RGB(255, 255, 255));
        RECT textRect = { ITEM_MARGIN + 54, itemY, WINDOW_WIDTH - ITEM_MARGIN - 15, itemY + ITEM_HEIGHT };
        DrawTextW(hdcMem, filteredItems[i].name.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    
    if (totalItems == 0) {
        SetTextColor(hdcMem, RGB(160, 160, 160));
        RECT emptyTextRect = { 20, listStart + 20, WINDOW_WIDTH - 20, listStart + 80 };
        DrawTextW(hdcMem, L"Tidak ada aplikasi yang cocok.", -1, &emptyTextRect, DT_CENTER | DT_TOP | DT_SINGLELINE);
    }
    
    SelectObject(hdcMem, hOldFont);
    SelectClipRgn(hdcMem, NULL);
    DeleteObject(hClipRgn);
}

void UIRenderer::DrawFooter(HDC hdcMem, HFONT hFontMain, HICON hPowerIcon, bool powerHovered) {
    HBRUSH hLogoBrush = CreateSolidBrush(RGB(0, 120, 215));
    HGDIOBJ hOldBrush = SelectObject(hdcMem, hLogoBrush);
    HPEN hLogoNullPen = CreatePen(PS_NULL, 0, 0);
    HGDIOBJ hOldPen = SelectObject(hdcMem, hLogoNullPen);
    
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
    
    HGDIOBJ hOldFont = SelectObject(hdcMem, hFontMain);
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, RGB(255, 255, 255));
    RECT logoTextRect = { 48, 540, 150, 600 };
    DrawTextW(hdcMem, L"Windows", -1, &logoTextRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdcMem, hOldFont);
    
    if (powerHovered) {
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
    
    // Menggambar Ikon Power (Shutdown) secara manual menggunakan bentuk primitif
    HPEN hPowerPen = CreatePen(PS_SOLID, 2, RGB(250, 60, 60)); // Warna merah
    HGDIOBJ hOldPen2 = SelectObject(hdcMem, hPowerPen);
    HGDIOBJ hOldBr2 = SelectObject(hdcMem, GetStockObject(NULL_BRUSH)); // Tengah bolong
    
    int cx = WINDOW_WIDTH - 34; // Titik tengah X
    int cy = 566;               // Titik tengah Y
    int r = 8;                  // Jari-jari lingkaran
    
    // Menggambar lingkaran setengah / terputus di atas (berlawanan jarum jam)
    Arc(hdcMem, cx - r, cy - r, cx + r, cy + r, 
        cx + 6, cy - 8,   // Start (Kanan atas)
        cx - 6, cy - 8);  // End (Kiri atas)
        
    // Menggambar garis lurus vertikal di tengah atas
    MoveToEx(hdcMem, cx, cy - r - 2, NULL);
    LineTo(hdcMem, cx, cy + 2);
    
    SelectObject(hdcMem, hOldPen2);
    SelectObject(hdcMem, hOldBr2);
    DeleteObject(hPowerPen);
}

void UIRenderer::DrawWindowBorder(HDC hdcMem) {
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 70));
    HGDIOBJ hOldPen = SelectObject(hdcMem, hBorderPen);
    HGDIOBJ hOldBr = SelectObject(hdcMem, GetStockObject(NULL_BRUSH));
    
    RoundRect(hdcMem, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 12, 12);
    
    SelectObject(hdcMem, hOldPen);
    SelectObject(hdcMem, hOldBr);
    DeleteObject(hBorderPen);
}
