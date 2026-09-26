// owen.c — backend redirect hook (port of Owen.c by abdallhrgx).
//
// Hook the engine's FCurlHttpRequest::ProcessRequest so every Epic services
// request (login, matchmaking, cloudstorage, ...) is redirected to our own
// backend (Voltronite on :3551). Version offsets are BAKED at build time by
// scripts/analyze_libue4.py — no runtime memory scanning.
//
// The embedded ARM64 Dobby reimplementation (trampoline + reloc) is kept
// verbatim from the original Owen.c.
#define _GNU_SOURCE
#include <android/log.h>
#define OWEN_LOG_TAG "Erbium-Owen"
#define LOGOW(...) __android_log_print(ANDROID_LOG_INFO, OWEN_LOG_TAG, __VA_ARGS__)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static char g_backend_url[4096] = "http://127.0.0.1:3551";

typedef uint16_t TCHAR;

struct FString {
    TCHAR*  data;
    int32_t length;
    int32_t capacity;
};

static char* FString_ToCStr(const struct FString* str) {
    if (!str || !str->data || str->length <= 0) return NULL;
    int maxBytes = (str->length * 3) + 1;
    char* out = (char*)malloc(maxBytes);
    if (!out) return NULL;
    int o = 0;
    int i;
    for (i = 0; i < str->length - 1; i++) {
        uint16_t c = str->data[i];
        if (c <= 0x7F) { out[o++] = (char)c; }
        else if (c <= 0x7FF) { out[o++] = (char)(0xC0 | (c >> 6)); out[o++] = (char)(0x80 | (c & 0x3F)); }
        else { out[o++] = (char)(0xE0 | (c >> 12)); out[o++] = (char)(0x80 | ((c >> 6) & 0x3F)); out[o++] = (char)(0x80 | (c & 0x3F)); }
    }
    out[o] = 0;
    return out;
}

static struct FString FString_FromCStr(const char* str) {
    struct FString fs;
    memset(&fs, 0, sizeof(fs));
    if (!str) return fs;
    int utf16len = 0;
    const unsigned char* s = (const unsigned char*)str;
    while (*s) {
        if ((*s & 0x80) == 0) { s += 1; utf16len += 1; }
        else if ((*s & 0xE0) == 0xC0) { s += 2; utf16len += 1; }
        else if ((*s & 0xF0) == 0xE0) { s += 3; utf16len += 1; }
        else { s += 4; utf16len += 1; }
    }
    fs.length = fs.capacity = utf16len + 1;
    fs.data = (TCHAR*)malloc(fs.capacity * sizeof(TCHAR));
    if (!fs.data) { fs.length = fs.capacity = 0; return fs; }
    int i = 0;
    s = (const unsigned char*)str;
    while (*s) {
        if ((*s & 0x80) == 0) { fs.data[i++] = *s++; }
        else if ((*s & 0xE0) == 0xC0) { fs.data[i++] = (uint16_t)(((s[0] & 0x1F) << 6) | (s[1] & 0x3F)); s += 2; }
        else if ((*s & 0xF0) == 0xE0) { fs.data[i++] = (uint16_t)(((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F)); s += 3; }
        else { fs.data[i++] = '?'; s += 4; }
    }
    fs.data[i] = 0;
    return fs;
}

static void FString_Free(struct FString* str) {
    if (!str) return;
    if (str->data) free(str->data);
    str->data = NULL;
    str->length = 0;
    str->capacity = 0;
}

#define URL_HOST_MAX 1024
#define URL_PATH_MAX 4096
#define URL_TOTAL_MAX (URL_HOST_MAX + URL_PATH_MAX)

struct Url {
    char host[URL_HOST_MAX];
    char pathAndQuery[URL_PATH_MAX];
};

static void Url_ParseUrl(const char* url, struct Url* out) {
    if (!url || !out) return;
    out->host[0] = '\0';
    out->pathAndQuery[0] = '\0';
    if (url[0] == '\0') return;
    const char* proto_sep = "://";
    const char* remainder = NULL;
    const char* path_start = NULL;
    const char* proto_end = strstr(url, proto_sep);
    if (proto_end) {
        size_t host_len = proto_end - url + 3;
        if (host_len >= URL_HOST_MAX) host_len = URL_HOST_MAX - 1;
        strncpy(out->host, url, host_len);
        out->host[host_len] = '\0';
        remainder = proto_end + 3;
        path_start = strchr(remainder, '/');
        if (path_start) {
            size_t domain_len = path_start - remainder;
            if (strlen(out->host) + domain_len < URL_HOST_MAX) strncat(out->host, remainder, domain_len);
            size_t path_len = strlen(path_start);
            if (path_len >= URL_PATH_MAX) path_len = URL_PATH_MAX - 1;
            strncpy(out->pathAndQuery, path_start, path_len);
            out->pathAndQuery[path_len] = '\0';
        } else {
            if (strlen(out->host) + strlen(remainder) < URL_HOST_MAX) strcat(out->host, remainder);
        }
    } else {
        path_start = strchr(url, '/');
        if (path_start) {
            size_t domain_len = path_start - url;
            if (domain_len >= URL_HOST_MAX) domain_len = URL_HOST_MAX - 1;
            strncpy(out->host, url, domain_len);
            out->host[domain_len] = '\0';
            size_t path_len = strlen(path_start);
            if (path_len >= URL_PATH_MAX) path_len = URL_PATH_MAX - 1;
            strncpy(out->pathAndQuery, path_start, path_len);
            out->pathAndQuery[path_len] = '\0';
        } else {
            strncpy(out->host, url, URL_HOST_MAX - 1);
            out->host[URL_HOST_MAX - 1] = '\0';
        }
    }
}

static int ShouldRedirect(const char* host) {
    char stripped[URL_HOST_MAX];
    strncpy(stripped, host, sizeof(stripped) - 1);
    stripped[sizeof(stripped) - 1] = '\0';
    if (strncmp(stripped, "http://", 7) == 0) memmove(stripped, stripped + 7, strlen(stripped + 7) + 1);
    else if (strncmp(stripped, "https://", 8) == 0) memmove(stripped, stripped + 8, strlen(stripped + 8) + 1);
    char* colon = strchr(stripped, ':');
    if (colon) *colon = '\0';
    const char* domains[] = {
        "game-social.epicgames.com",
        "ol.epicgames.com",
        "ol.epicgames.net",
        "on.epicgames.com",
        "ak.epicgames.com",
        "epicgames.dev"
    };
    int num_domains = sizeof(domains) / sizeof(domains[0]);
    int i;
    for (i = 0; i < num_domains; i++) {
        size_t dlen = strlen(domains[i]);
        size_t slen = strlen(stripped);
        if (slen >= dlen && strcmp(stripped + slen - dlen, domains[i]) == 0) return 1;
    }
    return 0;
}

static void CreateUrl(const char* host, const char* pathAndQuery, char* out, size_t out_size) {
    if (!out || out_size == 0) return;
    size_t host_len = host ? strlen(host) : 0;
    size_t path_len = pathAndQuery ? strlen(pathAndQuery) : 0;
    size_t total = host_len + path_len;
    if (total >= out_size) total = out_size - 1;
    if (host_len > 0) memcpy(out, host, host_len > total ? total : host_len);
    if (path_len > 0 && total > host_len) memcpy(out + host_len, pathAndQuery, total - host_len);
    out[total] = '\0';
}

static void url_encode(const char* src, char* dst, size_t dst_size) {
    if (!dst || dst_size == 0) return;
    const char* hex = "0123456789ABCDEF";
    size_t i = 0, j = 0;
    while (src[i] && j + 3 < dst_size) {
        unsigned char c = src[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            dst[j++] = c;
        } else {
            dst[j++] = '%';
            dst[j++] = hex[c >> 4];
            dst[j++] = hex[c & 15];
        }
        i++;
    }
    dst[j] = '\0';
}

static void send_version_report(const char* version) {
    struct Url url;
    Url_ParseUrl(g_backend_url, &url);
    char host[256];
    strncpy(host, url.host, sizeof(host)-1);
    if (strncmp(host, "http://", 7) == 0) memmove(host, host+7, strlen(host+7)+1);
    else if (strncmp(host, "https://", 8) == 0) memmove(host, host+8, strlen(host+8)+1);
    char* port_str = "80";
    char* colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        port_str = colon+1;
    }
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err) return;
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) { freeaddrinfo(res); return; }
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) { close(sock); freeaddrinfo(res); return; }
    char body[512];
    snprintf(body, sizeof(body), "GameVersion = \"%s\"", version);
    char request[1024];
    snprintf(request, sizeof(request),
        "POST / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s", host, strlen(body), body);
    send(sock, request, strlen(request), 0);
    close(sock);
    freeaddrinfo(res);
}

