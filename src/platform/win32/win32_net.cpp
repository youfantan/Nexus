#include <include/platform/win32/win32_net.h>

namespace Nexus::Net {
    void CloseSocket(io_handle_t handle) {
        ::closesocket(handle);
    }
    bool SetNonblockingSocket(io_handle_t handle) {
        u_long arg = 1;
        return ioctlsocket(handle, FIONBIO, &arg) != SOCKET_ERROR;
    }
    int GetLastNetworkError() {
        return WSAGetLastError();
    }
    int GetLastSystemError() {
        return GetLastError();
    }


}