/*
 * qcom-band-menu-release.c - Release Qualcomm QRTR RAT and LTE/NR control menu.
 *
 * Dynamically discovers QRTR service 3, instance 1, then sends the
 * experimentally observed command 0x33 requests used by NSG.
 *
 * Running without arguments opens an interactive menu.
 *
 * Supported operations:
 *   qcom-band-menu rat auto
 *   qcom-band-menu rat lte
 *   qcom-band-menu rat nr
 *   qcom-band-menu rat lte nr
 *
 *   qcom-band-menu lte-bands 1 3 8 28
 *
 *   qcom-band-menu nr-bands --lte 1 3 8 28 --nr 1 28 41 78
 *
 *   qcom-band-menu nr-mode both --lte 1 3 8 28 --nr 1 28 41 78
 *   qcom-band-menu nr-mode nsa  --lte 1 3 8 28 --nr 78
 *   qcom-band-menu nr-mode sa   --lte 1 3 8 28 --nr 78
 *
 * Notes:
 * - LTE band N maps to bit N-1 in an 8-byte little-endian bitmap.
 * - NR band nN maps to bit N-1 in a 64-byte little-endian bitmap.
 * - The observed NR-band request also carries the current LTE bitmap,
 *   therefore --lte and --nr are both required for nr-bands/nr-mode.
 * - GSM and WCDMA band locking are intentionally not implemented.
 *
 * Freestanding ARM64 static build:
 *   clang --target=aarch64-linux-gnu -fuse-ld=lld -O2 -nostdlib -static \
 *     -fno-stack-protector -fno-builtin -Wl,-e,_start \
 *     -Wl,--build-id=none -o qcom-band-menu-release qcom-band-menu-release.c
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef long           s64;

enum {
    SYS_close         = 57,
    SYS_read          = 63,
    SYS_write         = 64,
    SYS_exit          = 93,
    SYS_clock_gettime = 113,
    SYS_socket        = 198,
    SYS_connect       = 203,
    SYS_sendto        = 206,
    SYS_recvfrom      = 207,
    SYS_setsockopt    = 208,
};

enum {
    AF_QIPCRTR = 42,
    SOCK_DGRAM = 2,
    SOL_SOCKET = 1,
    SO_RCVTIMEO = 20,
    CLOCK_MONOTONIC = 1,
};

#define QRTR_CTRL_NODE 1u
#define QRTR_PORT_CTRL 0xFFFFFFFEu
#define RAT_SERVICE 3u
#define RAT_INSTANCE 1u

struct sockaddr_qrtr {
    u16 sq_family;
    u16 __pad;
    u32 sq_node;
    u32 sq_port;
};

struct qrtr_ctrl_pkt {
    u32 command;
    u32 service;
    u32 instance;
    u32 node;
    u32 port;
};

struct timeval64 { s64 tv_sec; s64 tv_usec; };
struct timespec64 { s64 tv_sec; s64 tv_nsec; };

enum {
    QRTR_TYPE_NEW_LOOKUP = 0x0A,
    QRTR_TYPE_NEW_SERVER = 0x04,
};

static inline s64 syscall1(s64 n, s64 a0) {
    register s64 x8 __asm__("x8") = n;
    register s64 x0 __asm__("x0") = a0;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;
}
static inline s64 syscall2(s64 n, s64 a0, s64 a1) {
    register s64 x8 __asm__("x8") = n;
    register s64 x0 __asm__("x0") = a0;
    register s64 x1 __asm__("x1") = a1;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x1), "r"(x8) : "memory");
    return x0;
}
static inline s64 syscall3(s64 n, s64 a0, s64 a1, s64 a2) {
    register s64 x8 __asm__("x8") = n;
    register s64 x0 __asm__("x0") = a0;
    register s64 x1 __asm__("x1") = a1;
    register s64 x2 __asm__("x2") = a2;
    __asm__ volatile("svc 0" : "+r"(x0) : "r"(x1), "r"(x2), "r"(x8) : "memory");
    return x0;
}
static inline s64 syscall5(s64 n, s64 a0, s64 a1, s64 a2, s64 a3, s64 a4) {
    register s64 x8 __asm__("x8") = n;
    register s64 x0 __asm__("x0") = a0;
    register s64 x1 __asm__("x1") = a1;
    register s64 x2 __asm__("x2") = a2;
    register s64 x3 __asm__("x3") = a3;
    register s64 x4 __asm__("x4") = a4;
    __asm__ volatile("svc 0" : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x8)
                     : "memory");
    return x0;
}
static inline s64 syscall6(s64 n, s64 a0, s64 a1, s64 a2, s64 a3, s64 a4, s64 a5) {
    register s64 x8 __asm__("x8") = n;
    register s64 x0 __asm__("x0") = a0;
    register s64 x1 __asm__("x1") = a1;
    register s64 x2 __asm__("x2") = a2;
    register s64 x3 __asm__("x3") = a3;
    register s64 x4 __asm__("x4") = a4;
    register s64 x5 __asm__("x5") = a5;
    __asm__ volatile("svc 0" : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
                     : "memory");
    return x0;
}

static u64 c_strlen(const char *s) { u64 n = 0; while (s[n]) n++; return n; }
static int c_streq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}
static void mem_zero(u8 *p, u64 n) { while (n--) *p++ = 0; }
static void out(const char *s) { (void)syscall3(SYS_write, 1, (s64)s, (s64)c_strlen(s)); }
static void out_hex8(u8 v) {
    static const char h[] = "0123456789ABCDEF";
    char b[2]; b[0] = h[(v >> 4) & 15]; b[1] = h[v & 15];
    (void)syscall3(SYS_write, 1, (s64)b, 2);
}
static void out_hex16(u16 v) { out_hex8((u8)(v >> 8)); out_hex8((u8)v); }
static void out_hex32(u32 v) { out_hex16((u16)(v >> 16)); out_hex16((u16)v); }
static void dump_bytes(const char *label, const u8 *p, u64 n) {
    u64 i; out(label);
    for (i = 0; i < n; i++) { out_hex8(p[i]); out(i + 1 == n ? "\n" : " "); }
}
static int parse_uint(const char *s, u32 *value) {
    u32 v = 0; int digits = 0;
    if (!s || !*s) return 0;
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        if (v > 100000u) return 0;
        v = v * 10u + (u32)(*s - '0');
        s++; digits++;
    }
    if (!digits) return 0;
    *value = v; return 1;
}
static void print_errno(const char *where, s64 result) {
    u32 e = (u32)(-result);
    out(where); out(" failed, errno=0x"); out_hex32(e);
    if (e == 11) out(" (EAGAIN: timeout / no reply)");
    else if (e == 13) out(" (EACCES)");
    else if (e == 19) out(" (ENODEV)");
    else if (e == 97) out(" (EAFNOSUPPORT)");
    out("\n");
}
static u16 make_txid(void) {
    struct timespec64 ts;
    s64 r = syscall2(SYS_clock_gettime, CLOCK_MONOTONIC, (s64)&ts);
    if (r < 0) return 1;
    return (u16)((u64)ts.tv_nsec ^ (u64)ts.tv_sec ^ ((u64)ts.tv_nsec >> 16));
}

static int discover_service(u32 *node_out, u32 *port_out) {
    struct qrtr_ctrl_pkt req, rsp;
    struct sockaddr_qrtr addr;
    struct timeval64 timeout;
    s64 fd, r; int attempt;

    fd = syscall3(SYS_socket, AF_QIPCRTR, SOCK_DGRAM, 0);
    if (fd < 0) { print_errno("discovery socket", fd); return -1; }
    timeout.tv_sec = 2; timeout.tv_usec = 0;
    r = syscall5(SYS_setsockopt, fd, SOL_SOCKET, SO_RCVTIMEO,
                 (s64)&timeout, (s64)sizeof(timeout));
    if (r < 0) { print_errno("discovery setsockopt", r); syscall1(SYS_close, fd); return -2; }

    addr.sq_family = AF_QIPCRTR; addr.__pad = 0;
    addr.sq_node = QRTR_CTRL_NODE; addr.sq_port = QRTR_PORT_CTRL;
    req.command = QRTR_TYPE_NEW_LOOKUP; req.service = RAT_SERVICE;
    req.instance = RAT_INSTANCE; req.node = 0; req.port = 0;

    out("Discovering QRTR service 3, instance 1...\n");
    for (attempt = 0; attempt < 3; attempt++) {
        r = syscall6(SYS_sendto, fd, (s64)&req, sizeof(req), 0,
                     (s64)&addr, sizeof(addr));
        if (r != (s64)sizeof(req)) continue;
        for (;;) {
            r = syscall6(SYS_recvfrom, fd, (s64)&rsp, sizeof(rsp), 0, 0, 0);
            if (r < 0) break;
            if (r != (s64)sizeof(rsp)) continue;
            if (rsp.command != QRTR_TYPE_NEW_SERVER) continue;
            if (rsp.service != RAT_SERVICE || rsp.instance != RAT_INSTANCE) continue;
            if (!rsp.port) continue;
            *node_out = rsp.node; *port_out = rsp.port;
            syscall1(SYS_close, fd); return 0;
        }
    }
    syscall1(SYS_close, fd);
    out("Could not discover QRTR service 3, instance 1.\n");
    return -3;
}

static int send_command(u8 *request, u64 request_len) {
    u8 response[128];
    struct sockaddr_qrtr addr;
    struct timeval64 timeout;
    u32 node, port;
    u16 txid;
    s64 fd, r;

    if (discover_service(&node, &port) != 0) return 13;
    out("Found endpoint: node=0x"); out_hex32(node);
    out(" port=0x"); out_hex32(port); out("\n");

    fd = syscall3(SYS_socket, AF_QIPCRTR, SOCK_DGRAM, 0);
    if (fd < 0) { print_errno("socket", fd); return 3; }
    timeout.tv_sec = 3; timeout.tv_usec = 0;
    r = syscall5(SYS_setsockopt, fd, SOL_SOCKET, SO_RCVTIMEO,
                 (s64)&timeout, sizeof(timeout));
    if (r < 0) { print_errno("setsockopt", r); syscall1(SYS_close, fd); return 4; }

    addr.sq_family = AF_QIPCRTR; addr.__pad = 0;
    addr.sq_node = node; addr.sq_port = port;
    r = syscall3(SYS_connect, fd, (s64)&addr, sizeof(addr));
    if (r < 0) { print_errno("connect", r); syscall1(SYS_close, fd); return 5; }

    txid = make_txid(); if (!txid) txid = 1;
    request[1] = (u8)txid; request[2] = (u8)(txid >> 8);

    r = syscall6(SYS_sendto, fd, (s64)request, request_len, 0, 0, 0);
    if (r < 0) { print_errno("sendto", r); syscall1(SYS_close, fd); return 6; }
    if (r != (s64)request_len) { out("sendto returned a short length\n"); syscall1(SYS_close, fd); return 7; }

    r = syscall6(SYS_recvfrom, fd, (s64)response, sizeof(response), 0, 0, 0);
    if (r < 0) { print_errno("recvfrom", r); syscall1(SYS_close, fd); return 8; }
    syscall1(SYS_close, fd);

    if (r < 14) { out("Failure: short modem reply.\n"); return 9; }
    if (response[0] != 0x02 || response[1] != request[1] ||
        response[2] != request[2] || response[3] != 0x33) {
        out("Failure: response header or transaction ID mismatch.\n"); return 10;
    }
    if (response[7] == 0x02 && response[8] == 0x04 && response[9] == 0x00) {
        u16 result = (u16)response[10] | ((u16)response[11] << 8);
        u16 error = (u16)response[12] | ((u16)response[13] << 8);
        if (!result) { out("Success! Command accepted.\n"); return 0; }
        out("Failure: modem result=0x"); out_hex16(result);
        out(" error=0x"); out_hex16(error); out("\n"); return 11;
    }
    out("Failure: expected result TLV not found.\n"); return 12;
}

static int add_lte_band(u8 mask[8], const char *s) {
    u32 band, bit;
    if (!parse_uint(s, &band) || band < 1 || band > 64) return 0;
    bit = band - 1; mask[bit / 8] |= (u8)(1u << (bit % 8)); return 1;
}
static int add_nr_band(u8 mask[64], const char *s) {
    u32 band, bit;
    if (!parse_uint(s, &band) || band < 1 || band > 512) return 0;
    bit = band - 1; mask[bit / 8] |= (u8)(1u << (bit % 8)); return 1;
}
static int bitmap_nonzero(const u8 *p, u64 n) {
    while (n--) if (*p++) return 1;
    return 0;
}

static int do_rat(int argc, char **argv) {
    u8 req[16] = {0x00,0,0,0x33,0x00,0x09,0x00,0x17,0x01,0x00,0x01,0x11,0x02,0x00,0,0};
    u8 mask = 0; int i;
    if (argc < 3) return -1;
    if (argc == 3 && c_streq(argv[2], "auto")) mask = 0xFF;
    else {
        for (i = 2; i < argc; i++) {
            if (c_streq(argv[i], "lte")) mask |= 0x10;
            else if (c_streq(argv[i], "nr") || c_streq(argv[i], "nr5g")) mask |= 0x40;
            else return -1;
        }
    }
    req[14] = mask;
    return send_command(req, sizeof(req));
}

static int do_lte_bands(int argc, char **argv) {
    u8 req[22]; u8 mask[8]; int i;
    if (argc < 3) return -1;
    mem_zero(req, sizeof(req)); mem_zero(mask, sizeof(mask));
    for (i = 2; i < argc; i++) if (!add_lte_band(mask, argv[i])) return -1;
    if (!bitmap_nonzero(mask, 8)) return -1;

    req[0]=0x00; req[3]=0x33; req[4]=0x00; req[5]=0x0F; req[6]=0x00;
    req[7]=0x17; req[8]=0x01; req[9]=0x00; req[10]=0x01;
    req[11]=0x15; req[12]=0x08; req[13]=0x00;
    for (i=0;i<8;i++) req[14+i]=mask[i];
    return send_command(req, sizeof(req));
}

static int parse_nr_lists(int argc, char **argv, int start,
                          u8 lte[8], u8 nr[64]) {
    int mode = 0, i;
    for (i = start; i < argc; i++) {
        if (c_streq(argv[i], "--lte")) { mode = 1; continue; }
        if (c_streq(argv[i], "--nr")) { mode = 2; continue; }
        if (mode == 1) { if (!add_lte_band(lte, argv[i])) return 0; }
        else if (mode == 2) { if (!add_nr_band(nr, argv[i])) return 0; }
        else return 0;
    }
    return bitmap_nonzero(lte, 8) && bitmap_nonzero(nr, 64);
}

static int do_nr_bands_or_mode(int argc, char **argv, int include_mode) {
    u8 req[96], lte[8], nr[64];
    int i, pos = 0, start = 2;
    u32 mode_value = 0;

    mem_zero(req, sizeof(req)); mem_zero(lte, sizeof(lte)); mem_zero(nr, sizeof(nr));
    if (include_mode) {
        if (argc < 5) return -1;
        if (c_streq(argv[2], "both")) mode_value = 0;
        else if (c_streq(argv[2], "nsa")) mode_value = 1;
        else if (c_streq(argv[2], "sa")) mode_value = 2;
        else return -1;
        start = 3;
    } else if (argc < 5) return -1;

    if (!parse_nr_lists(argc, argv, start, lte, nr)) return -1;

    req[pos++]=0x00; req[pos++]=0; req[pos++]=0; req[pos++]=0x33;
    req[pos++]=0x00; req[pos++]=(u8)(include_mode ? 0x59 : 0x52); req[pos++]=0x00;
    req[pos++]=0x17; req[pos++]=0x01; req[pos++]=0x00; req[pos++]=0x01;
    req[pos++]=0x15; req[pos++]=0x08; req[pos++]=0x00;
    for (i=0;i<8;i++) req[pos++]=lte[i];
    req[pos++]=0x2B; req[pos++]=0x40; req[pos++]=0x00;
    for (i=0;i<64;i++) req[pos++]=nr[i];
    if (include_mode) {
        req[pos++]=0x2E; req[pos++]=0x04; req[pos++]=0x00;
        req[pos++]=(u8)mode_value; req[pos++]=0; req[pos++]=0; req[pos++]=0;
    }

    return send_command(req, (u64)pos);
}



static void out_u32(u32 v) {
    char b[11]; int i = 0, j;
    if (v == 0) { out("0"); return; }
    while (v && i < 10) { b[i++] = (char)('0' + (v % 10)); v /= 10; }
    for (j = i - 1; j >= 0; j--) (void)syscall3(SYS_write, 1, (s64)&b[j], 1);
}

static s64 read_line(char *buf, u64 cap) {
    s64 n; u64 i;
    if (cap < 2) return -1;
    n = syscall3(SYS_read, 0, (s64)buf, (s64)(cap - 1));
    if (n <= 0) return n;
    buf[n] = 0;
    for (i = 0; i < (u64)n; i++) {
        if (buf[i] == '\r' || buf[i] == '\n') { buf[i] = 0; break; }
    }
    return n;
}

static int is_sep(char c) { return c == ',' || c == ' ' || c == '\t'; }

static int parse_band_line(char *line, u8 *mask, u64 mask_len, int is_nr) {
    char *p = line, *start;
    u32 value; int count = 0;
    mem_zero(mask, mask_len);
    while (*p) {
        while (*p && is_sep(*p)) p++;
        if (!*p) break;
        start = p;
        while (*p && !is_sep(*p)) p++;
        if (*p) *p++ = 0;
        if (!parse_uint(start, &value)) return 0;
        if (is_nr) {
            if (value < 1 || value > 512) return 0;
            mask[(value - 1) / 8] |= (u8)(1u << ((value - 1) % 8));
        } else {
            if (value < 1 || value > 64) return 0;
            mask[(value - 1) / 8] |= (u8)(1u << ((value - 1) % 8));
        }
        count++;
    }
    return count > 0;
}

static void print_band_list(const u8 *mask, u32 max_band, const char *prefix) {
    u32 band; int first = 1;
    for (band = 1; band <= max_band; band++) {
        u32 bit = band - 1;
        if (mask[bit / 8] & (u8)(1u << (bit % 8))) {
            if (!first) out(",");
            out(prefix); out_u32(band); first = 0;
        }
    }
    if (first) out("Not set this session");
    out("\n");
}

static int send_lte_mask(const u8 mask[8]) {
    u8 req[22]; int i;
    mem_zero(req, sizeof(req));
    req[0]=0x00; req[3]=0x33; req[4]=0x00; req[5]=0x0F; req[6]=0x00;
    req[7]=0x17; req[8]=0x01; req[9]=0x00; req[10]=0x01;
    req[11]=0x15; req[12]=0x08; req[13]=0x00;
    for (i=0;i<8;i++) req[14+i]=mask[i];
    return send_command(req, sizeof(req));
}

static int send_nr_config(const u8 lte[8], const u8 nr[64], int include_mode, u32 mode_value) {
    u8 req[96]; int i, pos = 0;
    mem_zero(req, sizeof(req));
    req[pos++]=0x00; req[pos++]=0; req[pos++]=0; req[pos++]=0x33;
    req[pos++]=0x00; req[pos++]=(u8)(include_mode ? 0x59 : 0x52); req[pos++]=0x00;
    req[pos++]=0x17; req[pos++]=0x01; req[pos++]=0x00; req[pos++]=0x01;
    req[pos++]=0x15; req[pos++]=0x08; req[pos++]=0x00;
    for (i=0;i<8;i++) req[pos++]=lte[i];
    req[pos++]=0x2B; req[pos++]=0x40; req[pos++]=0x00;
    for (i=0;i<64;i++) req[pos++]=nr[i];
    if (include_mode) {
        req[pos++]=0x2E; req[pos++]=0x04; req[pos++]=0x00;
        req[pos++]=(u8)mode_value; req[pos++]=0; req[pos++]=0; req[pos++]=0;
    }
    return send_command(req, (u64)pos);
}

static int interactive_menu(void) {
    u8 lte[8], nr[64], tmp_lte[8], tmp_nr[64];
    int have_lte = 0, have_nr = 0;
    u32 mode = 0;
    char line[512];
    mem_zero(lte, sizeof(lte)); mem_zero(nr, sizeof(nr));

    for (;;) {
        out("\n========================================\n");
        out(" Qualcomm Forcings Menu\n");
        out("========================================\n");
        out("(1) Select RAT Mode\n");
        out("(2) Select LTE Band(s)\n");
        out("(3) Select NR Band(s)\n");
        out("(4) Select NR Mode\n");
        out("(5) Exit\n");
        out("Select: ");
        if (read_line(line, sizeof(line)) <= 0) return 0;

        if (c_streq(line, "1")) {
            char *args_auto[] = {"qcom-band-menu", "rat", "auto"};
            char *args_lte[]  = {"qcom-band-menu", "rat", "lte"};
            char *args_nr[]   = {"qcom-band-menu", "rat", "nr"};
            char *args_both[] = {"qcom-band-menu", "rat", "lte", "nr"};
            out("RAT mode:\n");
            out("(1) AUTO\n");
            out("(2) LTE only\n");
            out("(3) NR only\n");
            out("(4) LTE + NR\n");
            out("Select: ");
            if (read_line(line, sizeof(line)) <= 0) continue;
            if (c_streq(line, "1")) (void)do_rat(3, args_auto);
            else if (c_streq(line, "2")) (void)do_rat(3, args_lte);
            else if (c_streq(line, "3")) (void)do_rat(3, args_nr);
            else if (c_streq(line, "4")) (void)do_rat(4, args_both);
            else out("Invalid RAT selection.\n");
        } else if (c_streq(line, "2")) {
            out("Enter LTE bands separated by commas (example: 1,3,8,28): ");
            if (read_line(line, sizeof(line)) <= 0) continue;
            if (!parse_band_line(line, tmp_lte, 8, 0)) {
                out("Invalid LTE list. Valid range is B1-B64.\n"); continue;
            }
            if (send_lte_mask(tmp_lte) == 0) {
                int i; for (i=0;i<8;i++) lte[i]=tmp_lte[i]; have_lte=1;
            }
        } else if (c_streq(line, "3")) {
            if (!have_lte) {
                out("Select LTE bands first; the observed NR command includes the LTE bitmap.\n");
                continue;
            }
            out("Enter NR bands separated by commas (example: 1,28,41,78): ");
            if (read_line(line, sizeof(line)) <= 0) continue;
            if (!parse_band_line(line, tmp_nr, 64, 1)) {
                out("Invalid NR list. Valid range is n1-n512.\n"); continue;
            }
            if (send_nr_config(lte, tmp_nr, 0, 0) == 0) {
                int i; for (i=0;i<64;i++) nr[i]=tmp_nr[i]; have_nr=1;
            }
        } else if (c_streq(line, "4")) {
            if (!have_lte || !have_nr) {
                out("Select LTE and NR bands first; the captured NR-mode command includes both masks.\n");
                continue;
            }
            out("NR mode:\n");
            out("(1) NSA + SA\n");
            out("(2) NSA only\n");
            out("(3) SA only\n");
            out("Select: ");
            if (read_line(line, sizeof(line)) <= 0) continue;
            if (c_streq(line, "1")) mode = 0;
            else if (c_streq(line, "2")) mode = 1;
            else if (c_streq(line, "3")) mode = 2;
            else { out("Invalid mode selection.\n"); continue; }
            (void)send_nr_config(lte, nr, 1, mode);
        } else if (c_streq(line, "5") || c_streq(line, "q") || c_streq(line, "quit")) {
            out("Exiting.\n"); return 0;
        } else {
            out("Invalid menu selection.\n");
        }
    }
}

static void usage(void) {
    out("Usage:\n");
    out("  qcom-band-menu rat auto|lte|nr [lte nr]\n");
    out("  qcom-band-menu lte-bands BAND [BAND ...]\n");
    out("  qcom-band-menu nr-bands --lte BAND... --nr BAND...\n");
    out("  qcom-band-menu nr-mode both|nsa|sa --lte BAND... --nr BAND...\n");
    out("Examples:\n");
    out("  qcom-band-menu lte-bands 1 3 8 28\n");
    out("  qcom-band-menu nr-bands --lte 1 3 8 28 --nr 1 28 41 78\n");
    out("  qcom-band-menu nr-mode sa --lte 1 3 8 28 --nr 78\n");
}

static int main_impl(int argc, char **argv) {
    int rc;
    if (argc < 2) return interactive_menu();
    if (c_streq(argv[1], "rat")) rc = do_rat(argc, argv);
    else if (c_streq(argv[1], "lte-bands")) rc = do_lte_bands(argc, argv);
    else if (c_streq(argv[1], "nr-bands")) rc = do_nr_bands_or_mode(argc, argv, 0);
    else if (c_streq(argv[1], "nr-mode")) rc = do_nr_bands_or_mode(argc, argv, 1);
    else rc = -1;
    if (rc < 0) { usage(); return 2; }
    return rc;
}

void c_start(u64 *stack) {
    int argc = (int)stack[0];
    char **argv = (char **)&stack[1];
    int rc = main_impl(argc, argv);
    syscall1(SYS_exit, rc);
    for (;;) { }
}

__asm__(
    ".global _start\n"
    ".type _start, %function\n"
    "_start:\n"
    "mov x0, sp\n"
    "bl c_start\n"
    "b .\n"
);
