#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0601

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <locale.h>

#pragma comment(lib, "Ws2_32.lib")

#pragma pack(push, 1)
typedef struct eth_header {
    unsigned char dst[6];
    unsigned char src[6];
    unsigned short type;
} eth_header_t;

typedef struct ip_header {
    unsigned char  ihl_version;
    unsigned char  tos;
    unsigned short total_length;
    unsigned short id;
    unsigned short flags_offset;
    unsigned char  ttl;
    unsigned char  protocol;
    unsigned short checksum;
    unsigned int   src;
    unsigned int   dst;
} ip_header_t;

typedef struct tcp_header {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned int   seq;
    unsigned int   ack;
    unsigned short offset_flags;
    unsigned short window;
    unsigned short checksum;
    unsigned short urgent_ptr;
} tcp_header_t;

typedef struct udp_header {
    unsigned short src_port;
    unsigned short dst_port;
    unsigned short length;
    unsigned short checksum;
} udp_header_t;

typedef struct icmp_header {
    unsigned char  type;
    unsigned char  code;
    unsigned short checksum;
    unsigned int   rest;
} icmp_header_t;
#pragma pack(pop)

static volatile LONG g_running = 1;

static BOOL WINAPI on_ctrl(DWORD type) {
    (void)type;
    InterlockedExchange(&g_running, 0);
    return TRUE;
}

static void enable_utf8_console(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF8");
}

