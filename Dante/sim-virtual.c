// Alunos: Dante Navaza 2321406 e Marcela Issa 2310746

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define QuadrosMaximo 262144
#define LinhaMaxima 100
#define AcessosMaximo 1000000

typedef struct {
    int pagina;
    int r;
    int m;
    unsigned long acesso;
    int valido;
} Quadro;

Quadro* todosquadros;
unsigned int totalquadros;
unsigned int tamanhopagina;
unsigned int memoriafisica;
unsigned long tempoagora = 0;
unsigned long faltapagina = 0;
unsigned long paginasmodificadas = 0;

char metodo[20];
int ponteiroatual = 0;
unsigned int listaacessos[AcessosMaximo];
int acessostotal = 0;
int posicaoatual = 0;

int escolherpaginaLRU() {
    int menor = 0;
    for (int i = 1; i < totalquadros; i++) {
        if (todosquadros[i].acesso < todosquadros[menor].acesso) menor = i;
    }
    return menor;
}

int escolherpaginaSegundo() {
    static int pos = 0;
    while (1) {
        if (!todosquadros[pos].r) {
            int escolhido = pos;
            pos = (pos + 1) % totalquadros;
            return escolhido;
        } else {
            todosquadros[pos].r = 0;
            pos = (pos + 1) % totalquadros;
        }
    }
}

int escolherpaginaClock() {
    while (1) {
        if (!todosquadros[ponteiroatual].r) {
            int escolhido = ponteiroatual;
            ponteiroatual = (ponteiroatual + 1) % totalquadros;
            return escolhido;
        } else {
            todosquadros[ponteiroatual].r = 0;
            ponteiroatual = (ponteiroatual + 1) % totalquadros;
        }
    }
}

int escolherpaginaOtimo() {
    int maislonge = -1;
    int indice = -1;
    for (int i = 0; i < totalquadros; i++) {
        int proximo = -1;
        for (int j = posicaoatual; j < acessostotal; j++) {
            if (listaacessos[j] == todosquadros[i].pagina) {
                proximo = j;
                break;
            }
        }
        if (proximo == -1) return i;
        if (proximo > maislonge) {
            maislonge = proximo;
            indice = i;
        }
    }
    return indice;
}

int convertervalor(char* texto) {
    char* fim;
    long valor = strtol(texto, &fim, 10);
    if (*fim != '\0' || valor < 0) {
        printf("Valor inválido: %s\n", texto);
        exit(1);
    }
    return (int)valor;
}

int main(int quantidadeargumentos, char* argumentos[]) {
    if (quantidadeargumentos != 5) {
        printf("Uso correto: %s metodo arquivo tamanhopagina memoria\n", argumentos[0]);
        return 1;
    }

    printf("prompt> sim-virtual %s %s %s %s\n", argumentos[1], argumentos[2], argumentos[3], argumentos[4]);
    printf("Executando simulador...\n");

    strcpy(metodo, argumentos[1]);
    tamanhopagina = convertervalor(argumentos[3]);
    memoriafisica = convertervalor(argumentos[4]);
    totalquadros = (memoriafisica * 1024) / tamanhopagina;

    FILE* arquivo = fopen(argumentos[2], "r");
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    int deslocamento = (int)log2(tamanhopagina * 1024);

    if (strcmp(metodo, "otimo") == 0) {
        unsigned int endereco;
        char tipo;
        while (fscanf(arquivo, "%x %c", &endereco, &tipo) != EOF && acessostotal < AcessosMaximo) {
            listaacessos[acessostotal++] = endereco >> deslocamento;
        }
        fclose(arquivo);
        arquivo = fopen(argumentos[2], "r");
        if (!arquivo) {
            perror("Erro ao reabrir o arquivo");
            return 1;
        }
    }

    todosquadros = malloc(sizeof(Quadro) * totalquadros);
    for (int i = 0; i < totalquadros; i++) todosquadros[i].valido = 0;

    unsigned int addr;
    char acao;

    while (fscanf(arquivo, "%x %c", &addr, &acao) != EOF) {
        unsigned int pag = addr >> deslocamento;
        tempoagora++;

        int quadroencontrado = -1;
        for (int i = 0; i < totalquadros; i++) {
            if (todosquadros[i].valido && todosquadros[i].pagina == pag) {
                quadroencontrado = i;
                break;
            }
        }

        if (quadroencontrado != -1) {
            todosquadros[quadroencontrado].r = 1;
            if (acao == 'W') todosquadros[quadroencontrado].m = 1;
            todosquadros[quadroencontrado].acesso = tempoagora;
        } else {
            faltapagina++;

            int posicaovazia = -1;
            for (int i = 0; i < totalquadros; i++) {
                if (!todosquadros[i].valido) {
                    posicaovazia = i;
                    break;
                }
            }

            if (posicaovazia == -1) {
                if (strcmp(metodo, "LRU") == 0 || strcmp(metodo, "lru") == 0)
                    posicaovazia = escolherpaginaLRU();
                else if (strcmp(metodo, "2nd") == 0)
                    posicaovazia = escolherpaginaSegundo();
                else if (strcmp(metodo, "clock") == 0)
                    posicaovazia = escolherpaginaClock();
                else if (strcmp(metodo, "otimo") == 0)
                    posicaovazia = escolherpaginaOtimo();
                else {
                    printf("Metodo %s nao reconhecido\n", metodo);
                    exit(1);
                }
            }

            if (todosquadros[posicaovazia].valido && todosquadros[posicaovazia].m) paginasmodificadas++;

            todosquadros[posicaovazia].pagina = pag;
            todosquadros[posicaovazia].r = 1;
            todosquadros[posicaovazia].m = (acao == 'W');
            todosquadros[posicaovazia].acesso = tempoagora;
            todosquadros[posicaovazia].valido = 1;
        }

        if (strcmp(metodo, "otimo") == 0 && posicaoatual < acessostotal) posicaoatual++;
    }

    fclose(arquivo);

    printf("Arquivo de entrada: %s\n", argumentos[2]);
    printf("Tamanho da memória fisica: %u MB\n", memoriafisica);
    printf("Tamanho das páginas: %u KB\n", tamanhopagina);
    printf("Algoritmo de substituição: %s\n", metodo);
    printf("Número de Falta de Páginas: %lu\n", faltapagina);
    printf("Número de Páginas Escritas: %lu\n", paginasmodificadas);

    free(todosquadros);
    return 0;
}