typedef uintptr_t addr_t;
typedef uint32_t addr32_t;

static const char* detected_version = NULL;
static uint32_t ue_pr_offset = 0;
static uint16_t ue_geturl_off = 0;
static uint32_t ue_seturl_off = 0;
static uint32_t eos_pr_offset = 0;
static uint16_t eos_geturl_off = 0;
static uint32_t eos_seturl_off = 0;
typedef uint64_t addr64_t;
typedef void *asm_func_t;

#define ARM64_TMP_REG_NDX_0 17

typedef union _FPReg {
  __int128_t q;
  struct { double d1; double d2; } d;
  struct { float f1; float f2; float f3; float f4; } f;
} FPReg;

typedef struct {
  uint64_t dmmpy_0;
  uint64_t sp;
  uint64_t dmmpy_1;
  union {
    uint64_t x[29];
    struct { uint64_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15, x16, x17, x18, x19, x20, x21, x22, x23, x24, x25, x26, x27, x28; } regs;
  } general;
  uint64_t fp;
  uint64_t lr;
  union {
    FPReg q[32];
    struct { FPReg q0, q1, q2, q3, q4, q5, q6, q7; FPReg q8, q9, q10, q11, q12, q13, q14, q15, q16, q17, q18, q19, q20, q21, q22, q23, q24, q25, q26, q27, q28, q29, q30, q31; } regs;
  } floating;
} DobbyRegisterContext;

typedef void (*dobby_instrument_callback_t)(void *address, DobbyRegisterContext *ctx);
typedef addr_t (*dobby_alloc_near_code_callback_t)(uint32_t size, addr_t pos, size_t range);

#define ALIGN_FLOOR(address, range) ((uintptr_t)(address) & ~((uintptr_t)(range) - 1))
#define ALIGN_CEIL(address, range) (((uintptr_t)(address) + (uintptr_t)(range) - 1) & ~((uintptr_t)(range) - 1))
#define ALIGN(address, range) ALIGN_FLOOR(address, range)

#define LeftShift(a, b, c) (((a) & ((1 << (b)) - 1)) << (c))
#define RightShift(a, b, c) (((a) >> (c)) & ((1 << (b)) - 1))

#define submask(x) ((1L << ((x) + 1)) - 1)
#define bits_(obj, st, fn) (((obj) >> (st)) & submask((fn) - (st)))
#define bit_(obj, st) (((obj) >> (st)) & 1)

#define set_bit_(obj, st, b) (obj) = ((~(1 << (st))) & (obj)) | ((b) << (st))
#define set_bits_(obj, st, fn, b) (obj) = ((~(submask((fn) - (st)) << (st))) & (obj)) | ((b) << (st))

#define TMP_REG_0_CODE 17
#define TMP_REG_0 17

#define kRdShift 0
#define kRnShift 5
#define kRtShift 0
#define kRt2Shift 10
#define kRmShift 16

#define Rd_(rd) ((rd) << kRdShift)
#define Rt_(rt) ((rt) << kRtShift)
#define Rn_(rn) ((rn) << kRnShift)
#define Rm_(rm) ((rm) << kRmShift)

#define UnconditionalBranchFixed 0x14000000
#define UnconditionalBranchFixedMask 0x7C000000
#define UnconditionalBranchMask 0xFC000000
#define B_ 0x14000000
#define BL 0x94000000

#define UnconditionalBranchToRegisterFixed 0xD6000000
#define BR 0xD61F0000
#define BLR 0xD63F0000
#define RET 0xD65F0000

#define LoadRegLiteralFixed 0x18000000
#define LoadRegLiteralFixedMask 0x3B000000
#define LoadRegLiteralMask 0xFF000000

#define PCRelAddressingFixed 0x10000000
#define PCRelAddressingFixedMask 0x1F000000
#define PCRelAddressingMask 0x9F000000
#define ADR 0x10000000
#define ADRP 0x90000000

#define CompareBranchFixed 0x34000000
#define CompareBranchFixedMask 0x7E000000
#define CompareBranchMask 0xFF000000

#define ConditionalBranchFixed 0x54000000
#define ConditionalBranchFixedMask 0xFE000000
#define ConditionalBranchMask 0xFF000010

#define TestBranchFixed 0x36000000
#define TestBranchFixedMask 0x7E000000
#define TestBranchMask 0x7F000000

#define MoveWideImmediateFixed 0x12800000
#define MOVZ 0x40000000
#define MOVK 0x60000000
#define MOVN 0x00000000
#define SixtyFourBits 0x80000000

#define AddSubImmediateFixed 0x11000000
#define ADD_X_IMM 0x91000000
#define ADD_W_IMM 0x11000000
#define SUB_X_IMM 0xD1000000
#define SUB_W_IMM 0x51000000

#define LoadStoreUnsignedOffsetFixed 0x39000000
#define LDR_X 0xF9400000
#define LDR_W 0xB9400000
#define STR_X 0xF9000000
#define STR_W 0xB9000000

#define LogicalShiftedFixed 0x0A000000
#define ORR 0x20000000
#define ORR_X_SHIFT 0xAA000000
#define ORR_W_SHIFT 0x2A000000

#define NOP 0xD503201F
#define BRK 0xD4200000

#define kNoAccess 0
#define kRead 1
#define kWrite 2
#define kExecute 4
#define kReadWrite (kRead | kWrite)
#define kReadExecute (kRead | kExecute)
#define kReadWriteExecute (kRead | kWrite | kExecute)

#define MEM_PERM_R 0x1
#define MEM_PERM_W 0x2
#define MEM_PERM_X 0x4

#define TRAMPOLINE_UNKNOWN 0
#define TRAMPOLINE_ARM64_B_XXX 1
#define TRAMPOLINE_ARM64_B_XXX_AND_FORWARD_TRAMP 2
#define TRAMPOLINE_ARM64_ADRP_ADD_BR 3
#define TRAMPOLINE_ARM64_LDR_BR 4
#define FORWARD_TRAMPOLINE_ARM64 5

#define ARM64_B_XXX_RANGE ((1ULL << 25) << 2)

static inline int64_t SignExtend(unsigned long x, int M, int N) {
  char sign_bit = bit_(x, M - 1);
  unsigned long sign_mask = 0 - sign_bit;
  x |= ((sign_mask >> M) << M);
  return (int64_t)x;
}

static inline int64_t decode_imm14_offset(uint32_t instr) {
  int64_t imm14 = bits_(instr, 5, 18);
  int64_t offset = (imm14 << 2);
  offset = SignExtend(offset, 2 + 14, 64);
  return offset;
}

