#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define LOCAL_SERVER_PORT 1500
#define TAM_MENSAGEM 1024

typedef struct{
    char nome_arquivo[30];
    char ip_cliente[20];
}arquivos;

void adicionaArquivos(){

    FILE *banco_de_dados;
    int quantidade, quantidade_arquivos_existentes = 0;
    char *nome_arquivo, *ip_cliente;
    arquivos arquivo;

    banco_de_dados = fopen("banco_de_dados.txt", "ab+");
    fread(&quantidade_arquivos_existentes, sizeof(int), 1, banco_de_dados);

    printf("Digite a quantidade de arquivos que deseja adicionar: ");
    scanf("%d", &quantidade);
    quantidade_arquivos_existentes += quantidade;

    for(int i=0; i<quantidade; i++){

        printf("\n");
        printf("Digite o nome do arquivo: ");
        scanf("%s", arquivo.nome_arquivo);
        strcpy(arquivo.ip_cliente, "127.0.0.1");
        fwrite(&arquivo, sizeof(arquivos), 1, banco_de_dados);
    }

    fclose(banco_de_dados);
    printf("Arquivo(s) adicionados com sucesso!\n");
}

int main(int argc, char *argv[]) {
  
    int opcao, quantidade_arquivos, sd, rc, n, cliLen;
    struct sockaddr_in cliAddr, servAddr;
    char msg[TAM_MENSAGEM], ip_cliente[16], nome_arquivo[30];

    FILE *banco_de_dados;

    banco_de_dados = fopen("banco_de_dados.txt", "rt");

    fseek(banco_de_dados, 0, SEEK_END); // goto end of file
    
    if (ftell(banco_de_dados) == 0){
        printf("Banco de dados vazio!\n");
        printf("Por favor, insira os arquivos manualmente no arquivo txt!\n");
        return -1;
    }

    fseek(banco_de_dados, 0, SEEK_SET); // goto begin of file

    while( (fscanf(banco_de_dados, "%s %s\n", &nome_arquivo, &ip_cliente)) != EOF)    {

        printf("O arquivo '%s' foi encontrado no cliente %s\n", nome_arquivo,   ip_cliente);

    }
    /*
    printf("Deseja adicionar novos arquivos no servidor?\n");
    printf("1-Sim ou 2-Não\n");
    printf("Digite a opção desejada: ");
    scanf("%d", &opcao);
    
    do{
        printf("\n");
        printf("Por favor, digite uma opção válida!\n");
        printf("Digite a opção desejada: ");
        scanf("%d", &opcao);

    }while(opcao < 1 || opcao >= 3);

    if(opcao == 1){
        adicionaArquivos();
    }
    */

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

    printf("%s: waiting for data on port UDP %u\n", 
        argv[0],LOCAL_SERVER_PORT);

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
        printf("%s: from %s:UDP%u : %s \n", 
        argv[0],inet_ntoa(cliAddr.sin_addr),
        ntohs(cliAddr.sin_port),msg);
        
    }/* end of server infinite loop */

    return 0;

}
