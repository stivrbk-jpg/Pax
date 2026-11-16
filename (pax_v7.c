/* ==========================================================
 * PAX v7.0 - SOVEREIGN ZERO-MALLOC ARENA ALLOCATOR
 * Maximum performance, zero fragmentation, 64MB hard limit.
 * ========================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PAX_ARENA_SIZE (64 * 1024 * 1024) // Exakt 64 MB statischer RAM

typedef struct {
    uint8_t buffer[PAX_ARENA_SIZE];
    size_t offset;
} PaxArena;

static PaxArena global_arena = { .offset = 0 };

// Blitzschnelle O(1) Zuweisung ohne Heap-Fragmentation
void* pax_arena_alloc(size_t size) {
    // Ausrichtung auf 8-Byte-Grenzen für maximale CPU-Geschwindigkeit
    size_t aligned_size = (size + 7) & ~7;
    
    if (global_arena.offset + aligned_size > PAX_ARENA_SIZE) {
        fprintf(stderr, "[PAX CRITICAL] Arena overflow! 64MB boundary reached.\n");
        exit(1);
    }
    
    void* ptr = &global_arena.buffer[global_arena.offset];
    global_arena.offset += aligned_size;
    return ptr;
}

// Kompromissloser Reset: Löscht den gesamten Speicher in einem einzigen Taktzyklus
void pax_arena_reset(void) {
    global_arena.offset = 0;
}

size_t pax_arena_used(void) {
    return global_arena.offset;
}