static inline uint32_t encode_imm14_offset(uint32_t instr, int64_t offset) {
  uint32_t imm14 = bits_((offset >> 2), 0, 13);
  set_bits_(instr, 5, 18, imm14);
  return instr;
}

static inline int64_t decode_imm19_offset(uint32_t instr) {
  int64_t imm19 = bits_(instr, 5, 23);
  int64_t offset = (imm19 << 2);
  offset = SignExtend(offset, 2 + 19, 64);
  return offset;
}

static inline uint32_t encode_imm19_offset(uint32_t instr, int64_t offset) {
  uint32_t imm19 = bits_((offset >> 2), 0, 18);
  set_bits_(instr, 5, 23, imm19);
  return instr;
}

static inline int64_t decode_imm26_offset(uint32_t instr) {
  int64_t imm26 = bits_(instr, 0, 25);
  int64_t offset = (imm26 << 2);
  offset = SignExtend(offset, 2 + 26, 64);
  return offset;
}

static inline uint32_t encode_imm26_offset(uint32_t instr, int64_t offset) {
  uint32_t imm26 = bits_((offset >> 2), 0, 25);
  set_bits_(instr, 0, 25, imm26);
  return instr;
}

static inline int64_t decode_immhi_immlo_offset(uint32_t instr) {
  uint32_t immlo = bits_(instr, 29, 30);
  uint32_t immhi = bits_(instr, 5, 23);
  int64_t imm = (int64_t)immlo | ((int64_t)immhi << 2);
  imm = SignExtend(imm, 2 + 19, 64);
  return imm;
}

static inline int64_t decode_immhi_immlo_zero12_offset(uint32_t instr) {
  int64_t imm = decode_immhi_immlo_offset(instr);
  imm = imm << 12;
  return imm;
}

static inline int decode_rt(uint32_t instr) { return bits_(instr, 0, 4); }
static inline int decode_rd(uint32_t instr) { return bits_(instr, 0, 4); }

static inline int inst_is_b_bl(uint32_t instr) { return (instr & UnconditionalBranchFixedMask) == UnconditionalBranchFixed; }
static inline int inst_is_ldr_literal(uint32_t instr) { return ((instr & LoadRegLiteralFixedMask) == LoadRegLiteralFixed); }
static inline int inst_is_adr(uint32_t instr) { return (instr & PCRelAddressingFixedMask) == PCRelAddressingFixed && (instr & PCRelAddressingMask) == ADR; }
static inline int inst_is_adrp(uint32_t instr) { return (instr & PCRelAddressingFixedMask) == PCRelAddressingFixed && (instr & PCRelAddressingMask) == ADRP; }
static inline int inst_is_b_cond(uint32_t instr) { return (instr & ConditionalBranchFixedMask) == ConditionalBranchFixed; }
static inline int inst_is_compare_b(uint32_t instr) { return (instr & CompareBranchFixedMask) == CompareBranchFixed; }
static inline int inst_is_test_b(uint32_t instr) { return (instr & TestBranchFixedMask) == TestBranchFixed; }

static int os_page_size(void) { return (int)sysconf(_SC_PAGESIZE); }

static void clear_cache_arm64(void *start, void *end) {
  uint64_t xstart = (uint64_t)(uintptr_t)start;
  uint64_t xend = (uint64_t)(uintptr_t)end;
  static uint64_t ctr_el0 = 0;
  if (ctr_el0 == 0) {
    __asm__ __volatile__("mrs %0, ctr_el0" : "=r"(ctr_el0));
  }
  uint64_t addr;
  if (((ctr_el0 >> 28) & 0x1) == 0x0) {
    const size_t dcache_line_size = 4 << ((ctr_el0 >> 16) & 15);
    for (addr = xstart & ~(dcache_line_size - 1); addr < xend; addr += dcache_line_size)
      __asm__ __volatile__("dc cvau, %0" ::"r"(addr));
  }
  __asm__ __volatile__("dsb ish");
  if (((ctr_el0 >> 29) & 0x1) == 0x0) {
    const size_t icache_line_size = 4 << ((ctr_el0 >> 0) & 15);
    for (addr = xstart & ~(icache_line_size - 1); addr < xend; addr += icache_line_size)
      __asm__ __volatile__("ic ivau, %0" ::"r"(addr));
    __asm__ __volatile__("dsb ish");
  }
  __asm__ __volatile__("isb sy");
}

typedef struct { addr_t start_; size_t size; } MemRange;
static inline void MemRange_init(MemRange *r, addr_t start, size_t size) { r->start_ = start; r->size = size; }
static inline addr_t MemRange_addr(const MemRange *r) { return r->start_; }
static inline addr_t MemRange_start(const MemRange *r) { return r->start_; }
static inline addr_t MemRange_end(const MemRange *r) { return r->start_ + r->size; }
static inline void MemRange_resize(MemRange *r, size_t s) { r->size = s; }
typedef MemRange MemBlock;
static inline void MemBlock_init(MemBlock *b, addr_t start, size_t size) { b->start_ = start; b->size = size; }

static MemRange MemRange_intersect(const MemRange *a, const MemRange *b) {
  MemRange r;
  addr_t s = a->start_ > b->start_ ? a->start_ : b->start_;
  addr_t e = (a->start_ + a->size) < (b->start_ + b->size) ? (a->start_ + a->size) : (b->start_ + b->size);
  if (s < e) MemRange_init(&r, s, e - s);
  else MemRange_init(&r, 0, 0);
  return r;
}

typedef struct { uint8_t *buffer; uint32_t buffer_size; uint32_t buffer_capacity; } MemBuffer;
static void MemBuffer_init(MemBuffer *b) { b->buffer_capacity = 64; b->buffer = (uint8_t *)malloc(b->buffer_capacity); b->buffer_size = 0; }
static void MemBuffer_destroy(MemBuffer *b) { if (b->buffer) { free(b->buffer); b->buffer = NULL; } b->buffer_size = 0; b->buffer_capacity = 0; }
static inline uint8_t *MemBuffer_data(MemBuffer *b) { return b->buffer; }
static inline uint32_t MemBuffer_size(MemBuffer *b) { return b->buffer_size; }
static void MemBuffer_ensure_capacity(MemBuffer *b, uint32_t in_size) {
  if (b->buffer_size + in_size > b->buffer_capacity) {
    uint32_t new_capacity = b->buffer_capacity * 2;
    while (new_capacity < b->buffer_size + in_size) new_capacity *= 2;
    uint8_t *new_buffer = (uint8_t *)malloc(new_capacity);
    memcpy(new_buffer, b->buffer, b->buffer_size);
    free(b->buffer);
    b->buffer = new_buffer;
    b->buffer_capacity = new_capacity;
  }
}
static void MemBuffer_emit(MemBuffer *b, const void *in_buffer, uint32_t size) { MemBuffer_ensure_capacity(b, size); memcpy(b->buffer + b->buffer_size, in_buffer, size); b->buffer_size += size; }
static void MemBuffer_emit_u32(MemBuffer *b, uint32_t value) { MemBuffer_emit(b, &value, sizeof(value)); }
static void MemBuffer_emit_u64(MemBuffer *b, uint64_t value) { MemBuffer_emit(b, &value, sizeof(value)); }
static void MemBuffer_rewrite_inst(MemBuffer *b, uint32_t offset, uint32_t inst) { *(uint32_t *)(b->buffer + offset) = inst; }
static uint32_t MemBuffer_load_inst(MemBuffer *b, uint32_t offset) { return *(uint32_t *)(b->buffer + offset); }
static MemBlock MemBuffer_dup(MemBuffer *b) { uint8_t *copy = (uint8_t *)malloc(b->buffer_size); memcpy(copy, b->buffer, b->buffer_size); MemBlock m; MemBlock_init(&m, (addr_t)copy, b->buffer_size); return m; }

