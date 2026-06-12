# Tugas Besar Grafika Komputer - Start Menu Clone

Proyek ini adalah tiruan (clone) dari Windows Start Menu yang dibuat pakai C++ dan Win32 GDI murni. Sesuai ketentuan tugas, proyek ini tidak memakai framework GUI modern (seperti Qt/ImGui) ataupun control UI bawaan Windows (CreateWindow BUTTON, dll). Semua tampilan digambar manual dari nol memakai fungsi primitif GDI.

## Fitur Utama

- **Menggambar Manual**: Semua background, kotak hover, text, dan tombol digambar manual lewat kode.
- **Double Buffering**: Mencegah tampilan berkedip (flicker) pas mouse digerakkan atau pas scrolling.
- **Load Icon Otomatis**: Icon aplikasi dibaca langsung dari file `.exe` secara dinamis.
- **Kolom Search**: Tinggal ketik karakter buat cari aplikasi secara langsung (real-time filter). Tekan **Enter** buat langsung buka hasil teratas.
- **Scroll Mouse**: List aplikasi bisa di-scroll pakai wheel mouse kalau jumlah aplikasinya banyak.
- **Auto Close**: Start menu otomatis menutup sendiri kalau kita klik di luar area jendela (losing focus).
- **Tombol Power**: Tombol di pojok kanan bawah buat nutup aplikasi start menu ini (dengan konfirmasi dialog).

## Struktur File

- `main.cpp` : Kode program utama (looping window, render GDI, input keyboard & mouse).
- `apps.txt` : File text buat edit daftar aplikasi yang mau ditampilin (format: `Nama Aplikasi|Path File`).
- `compile.bat` : Script compiler otomatis lewat g++ (MinGW).

## Cara Compile & Jalankan

1. Pastikan **MinGW (GCC)** sudah terinstal di PC dan path-nya sudah disetting di Environment Variables.
2. Klik dua kali file `compile.bat` untuk mengompilasi program.
3. Setelah selesai, buka file `StartMenuClone.exe` yang terbuat di folder yang sama.
4. Anda bisa mengedit daftar aplikasi langsung di file `apps.txt` tanpa perlu mengompilasi ulang kode program.
