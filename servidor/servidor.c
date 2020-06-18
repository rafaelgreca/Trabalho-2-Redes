/*
Rafael Greca Vieira - 2018000434
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define porta_servidor 1500
#define tamanho_buffer 1024
#define porta_clienteb 1502
#define porta_clientea 1501

//estrutura do bloco de resposta do cliente A pro servidor
typedef struct bloco{
    int porta_cliente;          //porta do cliente que possui o pacote
    char nome_arquivo[30];      //nome do arquivo que ele possui
} bloco;

int main(){
    
    //variáveis
    int sd, flag = 0;
    socklen_t cliente_tam;
    struct sockaddr_in addr_servidor, addr_cliente;
    char *buffer, nome_arquivo[30], porta_cliente[7], porta_cliente_arquivo[7];
    
    buffer = (char*) malloc(tamanho_buffer * sizeof(char));

    //aloca memória para o buffer
    if(!buffer){
        printf("Não foi possível alocar memória para o buffer!\n");
    }

    FILE *banco_de_dados;

    //Abre o arquivo do banco de dados
    banco_de_dados = fopen("banco_de_dados.txt", "rb");

    //Criação do socket local
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0){
        printf("Erro ao criar o socket!\n");
        return -1;
    }

    //Dados do socket do servidor
    memset(&addr_servidor, 0, sizeof(addr_servidor));
    addr_servidor.sin_family = AF_INET;
    addr_servidor.sin_port = htons(porta_servidor);
    addr_servidor.sin_addr.s_addr = inet_addr("127.0.0.1");

    //Bind no socket
    if(bind(sd,(struct sockaddr *)&addr_servidor , sizeof(addr_servidor))<0){
        printf("Erro ao realizar bind no socket!\n");
        return -1;
    }

    //Aguardando as conexoes
    listen(sd, 2);
    printf("Aguardando transições...\n");
    
    //---------COMUNICAÇÃO COM O CLIENTE A---------
    while(1){
        
        memset(buffer,'\0', tamanho_buffer);
        
        cliente_tam = sizeof(addr_cliente);
        
        //recebe a mensagem do cliente A e armazena no buffer
        if ((recvfrom(sd, buffer, tamanho_buffer, 0, (struct sockaddr *) &addr_cliente, &cliente_tam)) < 0){
            printf("Erro em receber a mensagem do cliente!\n");
            return -1;
        }

        //se o buffer não for vazio, quer dizer que ele recebeu o nome do arquivo do clienteA
        if(buffer[0] != '\0'){
            
            //vai para o final do arquivo de banco de dados
            fseek(banco_de_dados, 0, SEEK_END);
            
            //checa se o arquivo está vazio
            if (ftell(banco_de_dados) == 0){
                printf("Banco de dados vazio!\n");
                printf("Por favor, insira os arquivos manualmente no arquivo txt!\n");
                printf("Formato: NOMEDOARQUIVO PORTA_CLIENTE_QUE_POSSUI_ARQUIVO\n");
                printf("Exemplo: teste.txt 10309\n");
                return -1;
            }

            //volta para o começo do arquivo
            fseek(banco_de_dados, 0, SEEK_SET);

            //procura o arquivo no banco de dados
            while((fscanf(banco_de_dados, "%s %s\n", nome_arquivo, porta_cliente)) != EOF){

                //arquivo foi encontrado, então salva o ip do cliente
                if((strcmp(nome_arquivo, buffer))== 0){
                    strcpy(porta_cliente_arquivo, porta_cliente);
                    flag = 1;
                }
                
            }

            memset(buffer,'\0', tamanho_buffer);
            
            //se o arquivo requisitado pelo clienteA estiver no banco de dados
            if(flag == 1){
                
                buffer[0] = '1';

                //copia a porta do cliente para o buffer
                strcat(buffer,porta_cliente_arquivo);
                
                printf("Arquivo encontrado na porta: %s\n", porta_cliente_arquivo);

                //envia a porta do cliente que possui o arquivo para o cliente A
                if (sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &addr_cliente, cliente_tam) < 0){
                    printf("Erro ao enviar a mensagem pro cliente!\n");
                    return -1;
                }

                printf("Porta do cliente que possui o arquivo enviado com sucesso\n");

            }
            else{

                //não achou o arquivo no banco de dados
                char error[] = "Erro";
                strcat(buffer, error);

                //envia a mensagem de erro (armazenada no buffer) para o cliente A
                if (sendto(sd, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, cliente_tam) < 0){
                    printf("Erro ao enviar a mensagem pro cliente!\n");
                }

                printf("Arquivo nao foi localizado no banco de dados!\n");
                return -1;
            }
        }

        fclose(banco_de_dados);
        break;
    }

    //---------ATUALIZAÇÃO DO BANCO DE DADOS---------
    //atualiza o banco de dados com a porta do cliente que recebeu o arquivo
    bloco blk;

    while(1){

        FILE *banco_de_dados;
        memset(&blk,0x0, sizeof(bloco));
        memset(buffer,'\0', tamanho_buffer);

        banco_de_dados = fopen("banco_de_dados.txt", "a+");

        //erro ao tentar abrir o banco de dados
        if(!banco_de_dados){
            printf("Nao foi possivel abrir o arquivo do bando de dados!\n");
            return -1;
        }

        //recebe o bloco de resposta do cliente A
        if ((recvfrom(sd, &blk, sizeof(blk), 0, (struct sockaddr *) &addr_cliente, &cliente_tam)) < 0){
            printf("Erro em receber a mensagem do cliente A!\n");
            return -1;
        }

        printf("\nO cliente de porta %d agora possui o arquivo %s!\n", blk.porta_cliente, blk.nome_arquivo);
        printf("Aguarde enquanto o banco de dados eh atualizado...\n");

        strcpy(buffer, "1");

        //manda uma mensagem de confirmação pro cliente A
        if ((sendto(sd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr_cliente, cliente_tam)) < 0){
            printf("Erro em enviar a mensagem do cliente A!\n");
            return -1;
        }

        //atualiza o banco de dados

        //escreve no arquivo
        fprintf(banco_de_dados, "\n%s %d", blk.nome_arquivo, blk.porta_cliente);

        //limpa os buffers do arquivo
        fflush(banco_de_dados);
        
        printf("\nBanco de dados atualizado com sucesso!\n");
        fclose(banco_de_dados);
        
        break;
    }
}