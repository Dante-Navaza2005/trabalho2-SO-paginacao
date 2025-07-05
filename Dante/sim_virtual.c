#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>

#define MAX_PAGES 1048576 / 8 // 1MB / 8KB

typedef struct {
    unsigned page_number;
    char rw;
} MemoryAccess;

typedef struct {
    int frame;
    unsigned last_used;
    bool reference;
    bool valid;
} PageTableEntry;

PageTableEntry *page_table;
int *frame_table;
unsigned time_counter = 0;
int page_faults = 0;
int total_accesses = 0;

unsigned get_page_number(unsigned addr, int page_size_kb) {
    return addr / (page_size_kb * 1024);
}

int find_victim_lru(int n_frames) {
    int victim = -1;
    unsigned oldest = UINT_MAX;
    for (int i = 0; i < n_frames; i++) {
        if (frame_table[i] != -1 && page_table[frame_table[i]].last_used < oldest) {
            oldest = page_table[frame_table[i]].last_used;
            victim = i;
        }
    }
    return victim;
}

void simulate_lru(FILE *fp, int page_size_kb, int mem_size_mb) {
    int n_frames = (mem_size_mb * 1024) / page_size_kb;
    frame_table = malloc(n_frames * sizeof(int));
    for (int i = 0; i < n_frames; i++) frame_table[i] = -1;

    unsigned addr;
    char rw;
    while (fscanf(fp, "%x %c", &addr, &rw) == 2) {
        unsigned page = get_page_number(addr, page_size_kb);
        total_accesses++;
        time_counter++;

        if (!page_table[page].valid) {
            page_faults++;
            int placed = 0;
            for (int i = 0; i < n_frames; i++) {
                if (frame_table[i] == -1) {
                    frame_table[i] = page;
                    page_table[page] = (PageTableEntry){i, time_counter, false, true};
                    placed = 1;
                    break;
                }
            }
            if (!placed) {
                int victim_frame = find_victim_lru(n_frames);
                int victim_page = frame_table[victim_frame];
                page_table[victim_page].valid = false;
                frame_table[victim_frame] = page;
                page_table[page] = (PageTableEntry){victim_frame, time_counter, false, true};
            }
        } else {
            page_table[page].last_used = time_counter;
        }
    }

    printf("Total accesses: %d\n", total_accesses);
    printf("Page faults: %d\n", page_faults);
    printf("Fault rate: %.2f%%\n", (100.0 * page_faults / total_accesses));

    free(frame_table);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s [algoritmo] [arquivo.log] [tam_pagina_KB] [tam_mem_MB]\n", argv[0]);
        return 1;
    }

    char *algoritmo = argv[1];
    char *filename = argv[2];
    int page_size_kb = atoi(argv[3]);
    int mem_size_mb = atoi(argv[4]);

    page_table = calloc(MAX_PAGES, sizeof(PageTableEntry));

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        return 1;
    }

    if (strcmp(algoritmo, "LRU") == 0) {
        simulate_lru(fp, page_size_kb, mem_size_mb);
    } else {
        fprintf(stderr, "Algoritmo nao implementado: %s\n", algoritmo);
    }

    fclose(fp);
    free(page_table);
    return 0;
}
