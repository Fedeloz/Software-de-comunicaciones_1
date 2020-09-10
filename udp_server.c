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

                // ANCHOR Constantes globales

#define PUERTO_ORQUESTADOR 4000
#define T 128
#define MAX 1024
#define P_SERV 4950
#define P_ALIVE 4951
uint16_t identificator = 0;

               // ANCHOR funciones auxiliares

ssize_t read_n(int fifo1, char * mensaje, size_t longitud_mensaje);
ssize_t write_n(int fifo1, char * mensaje, size_t longitud_mensaje);
int maxfds(fd_set fd, int max);
void copia_fd_set(fd_set *destino, fd_set *origen, int size);
void sustituir(char *origen, char * destino);

                // ANCHOR main

int main(int argc, char *argv[]){
    
    char b[T];      // Buffer
    uint8_t ACK;

    if(argc < 2){
        printf("Uso: %s <dirección IP>\n", argv[0]);
        exit(-1);
    }

                // ANCHOR registrar en el orquestador

    int s_reg;     // Socked Descriptor
    int leidos, enviados;       // Bytes
    uint32_t IP_orquestador;      // IP en uint32_t
    struct sockaddr_in serv_addr_s_reg;       // Direccion server

    IP_orquestador = inet_addr(argv[1]);
    if(IP_orquestador < 0){
        perror("inet_addr IP_orquestador");
        exit(-1);
    }

                // Socket descriptor

    s_reg = socket(PF_INET, SOCK_STREAM, 0);
    if(s_reg < 0){
        perror("Socket Descriptor");
        exit(-1);
    }

                // sockaddr_in


    memset(&serv_addr_s_reg, 0, sizeof(serv_addr_s_reg));
    serv_addr_s_reg.sin_family = AF_INET;
    memcpy(&serv_addr_s_reg.sin_addr, &IP_orquestador, 4);
    serv_addr_s_reg.sin_port = htons(PUERTO_ORQUESTADOR);

                // connect

    if(connect(s_reg, (struct sockaddr*) &serv_addr_s_reg, sizeof(serv_addr_s_reg)) < 0){
        perror("connect sd_orquestador");
        exit(-1);
    }

                // send

    uint16_t p_serv = htons(P_SERV);
    uint16_t p_alive = htons(P_ALIVE);

    enviados = send(s_reg, (char*) &identificator, sizeof(identificator), 0);
    if(enviados != sizeof(uint16_t)){
        perror("send identificador");
        exit(-1);
    }enviados = send(s_reg, (char*) &p_serv, sizeof(p_serv), 0);
    if(enviados != sizeof(uint16_t)){
        perror("send p_serv");
        exit(-1);
    }enviados = send(s_reg, (char*) &p_alive, sizeof(p_alive), 0);
    if(enviados != sizeof(uint16_t)){
        perror("send p_alive");
        exit(-1);
    }

                // recv

    leidos = recv(s_reg, &ACK, 1, 0);     // Se lee 1 byte
    if(leidos < 0){
        perror("recv orquestador");
        close(s_reg);
        exit(1);
    }
    printf("Recibido: %d\n", ACK);
    if(close(s_reg) < 0){
        perror("close sd_orquestador");
        exit(-1);
    }

    if(ACK == 1){
        printf("Servicio ya ofrecido por otro servidor\n");
        
    }else{

                // ANCHOR s_alive

        int s_alive, n_s_alive;
        struct sockaddr_in serv_addr_s_alive;       // Direccion server
        socklen_t addr_len;


                // socket descriptor y sockaddr_in

        if((s_alive = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            perror("s_alive");
            exit(1);
        }
        memset(&serv_addr_s_alive, 0, sizeof(serv_addr_s_alive));
        serv_addr_s_alive.sin_family = AF_INET;
        serv_addr_s_alive.sin_addr.s_addr = INADDR_ANY;     // Solo puerto
        serv_addr_s_alive.sin_port = htons(P_ALIVE);

                // bind

         if(bind(s_alive, (struct sockaddr*) &serv_addr_s_alive, sizeof(serv_addr_s_alive)) < 0){
            perror("bind s_alive");
            exit(1);
        }

                // listen

        if (listen(s_alive, 1) < 0) {
            perror("listen s_alive");
            exit(1);
        }

                // ANCHOR s_udp

        int s_udp;
        struct sockaddr_in cli_addr;
        socklen_t cli_len;
        struct sockaddr_in serv_addr_s_udp;       // Direccion server


                // socket descriptor y sockaddr_in

        s_udp = socket(PF_INET, SOCK_DGRAM, 0);
        if (s_udp < 0) {
            perror("socket s_udp");
            exit(1);
        }
        memset(&serv_addr_s_udp, 0, sizeof(serv_addr_s_udp));
        serv_addr_s_udp.sin_family = AF_INET;
        serv_addr_s_udp.sin_port = htons(P_SERV);
        serv_addr_s_udp.sin_addr.s_addr = INADDR_ANY;

                // bind

        if (bind(s_udp, (struct sockaddr *) &serv_addr_s_udp, sizeof(serv_addr_s_udp)) < 0) {
            perror("bind s_udp");
            exit(1);
        }

                // ANCHOR select

        fd_set rfd, rfd_copia;
        FD_ZERO(&rfd);      //Inicializo conjunto descriptores
        FD_SET(0, &rfd);        //Añado el descriptor de teclado
        FD_SET(s_alive, &rfd);     //Añado s_alive
        FD_SET(s_udp, &rfd);     //Añado s_udp


        while(1){
            copia_fd_set(&rfd_copia, &rfd, sizeof(rfd));      //Hago la copia
            int result = select(maxfds(rfd_copia, MAX), &rfd_copia, NULL, NULL, NULL);
            if(result < 0){
                perror("Select");
                exit(-1);
            }   
            if(FD_ISSET(s_alive, &rfd_copia)){
            // Se acepta la conexión y se cierra, con eso basta                
                cli_len = sizeof(cli_addr);
                n_s_alive = accept(s_alive,  (struct sockaddr*) &cli_addr, &addr_len);
                if(n_s_alive < 0){
                    perror("accept n_s_alive");
                    exit(-1);
                }
                printf("keep alive recibido!\n");
                if(close(n_s_alive) < 0){
                    perror("close n_s_alive");
                    exit(-1);
                }
            }else if(FD_ISSET(s_udp, &rfd_copia)){      // s_udp
                int l;
                cli_len = sizeof(cli_addr);

                leidos = recvfrom(s_udp, b, T, 0, (struct sockaddr *) &cli_addr, &cli_len);
                if (leidos < 0) {
                    perror("recvfrom s_udp");
                    exit(1);
                }
                b[leidos] = '\0';
                printf("Mensaje recibido: %s\n", b);
             
                char c[T];
                sustituir(b, c);        // ANCHOR sustituir
                l = strlen(c);
                printf("Enviando mensaje: %s, de tamaño %d\n", c, l);

                enviados = sendto(s_udp, &c, l,  0, (struct sockaddr *) &cli_addr, cli_len);
                if(enviados != l){
                    perror("sendto c");
                    exit(1);
                }
            }
        }

                // ANCHOR close
        
        if(close(s_alive) < 0){
            perror("close s_alive");
            exit(-1);
        }
        if(close(s_udp) < 0){
            perror("close s_udp");
            exit(-1);
        }
    }

    return 0;
}

                // ANCHOR sustituir

void sustituir(char *origen, char * destino){
    *destino = '\0';
    char aux[1];
    for(int i = 0; i < strlen(origen); i++){
        if(isdigit(origen[i])){
            strcat(destino, "|");
            char a[1];
            sprintf(a, &origen[i]);
            a[1] = '\0';
            int j = atoi(&a[0]);
            for(int k = 0; k < j; k++){
                strcat(destino, "*");
            }
            strcat(destino, "|");
        }else{
            *aux = origen[i];
            aux[1] = '\0';
            strcat(destino, aux);       // Si no es num añadimos la letra al final
        }
    }
    destino[strlen(destino)] = '\0';
}

                //  ANCHOR  copia

void copia_fd_set(fd_set *destino, fd_set *origen, int size){
    FD_ZERO(destino);
    for(int i = 0; i < size; i++){
        if(FD_ISSET(i, origen)){
            FD_SET(i, destino);
        }
    }
}

                //  ANCHOR  maxfds

int maxfds(fd_set fd, int max){
    if(sizeof(fd) > max){
        return max +1;
    }else{
        return sizeof(fd) +1;
    }
}
                //  ANCHOR  read_n

ssize_t read_n(int fifo1, char * mensaje, size_t longitud_mensaje) {
  ssize_t a_leer = longitud_mensaje;
  ssize_t total_leido = 0;
  ssize_t leido;
  
  do {
    errno = 0;
    leido = read(fifo1, mensaje + total_leido, a_leer);
    if (leido >= 0) {
      total_leido += leido;
      a_leer -= leido;
    }
  } while((
      (leido > 0) && (total_leido < longitud_mensaje)) ||
      (errno == EINTR));
  
  if (total_leido > 0) {
    return total_leido;
  } else {
    /* Para permitir que devuelva un posible error en la llamada a read() */
    return leido;
  }
}
                //  ANCHOR write_n

ssize_t write_n(int fifo1, char * mensaje, size_t longitud_mensaje) {
  ssize_t a_escribir = longitud_mensaje;
  ssize_t total_escrito = 0;
  ssize_t escrito;
  
  do {
    errno = 0;
    escrito = write(fifo1, mensaje + total_escrito, a_escribir);
    if (escrito >= 0) {
      total_escrito += escrito ;
      a_escribir -= escrito ;
    }
  } while(
        ((escrito > 0) && (total_escrito < longitud_mensaje)) ||
        (errno == EINTR));
  
  if (total_escrito > 0) {
    return total_escrito;
  } else {
    /* Para permitir que devuelva un posible error de la llamada a write */
    return escrito;
  }
}