typedef struct simple_linear_allocator_t { uint8_t *buffer; uint32_t size; uint32_t capacity; uint32_t builtin_alignment; } simple_linear_allocator_t;
static void simple_linear_allocator_init(simple_linear_allocator_t *a, uint8_t *buffer, uint32_t capacity, uint32_t alignment) { a->buffer = buffer; a->capacity = capacity; a->builtin_alignment = alignment ? alignment : 1; a->size = 0; }
static uint8_t *simple_linear_allocator_cursor(simple_linear_allocator_t *a) { return a->buffer + a->size; }
static uint8_t *simple_linear_allocator_alloc(simple_linear_allocator_t *a, uint32_t in_size, uint32_t in_alignment) {
  uint32_t alignment = in_alignment ? in_alignment : a->builtin_alignment;
  uint32_t gap_size = ALIGN_CEIL((uintptr_t)simple_linear_allocator_cursor(a), alignment) - (uintptr_t)simple_linear_allocator_cursor(a);
  a->size += gap_size;
  if (a->size + in_size > a->capacity) return NULL;
  uint8_t *data = simple_linear_allocator_cursor(a);
  a->size += in_size;
  return data;
}

#define MAX_PAGE_ALLOCATORS 64
typedef struct { simple_linear_allocator_t *code_page_allocators[MAX_PAGE_ALLOCATORS]; int code_page_count; simple_linear_allocator_t *data_page_allocators[MAX_PAGE_ALLOCATORS]; int data_page_count; } MemoryAllocator;
static MemoryAllocator gMemoryAllocator;

static void *os_memory_allocate(size_t size, int access) {
  int prot = 0;
  if (access & kRead) prot |= PROT_READ;
  if (access & kWrite) prot |= PROT_WRITE;
  if (access & kExecute) prot |= PROT_EXEC;
  int flags = MAP_PRIVATE | MAP_ANONYMOUS;
  void *result = mmap(NULL, size, prot, flags, -1, 0);
  if (result == MAP_FAILED) return NULL;
  return result;
}

static int os_memory_set_permission(void *address, size_t size, int access) {
  int prot = 0;
  if (access & kRead) prot |= PROT_READ;
  if (access & kWrite) prot |= PROT_WRITE;
  if (access & kExecute) prot |= PROT_EXEC;
  return mprotect(address, size, prot) == 0;
}

static MemBlock alloc_mem_block(size_t in_size, int is_exec) {
  if (in_size > (size_t)os_page_size()) { MemBlock b; MemBlock_init(&b, 0, 0); return b; }
  uint8_t *result = NULL;
  int count;
  simple_linear_allocator_t **allocators;
  int *count_ptr;
  if (is_exec) { count = gMemoryAllocator.code_page_count; allocators = gMemoryAllocator.code_page_allocators; count_ptr = &gMemoryAllocator.code_page_count; }
  else { count = gMemoryAllocator.data_page_count; allocators = gMemoryAllocator.data_page_allocators; count_ptr = &gMemoryAllocator.data_page_count; }
  for (int i = 0; i < count; i++) { result = (uint8_t *)simple_linear_allocator_alloc(allocators[i], (uint32_t)in_size, 0); if (result) break; }
  if (!result) {
    if (*count_ptr >= MAX_PAGE_ALLOCATORS) { MemBlock b; MemBlock_init(&b, 0, 0); return b; }
    void *page = os_memory_allocate(os_page_size(), kNoAccess);
    if (!page) { MemBlock b; MemBlock_init(&b, 0, 0); return b; }
    os_memory_set_permission(page, os_page_size(), is_exec ? kReadExecute : kReadWrite);
    simple_linear_allocator_t *pa = (simple_linear_allocator_t *)malloc(sizeof(simple_linear_allocator_t));
    simple_linear_allocator_init(pa, (uint8_t *)page, os_page_size(), 8);
    allocators[*count_ptr] = pa; (*count_ptr)++;
    result = (uint8_t *)simple_linear_allocator_alloc(pa, (uint32_t)in_size, 0);
  }
  MemBlock b; MemBlock_init(&b, (addr_t)result, in_size); return b;
}

static MemBlock alloc_exec_block(size_t size) { return alloc_mem_block(size, 1); }
static MemBlock alloc_data_block(size_t size) { return alloc_mem_block(size, 0); }

