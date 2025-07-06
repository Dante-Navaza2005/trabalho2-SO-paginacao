#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define QuadrosMaximo 262144 
#define LinhaMaxima 100 
#define AcessosMaximo 1000000

typedef struct {
    int pagina; // número da página
    int r; // bit de referência
    int m; // bit de modificação
    unsigned long acesso; // tempo de acesso
    int valido; // fala se o quadro está válido
} Quadro;

Quadro* todosquadros;
unsigned int totalquadros; // total de quadros na memória
unsigned int tamanhopagina; // tamanho da página em KB
unsigned int memoriafisica; // tamanho da memória física em MB
unsigned long tempoagora = 0; // tempo atual do simulador
unsigned long faltapagina = 0; // número de faltas de página
unsigned long paginasmodificadas = 0; // número de páginas modificadas  

char metodo[20]; // método de substituição de página
int ponteiroatual = 0;
unsigned int listaacessos[AcessosMaximo];
int acessostotal = 0;
int posicaoatual = 0;

// Funções para escolher a página a ser substituída
int escolherpaginaLRU() {
    int menor = 0;
    // Encontra o quadro com o menor tempo de acesso
    for (int i = 1; i < totalquadros; i++) {
        if (todosquadros[i].acesso < todosquadros[menor].acesso) menor = i;
    }
    return menor;
}

