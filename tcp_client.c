#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>


                // ctes globales

#define PUERTO_ORQUESTADOR 4001
#define T 16
uint16_t identificador = 1;


int main(int argc, char *argv[]){

    if(argc < 2){
        printf("Uso: %s <dirección IP>\n", argv[0]);
        exit(-1);
    }

    char b[T];
    uint8_t ACK;

                // ANCHOR pedir al orquestador

    int s_serv;     // Socked Descriptor
    int leidos, enviados;       // Bytes
    uint32_t IP_orquestador;      // IP en uint32_t
    struct sockaddr_in orq_addr;       // Direccion server

    IP_orquestador = inet_addr(argv[1]);
    if(IP_orquestador == -1){
        perror("inet_addr IP_ORQUESTADOR");
        exit(1);
    }

                // Socket descriptor

    s_serv = socket(PF_INET, SOCK_STREAM, 0);
    if(s_serv < 0){
        perror("Socket Descriptor");
        exit(-1);
    }

                // sockaddr_in


    memset(&orq_addr, 0, sizeof(orq_addr));
    orq_addr.sin_family = AF_INET;
    memcpy(&orq_addr.sin_addr, &IP_orquestador, 4);
    orq_addr.sin_port = htons(PUERTO_ORQUESTADOR);

                // connect

    if(connect(s_serv, (struct sockaddr*) &orq_addr, sizeof(orq_addr)) < 0){
        perror("connect s_serv");
        exit(-1);
    }

                // send

    printf("Enviando identificador: %d\n", identificador);
    uint16_t enviar = htons(identificador);
    enviados = send(s_serv, (char*) &enviar, sizeof(enviar), 0);
    if(enviados != sizeof(uint16_t)){
        perror("send identificador");
        exit(-1);
    }

                // recv

    leidos = recv(s_serv, &ACK, 1, 0);
    if(leidos != sizeof(ACK)){
        perror("recv ACK");
        exit(-1);
    }


    if(ACK == 1){
        printf("No existe servidor asociado\n");   
        if(close(s_serv) < 0){
                perror("close s_serv");
                exit(-1);
        }     
    }else{
        printf("Servicios ofecidos por servidor asociado!\n");
        uint32_t IP_TCP;
        uint16_t PUERTO_TCP;
        int s_file;
        struct sockaddr_in tcp_addr;
        
        leidos = recv(s_serv, &IP_TCP, sizeof(uint32_t), 0);
        leidos = recv(s_serv, &PUERTO_TCP, 2, 0);
        if(leidos != sizeof(PUERTO_TCP)){
            perror("recv PUERTO_UDP");
            exit(1);
        }
        if(close(s_serv) < 0){
                perror("close s_serv");
                exit(-1);
        } 

                // sockaddr_in

        memset(&tcp_addr, 0, sizeof(tcp_addr));
        tcp_addr.sin_family = AF_INET;
        tcp_addr.sin_port = PUERTO_TCP;
        memcpy(&tcp_addr.sin_addr, &IP_TCP, 4);
        
        printf("IP_UDP = %s\n", inet_ntoa(tcp_addr.sin_addr));
        printf("Puerto UDP: %d\n", htons(tcp_addr.sin_port));


	    printf("Introduce el fichero a enviar:\n");
        char fichero[T];
	    scanf("%s",fichero);

        while(strcmp(fichero,"fin") != 0){

                // socket

            s_file = socket(PF_INET, SOCK_STREAM, 0);
            if(s_file < 0){
                perror("socket s_file");
                exit(1);
            }

                // connect

            printf("Conectando con el servidor tcp...\n");

            if(connect(s_file, (struct sockaddr*) &tcp_addr, sizeof(tcp_addr)) < 0){
                perror("connect s_file");
                exit(-1);
            }
            printf("Conectado con el servidor tcp!\n");

            uint16_t tam;
            int fd;
            strcat(fichero, ".txt");
            if(   (fd = open(fichero, O_RDONLY))  < 0){
                perror("Open fd");
                exit(-1);
            }
            printf("Fichero '%s' abierto!\n", fichero);
            tam = read(fd, b, T);
            if(tam < 0){
                perror("Read");
                exit(-1);
            }
            enviados = send(s_file, &tam, sizeof(tam), 0);      // Enviar tam
            //printf("enviados: %d, sizeof(tam): %d\n", enviados, sizeof(tam));
            if(enviados != sizeof(tam)){
                perror("enviados != tam, tam1");
                close(fd);
                exit(1);
            }
            printf("Enviando: \n");
            write(1, b, tam);
            printf("\n");
            enviados = send(s_file, b, tam, 0);      // Enviar data
            if(enviados != tam){
                perror("enviados != tam, b1");
                close(fd);
                exit(1);
            }

            while(tam > 0){
                tam = read(fd, b, T);
                enviados = send(s_file, &tam, sizeof(tam), 0);      // Enviar tam
                if(enviados != sizeof(tam)){
                    perror("enviados != tam, tam2");
                    close(fd);
                    exit(1);
                }
                //write(1, b, tam);
                //printf("\n");
                enviados = send(s_file, b, tam, 0);      // Enviar data
                if(enviados != tam){
                    perror("enviados != tam, b2");
                    close(fd);
                    exit(1);
                }
            }
            tam = 0;
            enviados = send(s_file, &tam, sizeof(tam), 0);      // Enviar tamaño 0
            if(close (fd) < 0){
                    perror("Close");
                    exit(-1);
            }
            printf("fichero enviado!\n");
            printf("Introduce el fichero a enviar:\n");
	        scanf("%s",fichero);
            if(close(s_file) < 0){
                perror("close s_file");
                exit(1);
            }
        }
    }
    return 0;
}

