#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#include "netutil.h"
#include "split.h"
#include "ioutil.h"

#define MAX_CLIENTS 10
#define MAX_FILES 50

sem_t clients_sem;
sem_t cli_arr_mutex;
sem_t f_arr_mutex;

typedef struct {
    char file_name[PATH_MAX];
    sem_t file_mutex;
}file_state;

typedef struct {
    file_state *file_array[MAX_FILES];
    int size;
}file_state_array;

int client_array[MAX_CLIENTS];
file_state_array *fs_array;


void sig_handler(int SIGNO);

int get_next_client();

int is_file_free(char *);

void close_client(int);

void close_server();

file_state *get_file_state(char *);

void *connection_handler(void *);



int main(int argc, char *argv[])
{
    if (signal(SIGINT, sig_handler) == SIG_ERR)
    {
        printf("Não é possível gerenciar o sinal SIGINT\n");
    }

    struct sockaddr_in *addr;
    int sd;
    int client_sd;

    if (argc != 2)
    // pthread_create(&pt, NULL, connection_handler, pclient);
    {
        fprintf(stderr, "Especifique o porto\n");
        exit(EXIT_FAILURE);
    }

    /* Criando o socket no servidor */
    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        printf("Não é possível utilizar socket\n");
        exit(EXIT_FAILURE);
    }

    /* Obter um diretório e um porto para escutar */
    addr = server_address(atoi(argv[1]));

    /* Insira o soquete na direção */
    if (bind(sd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)
    {
        printf("Operação bind falida\n");
        exit(EXIT_FAILURE);
    }

    /* Escutando o socket aberto */
    if (listen(sd, 10) < 0)
    {
        printf("Não é possivel escutar o socket\n");
        exit(EXIT_FAILURE);
    }

    printf("Servidor escutando pelo porto: %s\n", argv[1]);

    sem_init(&clients_sem, 0, MAX_CLIENTS);
    sem_init(&cli_arr_mutex, 0, 1);
    sem_init(&f_arr_mutex, 0, 1);
    memset(client_array,0,MAX_CLIENTS);
    fs_array = malloc(sizeof(file_state_array));
    fs_array->size = 0;

    /* todas as solicitações são tratadas até que o processo seja concluído */
    while (1)
    {
        printf("Esperando conexões...\n");

        /*Aguarde até que um cliente se conecte */
        client_sd = accept(sd, 0, 0);
        if (client_sd < 0)
        {
            printf("Não é possível aceitar o cliente\n");
            continue;
        }
        printf("Cliente %d conectado\n", client_sd);

        pthread_t pt;
        int *pclient = malloc(sizeof(int));
        *pclient = client_sd;

        int index = get_next_client();

        sem_wait(&clients_sem);
        sem_wait(&cli_arr_mutex);
        client_array[index] = client_sd;
        sem_post(&cli_arr_mutex);

        pthread_create(&pt, NULL, connection_handler, pclient);
    }
}

int get_next_client(){
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (client_array[i] == 0){
            return i;
        }
    }
    return -1;
}

void close_client(int client){
    
    for (int i = 0; i < MAX_CLIENTS; i++){
        if (client == client_array[i])
        {
            close(client);
            client_array[i] = 0;
            break;
        }
    }
    sem_post(&clients_sem);

}

void close_server(){
    int i = 0;
    sem_wait(&cli_arr_mutex);

    while (client_array[i] != 0 && i < MAX_CLIENTS){
        close_client(client_array[i]);
        i++;
    }

    sem_post(&cli_arr_mutex);
}