static int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t buffer_size) {
  int page_size = os_page_size();
  uintptr_t patch_page = ALIGN_FLOOR(address, page_size);
  uintptr_t patch_end_page = ALIGN_FLOOR((uintptr_t)address + buffer_size, page_size);
  mprotect((void *)patch_page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
  if (patch_page != patch_end_page) mprotect((void *)patch_end_page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);
  memcpy(address, buffer, buffer_size);
  mprotect((void *)patch_page, page_size, PROT_READ | PROT_EXEC);
  if (patch_page != patch_end_page) mprotect((void *)patch_end_page, page_size, PROT_READ | PROT_EXEC);
  addr_t clear_start = (addr_t)address;
  clear_cache_arm64((void *)clear_start, (void *)(clear_start + buffer_size));
  return 0;
}

static void make_memory_readable(void *address, size_t size) {
  void *page = (void *)ALIGN_FLOOR(address, os_page_size());
  os_memory_set_permission(page, os_page_size(), kReadExecute);
}

typedef struct RelocDataLabel {
  addr_t pos; uint8_t data[8]; uint8_t data_size;
  struct { int link_type; uintptr_t inst_offset; } ref_insts[32]; int ref_insts_count;
} RelocDataLabel;

static void RelocDataLabel_init(RelocDataLabel *l, uint64_t data, uint8_t data_size) {
  l->pos = 0; memcpy(l->data, &data, data_size); l->data_size = data_size; l->ref_insts_count = 0;
}

static void RelocDataLabel_link_to(RelocDataLabel *l, int link_type, uint32_t pc_off) {
  if (l->ref_insts_count < 32) { l->ref_insts[l->ref_insts_count].link_type = link_type; l->ref_insts[l->ref_insts_count].inst_offset = pc_off; l->ref_insts_count++; }
}

static void RelocDataLabel_link_confused_instructions(RelocDataLabel *l, MemBuffer *buffer) {
  int i;
  for (i = 0; i < l->ref_insts_count; i++) {
    int64_t fixup_offset = (int64_t)l->pos - (int64_t)l->ref_insts[i].inst_offset;
    uint32_t inst = MemBuffer_load_inst(buffer, (uint32_t)l->ref_insts[i].inst_offset);
    uint32_t new_inst = encode_imm19_offset(inst, fixup_offset);
    MemBuffer_rewrite_inst(buffer, (uint32_t)l->ref_insts[i].inst_offset, new_inst);
  }
}

typedef struct {
  addr_t fixed_addr; MemBuffer code_buffer; RelocDataLabel *data_labels[64]; int data_labels_count;
} Assembler;

static void Assembler_init(Assembler *a, addr_t fixed_addr) { a->fixed_addr = fixed_addr; MemBuffer_init(&a->code_buffer); a->data_labels_count = 0; }
static void Assembler_destroy(Assembler *a) { int i; for (i = 0; i < a->data_labels_count; i++) free(a->data_labels[i]); MemBuffer_destroy(&a->code_buffer); }
static inline size_t Assembler_pc_offset(Assembler *a) { return MemBuffer_size(&a->code_buffer); }
static void Assembler_emit_u32(Assembler *a, uint32_t value) { MemBuffer_emit_u32(&a->code_buffer, value); }
static void Assembler_emit_u64(Assembler *a, uint64_t value) { MemBuffer_emit_u64(&a->code_buffer, value); }
static void Assembler_nop(Assembler *a) { Assembler_emit_u32(a, NOP); }
static void Assembler_ret(Assembler *a) { Assembler_emit_u32(a, RET); }
static void Assembler_br(Assembler *a, int rn) { Assembler_emit_u32(a, BR | Rn_(rn)); }
static void Assembler_blr(Assembler *a, int rn) { Assembler_emit_u32(a, BLR | Rn_(rn)); }
static void Assembler_b_imm(Assembler *a, int64_t imm) { int32_t imm26 = bits_((imm >> 2), 0, 25); Assembler_emit_u32(a, B_ | imm26); }
static void Assembler_adrp(Assembler *a, int rd, int64_t imm) { uint32_t immlo = LeftShift(bits_((imm >> 12), 0, 1), 2, 29); uint32_t immhi = LeftShift(bits_((imm >> 12), 2, 20), 19, 5); Assembler_emit_u32(a, ADRP | Rd_(rd) | immlo | immhi); }
static void Assembler_add_imm_x(Assembler *a, int rd, int rn, int64_t imm) { uint32_t imm12 = LeftShift(imm, 12, 10); Assembler_emit_u32(a, ADD_X_IMM | Rd_(rd) | Rn_(rn) | imm12); }
static void Assembler_ldr_literal_x(Assembler *a, int rt, int64_t imm) { uint32_t encoding = 0x58000000 | LeftShift((imm >> 2), 26, 5) | Rt_(rt); Assembler_emit_u32(a, encoding); }
static void Assembler_ldr_unsigned_x(Assembler *a, int rt, int rn, int64_t offset) { uint32_t imm12 = (uint32_t)(offset >> 3); Assembler_emit_u32(a, LoadStoreUnsignedOffsetFixed | LDR_X | LeftShift(imm12, 12, 10) | Rn_(rn) | Rt_(rt)); }
static void Assembler_ldr_unsigned_w(Assembler *a, int rt, int rn, int64_t offset) { uint32_t imm12 = (uint32_t)(offset >> 2); Assembler_emit_u32(a, LoadStoreUnsignedOffsetFixed | LDR_W | LeftShift(imm12, 12, 10) | Rn_(rn) | Rt_(rt)); }
static void Assembler_str_unsigned_x(Assembler *a, int rt, int rn, int64_t offset) { uint32_t imm12 = (uint32_t)(offset >> 3); Assembler_emit_u32(a, LoadStoreUnsignedOffsetFixed | STR_X | LeftShift(imm12, 12, 10) | Rn_(rn) | Rt_(rt)); }
static void Assembler_movz(Assembler *a, int rd, uint64_t imm, int shift) { if (shift > 0) shift /= 16; else shift = 0; uint32_t imm16 = LeftShift(imm, 16, 5); Assembler_emit_u32(a, MoveWideImmediateFixed | MOVZ | SixtyFourBits | LeftShift(shift, 2, 21) | imm16 | Rd_(rd)); }
static void Assembler_movk(Assembler *a, int rd, uint64_t imm, int shift) { if (shift > 0) shift /= 16; else shift = 0; uint32_t imm16 = LeftShift(imm, 16, 5); Assembler_emit_u32(a, MoveWideImmediateFixed | MOVK | SixtyFourBits | LeftShift(shift, 2, 21) | imm16 | Rd_(rd)); }
static void Assembler_orr_shift_x(Assembler *a, int rd, int rn, int rm) { Assembler_emit_u32(a, LogicalShiftedFixed | ORR | SixtyFourBits | Rm_(rm) | Rn_(rn) | Rd_(rd)); }
static void Assembler_mov_reg(Assembler *a, int rd, int rn) { Assembler_orr_shift_x(a, rd, rn, 31); }
static void Assembler_mov_imm64(Assembler *a, int rd, uint64_t imm) {
  const uint32_t w0 = (uint32_t)(imm & 0xFFFFFFFF);
  const uint32_t w1 = (uint32_t)(imm >> 32);
  const uint16_t h0 = (uint16_t)(w0 & 0xFFFF);
  const uint16_t h1 = (uint16_t)(w0 >> 16);
  const uint16_t h2 = (uint16_t)(w1 & 0xFFFF);
  const uint16_t h3 = (uint16_t)(w1 >> 16);
  Assembler_movz(a, rd, h0, 0);
  Assembler_movk(a, rd, h1, 16);
  Assembler_movk(a, rd, h2, 32);
  Assembler_movk(a, rd, h3, 48);
}

static void Assembler_adrp_add(Assembler *a, int rd, uint64_t from, uint64_t to) {
  uint64_t from_PAGE = ALIGN(from, 0x1000);
  uint64_t to_PAGE = ALIGN(to, 0x1000);
  uint64_t to_PAGEOFF = (uint64_t)to % 0x1000;
  Assembler_adrp(a, rd, (int64_t)(to_PAGE - from_PAGE));
  Assembler_add_imm_x(a, rd, rd, (int64_t)to_PAGEOFF);
}

static RelocDataLabel *Assembler_create_data_label(Assembler *a, uint64_t data, uint8_t data_size) {
  RelocDataLabel *l = (RelocDataLabel *)malloc(sizeof(RelocDataLabel));
  RelocDataLabel_init(l, data, data_size);
  if (a->data_labels_count < 64) a->data_labels[a->data_labels_count++] = l;
  return l;
}

static void Assembler_ldr_label(Assembler *a, int rt, RelocDataLabel *label) { RelocDataLabel_link_to(label, 0, (uint32_t)Assembler_pc_offset(a)); Assembler_ldr_literal_x(a, rt, 0); }
static void Assembler_literal_ldr_branch(Assembler *a, uint64_t address) { RelocDataLabel *label = Assembler_create_data_label(a, address, 8); Assembler_ldr_label(a, TMP_REG_0, label); Assembler_br(a, TMP_REG_0); }

static void Assembler_reloc_data_labels(Assembler *a) {
  int i;
  for (i = 0; i < a->data_labels_count; i++) {
    RelocDataLabel *l = a->data_labels[i];
    l->pos = Assembler_pc_offset(a);
    if (l->ref_insts_count > 0) RelocDataLabel_link_confused_instructions(l, &a->code_buffer);
    MemBuffer_emit(&a->code_buffer, l->data, l->data_size);
  }
}

static MemBlock Assembler_finalize(Assembler *a) {
  size_t buffer_size = MemBuffer_size(&a->code_buffer);
  MemBlock block = alloc_exec_block(buffer_size);
  if (MemRange_addr(&block) == 0) { MemBlock b; MemBlock_init(&b, 0, 0); return b; }
  addr_t fixed = MemRange_addr(&block);
  a->fixed_addr = fixed;
  DobbyCodePatch((void *)fixed, MemBuffer_data(&a->code_buffer), (uint32_t)MemBuffer_size(&a->code_buffer));
  MemBlock b; MemBlock_init(&b, fixed, MemBuffer_size(&a->code_buffer)); return b;
}

typedef struct { int type; MemBlock buffer; addr_t forward_addr; size_t forward_size; int has_forward; } Trampoline;

static void Trampoline_init(Trampoline *t, int type, MemBlock buffer) { t->type = type; t->buffer = buffer; t->forward_addr = 0; t->forward_size = 0; t->has_forward = 0; }
static inline addr_t Trampoline_addr(Trampoline *t) { return t->buffer.start_; }
static inline addr_t Trampoline_size(Trampoline *t) { return t->buffer.size; }

static Trampoline *generate_normal_trampoline(addr_t from, addr_t to) {
  Assembler as;
  Assembler_init(&as, from);
  int tramp_type = 0;
  uint64_t distance = (uint64_t)llabs((long long)(from - to));
  uint64_t adrp_range = ((uint64_t)1 << (2 + 19 + 12 - 1));
  if (distance < adrp_range) { tramp_type = TRAMPOLINE_ARM64_ADRP_ADD_BR; Assembler_adrp_add(&as, TMP_REG_0, from, to); Assembler_br(&as, TMP_REG_0); }
  else { tramp_type = TRAMPOLINE_ARM64_LDR_BR; Assembler_literal_ldr_branch(&as, (uint64_t)to); }
  Assembler_reloc_data_labels(&as);
  MemBlock blk = MemBuffer_dup(&as.code_buffer);
  Trampoline *t = (Trampoline *)malloc(sizeof(Trampoline));
  Trampoline_init(t, tramp_type, blk);
  Assembler_destroy(&as);
  return t;
}

static int g_enable_near_trampoline = 0;
static dobby_alloc_near_code_callback_t custom_alloc_near_code_handler = NULL;

#define MAX_REGIONS 1024
typedef struct { addr_t addr; size_t size; int perm; } MemRegionInfo;

static int get_memory_layout(MemRegionInfo *out_regions, int max_regions) {
  FILE *fp = fopen("/proc/self/maps", "r");
  if (!fp) return 0;
  int count = 0;
  char line[2048];
  while (fgets(line, sizeof(line), fp) && count < max_regions) {
    addr_t region_start, region_end, region_offset;
    char permissions[5] = {0};
    uint8_t dev_major, dev_minor;
    long inode;
    int path_index = 0;
    if (sscanf(line, "%lx-%lx %4c %lx %hhx:%hhx %ld %n", (unsigned long *)&region_start, (unsigned long *)&region_end, permissions, (unsigned long *)&region_offset, &dev_major, &dev_minor, &inode, &path_index) < 7) continue;
    int perm = 0;
    if (permissions[0] == 'r') perm |= MEM_PERM_R;
    if (permissions[1] == 'w') perm |= MEM_PERM_W;
    if (permissions[2] == 'x') perm |= MEM_PERM_X;
    out_regions[count].addr = region_start;
    out_regions[count].size = region_end - region_start;
    out_regions[count].perm = perm;
    count++;
  }
  fclose(fp);
  return count;
}

static MemBlock alloc_near_block_in_page(addr_t pos, size_t range, uint32_t in_size) {
  if (custom_alloc_near_code_handler) { addr_t a = custom_alloc_near_code_handler(in_size, pos, range); if (a) { MemBlock b; MemBlock_init(&b, a, in_size); return b; } }
  MemRange search_range;
  MemRange_init(&search_range, pos > range ? pos - range : 0, range * 2);
  for (int i = 0; i < gMemoryAllocator.code_page_count; i++) {
    simple_linear_allocator_t *al = gMemoryAllocator.code_page_allocators[i];
    addr_t cursor = (addr_t)al->buffer + al->size;
    uint32_t unused_size = al->capacity - al->size;
    MemRange unused_range; MemRange_init(&unused_range, cursor, unused_size);
    MemRange intersect = MemRange_intersect(&search_range, &unused_range);
    if (intersect.size < in_size) continue;
    addr_t gap_size = intersect.start_ - cursor;
    if (gap_size) simple_linear_allocator_alloc(al, (uint32_t)gap_size, 0);
    uint8_t *result = simple_linear_allocator_alloc(al, in_size, 0);
    MemBlock b; MemBlock_init(&b, (addr_t)result, in_size); return b;
  }
  MemRegionInfo regions[MAX_REGIONS];
  int region_count = get_memory_layout(regions, MAX_REGIONS);
  for (int i = 0; i < region_count; i++) {
    if (i >= region_count - 1) break;
    addr_t unused_start = regions[i].addr + regions[i].size;
    size_t unused_size = regions[i + 1].addr - unused_start;
    MemRange unused_range; MemRange_init(&unused_range, unused_start, unused_size);
    MemRange intersect = MemRange_intersect(&search_range, &unused_range);
    if (intersect.size < in_size) continue;
    void *unused_page = (void *)ALIGN_FLOOR(intersect.start_, os_page_size());
    void *page = mmap(unused_page, os_page_size(), PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (page == MAP_FAILED) continue;
    if (page != unused_page) continue;
    if (gMemoryAllocator.code_page_count >= MAX_PAGE_ALLOCATORS) break;
    simple_linear_allocator_t *pa = (simple_linear_allocator_t *)malloc(sizeof(simple_linear_allocator_t));
    simple_linear_allocator_init(pa, (uint8_t *)page, os_page_size(), 8);
    gMemoryAllocator.code_page_allocators[gMemoryAllocator.code_page_count++] = pa;
    return alloc_near_block_in_page(pos, range, in_size);
  }
  const uint8_t zero_seq[0x1000] = {0};
  for (int i = 0; i < region_count; i++) {
    if (!(regions[i].perm & MEM_PERM_X)) continue;
    MemRange r; MemRange_init(&r, regions[i].addr, regions[i].size);
    MemRange intersect = MemRange_intersect(&search_range, &r);
    if (intersect.size < in_size) continue;
    const uint8_t *p = (const uint8_t *)intersect.start_;
    size_t search_size = intersect.size;
    size_t needle = in_size + 3;
    void *found = NULL;
    for (size_t j = 0; j + needle <= search_size; j++) { if (memcmp(p + j, zero_seq, needle) == 0) { found = (void *)(p + j); break; } }
    if (!found) continue;
    found = (void *)ALIGN_CEIL(found, 4);
    MemBlock b; MemBlock_init(&b, (addr_t)found, in_size); return b;
  }
  MemBlock b; MemBlock_init(&b, 0, 0); return b;
}

static Trampoline *generate_fast_forward_trampoline(addr_t src, addr_t dst) {
  Assembler as; Assembler_init(&as, 0);
  uint32_t need = 4 * 4;
  MemBlock blk = alloc_near_block_in_page(src, ARM64_B_XXX_RANGE, need);
  if (MemRange_addr(&blk) == 0) { Assembler_destroy(&as); return NULL; }
  Assembler_ldr_unsigned_x(&as, TMP_REG_0, TMP_REG_0, 8);
  Assembler_br(&as, TMP_REG_0);
  Assembler_emit_u64(&as, (uint64_t)dst);
  as.fixed_addr = MemRange_addr(&blk);
  MemBlock finalized = Assembler_finalize(&as);
  Trampoline *t = (Trampoline *)malloc(sizeof(Trampoline));
  Trampoline_init(t, FORWARD_TRAMPOLINE_ARM64, finalized);
  Assembler_destroy(&as);
  return t;
}

static Trampoline *generate_near_trampoline(addr_t src, addr_t dst) {
  Assembler as; Assembler_init(&as, src);
  int tramp_type = 0;
  Trampoline *forward_tramp = NULL;
  if (llabs((long long)dst - (long long)src) < (long long)ARM64_B_XXX_RANGE) {
    tramp_type = TRAMPOLINE_ARM64_B_XXX; Assembler_b_imm(&as, (int64_t)dst - (int64_t)src);
  } else {
    tramp_type = TRAMPOLINE_ARM64_B_XXX_AND_FORWARD_TRAMP;
    forward_tramp = generate_fast_forward_trampoline(src, dst);
    if (!forward_tramp) { Assembler_destroy(&as); return NULL; }
    Assembler_b_imm(&as, (int64_t)Trampoline_addr(forward_tramp) - (int64_t)src);
  }
  MemBlock blk = MemBuffer_dup(&as.code_buffer);
  Trampoline *t = (Trampoline *)malloc(sizeof(Trampoline));
  Trampoline_init(t, tramp_type, blk);
  if (forward_tramp) { t->forward_addr = Trampoline_addr(forward_tramp); t->forward_size = Trampoline_size(forward_tramp); t->has_forward = 1; free(forward_tramp); }
  Assembler_destroy(&as);
  return t;
}

typedef struct { uint32_t id; addr_t fake_func_addr; addr_t addr; MemBlock patched; MemBlock relocated; uint8_t *origin_code; size_t origin_code_size; } InterceptEntry;

#define MAX_INTERCEPT_ENTRIES 256
static InterceptEntry g_entries[MAX_INTERCEPT_ENTRIES];
static int g_entries_count = 0;

static InterceptEntry *find_entry(addr_t addr) { int i; for (i = 0; i < g_entries_count; i++) { if (g_entries[i].patched.start_ == addr) return &g_entries[i]; } return NULL; }

static void remove_entry(addr_t addr) {
  int i;
  for (i = 0; i < g_entries_count; i++) {
    if (g_entries[i].patched.start_ == addr) {
      int j;
      for (j = i; j < g_entries_count - 1; j++) g_entries[j] = g_entries[j + 1];
      g_entries_count--; return;
    }
  }
}

static void backup_orig_code(InterceptEntry *e) { uint32_t tramp_size = (uint32_t)e->patched.size; e->origin_code = (uint8_t *)malloc(tramp_size); memcpy(e->origin_code, (void *)e->addr, tramp_size); e->origin_code_size = tramp_size; }

static void restore_orig_code(InterceptEntry *e) { if (!e->origin_code) return; DobbyCodePatch((void *)e->patched.start_, e->origin_code, (uint32_t)e->patched.size); free(e->origin_code); e->origin_code = NULL; }

typedef struct { addr_t origin_start; uint32_t origin_size; Assembler as; MemBlock relocated; int relocated_built; } ReloCtx;

static void ReloCtx_init(ReloCtx *ctx, addr_t origin, uint32_t size) { ctx->origin_start = origin; ctx->origin_size = size; Assembler_init(&ctx->as, 0); ctx->relocated_built = 0; MemBlock_init(&ctx->relocated, 0, 0); }
static void ReloCtx_destroy(ReloCtx *ctx) { Assembler_destroy(&ctx->as); }
static addr_t origin_cursor(ReloCtx *ctx, uint32_t offset) { return ctx->origin_start + offset; }

static int relocate(ReloCtx *ctx, int branch) {
  Assembler *as = &ctx->as;
  uint32_t offset = 0;
  while (offset < ctx->origin_size) {
    uint32_t inst = *(uint32_t *)(origin_cursor(ctx, offset));
    if (inst_is_b_bl(inst)) {
      int64_t off = decode_imm26_offset(inst);
      addr_t dst = origin_cursor(ctx, offset) + off;
      RelocDataLabel *label = Assembler_create_data_label(as, (uint64_t)dst, 8);
      Assembler_ldr_label(as, TMP_REG_0, label);
      if ((inst & UnconditionalBranchMask) == BL) Assembler_blr(as, TMP_REG_0);
      else Assembler_br(as, TMP_REG_0);
    } else if (inst_is_ldr_literal(inst)) {
      int64_t off = decode_imm19_offset(inst);
      addr_t dst = origin_cursor(ctx, offset) + off;
      int rt = decode_rt(inst);
      char opc = bits_(inst, 30, 31);
      Assembler_mov_imm64(as, TMP_REG_0, (uint64_t)dst);
      if (opc == 0b00) Assembler_ldr_unsigned_w(as, rt, TMP_REG_0, 0);
      else if (opc == 0b01) Assembler_ldr_unsigned_x(as, rt, TMP_REG_0, 0);
    } else if (inst_is_adr(inst)) {
      int64_t off = decode_immhi_immlo_offset(inst);
      addr_t dst = origin_cursor(ctx, offset) + off;
      int rd = decode_rd(inst);
      Assembler_mov_imm64(as, rd, (uint64_t)dst);
    } else if (inst_is_adrp(inst)) {
      int64_t off = decode_immhi_immlo_zero12_offset(inst);
      addr_t dst = origin_cursor(ctx, offset) + off;
      dst = ALIGN(dst, 0x1000);
      int rd = decode_rd(inst);
      Assembler_mov_imm64(as, rd, (uint64_t)dst);
    } else if (inst_is_b_cond(inst)) {
      int64_t off = decode_imm19_offset(inst);
      addr_t dst = origin_cursor(ctx, offset) + off;
      uint32_t branch_inst = inst;
      char cond = bits_(inst, 0, 3);
      cond = cond ^ 1;
      set_bits_(branch_inst, 0, 3, cond);
      int64_t new_off = 4 * 3;
      uint32_t imm19 = (uint32_t)(new_off >> 2);
      set_bits_(branch_inst, 5, 23, imm19);
      RelocDataLabel *label = Assembler_create_data_label(as, (uint64_t)dst, 8);
      Assembler_emit_u32(as, branch_inst);
      Assembler_ldr_label(as, TMP_REG_0, label);
      Assembler_br(as, TMP_REG_0);
    } else if (inst_is_compare_b(inst)) {
      int64_t off = decode_imm19_offset(inst);
      addr_t dst = origin_cursor(ctx, offset) + off;
      uint32_t branch_inst = inst;
      char op = bit_(inst, 24);
      op = op ^ 1;
      set_bit_(branch_inst, 24, op);
      int64_t new_off = 4 * 3;
      uint32_t imm19 = (uint32_t)(new_off >> 2);
      set_bits_(branch_inst, 5, 23, imm19);
      RelocDataLabel *label = Assembler_create_data_label(as, (uint64_t)dst, 8);
      Assembler_emit_u32(as, branch_inst);
      Assembler_ldr_label(as, TMP_REG_0, label);
      Assembler_br(as, TMP_REG_0);
    } else if (inst_is_test_b(inst)) {
      int64_t off = decode_imm14_offset(inst);
      addr_t dst = origin_cursor(ctx, offset) + off;
      uint32_t branch_inst = inst;
      char op = bit_(inst, 24);
      op = op ^ 1;
      set_bit_(branch_inst, 24, op);
      int64_t new_off = 4 * 3;
      uint32_t imm14 = (uint32_t)(new_off >> 2);
      set_bits_(branch_inst, 5, 18, imm14);
      RelocDataLabel *label = Assembler_create_data_label(as, (uint64_t)dst, 8);
      Assembler_emit_u32(as, branch_inst);
      Assembler_ldr_label(as, TMP_REG_0, label);
      Assembler_br(as, TMP_REG_0);
    } else {
      Assembler_emit_u32(as, inst);
    }
    offset += sizeof(uint32_t);
  }
  ctx->origin_size = offset;
  if (branch) Assembler_literal_ldr_branch(as, origin_cursor(ctx, offset));
  Assembler_reloc_data_labels(as);
  ctx->relocated = Assembler_finalize(as);
  ctx->relocated_built = 1;
  return 0;
}

int DobbyHook(void *address, void *fake_func, void **out_origin_func) {
  if (!address) return -1;
  make_memory_readable(address, 4);
  if (find_entry((addr_t)address)) return -1;
  if (g_entries_count >= MAX_INTERCEPT_ENTRIES) return -1;
  InterceptEntry *entry = &g_entries[g_entries_count];
  memset(entry, 0, sizeof(*entry));
  entry->addr = (addr_t)address;
  entry->fake_func_addr = (addr_t)fake_func;
  addr_t from = (addr_t)address;
  addr_t to = (addr_t)fake_func;
  Trampoline *trampoline = NULL;
  Trampoline *near_trampoline = NULL;
  if (g_enable_near_trampoline) near_trampoline = generate_near_trampoline(from, to);
  if (!near_trampoline) trampoline = generate_normal_trampoline(from, to);
  addr_t tramp_addr; size_t tramp_size;
  if (near_trampoline) { tramp_addr = Trampoline_addr(near_trampoline); tramp_size = Trampoline_size(near_trampoline); }
  else if (trampoline) { tramp_addr = Trampoline_addr(trampoline); tramp_size = Trampoline_size(trampoline); }
  else return -1;
  ReloCtx ctx; ReloCtx_init(&ctx, from, (uint32_t)tramp_size);
  relocate(&ctx, 1);
  if (ctx.relocated.size == 0) { ReloCtx_destroy(&ctx); if (trampoline) free(trampoline); if (near_trampoline) free(near_trampoline); return -1; }
  entry->patched.start_ = from;
  entry->patched.size = (size_t)ctx.origin_size;
  entry->relocated = ctx.relocated;
  backup_orig_code(entry);
  DobbyCodePatch((void *)from, (uint8_t *)tramp_addr, (uint32_t)tramp_size);
  if (out_origin_func) *out_origin_func = (void *)entry->relocated.start_;
  g_entries_count++;
  ReloCtx_destroy(&ctx);
  if (trampoline) { free(trampoline); }
  if (near_trampoline) { free(near_trampoline); }
  return 0;
}

static void* GetLibBase(const char* libName) {
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return NULL;
    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libName)) {
            uintptr_t base = strtoull(line, NULL, 16);
            fclose(fp);
            return (void*)base;
        }
    }
    fclose(fp);
    return NULL;
}

