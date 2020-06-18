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

//função responsável por realizar o checksum
long int checksum(char segmento[], int tamanho){

	long int checksum = 0;

	//percorrer o tamanho do segmento
	for(int i=0; i<tamanho; i++){
		
        //converte pra inteiro o caracter e soma
        checksum += (int) (segmento[i]);
	}

    //retorna o valor do checksum
	return checksum;
}

//função responsável por transferir o arquivo para o cliente A
int enviaPacote(char *mensagem, FILE *arquivo, int sd, struct sockaddr_in addr_cliente, socklen_t addr_tam){

    int tam = 0, contador_pacote = 0, num_seq = 0;
    char *buffer;
    pacote pkt;

    //aloca memória para o buffer
    buffer = (char *) malloc(tamanho_buffer * sizeof(buffer));

    if(!buffer){
        printf("Nao foi possivel alocar memoria para o buffer!\n");
        return -1;
    }
    
    memset(&pkt, 0x0, sizeof(pkt));

    //enquanto o arquivo não acabar
    while(!feof(arquivo)){
        
        contador_pacote++;
        num_seq++;

        printf("\nEnviando o pacote %d de numero de sequencia %d. Por favor, aguarde...\n", contador_pacote, num_seq);
        
        //le 512 bytes do arquivo
        tam = fread(pkt.segmento, 1, 512, arquivo);

        pkt.tam = tam;
        //strcpy(pkt.segmento, mensagem);
        pkt.numseq = num_seq;
        pkt.check_sum = checksum(pkt.segmento, pkt.tam);

        //envia o pacote para o cliente A
        if(sendto(sd, &pkt, sizeof(pkt)+1, 0, (struct sockaddr *) &addr_cliente, addr_tam) < 0){
            printf("Falha ao enviar o pacote de numero de sequencia = %d!\n", num_seq);
            return -1;
        }

        //recebe a mensagem de ack (confirmação de que recebeu o pacote) do cliente A
        if(recvfrom(sd, buffer, sizeof(buffer)+1, 0, (struct sockaddr *) &addr_cliente, &addr_tam) < 0){
            printf("Erro ao receber ack do cliente A!\n");
            return -1;
        }

        printf("%s recebido do cliente A!\n", buffer);
    }

    //depois que termina o arquivo, envia um pacote final com tamanho igual a zero
    //indicando para o cliente A que a transferência do arquivo acabou
    pkt.tam = 0;
    strcpy(pkt.segmento, "");
    pkt.numseq = 0;
    pkt.check_sum = 0;

    //envia o pacote final para o cliente A
    if(sendto(sd, &pkt, sizeof(pkt)+1, 0, (struct sockaddr *) &addr_cliente, addr_tam) < 0){
        printf("Erro ao tentar enviar o pacote final de confirmacao!\n");
        return -1;
    }
}

int main(){

    //variáveis
    int sd;
    struct sockaddr_in addr_cliente;
    char *buffer;
    socklen_t addr_tam;
    FILE *arquivo_cliente;

    //aloca memória para o buffer
    buffer = (char*) malloc(tamanho_buffer * sizeof(char));

    if(!buffer){
        printf("Não foi possível alocar memória para o buffer!\n");
    }

    //criação do socket local
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0){
        printf("Erro ao criar o socket!\n");
        return -1;
    }
    
    //dados do socket do cliente
    memset(&addr_cliente, 0, sizeof(addr_cliente));
    addr_cliente.sin_family = AF_INET;
    addr_cliente.sin_port = htons(porta_clienteb);
    addr_cliente.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind no socket
    if(bind(sd, (struct sockaddr *)&addr_cliente , sizeof(addr_cliente))<0){
        perror("bind");
        printf("Erro ao tentar dar bind no socket!\n");
        return -1;
    }

    listen(sd, 1);

    printf("Aguardando solicitações...\n");

    //---------COMUNICAÇÃO COM O CLIENTE A---------
    while(1){

        memset(buffer, '\0', tamanho_buffer);
        
        addr_tam = sizeof(addr_cliente);

        //recebe a mensagem requisitando o arquivo do cliente A e armazena no buffer
        if ((recvfrom(sd, buffer, tamanho_buffer+1, 0, (struct sockaddr *) &addr_cliente, &addr_tam)) < 0){
            printf("Erro ao tentar receber mensagens do cliente A!\n");
            return -1;
        }

        printf("Mensagem recebida do cliente A: %s\n", buffer);
        
        //se o buffer não estiver vazio, quer dizer que recebeu a mensagem
        if(buffer[0]!='\0'){
           
           //abre o arquivo requisitado do cliente
           arquivo_cliente = fopen(buffer,"rb");

           //arquivo não existe ou não possível abrir o arquivo
           if(!arquivo_cliente){

                strcpy(buffer,"Erro");
                
                if (sendto(sd, buffer, strlen(buffer)+1 , 0 , (struct sockaddr *) &addr_cliente, addr_tam) < 0){
                    printf("Erro ao enviar a mensagem para o cliente A!\n");
                    return -1;
                }
           }
           else{
                //o arquivo existe e não ouve problemas em abri-lo
                printf("O arquivo %s esta sendo enviado, por favor aguarde...\n", buffer);
                
                strcpy(buffer, "Transferindo");

                if(sendto(sd, buffer, strlen(buffer)+1 , 0 , (struct sockaddr *) &addr_cliente, addr_tam) < 0){
                   printf("Erro ao enviar a mensagem para o cliente A!\n");
                   return -1;
                }

                //aloca memória para a msg
                char *msg;

                msg = (char *) malloc(512 * sizeof(char));

                if(!msg){
                    printf("Nao foi possivel alocar memoria para a mensagem\n");
                }

                //função responsável por montar os pacotes e enviar para o cliente A
                enviaPacote(msg, arquivo_cliente, sd, addr_cliente, addr_tam);
           }

           printf("\n\nEnvio do arquivo finalizado sem erros!\n");
           break;
           fclose(arquivo_cliente);
        }
    }
}