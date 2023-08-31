# redes_computadores
Projeto de Implementação de Redes de Computadores

Especificação do Trabalho
Título: Cliente-Servidor FTP (Protocolo)
Objetivo: Desenvolver uma aplicação cliente-servidor FTP
Composição do grupo: 03 pessoas
Apresentação
Os integrantes do grupo deverão entregar um documento contendo a descrição do trabalho,
juntamente com todas as referências utilizadas para desenvolvê-lo. Nesse documento, cada parte do
trabalho (cliente, servidor e protocolo) deve ser descrita. Nessa descrição devem ser informadas toda
e qualquer referência utilizada, problemas e desafios encontrados e as soluções adotadas para
contornar esses problemas e desafios.
Os alunos devem estar prontos para apresentar e defender sua implementação (código fonte).

1. Servidor
a) Deve fornecer sessões FTP no modo ativo;
b) Deve aguardar requisições de clientes na porta 20.001/TCP para comandos, controle
e respostas. A porta 20.000/TCP deve ser para transferência de dados (canal de
dados reverso), que deve ser estabelecido pelo servidor;
c) Deve fornecer alguma mensagem, texto ou imagem, informando se a conexão foi
estabelecida com sucesso ou não;
d) Deve permitir o mínimo de 02 sessões e o máximo de 10 FTP (02-10 clientes FTP)
simultaneamente;
I. quando exceder a capacidade máxima de usuários simultâneos, deve ser
exibida a mensagem “Número de usuários excedido”;
e) Deve guardar o endereço socket do cliente quando receber mensagem de requisição
de login;
f) Deve guardar o nome do usuário;
g) Deve verificar, para cada comando recebido o endereço socket de origem;
h) Deve fornecer dois tipos de transferência
I. ASCII para arquivos texto
II. BINARY para arquivos binários;
i) A transferência de arquivos deve se dar via TCP.

2. Cliente
a) Deve solicitar conexão no porto 20.001/TCP
b) Deve possuir dois threads: um para comandos, controle e respostas, outro para
transferência de arquivos
c) O prompt de comando deve fornecer o nome da máquina e o diretório do servidor,
por exemplo: maquina01:/data/>

4. Formato da Mensagem
a) Os pacotes FTP devem ter a seguinte estrutura:
I. Campo para código do pacote (número de sequência)
II. Campo para código do comando
III. Campo para transferência de arquivo

6. Comandos
a) Devem ser suportados os seguintes comandos
I. QUIT
II. GET
III. PUT
IV. BIN
V. ASCII
VI. LS
VII. DELETE

8. Dicas
a) 1ª Etapa
I. Cliente e Servidor ECHO
 Servidor FTP: servidor ECHO
 Cliente FTP: cliente ECHO com dois threads (um para comandos,
controle e mensagens; outro para transferência de arquivo)
II. Implementação do Protocolo
 Servidor FTP: atendimento de um único cliente
 Cliente FTP: com dois threads (um para comandos, controle e
mensagens; outro para transferência de arquivo)
b) 2ª Etapa: Final
I. Servidor FTP: atendimento de no mínimo 02 e no máximo 10 clientes
II. Manter tabelas de controle no servidor
 Manter uma tabela de controle com uma entrada para cada cliente
 Para cada cliente armazenar, no mínimo:
 Login
 Endereço socket do cliente