static int (*CurlHook_OG_SETOPT)(void*, int, void*) = NULL;
static int (*FCurlHook_OGProcessRequest)(void*) = NULL;
static void (*FCurlHook_SetUrl)(void*, const void*) = NULL;
static int (*EOSFCurlHook_OGProcessRequest)(void*) = NULL;
static void (*EOSFCurlHook_SetUrl)(void*, const void*) = NULL;

static struct FString* FCurlHook_GetUrl(void* req) {
    return (struct FString*)((uintptr_t)req + (uintptr_t)ue_geturl_off);
}

static struct FString* EOSFCurlHook_GetUrl(void* req) {
    return (struct FString*)((uintptr_t)req + (uintptr_t)eos_geturl_off);
}

static int InternalProcessRequest(void* Req, int isEOS) {
    if (!Req) return 0;
    int (*OGProcessRequest)(void*) = isEOS ? EOSFCurlHook_OGProcessRequest : FCurlHook_OGProcessRequest;
    void (*SetUrl)(void*, const void*) = isEOS ? EOSFCurlHook_SetUrl : FCurlHook_SetUrl;
    struct FString* OGUrl = isEOS ? EOSFCurlHook_GetUrl(Req) : FCurlHook_GetUrl(Req);
    if (OGUrl->length == 0 || !OGUrl->data) return OGProcessRequest(Req);
    char* OGUrlCStr = FString_ToCStr(OGUrl);
    if (!OGUrlCStr) return OGProcessRequest(Req);
    struct Url url;
    Url_ParseUrl(OGUrlCStr, &url);
    if (ShouldRedirect(url.host)) {
        char newUrlCStr[URL_TOTAL_MAX];
        CreateUrl(g_backend_url, url.pathAndQuery, newUrlCStr, sizeof(newUrlCStr));
        struct FString* newUrl = (struct FString*)malloc(sizeof(struct FString));
        *newUrl = FString_FromCStr(newUrlCStr);
        SetUrl(Req, newUrl);
    }
    free(OGUrlCStr);
    return OGProcessRequest(Req);
}