file_state *get_file_state(char *file_name)
{
    file_state *fs = 0;

    sem_wait(&f_arr_mutex);

    for (int i = 0; i < fs_array->size; i++)
    {
        if (strcmp(file_name, fs_array->file_array[i]->file_name) == 0)
        {
            fs = fs_array->file_array[i];
        }
    }

    if (fs_array->size < MAX_FILES && fs == 0 && file_name != 0)
    {
        fs = malloc(sizeof(file_state));
        sem_init(&fs->file_mutex, 0, 1);
        strcpy(fs->file_name, file_name);
        fs_array->file_array[fs_array->size++] = fs;
    }

    sem_post(&f_arr_mutex);
    return fs;
}

void *connection_handler(void *client_socket)
{
    struct stat stats;
    int fd;

    char path[PATH_MAX];

    msg_header *header;

    int nread;
    int client_sd = *(int *)client_socket;

    free(client_socket);

    while (1)
    {
        /* Receber uma solicitação para enviar ou receber um arquivo do cliente */
        header = malloc(sizeof(msg_header));
        nread = read(client_sd, header, sizeof(msg_header));
        if (nread <= 0)
        {
            break;
        }

        printf("Ler do processo do cliente %d (%d bytes): %s %s\n", client_sd, nread, header->request, header->file_name);

        /* Inicialize em 0 */
        fd = 0;
        memset(path, 0, PATH_MAX);

        /* Crie um path para guardar e recuperar arquivos */
        //strcpy(path, "./archivos/");
        strcat(path, header->file_name);

        file_state *fs = get_file_state(header->file_name);
        if (fs == 0){
            continue;
        }

        sem_wait(&fs->file_mutex);

        /* Receber arquivo do cliente */
        if (strcmp(header->request, "put") == 0)
        {
            printf("Archivo recibido (%d), guardando en: %s\n", header->file_size, path);
            /**
             * Abre o arquivo em modo de leitura ou escrita
             * Permissões  de escritura e leitura */
            fd = open(path, O_RDWR | O_CREAT | O_TRUNC, header->mode);

            /* Se comprova que pode criar e guardar o arquivo */
            if (fd < 0)
            {
                printf("Não é possível guardar o arquivo\n");
                continue;
            }

            /* Se lê por partes (os sockets) contidos no arquivo */
            write_from_socket(client_sd, fd, header->file_size);

            /*Encerra o arquivo */
            close(fd);

            printf("O arquivo foi guardado com êxito\n");
        }
        else if (strcmp(header->request, "get") == 0)
        {

            /*verifica o arquivo mediante o stat() */
            if (stat(path, &stats) != 0)
            {
                printf("Não pode verificar o arquivo\n");
                printf("Comprove a rota do arquivo: %s\n", path);
            }

            /* Recupera o tamanho do arquivo  mediante stat*/
            header->file_size = stats.st_size;
            header->mode = stats.st_mode;

            /*Envia ao cliente o tamanho do arquivo */
            send(client_sd, header, sizeof(msg_header), 0);

            /*Comprova se o arquivo está vazio */
            if (stats.st_size <= 0)
            {
                continue;
            }
            /* Abre o arquivo */
            fd = open(path, O_RDONLY);

            /* Comprove se o arquivo pode ser aberto */
            if (fd <= 0)
            {
                continue;
            }

            printf("Enviando arquivo (%d): %s\n", header->file_size, header->file_name);

            /*Lê por partes o arquivo e envia para o cliente */
            send_to_socket(client_sd, fd, header->file_size);

            /* Encerra o arquivo */
            close(fd);


            printf("Arquivo enviado ao cliente: %d\n", client_sd);
        }
        else
        {
            printf("Não é possivel reconhecer o comando\n");
        }
            sem_post(&fs->file_mutex);
    }
    printf("Conexão terminada\n");

    /* Encerra a conexão */
    sem_wait(&cli_arr_mutex);
    close_client(client_sd);
    sem_post(&cli_arr_mutex);

    return 0;
}

void sig_handler(int SIGNO)
{
    if (SIGNO == SIGINT)
    {
        printf("\nTerminado o processo no servidor\n");
        close_server();
        exit(EXIT_SUCCESS);
    }
}
