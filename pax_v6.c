/*
 * ============================================================================
 * PAX AUTONOMOUS ENGINE v6.0 (pax.c) - PURE PAX [] EDITION
 * The World's Simplest, Zero-GC, Native Programming Language
 * 
 * Compile once:
 *   gcc -O3 pax_v6.c -o pax
 * ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET pax_socket_t;
  #define CLOSESOCK closesocket
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  typedef int pax_socket_t;
  #define INVALID_SOCKET -1
  #define CLOSESOCK close
#endif

#define PAX_ARENA_SIZE (1024 * 1024 * 32) // 32 MB Static Arena (Pax [])
#define MAX_VARS 256
#define MAX_CODE 65536
#define MAX_FUNCS 64
#define MAX_ROUTES 32

typedef enum {
    VAL_NONE = 0,
    VAL_INT = 1,
    VAL_STR = 2
} PaxValType;

typedef struct {
    PaxValType type;
    union {
        int64_t i;
        char* s;
    } as;
} PaxValue;

typedef struct {
    char name[64];
    PaxValue val;
} PaxVar;

typedef struct {
    uint8_t* memory;
    size_t offset;
    size_t capacity;
} PaxArena;

typedef struct {
    char path[128];
    char body[2048];
} PaxRoute;

enum {
    OP_HALT = 0x00,
    OP_PAX_OUT = 0x01,     // Universal Pax [] output opcode
    OP_SET_INT = 0x02,
    OP_SET_STR = 0x03,
    OP_ADD_INT = 0x04,
    OP_REQUIRE_GT = 0x06,
    OP_GUARD_ON = 0x0F,
    OP_WEB_SERVE = 0x10
};

PaxArena arena;
PaxVar vars[MAX_VARS];
int var_count = 0;
PaxRoute routes[MAX_ROUTES];
int route_count = 0;
int guard_active = 0;

void arena_init() {
    arena.memory = (uint8_t*)malloc(PAX_ARENA_SIZE);
    if (!arena.memory) {
        fprintf(stderr, "[Pax [] FATAL] Out of Arena RAM!\n");
        exit(1);
    }
    arena.offset = 0;
    arena.capacity = PAX_ARENA_SIZE;
}

void* arena_alloc(size_t size) {
    size_t aligned_size = (size + 7) & ~7;
    if (arena.offset + aligned_size > arena.capacity) {
        fprintf(stderr, "[Pax [] FATAL] Arena Memory Overflow!\n");
        exit(1);
    }
    void* ptr = arena.memory + arena.offset;
    arena.offset += aligned_size;
    return ptr;
}

char* arena_strdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* dest = (char*)arena_alloc(len);
    memcpy(dest, str, len);
    return dest;
}

int get_or_create_var(const char* name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) return i;
    }
    strncpy(vars[var_count].name, name, 63);
    vars[var_count].name[63] = '\0';
    vars[var_count].val.type = VAL_NONE;
    return var_count++;
}

char* trim(char* str) {
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void run_native_web_server(int port) {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
    pax_socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) return;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        CLOSESOCK(server_fd);
        return;
    }

    listen(server_fd, 16);
    printf("[PAX-NET] Sovereign Server Live on http://localhost:%d (Arena RAM: %zu bytes | Pax [] = Output)\n", port, arena.offset);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        pax_socket_t client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client == INVALID_SOCKET) continue;

        char req_buf[2048] = {0};
        int n = recv(client, req_buf, sizeof(req_buf) - 1, 0);
        if (n > 0) {
            char method[16] = {0}, path[128] = {0};
            sscanf(req_buf, "%15s %127s", method, path);

            const char* resp_body = "404 - Pax Page Not Found";
            int found = 0;
            for (int i = 0; i < route_count; i++) {
                if (strcmp(routes[i].path, path) == 0) {
                    resp_body = routes[i].body;
                    found = 1;
                    break;
                }
            }

            char http_resp[4096];
            snprintf(http_resp, sizeof(http_resp),
                     "HTTP/1.1 %s\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",
                     found ? "200 OK" : "404 Not Found", strlen(resp_body), resp_body);
            send(client, http_resp, (int)strlen(http_resp), 0);
        }
        CLOSESOCK(client);
    }
}

size_t compile_pax_source(const char* src_path, uint8_t* bytecode) {
    FILE* f = fopen(src_path, "r");
    if (!f) { fprintf(stderr, "[PAX] File not found: %s\n", src_path); exit(1); }

    size_t pc = 0;
    memcpy(bytecode + pc, "PAX1", 4); pc += 4;
    int web_port = 0;

    char line_buf[1024];
    while (fgets(line_buf, sizeof(line_buf), f)) {
        char* line = trim(line_buf);
        if (line[0] == '\0' || line[0] == '#' || strcmp(line, "run") == 0 || strcmp(line, "]") == 0) continue;

        if (strcmp(line, "guard on") == 0) {
            bytecode[pc++] = OP_GUARD_ON;
            continue;
        }

        int port_val = 0;
        if (sscanf(line, "web on %d [", &port_val) == 1) {
            web_port = port_val;
            continue;
        }

        char page_path[128];
        if (sscanf(line, "page \"%127[^\"]\" [", page_path) == 1) {
            if (route_count < MAX_ROUTES) {
                int r_idx = route_count++;
                strncpy(routes[r_idx].path, page_path, 127);
                routes[r_idx].body[0] = '\0';
                while (fgets(line_buf, sizeof(line_buf), f)) {
                    char* inner = trim(line_buf);
                    if (strcmp(inner, "]") == 0) break;
                    if (strncmp(inner, "Pax [", 5) == 0) {
                        while (fgets(line_buf, sizeof(line_buf), f)) {
                            char* p_inner = trim(line_buf);
                            if (strcmp(p_inner, "]") == 0) break;
                            if (p_inner[0] == '"') {
                                p_inner++;
                                char* end_q = strrchr(p_inner, '"');
                                if (end_q) *end_q = '\0';
                                strncat(routes[r_idx].body, p_inner, 2000 - strlen(routes[r_idx].body));
                            }
                        }
                    }
                }
            }
            continue;
        }

        if (strcmp(line, "Pax [") == 0) {
            while (fgets(line_buf, sizeof(line_buf), f)) {
                char* inner = trim(line_buf);
                if (strcmp(inner, "]") == 0) break;
                if (inner[0] == '"') {
                    inner++;
                    char* end_q = strrchr(inner, '"');
                    if (end_q) *end_q = '\0';
                    uint16_t len = (uint16_t)strlen(inner);
                    bytecode[pc++] = OP_PAX_OUT;
                    bytecode[pc++] = 1;
                    memcpy(bytecode + pc, &len, 2); pc += 2;
                    memcpy(bytecode + pc, inner, len); pc += len;
                } else {
                    uint8_t nlen = (uint8_t)strlen(inner);
                    bytecode[pc++] = OP_PAX_OUT;
                    bytecode[pc++] = 2;
                    bytecode[pc++] = nlen;
                    memcpy(bytecode + pc, inner, nlen); pc += nlen;
                }
            }
            continue;
        }

        long long num;
        char vname[64];
        if (sscanf(line, "%63s += %lld", vname, &num) == 2) {
            uint8_t nlen = (uint8_t)strlen(vname);
            int64_t val64 = (int64_t)num;
            bytecode[pc++] = OP_ADD_INT;
            bytecode[pc++] = nlen;
            memcpy(bytecode + pc, vname, nlen); pc += nlen;
            memcpy(bytecode + pc, &val64, 8); pc += 8;
            continue;
        }
        if (sscanf(line, "%63s = %lld", vname, &num) == 2) {
            uint8_t nlen = (uint8_t)strlen(vname);
            int64_t val64 = (int64_t)num;
            bytecode[pc++] = OP_SET_INT;
            bytecode[pc++] = nlen;
            memcpy(bytecode + pc, vname, nlen); pc += nlen;
            memcpy(bytecode + pc, &val64, 8); pc += 8;
            continue;
        }
    }

    if (web_port > 0) {
        bytecode[pc++] = OP_WEB_SERVE;
        uint16_t p16 = (uint16_t)web_port;
        memcpy(bytecode + pc, &p16, 2); pc += 2;
    }

    bytecode[pc++] = OP_HALT;
    fclose(f);
    return pc;
}

void execute_pax_vm(uint8_t* code, size_t size) {
    size_t ip = 4;
    while (ip < size) {
        uint8_t op = code[ip++];
        if (op == OP_HALT) break;
        else if (op == OP_PAX_OUT) {
            uint8_t ptype = code[ip++];
            if (ptype == 1) {
                uint16_t len; memcpy(&len, code + ip, 2); ip += 2;
                printf("%.*s\n", len, (char*)(code + ip)); ip += len;
            } else if (ptype == 2) {
                uint8_t nlen = code[ip++]; char name[64] = {0};
                memcpy(name, code + ip, nlen); ip += nlen;
                int idx = get_or_create_var(name);
                if (vars[idx].val.type == VAL_INT) {
                    printf("%lld\n", (long long)vars[idx].val.as.i);
                } else if (vars[idx].val.type == VAL_STR) {
                    printf("%s\n", vars[idx].val.as.s);
                } else {
                    printf("nil\n");
                }
            }
        } else if (op == OP_SET_INT) {
            uint8_t nlen = code[ip++]; char name[64] = {0};
            memcpy(name, code + ip, nlen); ip += nlen;
            int64_t val; memcpy(&val, code + ip, 8); ip += 8;
            int idx = get_or_create_var(name);
            vars[idx].val.type = VAL_INT;
            vars[idx].val.as.i = val;
        } else if (op == OP_ADD_INT) {
            uint8_t nlen = code[ip++]; char name[64] = {0};
            memcpy(name, code + ip, nlen); ip += nlen;
            int64_t val; memcpy(&val, code + ip, 8); ip += 8;
            int idx = get_or_create_var(name);
            vars[idx].val.as.i += val;
        } else if (op == OP_WEB_SERVE) {
            uint16_t port; memcpy(&port, code + ip, 2); ip += 2;
            run_native_web_server(port);
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Pax v6.0 Pure Bracket Engine []\n");
        printf("Usage: pax run <file.pax>\n");
        return 0;
    }
    arena_init();
    uint8_t* code_buf = (uint8_t*)arena_alloc(MAX_CODE);
    if (strcmp(argv[1], "run") == 0) {
        size_t len = compile_pax_source(argv[2], code_buf);
        execute_pax_vm(code_buf, len);
    }
    free(arena.memory);
    return 0;
}