static int FCurlHook_ProcessRequest(void* Req) { return InternalProcessRequest(Req, 0); }
static int EOSFCurlHook_ProcessRequest(void* Req) { return InternalProcessRequest(Req, 1); }

static int CurlHook_SetOptHook(void* handle, int option, void* args) {
    if (!CurlHook_OG_SETOPT) return -1;
    if (option == 10002 && args != NULL) {
        const char* url = (const char*)args;
        if (url) {
            struct Url parsed;
            Url_ParseUrl(url, &parsed);
            if (ShouldRedirect(parsed.host)) {
                static __thread char newUrl[URL_TOTAL_MAX];
                CreateUrl(g_backend_url, parsed.pathAndQuery, newUrl, sizeof(newUrl));
                return CurlHook_OG_SETOPT(handle, option, (void*)newUrl);
            }
        }
    }
    return CurlHook_OG_SETOPT(handle, option, args);
}

static void setup_ue_hooks(void* base) {
    if (!ue_pr_offset) return;
    void* pr_addr = (void*)((uintptr_t)base + (uintptr_t)ue_pr_offset);
    FCurlHook_SetUrl = (void (*)(void*, const void*))((uintptr_t)base + (uintptr_t)ue_seturl_off);
    DobbyHook(pr_addr, (void*)FCurlHook_ProcessRequest, (void**)&FCurlHook_OGProcessRequest);
}

