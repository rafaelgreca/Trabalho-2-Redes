#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define LOCAL_SERVER_PORT 1500
#define TAM_MENSAGEM 1024

int main(int argc, char *argv[]) {
  
    int opcao, flag = 0, sd, rc, n, cliLen;
    struct sockaddr_in cliAddr, servAddr;
    char msg[TAM_MENSAGEM], encontrado[TAM_MENSAGEM], nome_arquivo[30], ip_cliente[20];

    FILE *banco_de_dados;

    banco_de_dados = fopen("banco_de_dados.txt", "rt");

    fseek(banco_de_dados, 0, SEEK_END); // goto end of file
    
    if (ftell(banco_de_dados) == 0){
        printf("Banco de dados vazio!\n");
        printf("Por favor, insira os arquivos manualmente no arquivo txt!\n");
        return -1;
    }

    fseek(banco_de_dados, 0, SEEK_SET); // goto begin of file

    /* socket creation */
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd<0) {
        printf("%s: cannot open socket \n",argv[0]);
        //exit(1);
    }

    /* bind local server port */
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(LOCAL_SERVER_PORT);
    
    if(bind(sd, (struct sockaddr *) &servAddr,sizeof(servAddr))<0) {
        printf("%s: cannot bind port number %d \n", 
        argv[0], LOCAL_SERVER_PORT);
        //exit(1);
    }

    printf("%s: waiting for data on port UDP %u\n", argv[0],LOCAL_SERVER_PORT);

    /* server infinite loop */
    while(1) {
        
        fflush(stdout);
        /* init buffer */
        memset(msg,0x0,TAM_MENSAGEM);

        /* receive message */
        cliLen = sizeof(cliAddr);
        n = recvfrom(sd, msg, TAM_MENSAGEM, 0, (struct sockaddr *) &cliAddr, &cliLen);

        if(n<0) {
            printf("%s: cannot receive data \n",argv[0]);
            continue;
        }
        
        /* print received message */
        printf("%s: from %s:UDP%u : %s \n", argv[0], inet_ntoa(cliAddr.sin_addr), ntohs(cliAddr.sin_port), msg);

        char ip_cliente[16], nome_arquivo[30];
        int flag = 0;

        while((fscanf(banco_de_dados, "%s %s\n", &nome_arquivo, &ip_cliente)) != EOF){

            if((strcmp(nome_arquivo, msg))== 0){
                strcpy(encontrado, ip_cliente);
                flag = 1;
            }
            
        }

        if(flag == 0){
            printf("Arquivo não encontrado!\n");
            exit(1);
        }
            printf("%s\n", encontrado);
            
    }/* end of server infinite loop */

    fclose(banco_de_dados);
    return 0;

}
