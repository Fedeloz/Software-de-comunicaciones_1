#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>

                // ANCHOR ctes Globales

#define MAX_SRV 10
#define T 128
#define S_REG 4000
#define S_SERV 4001
#define MAX 1024

                // ANCHOR Variables GLobales

struct DatosServicio{
    uint32_t ip;
    uint16_t p_serv;
    uint16_t p_alive;
};

struct DatosServicio d[MAX_SRV];

                // ANCHOR Funciones auxiliares

void copia_fd_set(fd_set *destino, fd_set *origen, int size);
int maxfds(fd_set fd, int max);
ssize_t read_n(int fifo1, char * mensaje, size_t longitud_mensaje);
ssize_t write_n(int fifo1, char * mensaje, size_t longitud_mensaje);

                // ANCHOR main

int main(int argc, char *argv[]){

    for(unsigned i = 0; i < MAX_SRV; i++){       // inicialización struct
        d[i].p_serv = 0;
    }

                // Variables main

    //char b[T];
    int leidos, enviados;

    struct timeval t, t_a;
    t.tv_sec = 5;
    t.tv_usec = 500000;
    t_a = t;

                // ANCHOR s_reg socket/bind/read/listen->accept

    int s_reg, n_s_reg;
    struct sockaddr_in serv_addr_s_reg;       // Direccion server
    struct sockaddr_in cli_addr_s_reg;
    socklen_t addr_len_s_reg;


                // socket descriptor y sockaddr_in

    if((s_reg = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("s_reg");
        exit(-1);
    }
    memset(&serv_addr_s_reg, 0, sizeof(serv_addr_s_reg));
    serv_addr_s_reg.sin_family = AF_INET;
    serv_addr_s_reg.sin_addr.s_addr = INADDR_ANY;     // Solo puerto
    serv_addr_s_reg.sin_port = htons(S_REG);

                // bind

    if(bind(s_reg, (struct sockaddr*) &serv_addr_s_reg, sizeof(serv_addr_s_reg)) < 0){
        perror("bind s_reg");
        exit(-1);
    }

                // listen

    if (listen(s_reg, 1) < 0) {
        perror("listen s_reg");
        exit(-1);
    }

                // ANCHOR s_serv

    int s_serv, n_s_serv;
    struct sockaddr_in serv_addr_s_serv;       // Direccion server
    struct sockaddr_in cli_addr_s_serv;
    socklen_t addr_len_s_serv;


                // socket descriptor y sockaddr_in

    if((s_serv = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("s_serv");
        exit(-1);
    }
    memset(&serv_addr_s_serv, 0, sizeof(serv_addr_s_serv));
    serv_addr_s_serv.sin_family = AF_INET;
    serv_addr_s_serv.sin_addr.s_addr = INADDR_ANY;     // Solo puerto
    serv_addr_s_serv.sin_port = htons(S_SERV);

                // bind

    if(bind(s_serv, (struct sockaddr*) &serv_addr_s_serv, sizeof(serv_addr_s_serv)) < 0){
        perror("bind s_serv");
        exit(-1);
    }

                // listen

    if (listen(s_serv, 1) < 0) {
        perror("listen s_serv");
        exit(-1);
    }

                // ANCHOR select

    fd_set rfd, rfd_copia;

    FD_ZERO(&rfd);      //Inicializo conjunto descriptores
    FD_SET(0, &rfd);        //Añado el descriptor de teclado
    FD_SET(s_reg, &rfd);     // Añado s_reg
    FD_SET(s_serv, &rfd);     // Añado s_serv

    printf("El orquestador está corriendo!\n");

    while(1){
        copia_fd_set(&rfd_copia, &rfd, sizeof(rfd));      // Hacer copia
        int result = select(maxfds(rfd_copia, MAX), &rfd_copia, NULL, NULL, &t_a);      //  ANCHOR  Select
        if(result < 0){
            perror("Select");
            exit(-1);
        }
        if(FD_ISSET(s_reg, &rfd_copia)){
            uint8_t ACK = 0;
            uint16_t identificador;
            uint16_t p_serv_;
            uint16_t p_alive_;
            addr_len_s_reg = sizeof(cli_addr_s_reg);
            
            n_s_reg = accept(s_reg, (struct sockaddr*) &cli_addr_s_reg, &addr_len_s_reg);
            if(n_s_reg < 0){
                perror("accept n_s_reg");
                exit(-1);
            }
            leidos = recv(n_s_reg, &identificador, 2, 0);
            if(leidos != sizeof(identificador)){
                perror("recv identificador n_s_reg");
                exit(-1);
            }
            identificador = ntohs(identificador);
            leidos = recv(n_s_reg, &p_serv_, 2, 0);
            if(leidos != sizeof(p_serv_)){
                perror("recv p_serv_ n_s_reg");
                exit(-1);
            }
            p_serv_ = ntohs(p_serv_);
            leidos = recv(n_s_reg, &p_alive_, 2, 0);
            if(leidos != sizeof(p_alive_)){
                perror("recv p_alive_ n_s_reg");
                exit(-1);
            }
            p_alive_ = htons(p_alive_);
            printf("Identificador de servidor recibido: %d\n", identificador);
            if((identificador != 1) && (identificador != 0)){
                ACK = 1;
            }else if(d[identificador].p_serv != 0){
                ACK = 1;
            }
            enviados = send(n_s_reg, &ACK, 1, 0);
            if(enviados != sizeof(ACK)){
                perror("send ACK n_s_reg");
                exit(-1);
            }
            printf("ACK  = %d enviado\n", ACK);
            if(close(n_s_reg) < 0){
                perror("close n_s_reg");
                exit(-1);
            }
            if(ACK == 0){
                d[identificador].ip = cli_addr_s_reg.sin_addr.s_addr;
                d[identificador].p_serv = p_serv_;
                d[identificador].p_alive = p_alive_;
                printf("Servidor registrado:\nIp: %s\np_serv: %d\np_alive: %d\n", inet_ntoa(cli_addr_s_reg.sin_addr), d[identificador].p_serv, d[identificador].p_alive);
            }
        }
        if(FD_ISSET(s_serv, &rfd_copia)){
            uint8_t ACK = 0;
            uint16_t identificador;
            uint16_t p_serv_;
            uint32_t ip;
            addr_len_s_serv = sizeof(cli_addr_s_serv);

            n_s_serv = accept(s_serv, (struct sockaddr*) &cli_addr_s_serv, &addr_len_s_serv);
            if(n_s_serv < 0){
                perror("accept n_s_serv");
                exit(-1);
            }
            leidos = recv(n_s_serv, &identificador, 2, 0);
            if(leidos != sizeof(identificador)){
                perror("recv identificador n_s_serv");
                exit(-1);
            }
            identificador = ntohs(identificador);
            printf("Identificador de cliente recibido: %d\n", identificador);
            if((identificador != 1) && (identificador != 0)){
                ACK = 1;
            }else if(d[identificador].p_serv == 0){
                ACK = 1;
            }
            enviados = send(n_s_serv, &ACK, 1, 0);      // No se hace conversión a host de ACK, sólo 1 byte
            if(enviados != sizeof(ACK)){
                perror("send ACK n_s_serv");
                exit(-1);
            }
            printf("ACK enviado: %d\n", ACK);

            ip = d[identificador].ip;
            p_serv_ = htons(d[identificador].p_serv);

            if(ACK == 0){
                enviados = send(n_s_serv, &ip, sizeof(uint32_t), 0);
                if(enviados != sizeof(ip)){
                    perror("send ip");
                    exit(-1);
                }
                enviados = send(n_s_serv, &p_serv_, sizeof(uint16_t), 0);
                if(enviados != sizeof(p_serv_)){
                    perror("send o_serv_");
                    exit(-1);
                }
            }
            if(close(n_s_serv) < 0){
                perror("close n_s_serv");
                exit(-1);
            }
        }
        if(result == 0){      // timeout s_alive

            for(unsigned i = 0; i < MAX_SRV; i++){

                if(d[i].p_serv != 0){

                // ANCHOR s_alive

                    int s_alive;

                                // Socket descriptor

                    s_alive = socket(PF_INET, SOCK_STREAM, 0);
                    if(s_alive < 0){
                        perror("s_alive");
                        exit(-1);
                    }

                    struct sockaddr_in tcp_addr_s_alive;
                    socklen_t tcp_len_s_alive;
                    tcp_len_s_alive = sizeof(tcp_addr_s_alive);
                    
                    memset(&tcp_addr_s_alive, 0, sizeof(tcp_addr_s_alive));
                    memcpy(&tcp_addr_s_alive.sin_addr, &d[i].ip, 4);
                    tcp_addr_s_alive.sin_family = AF_INET;
                    tcp_addr_s_alive.sin_port = ntohs(d[i].p_alive);

                    //printf("Keep alive enviado a server con identificador: %d\nPuerto: %d\nIP: %s\n", i, htons(tcp_addr_s_alive.sin_port), inet_ntoa(tcp_addr_s_alive.sin_addr));  
                    printf("Keep alive enviado a server con identificador: %d\n", i);  

                    if(connect(s_alive, (struct sockaddr*) &tcp_addr_s_alive, tcp_len_s_alive) == 0){
                        if(close(s_alive) < 0){
                            perror("close s_alive 1");
                            exit(-1);
                        }
                    }else{
                        printf("Reintentando...\n");
                        sleep(1);
                        if(connect(s_alive, (struct sockaddr*) &tcp_addr_s_alive, tcp_len_s_alive) == 0){
                            if(close(s_alive) < 0){
                                perror("close s_alive 2");
                                exit(-1);
                            }
                        }else{
                            printf("Reintentando...\n");
                            sleep(1);
                            if(connect(s_alive, (struct sockaddr*) &tcp_addr_s_alive, tcp_len_s_alive) == 0){
                                if(close(s_alive) < 0){
                                    perror("close s_alive 3");
                                    exit(-1);
                                }
                            }else{
                                printf("Server desconectado\n");                                
                                d[i].p_serv = 0;
                            }
                        }
                    }
                }
            }
            t_a = t;        // Lo pongo aquí porque la comprobación ha de ser periódica
        }
    }
                // ANCHOR close

    if(close(s_reg) < 0){
        perror("Close s_reg");
        exit(-1);
    }
    if(close(s_serv) < 0){
        perror("Close s_serv");
        exit(-1);
    }


    return 0;
}

                // ANCHOR copia_fd_set

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