static void setup_eos_hooks(void* base) {
    if (!eos_pr_offset) return;
    void* pr_addr = (void*)((uintptr_t)base + (uintptr_t)eos_pr_offset);
    EOSFCurlHook_SetUrl = (void (*)(void*, const void*))((uintptr_t)base + (uintptr_t)eos_seturl_off);
    DobbyHook(pr_addr, (void*)EOSFCurlHook_ProcessRequest, (void**)&EOSFCurlHook_OGProcessRequest);
}


// Wait for a module to map (bounded poller used for libEOSSDK.so).
void* owen_wait_lib(const char* name) {
    for (int i = 0; i < 150; ++i) {  // 15s
        FILE* fp = fopen("/proc/self/maps", "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, name)) {
                    uintptr_t base = strtoull(line, NULL, 16);
                    fclose(fp);
                    return (void*)base;
                }
            }
            fclose(fp);
        }
        usleep(100000);
    }
    return NULL;
}

// ── Erbium integration ────────────────────────────────────────────────────
// Offsets arrive baked from android/Generated/offsets-<ver>.h; no scanning.

int owen_install(uintptr_t engine_base,
                 const char* backend_url,
                 uint32_t ue_pr, uint16_t ue_geturl, uint32_t ue_seturl,
                 uint32_t eos_pr, uint16_t eos_geturl, uint32_t eos_seturl,
                 const char* version_string)
{
    if (!engine_base || !backend_url) return -1;
    if (ue_pr == 0) {
        LOGOW("no Owen offsets for this build — redirect disabled");
        return -2;
    }
    strncpy(g_backend_url, backend_url, sizeof(g_backend_url) - 1);
    detected_version = version_string;
    ue_pr_offset = ue_pr;
    ue_geturl_off = ue_geturl;
    ue_seturl_off = ue_seturl;
    eos_pr_offset = eos_pr;
    eos_geturl_off = eos_geturl;
    eos_seturl_off = eos_seturl;

    setup_ue_hooks((void*)engine_base);
    LOGOW("UE hooks installed: ProcessRequest=0x%x SetURL=0x%x backend=%s",
          ue_pr, ue_seturl, g_backend_url);

    if (eos_pr) {
        // libEOSSDK may not be mapped yet — wait briefly (only needed 22.00+;
        // harmless on 21.30 where eos_pr == 0)
        extern void* owen_wait_lib(const char*);
        void* eos_base = owen_wait_lib("libEOSSDK.so");
        if (eos_base) {
            setup_eos_hooks(eos_base);
            LOGOW("EOS hooks installed at %p", eos_base);
        }
    }

    if (detected_version && strlen(detected_version) >= 10)
        send_version_report(detected_version);
    return 0;
}
