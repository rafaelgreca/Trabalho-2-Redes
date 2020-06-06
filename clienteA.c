#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* memset() */
#include <sys/time.h> /* select() */ 

#define REMOTE_SERVER_PORT 1500
#define TAM_MENSAGEM 1024

int main(int argc, char *argv[]){

    int Socket, rc, i;
    struct sockaddr_in clienteAddr, servidorRemotoAddr;
    struct hostent *nome_servidor;

    //pega o endereço do servidor passado no parâmetro
    nome_servidor = gethostbyname(argv[1]);
    
    if(!nome_servidor){
        printf("Endereço do servidor não foi passado com parâmetro!\n");
        return -1;
    }

    //criação do socket
    //protocolo (nosso caso é AF_INET (ARPA INTERNET PROTOCOLS))
    //tipo do socket (nosso caso é SOCK_DGRAM)
    //número do procolo (nosso caso é 0 = IP)
    Socket = socket(AF_INET, SOCK_DGRAM, 0);

    if(Socket < 0){
        printf("Erro ao iniciar o socket!\n");
        return -1;
    }

    servidorRemotoAddr.sin_family = nome_servidor->h_addrtype;
    memcpy((char *) &servidorRemotoAddr.sin_addr.s_addr, nome_servidor->h_addr_list[0], nome_servidor->h_length);
    servidorRemotoAddr.sin_port = htons(REMOTE_SERVER_PORT);
    
    /* bind any port */
    clienteAddr.sin_family = AF_INET;
    clienteAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clienteAddr.sin_port = htons(0);
  
    if(bind(Socket, (struct sockaddr *) &clienteAddr, sizeof(clienteAddr))<0) {
      printf("%s: cannot bind port\n", argv[0]);
      return -1;
    }

    /* send data */
    for(i=2;i<argc;i++) {
      rc = sendto(Socket, argv[i], strlen(argv[i])+1, 0, (struct sockaddr *) &servidorRemotoAddr, 
	sizeof(servidorRemotoAddr));
 
      if(rc<0) {
        printf("%s: cannot send data %d \n",argv[0],i-1);
        close(Socket);
        return -1;
      }

    }

    return 0;
}