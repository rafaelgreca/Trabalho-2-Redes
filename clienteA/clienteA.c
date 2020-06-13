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
#define tam_buffer 1024

typedef struct pacote{
	int numseq;     				//Armazena o numero sequencial, identificador do pacote
	long int check_sum;				//Armazena o checksum do pacote
	char segmento[256];				//Armazena um segmento do arquivo que sera enviado
	int flag;						//Valor 1 = CONTINUAR | valor 0 = FINALIZAR
	int tam;
} pacote;

typedef struct bloco{
    int porta_cliente;
    char nome_arquivo[30];
} bloco;

int receberArquivo(int sd, struct sockaddr_in addr_cliente, socklen_t addr_tam, char nomearq[]){

    pacote pkt;
    FILE *arquivo_transferido;
    char *buffer, ack[] = "ack", *aux;
    int contador_pacote = 0;

    buffer = (char *) malloc(tam_buffer * sizeof(char));
    aux = (char *) malloc(tam_buffer * sizeof(char));

    if(!buffer){
        printf("Nao foi possivel alocar memoria para o buffer!\n");
    }

    arquivo_transferido = fopen(nomearq, "wb");

    if(!arquivo_transferido){
        printf("Nao foi possivel criar e abrir o arquivo!\n");
        return -1;
    }

    while(1){
        
        memset(&pkt, 0x0, sizeof(pkt));
        
        if ((recvfrom(sd, &pkt, sizeof(pacote), 0, (struct sockaddr *) &addr_cliente, &addr_tam)) < 0){
            printf("Nao foi possivel receber mensagens do cliente B!\n");
            return -1;
        }

        fwrite(pkt.segmento, 1, pkt.tam, arquivo_transferido);
        contador_pacote++;

        printf("\nPacote %d recebido!\nNumero sequencia: %d\nChecksum: %li\nTamanho: %d\n", contador_pacote, pkt.numseq, pkt.check_sum, pkt.tam);
        sprintf(aux, "%d", pkt.numseq);
        strcat(ack, aux);
        strcpy(buffer, ack);

        if (sendto(sd, buffer, strlen(buffer), 0 ,(struct sockaddr *) &addr_cliente, addr_tam) < 0){
            printf("Erro ao tentar enviar o ack %s para o servidor!\n", aux);
            return -1;
        }

        strcpy(ack, "ack");
        
        if(pkt.tam == 0){
            break;
        }
    }

    fclose(arquivo_transferido);
    return 0;
}

int avisaServidor(int sd, struct sockaddr_in addr_servidor, socklen_t addr_tam, char nomearq[]){

    bloco blk;
    char *buffer;

    buffer = (char *) malloc(tam_buffer * sizeof(char));

    if(!buffer){
        printf("Nao foi possivel alocar memoria para o buffer!\n");
    }

    memset(&blk, 0x0, sizeof(blk));
    memset(buffer, '\0', sizeof(buffer));

    strcpy(blk.nome_arquivo, nomearq);
    blk.porta_cliente = 1039;

    if (sendto(sd, &blk, sizeof(blk), 0 ,(struct sockaddr *) &addr_servidor, addr_tam) < 0){
        printf("Erro ao tentar enviar o bloco de mensagem para o servidor!\n");
        return -1;
    }

    printf("\nArquivo transferido!\n");
    printf("\nMensagem enviada com sucesso!\n");
    printf("Aguardando a resposta do servidor...\n");

    if (recvfrom(sd, buffer, sizeof(buffer), 0 ,(struct sockaddr *) &addr_servidor, &addr_tam) < 0){
        printf("Erro ao tentar receber a mensagem de confirmacao do servidor!\n");
        return -1;
    }

    if(buffer){

        if(buffer[0] == '1'){

            printf("\nA mensagem do servidor foi recebida com sucesso e o banco de dados foi atualizado!\n");
        }
    }

    return 0;

}