static void print_mac(const unsigned char mac[6]) {
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static const char* proto_name(unsigned char p) {
    switch (p) {
    case 1:  return "ICMP";
    case 6:  return "TCP";
    case 17: return "UDP";
    default: return "OTHER";
    }
}

static const char* ethertype_name(uint16_t type) {
    switch (type) {
    case 0x0800: return "IPv4";
    case 0x0806: return "ARP";
    case 0x86DD: return "IPv6";
    default: return "Unknown";
    }
}

static void print_tcp_flags(uint16_t flags) {
    bool first = true;
    struct { uint16_t bit; const char* name; } map[] = {
        {0x002, "SYN"}, {0x010, "ACK"}, {0x001, "FIN"}, {0x004, "RST"},
        {0x008, "PSH"}, {0x020, "URG"}, {0x040, "ECE"}, {0x080, "CWR"}
    };
    printf("FLAGS: ");
    for (int i = 0; i < (int)(sizeof(map) / sizeof(map[0])); i++) {
        if (flags & map[i].bit) {
            if (!first) printf(" ");
            printf("%s", map[i].name);
            first = false;
        }
    }
    if (first) printf("NONE");
}

static bool looks_like_ipv4(const unsigned char* buf, int len, int* out_ihl_bytes) {
    if (!buf || len < (int)sizeof(ip_header_t)) return false;

    unsigned char vihl = buf[0];
    int ver = (vihl >> 4) & 0x0F;
    int ihl = (vihl & 0x0F);

    if (ver != 4) return false;
    if (ihl < 5 || ihl > 15) return false;

    int ihl_bytes = ihl * 4;
    if (len < ihl_bytes) return false;

    if (out_ihl_bytes) *out_ihl_bytes = ihl_bytes;
    return true;
}

static void handle_ipv4(const unsigned char* buf, int len) {
    int ihl_bytes = 0;
    if (!looks_like_ipv4(buf, len, &ihl_bytes)) return;
    if (len < ihl_bytes) return;

    const ip_header_t* ip = (const ip_header_t*)buf;

    uint16_t total_len = ntohs(ip->total_length);
    if (total_len < (uint16_t)ihl_bytes) return;

    int effective_len = len;
    if (total_len > 0 && total_len < (uint16_t)len) {
        effective_len = (int)total_len;
    }

    struct in_addr src_addr, dst_addr;
    src_addr.s_addr = ip->src;
    dst_addr.s_addr = ip->dst;

    char src_buf[INET_ADDRSTRLEN] = { 0 };
    char dst_buf[INET_ADDRSTRLEN] = { 0 };

    if (!inet_ntop(AF_INET, &src_addr, src_buf, (socklen_t)sizeof(src_buf))) {
        const char* t = inet_ntoa(src_addr);
        if (t) strncpy(src_buf, t, sizeof(src_buf) - 1);
    }
    if (!inet_ntop(AF_INET, &dst_addr, dst_buf, (socklen_t)sizeof(dst_buf))) {
        const char* t = inet_ntoa(dst_addr);
        if (t) strncpy(dst_buf, t, sizeof(dst_buf) - 1);
    }

    printf("[Ethernet] Layer 2 not available (raw socket limitation)\n");

    unsigned char version = (ip->ihl_version >> 4) & 0x0F;
    unsigned char ihl = ip->ihl_version & 0x0F;

    printf("[IP]       SRC: %-15s DST: %-15s\n", src_buf, dst_buf);
    printf("           Protocol: %-6s TTL: %-3u Version: %u IHL: %u (%u bytes)\n",
        proto_name(ip->protocol), (unsigned)ip->ttl, version, ihl, ihl_bytes);
    printf("           Total Length: %u Header Checksum: 0x%04X\n",
        (unsigned)total_len, ntohs(ip->checksum));

    const unsigned char* l4 = buf + ihl_bytes;
    int l4_len = effective_len - ihl_bytes;
    if (l4_len <= 0) return;

    if (ip->protocol == 6) {
        if (l4_len < (int)sizeof(tcp_header_t)) return;
        const tcp_header_t* tcp = (const tcp_header_t*)l4;

        uint16_t sport = ntohs(tcp->src_port);
        uint16_t dport = ntohs(tcp->dst_port);

        uint16_t off_flags = ntohs(tcp->offset_flags);
        uint8_t data_offset = (uint8_t)((off_flags >> 12) & 0x0F) * 4;
        if (data_offset < 20) data_offset = 20;
        if (data_offset > (uint8_t)l4_len) return;

        uint16_t flags = off_flags & 0x01FF;

        printf("[TCP]      SRC PORT: %-5u DST PORT: %-5u\n", (unsigned)sport, (unsigned)dport);
        printf("           ");
        print_tcp_flags(flags);
        printf("\n");
        printf("           SEQ: %u ACK: %u Window: %u Header Len: %u bytes\n",
            ntohl(tcp->seq), ntohl(tcp->ack),
            ntohs(tcp->window), (unsigned)data_offset);
    }
    else if (ip->protocol == 17) {
        if (l4_len < (int)sizeof(udp_header_t)) return;
        const udp_header_t* udp = (const udp_header_t*)l4;

        uint16_t sport = ntohs(udp->src_port);
        uint16_t dport = ntohs(udp->dst_port);
        uint16_t ulen = ntohs(udp->length);
        if (ulen < sizeof(udp_header_t)) return;

        printf("[UDP]      SRC PORT: %-5u DST PORT: %-5u\n", (unsigned)sport, (unsigned)dport);
        printf("           Length: %u bytes Checksum: 0x%04X\n",
            (unsigned)ulen, ntohs(udp->checksum));
    }
    else if (ip->protocol == 1) {
        if (l4_len < (int)sizeof(icmp_header_t)) return;
        const icmp_header_t* icmp = (const icmp_header_t*)l4;

        const char* icmp_type_str = "Unknown";
        switch (icmp->type) {
        case 0: icmp_type_str = "Echo Reply"; break;
        case 8: icmp_type_str = "Echo Request"; break;
        case 3: icmp_type_str = "Destination Unreachable"; break;
        case 11: icmp_type_str = "Time Exceeded"; break;
        }

        printf("[ICMP]     Type: %u (%s) Code: %u Checksum: 0x%04X\n",
            (unsigned)icmp->type, icmp_type_str, (unsigned)icmp->code, ntohs(icmp->checksum));
    }
}

static void handle_packet(const unsigned char* buf, int len) {
    int ihl_bytes = 0;
    if (looks_like_ipv4(buf, len, &ihl_bytes)) {
        handle_ipv4(buf, len);
        printf("---------------------------------------------------------\n");
        return;
    }

    if (len < (int)sizeof(eth_header_t)) return;
    const eth_header_t* eth = (const eth_header_t*)buf;
    uint16_t etype = ntohs(eth->type);

    printf("[Ethernet] SRC MAC: ");
    print_mac(eth->src);
    printf(" DST MAC: ");
    print_mac(eth->dst);
    printf(" TYPE: 0x%04X (%s)\n", (unsigned)etype, ethertype_name(etype));

    if (etype == 0x0800) {
        handle_ipv4(buf + (int)sizeof(eth_header_t), len - (int)sizeof(eth_header_t));
    }
    else {
        printf("[IP]       <not IPv4 (EtherType 0x%04X)>\n", (unsigned)etype);
    }

    printf("---------------------------------------------------------\n");
}

static bool is_bad_ipv4_str(const char* ip) {
    if (!ip || !ip[0]) return true;
    if (strcmp(ip, "127.0.0.1") == 0) return true;
    if (strcmp(ip, "0.0.0.0") == 0)   return true;
    if (strncmp(ip, "169.254.", 8) == 0) return true;
    return false;
}

static bool add_candidate(char arr[][64], int* count, int max, const char* ip) {
    if (!ip || !count) return false;
    if (is_bad_ipv4_str(ip)) return false;

    for (int i = 0; i < *count; i++) {
        if (strcmp(arr[i], ip) == 0) return false;
    }
    if (*count >= max) return false;

    strncpy(arr[*count], ip, 63);
    arr[*count][63] = '\0';
    (*count)++;
    return true;
}

static bool detect_local_ipv4_by_route(char* out_ip, size_t out_sz) {
    if (!out_ip || out_sz == 0) return false;

    SOCKET tmp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (tmp == INVALID_SOCKET) return false;

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(53);

    if (inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr) != 1) {
        closesocket(tmp);
        return false;
    }

    if (connect(tmp, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR) {
        closesocket(tmp);
        return false;
    }

    struct sockaddr_in local;
    int local_len = sizeof(local);
    memset(&local, 0, sizeof(local));

    if (getsockname(tmp, (struct sockaddr*)&local, &local_len) == SOCKET_ERROR) {
        closesocket(tmp);
        return false;
    }

    closesocket(tmp);

    char buf[INET_ADDRSTRLEN] = { 0 };
    if (!inet_ntop(AF_INET, &local.sin_addr, buf, (socklen_t)sizeof(buf))) {
        return false;
    }
    if (is_bad_ipv4_str(buf)) return false;

    strncpy(out_ip, buf, out_sz - 1);
    out_ip[out_sz - 1] = '\0';
    return true;
}

static bool collect_local_ipv4_legacy(char arr[][64], int* count, int max) {
    if (!arr || !count) return false;

    char hostname[256] = { 0 };
    if (gethostname(hostname, (int)sizeof(hostname)) == SOCKET_ERROR) {
        return false;
    }

    struct hostent* he = gethostbyname(hostname);
    if (!he || !he->h_addr_list) {
        return false;
    }

    bool any = false;
    for (int i = 0; he->h_addr_list[i] != NULL; i++) {
        struct in_addr addr;
        memcpy(&addr, he->h_addr_list[i], sizeof(addr));

        const char* ip = inet_ntoa(addr);
        if (ip && add_candidate(arr, count, max, ip)) {
            any = true;
        }
    }
    return any;
}

static bool parse_ipv4_or_fail(const char* ip_str, struct in_addr* out) {
    if (!ip_str || !out) return false;
    if (inet_pton(AF_INET, ip_str, out) == 1) return true;
    return false;
}

static bool try_bind_raw(SOCKET s, const char* ip_str) {
    struct in_addr ip;
    if (!parse_ipv4_or_fail(ip_str, &ip)) return false;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr = ip;

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    enable_utf8_console();
    SetConsoleCtrlHandler(on_ctrl, TRUE);

    printf("==========================================================\n");
    printf("   Network Packet Sniffer - Lab #6\n");
    printf("   Using WinAPI, Winsock2, and Raw Sockets\n");
    printf("==========================================================\n\n");

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    char candidates[16][64];
    int cand_count = 0;

    if (argc >= 2 && argv[1] && argv[1][0]) {
        add_candidate(candidates, &cand_count, 16, argv[1]);
    }
    else {
        char route_ip[64] = { 0 };
        if (detect_local_ipv4_by_route(route_ip, sizeof(route_ip))) {
            add_candidate(candidates, &cand_count, 16, route_ip);
        }

        collect_local_ipv4_legacy(candidates, &cand_count, 16);

        if (cand_count == 0) {
            fprintf(stderr, "Failed to detect local IPv4. Pass it as argument: sniffer.exe <IP>\n");
            WSACleanup();
            return 1;
        }
    }

    printf("Detected local IPv4 candidates:\n");
    for (int i = 0; i < cand_count; i++) {
        printf("  [%d] %s\n", i + 1, candidates[i]);
    }
    printf("\n");

    SOCKET s = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
    if (s == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed. Run as Administrator. WSA=%d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    const char* selected_ip = NULL;
    for (int i = 0; i < cand_count; i++) {
        if (try_bind_raw(s, candidates[i])) {
            selected_ip = candidates[i];
            break;
        }
    }

    if (!selected_ip) {
        fprintf(stderr, "bind() failed for all detected IPs. Try passing correct IP manually.\n");
        closesocket(s);
        WSACleanup();
        return 1;
    }

    printf("Binding to interface: %s\n", selected_ip);

    DWORD flag = RCVALL_ON;
    DWORD bytesRet = 0;
    if (WSAIoctl(s, SIO_RCVALL, &flag, sizeof(flag), NULL, 0, &bytesRet, NULL, NULL) == SOCKET_ERROR) {
        fprintf(stderr, "WSAIoctl(SIO_RCVALL) failed. Run as Administrator. WSA=%d\n", WSAGetLastError());
        closesocket(s);
        WSACleanup();
        return 1;
    }

    printf("\n[*] Promiscuous mode enabled\n");
    printf("[*] Starting packet capture... (Press Ctrl+C to stop)\n");
    printf("==========================================================\n\n");

    unsigned char buffer[65536];
    unsigned long packet_count = 0;

    while (InterlockedCompareExchange(&g_running, 1, 1)) {
        int bytes = recv(s, (char*)buffer, (int)sizeof(buffer), 0);
        if (bytes == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAEINTR) break;
            fprintf(stderr, "recv() error: %d\n", err);
            break;
        }
        if (bytes > 0) {
            packet_count++;
            printf("[Packet #%lu] Size: %d bytes\n", packet_count, bytes);
            handle_packet(buffer, bytes);
        }
    }

    flag = RCVALL_OFF;
    WSAIoctl(s, SIO_RCVALL, &flag, sizeof(flag), NULL, 0, &bytesRet, NULL, NULL);

    closesocket(s);
    WSACleanup();

    printf("\n==========================================================\n");
    printf("Capture stopped. Total packets captured: %lu\n", packet_count);
    printf("==========================================================\n");
    return 0;
}