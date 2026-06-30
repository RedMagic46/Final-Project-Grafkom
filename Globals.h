#pragma once

#include <windows.h>
#include <string>

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
    HBITMAP hBitmap;
    bool isHovered;
};
