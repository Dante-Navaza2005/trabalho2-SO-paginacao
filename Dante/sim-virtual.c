// Alunos: Dante Navaza 2321406 e Marcela Issa 2310746

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_FRAMES 262144
#define MAX_LINE 100
#define MAX_ACESSOS 1000000

typedef struct {
    int page_number;
    int R;
    int M;
    unsigned long last_access_time;
    int valid;
} Frame;

Frame *frames;
unsigned int num_frames;
unsigned int page_size_kb;
unsigned int mem_size_mb;
unsigned long time_counter = 0;
unsigned long page_faults = 0;
unsigned long dirty_writes = 0;

char algoritmo[20];
int ponteiro_clock = 0;
unsigned int acessos[MAX_ACESSOS];
int total_acessos = 0;
int posicao_atual = 0;

int encontrar_pagina(int page);
int substituir_pagina(int page);
int get_free_frame();
int escolher_vitima_lru();
int escolher_vitima_segunda_chance();
int escolher_vitima_clock();
int escolher_vitima_otima();
unsigned int get_page(unsigned int addr);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Uso: %s algoritmo arquivo.log tamanho_pagina_KB tamanho_memoria_MB\n", argv[0]);
        return 1;
    }

    printf("prompt> sim-virtual %s %s %s %s\n", argv[1], argv[2], argv[3], argv[4]);
    printf("Executando o simulador...\n");

    strcpy(algoritmo, argv[1]);
    FILE *file = fopen(argv[2], "r");
    page_size_kb = atoi(argv[3]);
    mem_size_mb = atoi(argv[4]);

    if (!file) {
        perror("Erro ao abrir arquivo de entrada");
        return 1;
    }

    int s = (int)log2(page_size_kb * 1024);

    if (strcmp(algoritmo, "otimo") == 0) {
        unsigned int end_temp;
        char rw_temp;
        while (fscanf(file, "%x %c", &end_temp, &rw_temp) != EOF && total_acessos < MAX_ACESSOS) {
            acessos[total_acessos++] = end_temp >> s;
        }
        rewind(file);
    }

    num_frames = (mem_size_mb * 1024) / page_size_kb;
    frames = (Frame *)malloc(sizeof(Frame) * num_frames);
    for (int i = 0; i < num_frames; i++) frames[i].valid = 0;

    unsigned int addr;
    char rw;

    while (fscanf(file, "%x %c", &addr, &rw) != EOF) {
        unsigned int page = addr >> s;
        int frame_index = encontrar_pagina(page);
        time_counter++;

        if (frame_index != -1) {
            frames[frame_index].R = 1;
            if (rw == 'W') frames[frame_index].M = 1;
            frames[frame_index].last_access_time = time_counter;
        } else {
            page_faults++;
            int target = get_free_frame();
            if (target == -1) {
                target = substituir_pagina(page);
            }
            if (frames[target].valid && frames[target].M) dirty_writes++;

            frames[target].page_number = page;
            frames[target].R = 1;
            frames[target].M = (rw == 'W') ? 1 : 0;
            frames[target].last_access_time = time_counter;
            frames[target].valid = 1;
        }

        if (strcmp(algoritmo, "otimo") == 0 && posicao_atual < total_acessos)
            posicao_atual++;
    }

    fclose(file);

    printf("Arquivo de entrada: %s\n", argv[2]);
    printf("Tamanho da memoria fisica: %u MB\n", mem_size_mb);
    printf("Tamanho das páginas: %u KB\n", page_size_kb);
    printf("Algoritmo de substituição: %s\n", algoritmo);
    printf("Número de Faltas de Páginas: %lu\n", page_faults);
    printf("Número de Páginas Escritas: %lu\n", dirty_writes);

    free(frames);
    return 0;
}

int encontrar_pagina(int page) {
    for (int i = 0; i < num_frames; i++) {
        if (frames[i].valid && frames[i].page_number == page)
            return i;
    }
    return -1;
}

int substituir_pagina(int page) {
    if (strcmp(algoritmo, "LRU") == 0) {
        return escolher_vitima_lru();
    } else if (strcmp(algoritmo, "2nd") == 0) {
        return escolher_vitima_segunda_chance();
    } else if (strcmp(algoritmo, "clock") == 0) {
        return escolher_vitima_clock();
    } else if (strcmp(algoritmo, "otimo") == 0) {
        return escolher_vitima_otima();
    } else {
        printf("Algoritmo %s não implementado ainda.\n", algoritmo);
        exit(1);
    }
}

int get_free_frame() {
    for (int i = 0; i < num_frames; i++) {
        if (!frames[i].valid) return i;
    }
    return -1;
}

int escolher_vitima_lru() {
    int menor = 0;
    for (int i = 1; i < num_frames; i++) {
        if (frames[i].last_access_time < frames[menor].last_access_time) {
            menor = i;
        }
    }
    return menor;
}

int escolher_vitima_segunda_chance() {
    static int pos = 0;
    while (1) {
        if (!frames[pos].R) {
            int escolhido = pos;
            pos = (pos + 1) % num_frames;
            return escolhido;
        } else {
            frames[pos].R = 0;
            pos = (pos + 1) % num_frames;
        }
    }
}

int escolher_vitima_clock() {
    while (1) {
        if (!frames[ponteiro_clock].R) {
            int escolhido = ponteiro_clock;
            ponteiro_clock = (ponteiro_clock + 1) % num_frames;
            return escolhido;
        } else {
            frames[ponteiro_clock].R = 0;
            ponteiro_clock = (ponteiro_clock + 1) % num_frames;
        }
    }
}

int escolher_vitima_otima() {
    int mais_longe = -1;
    int indice_escolhido = -1;

    for (int i = 0; i < num_frames; i++) {
        int proximo_uso = -1;
        for (int j = posicao_atual; j < total_acessos; j++) {
            if (acessos[j] == frames[i].page_number) {
                proximo_uso = j;
                break;
            }
        }
        if (proximo_uso == -1) {
            return i;
        }
        if (proximo_uso > mais_longe) {
            mais_longe = proximo_uso;
            indice_escolhido = i;
        }
    }
    return indice_escolhido;
}

unsigned int get_page(unsigned int addr) {
    int s = (int)log2(page_size_kb * 1024);
    return addr >> s;
}