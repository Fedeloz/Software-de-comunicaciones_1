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

                // ctes globales

#define PUERTO_ORQUESTADOR 4001
#define T 128
uint16_t identificador = 0;

int main(int argc, char *argv[]){

    if(argc < 3){
        printf("Uso: %s <dirección IP> <cadena de caracteres>\n", argv[0]);
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
        perror("connect sd_orquestador");
        exit(-1);
    }

                // send
    uint16_t enviar = htons(identificador);
    enviados = send(s_serv, &enviar, sizeof(enviar), 0);
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
    
        uint32_t IP_UDP;      // IP servidor en uint32_t
        uint16_t PUERTO_UDP;
        int s_udp;      // socket descriptor
        struct sockaddr_in udp_addr;       // Dirección del servidor
        socklen_t serv_len;     // Longitud de la dirección del servidor


        leidos = recv(s_serv, &IP_UDP, sizeof(uint32_t), 0);
        leidos = recv(s_serv, &PUERTO_UDP, 2, 0);
        if(leidos != sizeof(PUERTO_UDP)){
            perror("recv PUERTO_UDP");
            exit(1);
        }

        if(close(s_serv) < 0){
            perror("close s_serv");
            exit(-1);
        }

                // socket

        s_udp = socket(PF_INET, SOCK_DGRAM, 0);
        
        if (s_udp < 0) {
            perror("socket s_udp");
            exit(1);
        }

                // sockaddr_in

        memset(&udp_addr, 0, sizeof(udp_addr));
        udp_addr.sin_family = AF_INET;
        udp_addr.sin_port = PUERTO_UDP;
        memcpy(&udp_addr.sin_addr, &IP_UDP, 4);

                // sendto

        printf("IP_UDP = %s\n", inet_ntoa(udp_addr.sin_addr));        
        printf("Puerto UDP: %d\n", htons(udp_addr.sin_port));
        printf("Enviando datos...\n");
        
        sprintf(b, argv[2]);

        enviados = sendto(s_udp, b, strlen(b), 0, (struct sockaddr *) &udp_addr, sizeof(udp_addr));

        if (enviados != strlen(b)) {
            perror("sendto");
            close(s_udp);
            exit(1);
        }

        printf("Datos enviados: %s\n", b);

            // recvfrom

        serv_len = sizeof(udp_addr);
        leidos = recvfrom(s_udp, b, T, 0, (struct sockaddr *) &udp_addr, &serv_len);
        if(leidos  < 0){
            perror("recvfrom");
            close(s_udp);
            exit(1);
        }
        b[leidos] = 0;      // Para que sea una cadena
        
        printf("Mensaje recibido: \n\t-> %s (%d)\n", b, leidos);

        if(close(s_udp) < 0){
        perror("close s_udp");
        exit(-1);
        }
    }
    return 0;
}
