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

//estrutura do pacote
typedef struct pacote{
	int numseq;     	        //número de sequência do pacote			
	long int check_sum;			//valor do checksum do pacote	
	char segmento[512];			//segmento do pacote
	int tam;                    //tamanho do pacote
} pacote;

//estrutura do bloco de resposta do cliente A pro servidor
typedef struct bloco{
    int porta_cliente;          //porta do cliente que possui o pacote
    char nome_arquivo[30];      //nome do arquivo que ele possui
} bloco;

//função responsável por realizar o checksum
long int checksum(char segmento[], int tamanho){

	long int checksum = 0;

	//Percorrer o tamanho do segmento
	for(int i=0; i<tamanho; i++){
		
        //converte pra inteiro o caracter e soma
        checksum += (int) (segmento[i]);
	}

    //retorna o valor do checksum
	return checksum;
}

//função responsável por receber o arquivo enviado pelo cliente B
int receberArquivo(int sd, struct sockaddr_in addr_cliente, socklen_t addr_tam, char nomearq[]){

    pacote pkt;
    FILE *arquivo_transferido;
    char *buffer, *ack, *aux;
    int contador_pacote = 0;

    //aloca memória para as variáveis
    buffer = (char *) malloc(tamanho_buffer * sizeof(char));
    aux = (char *) malloc(tamanho_buffer * sizeof(char));
    ack = (char *) malloc(tamanho_buffer * sizeof(char));

    if(!ack){
        printf("Nao foi possivel alocar memoria para o ack!\n");
    }

    if(!aux){
        printf("Nao foi possivel alocar memoria para a variavel auxiliar!\n");
    }

    if(!buffer){
        printf("Nao foi possivel alocar memoria para o buffer!\n");
    }

    arquivo_transferido = fopen(nomearq, "wb");

    //erro ao tentar criar o arquivo
    if(!arquivo_transferido){
        printf("Nao foi possivel criar e abrir o arquivo!\n");
        return -1;
    }

    while(1){
        
        memset(&pkt, 0x0, sizeof(pacote));
        
        //recebe os pacotes enviados pelo cliente B
        if ((recvfrom(sd, &pkt, sizeof(pkt), 0, (struct sockaddr *) &addr_cliente, &addr_tam)) < 0){
            printf("Nao foi possivel receber mensagens do cliente B!\n");
            return -1;
        }

        //se o tamanho do arquivo for zero significa que não existem mais pacotes para
        //serem transmitidos, então finaliza a função
        if(pkt.tam == 0){
            break;
        }

        //escreve os valores do pacote dentro do arquivo
        fwrite(pkt.segmento, 1, pkt.tam, arquivo_transferido);
        contador_pacote++;

        //mensagem de pacote recebido do cliente B
        printf("\nPacote %d recebido!\nNumero sequencia: %d\nChecksum: %li\nTamanho: %d\n", contador_pacote, pkt.numseq, pkt.check_sum, pkt.tam);
        sprintf(aux, "%d", pkt.numseq);
        strcpy(ack, "ack");
        strcat(ack, aux);
        strcpy(buffer, ack);

        //retorna uma mensagem ack com o número de sequência do pacote
        //para o cliente B, mostrando que recebeu o pacote
        if (sendto(sd, buffer, strlen(buffer), 0 ,(struct sockaddr *) &addr_cliente, addr_tam) < 0){
            printf("Erro ao tentar enviar o ack %s para o servidor!\n", aux);
            return -1;
        }

        //reseta a variável de ack
        strcpy(ack, "ack");
        
    }

    //fecha o arquivo
    fclose(arquivo_transferido);
    return 0;
}

//função responsável por avisar o servidor que o cliente A também possui o arquivo
int avisaServidor(int sd, struct sockaddr_in addr_servidor, socklen_t addr_tam, char nomearq[]){

    bloco blk;
    char *buffer;

    //aloca memória para o buffer
    buffer = (char *) malloc(tamanho_buffer * sizeof(char));

    if(!buffer){
        printf("Nao foi possivel alocar memoria para o buffer!\n");
    }

    memset(&blk, 0x0, sizeof(blk));
    memset(buffer, '\0', sizeof(buffer));

    //cria um novo bloco de resposta
    //armazena o nome do arquivo e o ip do cliente
    strcpy(blk.nome_arquivo, nomearq);
    blk.porta_cliente = porta_clientea;

    //envia o bloco de resposta para o servidor
    if (sendto(sd, &blk, sizeof(blk), 0 ,(struct sockaddr *) &addr_servidor, addr_tam) < 0){
        printf("Erro ao tentar enviar o bloco de mensagem para o servidor!\n");
        return -1;
    }

    printf("\nArquivo transferido!\n");
    printf("\nMensagem enviada com sucesso!\n");
    printf("Aguardando a resposta do servidor...\n");

    //espera receber a mensagem de confirmação do servidor
    //avisando que recebeu o bloco de resposta do cliente A
    if (recvfrom(sd, buffer, sizeof(buffer)+1, 0 ,(struct sockaddr *) &addr_servidor, &addr_tam) < 0){
        printf("Erro ao tentar receber a mensagem de confirmacao do servidor!\n");
        return -1;
    }

    //se o buffer não está vazio
    if(buffer){

        //se tiver um 1 na primeira posição do buffer, quer dizer que a operação foi bem sucedida
        if(buffer[0] == '1'){

            printf("\nA mensagem do servidor foi recebida com sucesso e o banco de dados foi atualizado!\n");
        }else{

            printf("Não foi possível realizar a operacao!\n");
            
        }
    }

    return 0;

}

