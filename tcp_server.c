#include <unistd.h>
#include <fcntl.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <ctype.h>

                // ANCHOR ctes globales

#define T 128
#define MAX 1024
#define P_SERV 2119
#define P_ALIVE 2120
#define PUERTO_ORQUESTADOR 4000
uint16_t identificator = 1;

                // ANCHOR funciones auxiliares

ssize_t read_n(int fifo1, char * mensaje, size_t longitud_mensaje);
ssize_t write_n(int fifo1, char * mensaje, size_t longitud_mensaje);
int maxfds(fd_set fd, int max);
void copia_fd_set(fd_set *destino, fd_set *origen, int size);

                // ANCHOR main 

int main(int argc, char *argv[]){
    
    char b[T];
    uint8_t ACK;

    if(argc < 2){
        printf("Uso: %s <dirección IP>\n", argv[0]);
        exit(-1);
    }

    int s_reg;
    int leidos, enviados;
    uint32_t IP_orquestador;
    struct sockaddr_in orq_addr;

    IP_orquestador = inet_addr(argv[1]);
    if(IP_orquestador < 0){
        perror("inet_addr IP_orquestador");
        exit(-1);
    }

                // Socket descriptor

    s_reg = socket(PF_INET, SOCK_STREAM, 0);
    if(s_reg < 0){
        perror("Socket Descriptor s_reg");
        exit(-1);
    }

                // sockaddr_in

    memset(&orq_addr, 0, sizeof(orq_addr));
    orq_addr.sin_family = AF_INET;
    memcpy(&orq_addr.sin_addr, &IP_orquestador, 4);
    orq_addr.sin_port = htons(PUERTO_ORQUESTADOR);

                // connect

    if(connect(s_reg, (struct sockaddr*) &orq_addr, sizeof(orq_addr)) < 0){
        perror("connect s_reg");
        exit(-1);
    }

                // send

    uint16_t p_serv = htons(P_SERV);
    uint16_t p_alive = htons(P_ALIVE);
    uint16_t identificador = htons(identificator);

    enviados = send(s_reg, (char*) &identificador, sizeof(identificador), 0);
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
        exit(-1);
    }
    printf("Recibido: %d\n", ACK);
    if(close(s_reg) < 0){
            perror("close sd_orquestador");
            exit(-1);
    }
    if(ACK == 1){
        printf("Servicio ya ofrecido por otro servidor\n");
    }else{
        printf("Server registrado.\n");

                // ANCHOR s_alive
        
        int s_alive, n_s_alive;
        struct sockaddr_in serv_addr_s_alive;       // Direccion server
        struct sockaddr_in cli_addr_s_alive;
        socklen_t addr_len_s_alive;

                // socket descriptor y sockaddr_inc

        if((s_alive = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            perror("s_alive");
            exit(-1);
        }
        memset(&serv_addr_s_alive, 0, sizeof(serv_addr_s_alive));
        serv_addr_s_alive.sin_family = AF_INET;
        serv_addr_s_alive.sin_addr.s_addr = INADDR_ANY;     // Solo puerto
        serv_addr_s_alive.sin_port = htons(P_ALIVE);


                // bind

         if(bind(s_alive, (struct sockaddr*) &serv_addr_s_alive, sizeof(serv_addr_s_alive)) < 0){
            perror("bind s_alive");
            exit(-1);
        }

                // listen

        if (listen(s_alive, 1) < 0) {
            perror("listen s_alive");
            exit(-1);
        }

                // ANCHOR s_file

        int s_file, n_s_file;
        struct sockaddr_in serv_addr_s_file;       // Direccion server
        struct sockaddr_in cli_addr_s_file;
        socklen_t addr_len_s_file;

                // socket descriptor y sockaddr_in

        if((s_file = socket(PF_INET, SOCK_STREAM, 0)) < 0){
            perror("s_file");
            exit(-1);
        }
        memset(&serv_addr_s_file, 0, sizeof(serv_addr_s_file));
        serv_addr_s_file.sin_family = AF_INET;
        serv_addr_s_file.sin_addr.s_addr = INADDR_ANY;     // Solo puerto
        serv_addr_s_file.sin_port = htons(P_SERV);


                // bind

         if(bind(s_file, (struct sockaddr*) &serv_addr_s_file, sizeof(serv_addr_s_file)) < 0){
            perror("bind s_file");
            exit(-1);
        }

                // listen

        if (listen(s_file, 1) < 0) {
            perror("listen s_file");
            exit(-1);
        }

                // ANCHOR select

        fd_set rfd, rfd_copia;
        FD_ZERO(&rfd);      //Inicializo conjunto descriptores
        FD_SET(0, &rfd);        //Añado el descriptor de teclado
        FD_SET(s_alive, &rfd);     //Añado s_alive
        FD_SET(s_file, &rfd);     //Añado s_udp
        uint8_t indiceFichero = 1;

        while(1){
        
        copia_fd_set(&rfd_copia, &rfd, sizeof(rfd));      //Hago la copia
            int result = select(maxfds(rfd_copia, MAX), &rfd_copia, NULL, NULL, NULL);
            if(result < 0){
                perror("Select");
                exit(-1);
            }  
            if(FD_ISSET(s_alive, &rfd_copia)){
            // Se acepta la conexión y se cierra, con eso basta   
                addr_len_s_alive = sizeof(cli_addr_s_alive);             
                n_s_alive = accept(s_alive,  (struct sockaddr*) &cli_addr_s_alive, &addr_len_s_alive);
                if(n_s_alive < 0){
                    perror("accept n_s_alive");
                    exit(-1);
                }
                printf("keep alive recibido!\n");
                if(close(n_s_alive) < 0){
                    perror("close n_s_alive");
                    exit(-1);
                }
            }else if(FD_ISSET(s_file, &rfd_copia)){
                addr_len_s_file = sizeof(cli_addr_s_file);                            
                n_s_file = accept(s_file, (struct sockaddr*) &cli_addr_s_file, &addr_len_s_file);
                if(n_s_file < 0){
                    perror("accept n_s_file");
                    exit(-1);
                }

                char nombre[T];
                int fd;
                sprintf(nombre, "file%d.dat", indiceFichero);
                if(   (fd = open(nombre, O_WRONLY | O_CREAT | O_TRUNC, 0644))  < 0){
                    perror("Open fd_fich");
                    exit(-1);
                }

                printf("Recibiendo datos del cliente...\n");
                uint16_t tam;
                leidos = recv(n_s_file, &tam, 2, 0);
                if(leidos != sizeof(tam)){
                    perror("recv tam 1");
                    exit(-1);
                }
                leidos = recv(n_s_file, b, tam, 0);
                if(leidos != tam){
                    perror("recv b1");
                    exit(-1);
                }
                b[tam] = '\0';
                //printf("Recibido: %s\n", b);
                write_n(fd, b, tam);
                while(tam != 0){

                    leidos = recv(n_s_file, &tam, 2, 0);
                    if(leidos != sizeof(tam)){
                        perror("recv tam 2");
                        exit(-1);
                    }
                    leidos = recv(n_s_file, b, tam, 0);
                    if(leidos != tam){
                        perror("recv b2");
                        exit(-1);
                    }
                    b[tam] = '\0';
                    //printf("Recibido: %s\n", b);
                    write_n(fd, b, tam);
                }
                printf("Fichero recibido!\n");
                if(close(fd) < 0){
                    perror("close fd");
                    exit(-1);
                }
                if(close(n_s_file) < 0){
                    perror("close n_s_file");
                    exit(-1);
                }
                indiceFichero ++;
            }
        }

                // ANCHOR close

        if(close(s_alive) < 0){
            perror("close s_alive");
            exit(-1);
        }
        if(close(s_file) < 0){
            perror("close s_file");
            exit(-1);
        }


    }

    return 0;
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