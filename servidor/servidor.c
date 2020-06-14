#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define serv_porta 1500
#define tam_buffer 1024

typedef struct bloco{
    int porta_cliente;
    char nome_arquivo[30];
} bloco;

int main(){
    
    int sd, flag;
    socklen_t cliente_tam;
    struct sockaddr_in addr_servidor, addr_cliente;
    char *buffer, nome_arquivo[30], ip_cliente[20], ip_cliente_arquivo[20];
    buffer = (char*) malloc(tam_buffer * sizeof(char));

    if(!buffer){
        printf("Não foi possível alocar memória para o buffer!\n");
    }

    FILE *banco_de_dados, *banco_de_dados_aux;

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
    addr_servidor.sin_port = htons(serv_porta);
    addr_servidor.sin_addr.s_addr = htonl(INADDR_ANY);

    //Bind no socket
    if(bind(sd,(struct sockaddr *)&addr_servidor , sizeof(addr_servidor))<0){
        printf("Erro ao realizar bind no socket!\n");
        return -1;
    }

    ///aguardando conexoes
    listen(sd, 2);
    printf("Aguardando transições...\n");
    
    while(1){
        
        memset(buffer,'\0', tam_buffer);
        
        cliente_tam = sizeof(addr_cliente);
        
        if ((recvfrom(sd, buffer, tam_buffer, 0, (struct sockaddr *) &addr_cliente, &cliente_tam)) < 0){
            printf("Erro em receber a mensagem do cliente!\n");
            return -1;
        }

        //se o buffer não for vazio, quer dizer que ele recebeu o nome do arquivo do clienteA
        if(buffer[0]!='\0'){
            
            //vai para o final do arquivo de banco de dados
            fseek(banco_de_dados, 0, SEEK_END);
            
            //checa se o arquivo está vazio
            if (ftell(banco_de_dados) == 0){
                printf("Banco de dados vazio!\n");
                printf("Por favor, insira os arquivos manualmente no arquivo txt!\n");
                printf("Formato: NOMEDOARQUIVO IP_CLIENTE_QUE_POSSUI_ARQUIVO");
                return -1;
            }

            //volta para o começo do arquivo
            fseek(banco_de_dados, 0, SEEK_SET);

            //procura o arquivo no banco de dados
            while((fscanf(banco_de_dados, "%s %s\n", nome_arquivo, ip_cliente)) != EOF){

                if((strcmp(nome_arquivo, buffer))== 0){
                    strcpy(ip_cliente_arquivo, ip_cliente);
                    flag = 1;
                }
                
            }

            memset(buffer,'\0', tam_buffer);
            
            //se o arquivo requisitado pelo clienteA estiver no banco de dados
            if(flag == 1){
                
                buffer[0] = '1';

                strcat(buffer,ip_cliente_arquivo);
                
                printf("Arquivo encontrado no IP: %s\n", ip_cliente_arquivo);

                if (sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *) &addr_cliente, cliente_tam) < 0){
                    printf("Erro ao enviar a mensagem pro cliente!\n");
                    return -1;
                }

                printf("IP do cliente que possui o arquivo enviado com sucesso\n");

            }
            else{

                //não achou o arquivo no banco de dados
                char error[] = "Erro";
                strcat(buffer, error);

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

    //atualiza o banco de dados com a porta do cliente que recebeu o arquivo
    bloco blk;

    while(1){

        FILE *banco_de_dados;
        memset(&blk,0x0, sizeof(bloco));
        memset(buffer,'\0', tam_buffer);

        banco_de_dados = fopen("banco_de_dados.txt", "r+");
        banco_de_dados_aux = fopen("banco_de_dados1.txt", "w+");

        if(!banco_de_dados){
            printf("Nao foi possivel abrir o arquivo do bando de dados!\n");
            return -1;
        }

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

        int posicao_arquivo = 0;
        fseek(banco_de_dados, 0, SEEK_SET);

        //procura o arquivo no banco de dados
        while((fscanf(banco_de_dados, "%s %s\n", nome_arquivo, ip_cliente)) != EOF){

            //posicao_arquivo++;

            printf("%s %s\n", nome_arquivo, ip_cliente);

            //atualiza a linha do arquivo no banco de dados
            if((strcmp(nome_arquivo, blk.nome_arquivo))== 0){
                
                break;
                //strcpy(ip_cliente, blk.porta_cliente);
                //fprintf(banco_de_dados_aux, "%s %d\n", nome_arquivo, blk.porta_cliente);
            }else{
                //fprintf(banco_de_dados_aux, "%s %s\n", nome_arquivo, ip_cliente);
            }
                
        }

        
        fflush(banco_de_dados);
        /*
        //move o arquivo para a linha onde está o nome do arquivo
        fseek(banco_de_dados, posicao_arquivo, SEEK_SET);

        printf("%s %d %d\n", blk.nome_arquivo, blk.porta_cliente, posicao_arquivo);
        
        //reescreve a linha com a porta atualizada do cliente A
        //fprintf(banco_de_dados, "%s %d\n", blk.nome_arquivo, blk.porta_cliente);
        
        fwrite(&blk, 1, sizeof(blk), banco_de_dados);

        fflush(banco_de_dados); 

        printf("\nBanco de dados atualizado com sucesso!\n");
        fclose(banco_de_dados);
        */
        break;
    }
}