int main(int argc, char *argv[]){

    //variáveis
    char porta_cliente_arquivo[7];
    struct sockaddr_in addr_servidor, addr_cliente;
    int sd;
    socklen_t addr_tam, cliente_tam;

    char *buffer;

    //aloca memória para o buffer
    buffer = (char*) malloc(tamanho_buffer * sizeof(char));

    if(!buffer){
        printf("Não foi possível alocar memória para o buffer!\n");
    }

    //verifica se todos os valores foram passados no parâmetro
    if(argc < 2){
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
    addr_servidor.sin_addr.s_addr = inet_addr("127.0.0.1");

    //---------COMUNICAÇÃO COM O SERVIDOR---------
    while(1){
    
        strcpy(buffer, argv[1]);

        addr_tam = sizeof(addr_servidor);
        
        //envia a mensagem pro servidor
        //requisitando o arquivo que deseja
        if (sendto(sd, buffer, strlen(buffer), 0 ,(struct sockaddr *) &addr_servidor, addr_tam) < 0){
            printf("Erro ao tentar enviar a mensagem para o servidor!\n");
            close(sd);
            return -1;
        }

        printf("Aguardando a resposta do servidor...\n");
        
        while(1){

            memset(buffer,'\0', tamanho_buffer);

            //espera receber a resposta do servidor e que será armazenada no buffer
            if ((recvfrom(sd, buffer, tamanho_buffer, 0, (struct sockaddr *) &addr_servidor, &addr_tam)) < 0){
                printf("Erro ao receber a mensagem do servidor!\n");
                return -1;
            }

            //se o buffer estiver vazio, quer dizer que foi possível receber a mensagem do servidor
            if(buffer[0] != '\0'){

                //o arquivo foi localizado no banco de dados
                //o 1 na primeira posição sinaliza que o arquivo foi encontrado
                if(buffer[0] == '1'){

                    strcpy(porta_cliente_arquivo, &buffer[1]);
                    printf("O cliente que está na porta %s possui o arquivo!\n", porta_cliente_arquivo);
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
    
    //---------COMUNICAÇÃO COM O CLIENTE B (QUE POSSUI O ARQUIVO REQUISITADO)---------
    printf("Solicitando arquivo no cliente que possui o arquivo...\n");
    
    //Solicitando o arquivo para o cliente
    memset(&addr_cliente, 0, sizeof(addr_cliente));
    addr_cliente.sin_family = AF_INET;
    addr_cliente.sin_port = htons(porta_clienteb);
    addr_cliente.sin_addr.s_addr = inet_addr("127.0.0.1");

    memset(buffer,'\0', tamanho_buffer);
    strcpy(buffer, argv[1]);

    //envia a requisição com o nome do arquivo para o cliente B
    cliente_tam = sizeof(addr_cliente);

    //envia a mensagem de solicitação do arquivo para o cliente B
    if (sendto(sd, buffer, strlen(buffer) , 0 , (struct sockaddr *) &addr_cliente, cliente_tam) < 0){
        printf("Erro ao enviar a mensagem para o cliente B!\n");
        return -1;
    }

    printf("\nMensagem enviada ao cliente B com sucesso!\n");

    while(1){
        
        memset(buffer,'\0', tamanho_buffer);

        //verifica se o cliente B recebeu o arquivo
        if ((recvfrom(sd, buffer, tamanho_buffer, 0, (struct sockaddr *) &addr_cliente, &addr_tam)) < 0){
            printf("Nao foi possivel receber mensagem do cliente B!\n");
            return -1;
        }
        
        //se o buffer não estiver vazio, quer dizer que ele recebeu a mensagem do cliente B
        if(buffer){

            if(strcmp(buffer,"Transferindo") == 0){
                printf("Iniciando a transferencia do arquivo %s\n", argv[1]);
            }

        }else{
            //buffer vazio, não recebeu a mensagem
            printf("Nao foi possivel comecar a transferencia de arquivos!\n");
            return -1;
        }

        break;
    }

    //função que receberá o arquivo do cliente B
    receberArquivo(sd, addr_cliente, cliente_tam, argv[1]);

    //função que avisa o servidor que o cliente A possui o arquivo
    avisaServidor(sd, addr_servidor, addr_tam, argv[1]);

}