# Start Menu Clone

Proyek ini adalah tiruan dari Windows Start Menu yang dibuat menggunakan C++ dan Win32 GDI murni. Proyek ini tidak memakai framework GUI modern (seperti Qt/ImGui) ataupun control UI bawaan Windows (CreateWindow BUTTON, dll). Semua tampilan digambar manual menggunakan fungsi primitif GDI.

## Fitur

- Semua background, kotak hover, text, dan tombol digambar manual menggunakan fungsi GDI.
- Mencegah tampilan berkedip (flicker) pas mouse digerakkan atau pas scrolling.
- Icon aplikasi dibaca langsung dari file `.exe` secara dinamis.
- Tinggal ketik karakter buat cari aplikasi secara langsung. Tekan **Enter** buat langsung buka hasil teratas.
- List aplikasi bisa di-scroll pakai wheel mouse kalau jumlah aplikasinya banyak.
- Start menu otomatis menutup sendiri kalau kita klik di luar area jendela (losing focus).
- Tombol di pojok kanan bawah buat nutup aplikasi start menu ini.

## Struktur File

- `src/` : Folder berisi source code C++ (`main.cpp`, `StartMenuApp.cpp`, `StartMenuApp.h`, `UIRenderer.cpp`, `UIRenderer.h`, `Globals.h`).
- `assets/` : Folder berisi asset gambar bitmap ikon aplikasi (`.bmp`).
- `apps.txt` : File text buat edit daftar aplikasi yang mau ditampilin (format: `Nama Aplikasi|Path File`).
- `compile.bat` : Script compiler otomatis lewat g++ (MinGW).

## Cara Compile & Run

1. Pastikan **MinGW (GCC)** sudah terinstal di PC dan path-nya sudah disetting di Environment Variables.
2. Klik dua kali file `compile.bat` buat compile program.
3. Setelah selesai, buka file `StartMenu.exe` yang ada di folder yang sama.
4. Anda bisa mengedit daftar aplikasi langsung di file `apps.txt` tanpa perlu mengompilasi ulang kode program.
