#include <include/io/terminal.h>
#include <conio.h>
#include <cstdio>
#include <Windows.h>
int Nexus::IO::getch() {
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
        return 0;
    }
    return -1;
}