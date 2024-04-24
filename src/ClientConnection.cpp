//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2º de grado de Ingeniería Informática
//
//              This class processes an FTP transaction.
//
//****************************************************************************

#include "ClientConnection.h"
#include "FTPServer.h"

#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <langinfo.h>
#include <locale.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pwd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "common.h"

ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);

  char buffer[MAX_BUFF];

  control_socket = s;
  // Check the Linux man pages to know what fdopen does.
  fd = fdopen(s, "a+");
  if (fd == NULL) {
    std::cout << "Connection closed" << std::endl;

    fclose(fd);
    close(control_socket);
    ok = false;
    return;
  }

  ok = true;
  data_socket = -1;
  parar = false;
};

ClientConnection::~ClientConnection() {
  fclose(fd);
  close(control_socket);
}

int connect_TCP(uint32_t address, uint16_t port) {
  // Implement your code to define a socket here
  struct sockaddr_in sin;
  int s; // Socket descriptor

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = address; 
  sin.sin_port = htons(port);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    errexit("No se puede crear el socket: %s\n", strerror(errno));
  }
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    // Dar formato a host
    errexit("No se peude conectar con %s: %\n", "host", strerror(errno));
  return s;  // Return the socket descriptor.
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}

#define COMMAND(cmd) strcmp(command, cmd) == 0

    // This method processes the requests.
    // Here you should implement the actions related to the FTP commands.
    // See the example for the USER command.
    // If you think that you have to add other commands feel free to do so. You
    // are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
  if (!ok) {
    return;
  }

  fprintf(fd, "220 Service ready\n");

  while (!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "331 User name ok, need password\n");
    } 
    else if (COMMAND("PWD")) { // Print Working Directory on the server for the logged user
      // To be implemented by students
      char cwd[MAX_BUFF];
      getcwd(cwd, sizeof(cwd));
      fprintf(fd, "257 %s\n", cwd);
    }
    else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      if (strcmp(arg, "1234") == 0) {
        fprintf(fd, "230 User logged in\n");
      } else {
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }
    } 
    else if (COMMAND("PORT")) {
      fscanf(fd, "%s", arg);
      int a1, a2, a3, a4, p1,
          p2;  // ai -> address components, pi -> port components
      fscanf(fd, "%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, &a4, &p1, &p2);
      uint16_t port = p1 << 8 | p2;
      uint32_t ip =
          a1 << 24 | a2 << 16 | a3 << 8 |
          a4;  // If it does not work, check connect_TCP() implementation
      data_socket = connect_TCP(ip, port);
      fprintf(fd, "200 Command okay.\n");
    } 
    else if (COMMAND("PASV")) { 
      // To be implemented by students
      int s = define_socket_TCP(0);
      struct sockaddr_in fsin;
      socklen_t len = sizeof(fsin);
      getsockname(s, (struct sockaddr *)&fsin, &len);
      uint16_t port = fsin.sin_port;
      int p1 = (port >> 8) & 0xff;
      int p2 = port & 0xff;
      fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\n", p1, p2);
      data_socket = accept(s, (struct sockaddr *)&fsin, &len);
    } 
    else if (COMMAND("STOR")) { // Uploads a copy of a file to the server, replacing it if it exists
      // To be implemented by students
      fscanf(fd, "%s", arg);
      FILE *f = fopen(arg, "wb");
      if (f == NULL) {
        fprintf(fd, "550 File not found.\n");
        continue;
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
      }
      char* buffer;
      while(1) {
        recv(data_socket, buffer, MAX_BUFF, 0);
        printf("%s\n", buffer);
        if (sizeof(buffer) == 0) {
          break;
        }
      }
      fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
      close(data_socket);
      fclose(f);
    } 
    else if (COMMAND("RETR")) { // Hace que el servidor transfiera una copia del archivo, especificado en el argumento, al cliente.
      fscanf(fd, "%s", arg);
      // Abrir el fichero en modo lectura binario
      FILE *f = fopen(arg, "rb");
      if (f == NULL) {
        fprintf(fd, "550 File not found.\n");
        continue;
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
      }
      while (1) {
        char buffer[MAX_BUFF];
        int n = fread(buffer, 1, MAX_BUFF, f);
        if (n == 0) break;
        send(data_socket, buffer, n, 0);
        if (n < MAX_BUFF) { // If it does not reach the end, it is the last package
          break;
        }
      }
      fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
      close(data_socket);
      fclose(f);
    } 
    else if (COMMAND("LIST")) { // Hace que el servidor envíe una lista de los archivos en el directorio actual al cliente.
      // To be implemented by students
      struct dirent *e;
      DIR *d = opendir(".");
      fprintf(fd, "150 Here comes the directory listing.\n");
      while (e = readdir(d)) {
        send(data_socket, e->d_name, strlen(e->d_name), 0);
      }
      fprintf(fd, "226 Directory send OK.\n");
      close(data_socket);
      closedir(d);
      
    } 
    else if (COMMAND("SYST")) {
      fprintf(fd, "215 UNIX Type: L8.\n");
    }
    else if (COMMAND("TYPE")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "200 OK\n");
    }
    else if (COMMAND("QUIT")) {
      fprintf(fd,
              "221 Service closing control connection. Logged out if "
              "appropriate.\n");
      close(data_socket);
      parar = true;
      break;
    }
    else {
      fprintf(fd, "502 Command not implemented.\n");
      fflush(fd);
      printf("Comando : %s %s\n", command, arg);
      printf("Error interno del servidor\n");
    }
  }

  fclose(fd);

  return;
};
