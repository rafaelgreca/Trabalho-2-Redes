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

long int checksum(char segmento[],int dimensao){
	long int soma;
	int asc,i;

	//Inicializar variavel que armazena a soma
	soma = 0;

	//Percorrer cadeia de caracteres
	for(i=0 ; i<dimensao ; i++){
		asc = 0;
		asc = (int) segmento[i];		//Obter valor int correspondente ao byte
		if(asc >= 100 || asc <= -100)
			asc	= asc/20;				//Representar o bytepor seu valor dividido por 20
		else
			asc = asc/5;				//Representar o byte por seu valor dividido por 5

		soma += asc*i;					//Adicionar valor do byte na soma
	}

	//Devolver valor total do checksum
	return soma;
}

int enviaPacote(char *mensagem, FILE *arquivo, int sd, struct sockaddr_in addr_cliente, socklen_t addr_tam){

    int tam = 0, contador_pacote = 0, num_seq = 0;
    char *buffer;
    pacote pkt, pkt2;

    buffer = (char *) malloc(tam_buffer * sizeof(buffer));

    if(!buffer){
        printf("Nao foi possivel alocar memoria para o buffer!\n");
        return -1;
    }
    
    memset(&pkt, 0x0, sizeof(pkt));

    while(!feof(arquivo)){
        
        contador_pacote++;
        num_seq++;

        printf("\nEnviando o pacote %d de numero de sequencia %d. Por favor, aguarde...\n", contador_pacote, num_seq);
        tam = fread(pkt.segmento, 1, 256, arquivo);

        pkt.tam = tam;
        //strcpy(pkt.segmento, mensagem);
        pkt.flag = 1;
        pkt.numseq = num_seq;
        pkt.check_sum = checksum(pkt.segmento, pkt.tam);

        if(sendto(sd, &pkt, sizeof(pkt)+1, 0, (struct sockaddr *) &addr_cliente, addr_tam) < 0){
            printf("Falha ao enviar o pacote de numero de sequencia = %d!\n", num_seq);
        }

        if(recvfrom(sd, buffer, sizeof(buffer)+1, 0, (struct sockaddr *) &addr_cliente, &addr_tam) < 0){
            printf("Erro ao receber ack do cliente A!\n");
        }

        printf("%s recebido do cliente A!\n", buffer);
    }

    pkt.tam = 0;
    strcpy(pkt.segmento, "");
    pkt.flag = 0;
    pkt.numseq = 0;
    pkt.check_sum = 0;

    if(sendto(sd, &pkt, sizeof(pkt)+1, 0, (struct sockaddr *) &addr_cliente, addr_tam) < 0){
        printf("Erro ao tentar enviar o pacote final de confirmacao!\n");
    }
}

int main(){

    int sd;
    struct sockaddr_in addr_cliente;
    int rec_msg_tam,status,pkt_cont,msg_tam,er;
    char *buffer,ch_ree,*buffer_recv;
    socklen_t addr_tam;
    FILE *arquivo_cliente;

    buffer = (char*) malloc(tam_buffer * sizeof(char));

    if(!buffer){
        printf("Não foi possível alocar memória para o buffer!\n");
    }

    //criação do socket local
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0){
        printf("Erro ao criar o socket!\n");
        return -1;
    }
    
    memset(&addr_cliente, 0, sizeof(addr_cliente));
    addr_cliente.sin_family = AF_INET;
    addr_cliente.sin_port = htons(10334);
    addr_cliente.sin_addr.s_addr = inet_addr("127.0.0.1");

    // bind no socket
    if(bind(sd, (struct sockaddr *)&addr_cliente , sizeof(addr_cliente))<0){
        perror("bind");
        printf("Erro ao tentar dar bind no socket!\n");
        return -1;
    }

    listen(sd, 1);

    printf("Aguardando solicitações...\n");
    //ch_ree='1';
    
    while(1){

        memset(buffer, '\0', tam_buffer);
        
        addr_tam = sizeof(addr_cliente);

        if ((recvfrom(sd, buffer, tam_buffer+1, 0, (struct sockaddr *) &addr_cliente, &addr_tam)) < 0){
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

                char *msg;

                msg = (char *) malloc(256 * sizeof(char));

                if(!msg){
                    printf("Nao foi possivel alocar memoria para a mensagem\n");
                }

                enviaPacote(msg, arquivo_cliente, sd, addr_cliente, addr_tam);
           }

           printf("\n\nEnvio do arquivo finalizado sem erros!\n");
           break;
           fclose(arquivo_cliente);
        }
    }
}