int main(int argc, char *argv[]){

    char ip_cliente_arquivo[20];
    struct sockaddr_in addr_servidor, addr_cliente;
    int sd, flag = 0, pkt_cont, porta_cliente;
    socklen_t addr_tam, cliente_tam;

    char *buffer;
    buffer = (char*) malloc(tam_buffer * sizeof(char));

    if(!buffer){
        printf("Não foi possível alocar memória para o buffer!\n");
    }

    if(argc < 3){
        printf("Por favor, insira todos parâmetros na linha de comando!\n");
        exit(1);
    }

    //criação do socket local
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0){
        printf("Erro ao criar o socket!\n");
        return -1;
    }
    
    //dados do socket do servidor
    memset(&addr_servidor, 0, sizeof(addr_servidor));
    addr_servidor.sin_family = AF_INET;
    addr_servidor.sin_port = htons(porta_servidor);
    addr_servidor.sin_addr.s_addr = inet_addr(argv[1]);
    
    while(1){
    
        strcpy(buffer, argv[2]);

        addr_tam = sizeof(addr_servidor);
        
        if (sendto(sd, buffer, strlen(buffer), 0 ,(struct sockaddr *) &addr_servidor, sizeof(addr_servidor)) < 0){
            printf("Erro ao tentar enviar a mensagem para o servidor!\n");
            close(sd);
            return -1;
        }

        printf("Aguardando a resposta do servidor...\n");
        
        while(1){

            memset(buffer,'\0', tam_buffer);

            if ((recvfrom(sd, buffer, tam_buffer, 0, (struct sockaddr *) &addr_servidor, &addr_tam)) < 0){
                printf("Erro ao receber a mensagem do servidor!\n");
                return -1;
            }

            //se o buffer estiver vazio, quer dizer que foi possível receber a mensagem do servidor
            if(buffer[0] != '\0'){

                //o arquivo foi localizado no banco de dados
                if(buffer[0] == '1'){

                    strcpy(ip_cliente_arquivo,&buffer[1]);
                    flag = 1;
                    printf("O cliente de IP %s possui o arquivo!\n", ip_cliente_arquivo);
                    break;
                }
                else{
                    //arquivo não foi localizado
                    printf("Arquivo nao foi localizado no banco de dados!\n");
                    break;
                }
            }
        }

        break;
    }
    
    printf("Solicitando arquivo no cliente que possui o arquivo...\n");
    
    //Solicitando o arquivo para o cliente
    memset(&addr_cliente, 0, sizeof(addr_cliente));
    addr_cliente.sin_family = AF_INET;
    addr_cliente.sin_port = htons(10334);
    addr_cliente.sin_addr.s_addr = inet_addr(ip_cliente_arquivo);

    memset(buffer,'\0', tam_buffer);
    strcpy(buffer, argv[2]);

    //envia a requisição com o nome do arquivo para o cliente
    cliente_tam = sizeof(addr_cliente);

    if (sendto(sd, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, cliente_tam) < 0){
        printf("Erro ao enviar a mensagem para o cliente B!\n");
        return -1;
    }

    printf("\nMensagem enviada ao cliente B com sucesso!\n");

    while(1){
        
        memset(buffer,'\0', tam_buffer);

        if ((recvfrom(sd, buffer, tam_buffer, 0, (struct sockaddr *) &addr_cliente, &addr_tam)) < 0){
            printf("Nao foi possivel receber mensagem do cliente B!\n");
            return -1;
        }
        
        //se o buffer não estiver vazio, quer dizer que ele recebeu a mensagem do cliente B
        if(buffer){

            if(strcmp(buffer,"Transferindo")==0){
                printf("Iniciando a transferencia do arquivo %s\n", argv[2]);
            }
        }else{
            //buffer vazio, não recebeu a mensagem
            printf("Nao foi possivel comecar a transferencia de arquivos!\n");
            return -1;
        }

        break;
    }

    receberArquivo(sd, addr_cliente, cliente_tam, argv[2]);

    avisaServidor(sd, addr_servidor, addr_tam, argv[2]);

}