// Função para escolher a página a ser substituída pelo algoritmo de segundo chance
int escolherpaginaSegundo() {
    static int pos = 0;
    // Percorre os quadros procurando um quadro com bit de referência 0 
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

// Função para escolher a página a ser substituída pelo algoritmo de relógio
int escolherpaginaClock() {
    // Percorre os quadros procurando um quadro com bit de referência 0
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

// Função para escolher a página a ser substituída pelo algoritmo ótimo
int escolherpaginaOtimo() {
    int maislonge = -1;
    int indice = -1;
    // Percorre os quadros procurando a página que será acessada mais tarde
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

// Função para converter texto em valor inteiro
int convertervalor(char* texto) {
    char* fim;
    long valor = strtol(texto, &fim, 10); // Converte o texto para long 
    // Verifica se a conversão foi bem-sucedida
    if (*fim != '\0' || valor < 0) {
        printf("Valor inválido: %s\n", texto);
        exit(1);
    }
    return (int)valor;
}

int main(int quantidadeargumentos, char* argumentos[]) {
    // Verifica se o número de argumentos é correto
    if (quantidadeargumentos != 5) {
        printf("Uso correto: %s metodo arquivo tamanhopagina memoria\n", argumentos[0]);
        return 1;
    }

    printf("prompt> sim-virtual %s %s %s %s\n", argumentos[1], argumentos[2], argumentos[3], argumentos[4]);
    printf("Executando simulador...\n");

    strcpy(metodo, argumentos[1]); // Copia o método de substituição
    tamanhopagina = convertervalor(argumentos[3]); // Converte o tamanho da página de KB para bytes
    
    if (tamanhopagina != 8 && tamanhopagina != 32) {
        printf("Tamanho de página inválido. Apenas 8 KB ou 32 KB são suportados.\n");
        return 1;
    }
    
    memoriafisica = convertervalor(argumentos[4]); // Converte o tamanho da memória física de MB para bytes
    if (memoriafisica != 1 && memoriafisica != 2) {
        printf("Tamanho de memória física inválido. Apenas 1 MB ou 2 MB são suportados.\n");
        return 1;
    }
    
    totalquadros = (memoriafisica * 1024) / tamanhopagina; // Calcula o número total de quadros

    FILE* arquivo = fopen(argumentos[2], "r"); 
    if (!arquivo) {
        perror("Erro ao abrir o arquivo");
        return 1;
    }

    // Verifica se o número de quadros é válido
    int deslocamento = (int)log2(tamanhopagina * 1024);

    // Verifica se o número de quadros não excede o máximo permitido
    if (strcmp(metodo, "otimo") == 0) {
        unsigned int endereco;
        char tipo;
        // Lê os endereços do arquivo e armazena na lista de acessos
        while (fscanf(arquivo, "%x %c", &endereco, &tipo) != EOF && acessostotal < AcessosMaximo) {
            listaacessos[acessostotal++] = endereco >> deslocamento;
        }
        fclose(arquivo);
        // Reabre o arquivo para leitura novamente
        arquivo = fopen(argumentos[2], "r");
        if (!arquivo) {
            perror("Erro ao reabrir o arquivo");
            return 1;
        }
    }
    // Aloca memória para os quadros
    todosquadros = malloc(sizeof(Quadro) * totalquadros);
    // Verifica se a alocação foi bem-sucedida
    for (int i = 0; i < totalquadros; i++) todosquadros[i].valido = 0;

    unsigned int addr;
    char acao;

    // Lê os endereços do arquivo e processa as ações
    while (fscanf(arquivo, "%x %c", &addr, &acao) != EOF) {
        unsigned int pag = addr >> deslocamento; // Obtém o número da página
        tempoagora++;

        int quadroencontrado = -1;
        // Procura se a página já está na memória
        for (int i = 0; i < totalquadros; i++) {
            if (todosquadros[i].valido && todosquadros[i].pagina == pag) {
                quadroencontrado = i;
                break;
            }
        }
        // Se a página já está na memória, atualiza os bits de referência e modificação
        if (quadroencontrado != -1) {
            todosquadros[quadroencontrado].r = 1;
            // Atualiza o bit de modificação se a ação for de escrita
            if (acao == 'W') todosquadros[quadroencontrado].m = 1;
            todosquadros[quadroencontrado].acesso = tempoagora;
        } else {
            faltapagina++;

            int posicaovazia = -1;
            // Procura por um quadro vazio
            for (int i = 0; i < totalquadros; i++) {
                if (!todosquadros[i].valido) {
                    posicaovazia = i;
                    break;
                }
            }
            // Se não encontrou um quadro vazio, escolhe uma página para substituir
            if (posicaovazia == -1) {
                // Se não encontrou um quadro vazio, escolhe uma página para substituir
                if (strcmp(metodo, "LRU") == 0 || strcmp(metodo, "lru") == 0)
                    posicaovazia = escolherpaginaLRU();

                // Se o método for segundo chance, relógio ou ótimo, chama as funções correspondentes
                else if (strcmp(metodo, "2nd") == 0 || strcmp(metodo, "second") == 0)
                    posicaovazia = escolherpaginaSegundo();
                // Se o método for relógio, chama a função correspondente
                else if (strcmp(metodo, "clock") == 0)
                    posicaovazia = escolherpaginaClock();
                // Se o método for ótimo, chama a função correspondente
                else if (strcmp(metodo, "2nd") == 0)
                    posicaovazia = escolherpaginaSegundo();
                // Se o método for relógio, chama a função correspondente
                else if (strcmp(metodo, "clock") == 0)
                    posicaovazia = escolherpaginaClock();
                // Se o método for ótimo, chama a função correspondente
                else if (strcmp(metodo, "otimo") == 0)
                    posicaovazia = escolherpaginaOtimo();
                // Se o método não for reconhecido, exibe uma mensagem de erro
                else {
                    printf("Metodo %s nao reconhecido\n", metodo);
                    exit(1);
                }
            }

            // Se o quadro escolhido é válido, verifica se ele foi modificado
            if (todosquadros[posicaovazia].valido && todosquadros[posicaovazia].m) paginasmodificadas++;

            todosquadros[posicaovazia].pagina = pag; // Atualiza o número da página
            todosquadros[posicaovazia].r = 1; // Define o bit de referência como 1
            todosquadros[posicaovazia].m = (acao == 'W'); // Define o bit de modificação se a ação for de escrita
            todosquadros[posicaovazia].acesso = tempoagora; // Atualiza o tempo de acesso
            todosquadros[posicaovazia].valido = 1; // Marca o quadro como válido
        }
        // Se o método for ótimo, incrementa a posição atual
        if (strcmp(metodo, "otimo") == 0 && posicaoatual < acessostotal) posicaoatual++;
    }

    fclose(arquivo);

    printf("Arquivo de entrada: %s\n", argumentos[2]);
    printf("Tamanho da memória fisica: %u MB\n", memoriafisica);
    printf("Tamanho das páginas: %u KB\n", tamanhopagina);
    printf("Algoritmo de substituição: %s\n", metodo);
    printf("Número de Falta de Páginas: %lu\n", faltapagina);
    printf("Número de Páginas Escritas: %lu\n", paginasmodificadas);

    free(todosquadros); // Libera a memória alocada para os quadros
    return 0;
}
