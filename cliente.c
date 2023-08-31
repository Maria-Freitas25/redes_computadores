include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <limits.h>
#include "netutil.h"
#include "split.h"
#include "ioutil.h"

int main(int argc, char *argv[])
{
    struct sockaddr_in *addr;
    int sd;
    int fd;
    int client_sd; // Declare o descritor de socket do cliente

    client_sd = sd;
    char comando[PATH_MAX + 64];
    char file_to_delete[PATH_MAX]; // Alteração: aumentar o tamanho do buffer
    split_list *sp;
    char path[PATH_MAX];

    msg_header *header;

    struct stat stats;

    int nread;

    /* Primeiro, é obtido o endereço IP do host a ser conectado. */
    if (argc < 3)
    {
        printf("\nFaltam parametros de chamada.\nUso:\n\n%s nome_host\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Criando o socket com o servidor */
    sd = socket(PF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        printf("Não foi possivel criar socket\n");
        exit(EXIT_FAILURE);
    }

    /* Obtenha o endereço do servidor */
    addr = address_by_ip(argv[1], atoi(argv[2]));

    /* Conectando-se ao  servidor */
    if (connect(sd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) != 0)
    {
        printf("Não foi possivel se conectar com o  servidor\n");
        exit(EXIT_FAILURE);
    }
    printf("Connected to server\n");

    /* Receber comandos do usuario */
    while (1)
    {
        memset(comando, 0, PATH_MAX + 64);
        /* Ler comando por entrada padrão */
        printf("> ");
        fgets(comando, PATH_MAX + 64, stdin);

        sp = split(comando, " \r\n");
        if (sp->count == 0)
        {
            // interação seguinte
            continue;
        }
        printf("Comando digitado pelo  usuario: %s\n", comando);

        fd = 0;
        memset(path, 0, PATH_MAX);

        if (strcmp(sp->parts[0], "quit") == 0)
        {
            // romper o ciclo
            break;
        }
        else if (sp->count != 2)
        {
            continue;
        }
        //arquivo padrão
        strcat(path, sp->parts[1]);
       /* Inicializa o cabeçalho da mensagem a ser enviada ao servidor */
        header = malloc(sizeof(msg_header));

        if (strcmp(sp->parts[0], "put") == 0)
        {
            /* Verifica a existencia do arquivo por meio do  stat() */
            if (stat(path, &stats) != 0)
            {
                printf("Arquivo não encontrado\n");
                printf("Comprove a rota a rota e o nomedo arquivo: %s\n", path);
                continue;
            }

            /* O arquivo está vazio */
            if (stats.st_size <= 0)
            {
                printf("O arquivo esta vazio\n");
                continue;
            }

            /* Crie o cabeçalho para enviar ao servidor */
            strcpy(header->request, "put");         // petição
            strcpy(header->file_name, sp->parts[1]); // nome do arquivo
            header->file_size = stats.st_size;       // tamanho do arquivo
            header->mode = stats.st_mode;            // permissões do arquivo

            /* Envia a petição put para servidor */
            send(sd, header, sizeof(msg_header), 0);

            /* Abre o arquivo em modo leitura */
            fd = open(path, O_RDONLY);

            /* verifique se o arquivo pode ser aberto */
            if (fd < 0)
            {
                printf("Não é possível abrir o arquivo\n");
                continue;
            }

            /* Leia o conteúdo do arquivo em partes e envie-o para o servidor */
            printf("Enviando o arquivo (%d): %s\n", header->file_size, header->file_name);
            send_to_socket(sd, fd, stats.st_size);

            /* Fechar o arquivo */
            close(fd);
            printf("Arquivo enviado para o  servidor\n");
        }
        else if (strcmp(sp->parts[0], "get") == 0)
        {
            /* crie o cabeçalho para enviar ao servidor */
            strcpy(header->request, "get");         // petição
            strcpy(header->file_name, sp->parts[1]); // nome do arquivo

            /* Informa ao servidor que ele precisa receber um arquivo */
            send(sd, header, sizeof(msg_header), 0);

            /* Recebe o tamanho de arquivo solicitado */
            nread = read(sd, header, sizeof(msg_header));
            if (nread <= 0 || header->file_size <= 0)
            {
                printf("Não é possivel receber o arquivo\n");
                continue;
            }

            printf("Arquivo recebido (%d),aguardando em: %s\n", header->file_size, path);
            /**
             * Abre o arquivo em modo leitura/escritura
             * é criado se não existir ou for sobrescrito
             * as mesmas permissões do arquivo original são atribuídas
             */
            fd = open(path, O_RDWR | O_CREAT | O_TRUNC, header->mode);

            /* Verifica se o arquivo pode ser criado e/ou gravado */
            if (fd < 0)
            {
                printf("Não é possível salvar o arquivo\n");
                continue;
            }

            /* O conteúdo do arquivo é lido em partes (do soquete) */
            write_from_socket(sd, fd, header->file_size);

            /* fechar o  arquivo */
            close(fd);
            printf("O arquivo foi salvo com sucesso\n");
        }
else if (strcmp(header->command, "BIN") == 0) 
{
    /* Lidar com o modo binário aqui */

    /* Abra um arquivo no servidor para armazenar o arquivo binário */
    char server_path[PATH_MAX+64]; // Defina o caminho onde o arquivo será armazenado
    snprintf(server_path, sizeof(server_path), "./arquivos/%s", header->file_name);
    
    int server_fd = open(server_path, O_RDWR | O_CREAT | O_TRUNC , header->mode);
    if (server_fd < 0)
   {
        printf("Não foi possível criar o arquivo no servidor\n");
        continue; // Lida com o próximo cliente
    }

    /* Receba o arquivo binário do cliente e grave-o no servidor */
    write_from_socket(client_sd, server_fd, header->file_size);

    /* Feche o arquivo no servidor após a gravação */
    close(server_fd);

    printf("Arquivo binário (%s) recebido e armazenado no servidor\n", header->file_name);
} 
else if (strcmp(header->command, "ASCII") == 0) 
{
    /* Lidar com o modo ASCII aqui */

    /* Abra um arquivo no servidor para armazenar o arquivo em modo ASCII */
    char server_path[PATH_MAX+64]; // Defina o caminho onde o arquivo será armazenado
    snprintf(server_path, sizeof(server_path), "./arquivos/%s", header->file_name);
    
    int server_fd = open(server_path, O_RDWR | O_CREAT | O_TRUNC, header->mode);
    if (server_fd < 0) 
    {
        printf("Não foi possível criar o arquivo no servidor\n");
        continue; // Lida com o próximo cliente
    }

    /* Receba o arquivo em modo ASCII do cliente e converta para binário antes de gravar */
    char buffer[BUFSIZ];
    int bytes_received;
    while (header->file_size > 0) 
    {
        int to_receive = (header->file_size < BUFSIZ) ? header->file_size : BUFSIZ;
        bytes_received = recv(client_sd, buffer, to_receive, 0);
        if (bytes_received < 0) 
        {
            printf("Erro ao receber dados do cliente\n");
            break;
        }
        else if (bytes_received == 0)
        {
            printf("Cliente fechou a conexão prematuramente\n");
            break;
        }

        /* Aqui você deve implementar a conversão de ASCII para binário antes de escrever no arquivo */
        /* Exemplo de conversão simples: substituir '\n' por '\r\n' */
        for (int i = 0; i < bytes_received; i++)
        {
            if (buffer[i] == '\n')
            {
                char crlf[] = "\r\n";
                write(server_fd, crlf, 2);
            } else {
                write(server_fd, &buffer[i], 1);
            }
        }

        header->file_size -= bytes_received;
    }

    /* Feche o arquivo no servidor após a gravação */
    close(server_fd);

    printf("Arquivo em modo ASCII (%s) recebido e armazenado no servidor\n", header->file_name);
} 
else if (strcmp(header->command, "ls") == 0) {
    /* Lidar com o comando LS aqui */

    // Defina o caminho do diretório que deseja listar
    char directory_path[PATH_MAX+64]; 
  //  snprintf(directory_path, sizeof(directory_path), "./arquivos");
    
    // Tente abrir o diretório
    DIR *dir = opendir(directory_path);
    if (!dir) {
        printf("Não foi possível abrir o diretório\n");
        continue; // Lida com o próximo cliente
    }

    // Envie cada nome de arquivo de volta para o cliente
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        // Ignorar entradas especiais "." e ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Envie o nome do arquivo de volta para o cliente
        send(client_sd, entry->d_name, strlen(entry->d_name), 0);
        send(client_sd, "\n", 1, 0);
    }

    // Feche o diretório
    closedir(dir);

    // Envie um marcador de fim de lista para o cliente
    send(client_sd, "FIM_DA_LISTA\n", strlen("FIM_DA_LISTA\n"), 0);

    printf("Lista de arquivos enviada para o cliente\n");
}else if (strcmp(header->command, "DELETE") == 0) {
    /* Lidar com o comando DELETE aqui */

    // Construa o caminho completo do arquivo a ser excluído
    char file_to_delete[PATH_MAX+64];
    snprintf(file_to_delete, sizeof(file_to_delete), "./arquivos/%s", header->file_name);

    // Tente excluir o arquivo
    if (remove(file_to_delete) == 0) {
        // A exclusão foi bem-sucedida
        printf("Arquivo %s excluído\n", header->file_name);
        send(client_sd, "DELETE_OK\n", strlen("DELETE_OK\n"), 0);
    } else {
        // A exclusão falhou
        printf("Não foi possível excluir o arquivo %s\n", header->file_name);
        send(client_sd, "DELETE_ERROR\n", strlen("DELETE_ERROR\n"), 0);
    }
}  
 else
        {
            printf("Comando no valido!\n");
            continue;
         // siguiente iteracion
        }
    }

    printf("Connection terminated\n");
    close(sd);
}
