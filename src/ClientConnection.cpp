// Implementation of the ClientConnection class

// Authors: Daniel Pérez Rodríguez, Saúl Ruiz Fernández
// Date: 29/05/2024

#include "ClientConnection.h"

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

#include "FTPServer.h"
#include "common.h"

// Definition of the constructor
ClientConnection::ClientConnection(int s) {
  int sock = (int)(s);
  char buffer[MAX_BUFF];
  control_socket = s;
  // fdopen() opens the file for control connection from the control socket
  // Open for reading and appending at the end of the file (a+)
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

// Definition of the destructor
ClientConnection::~ClientConnection() {
  fclose(fd);
  close(control_socket);
}

// This function creates a TCP socket for data sending and connects it to the
// address and port
int connect_TCP(uint32_t address, uint16_t port) {
  struct sockaddr_in sin;
  int s;  // Socket descriptor

  memset(&sin, 0, sizeof(sin));   // Fill the struct with zeros (good practise)
  sin.sin_family = AF_INET;       // IPv4
  sin.sin_addr.s_addr = address;  // Address
  sin.sin_port = htons(port);     // Port

  s = socket(AF_INET, SOCK_STREAM, 0);
  std::cout << "socket: " << s << std::endl;
  if (s < 0) {
    errexit("No se puede crear el socket: %s\n", strerror(errno));
  }
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    close(s);
    // Dar formato a host
    errexit("No se puede conectar con %s\n", "cliente", strerror(errno));
  }
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
  fflush(fd);

  while (!parar) {
    fscanf(fd, "%s", command);
    fflush(fd);
    if (COMMAND("USER")) {
      fscanf(fd, "%s", arg);
      fflush(fd);
      fprintf(fd, "331 User name ok, need password\n");
      fflush(fd);
    } else if (COMMAND("PWD")) {  // Print Working Directory on the server for
                                  // the logged user
      // To be implemented by students
      char cwd[MAX_BUFF];
      getcwd(cwd, sizeof(cwd));
      fprintf(fd, "257 %s\n", cwd);
      fflush(fd);
    } else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      fflush(fd);
      if (strcmp(arg, "1234") == 0) {
        fprintf(fd, "230 User logged in\n");
        fflush(fd);
      } else {
        fprintf(fd, "530 Not logged in.\n");
        fflush(fd);
        parar = true;
      }
    } else if (COMMAND("PASV")) {
      // To be implemented by students
      int s = define_socket_TCP(0);
      struct sockaddr_in fsin;
      socklen_t len = sizeof(fsin);
      getsockname(s, (struct sockaddr *)&fsin, &len);
      uint16_t port = fsin.sin_port;
      int p1 = (port >> 8) & 0xff;
      int p2 = port & 0xff;
      fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\n", p1, p2);
      fflush(fd);
      data_socket = accept(s, (struct sockaddr *)&fsin, &len);
      fprintf(fd, "200 Command okay.\n");
      fflush(fd);
    } else if (COMMAND(
                   "PORT")) {  // The IP and the port are from the client, the
                               // server is at IP_ANNY(0.0.0.0/0) and port 2121
      fscanf(fd, "%s", arg);
      fflush(fd);
      std::cout << "arg: " << arg << std::endl;
      int a1, a2, a3, a4, p1,
          p2;  // ai -> address components, pi -> port components
      sscanf(arg, "%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, &a4, &p1, &p2);
      uint16_t port = p1 << 8 | p2;
      uint32_t ip =
          a1 << 24 | a2 << 16 | a3 << 8 |
          a4;  // If it does not work, check connect_TCP() implementation
      std::cout << "ip: " << ip << std::endl;
      std::cout << "port: " << port << std::endl;
      data_socket = connect_TCP(ip, port);
      std::cout << "data_socket: " << data_socket << std::endl;
      fprintf(fd, "200 Command okay.\n");
      fflush(fd);
    } else if (COMMAND("STOR")) {  // Uploads a copy of a file to the server,
                                   // replacing it if it exists
      // To be implemented by students
      fscanf(fd, "%s", arg);
      fflush(fd);
      FILE *f = fopen(arg, "wb");
      if (f == NULL) {
        fprintf(fd, "550 File not found.\n");
        fflush(fd);
        continue;
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);
      }
      char *buffer;
      while (1) {
        recv(data_socket, buffer, MAX_BUFF, 0);
        printf("%s\n", buffer);
        if (sizeof(buffer) == 0) {
          break;
        }
      }
      fprintf(
          fd,
          "226 Closing data connection. Requested file action successful.\n");
      fflush(fd);
      close(data_socket);
      fclose(f);
    } else if (COMMAND("RETR")) {  // Hace que el servidor transfiera una copia
                                   // del archivo, especificado en el argumento,
                                   // al cliente.
      fscanf(fd, "%s", arg);
      fflush(fd);
      // Abrir el fichero en modo lectura binario
      FILE *f = fopen(arg, "rb");
      if (f == NULL) {
        fprintf(fd, "550 File not found.\n");
        fflush(fd);
        continue;
      } else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);
      }
      while (1) {
        char buffer[MAX_BUFF];
        int n = fread(buffer, 1, MAX_BUFF, f);
        if (n == 0) break;
        send(data_socket, buffer, n, 0);
        if (n <
            MAX_BUFF) {  // If it does not reach the end, it is the last package
          break;
        }
      }
      fprintf(
          fd,
          "226 Closing data connection. Requested file action successful.\n");
      fflush(fd);
      close(data_socket);
      fclose(f);
    } else if (COMMAND(
                   "LIST")) {  // Hace que el servidor envíe una lista de los
                               // archivos en el directorio actual al cliente.
      // To be implemented by students
      struct dirent *e;
      DIR *d = opendir(".");
      fprintf(fd, "150 Here comes the directory listing.\n");
      fflush(fd);
      while (e = readdir(d)) {
        send(data_socket, e->d_name, strlen(e->d_name), 0);
      }
      fprintf(fd, "226 Directory send OK.\n");
      fflush(fd);
      close(data_socket);
      closedir(d);

    } else if (COMMAND("SYST")) {
      fprintf(fd, "215 UNIX Type: L8.\n");
      fflush(fd);
    } else if (COMMAND("TYPE")) {
      fscanf(fd, "%s", arg);
      fflush(fd);
      fprintf(fd, "200 OK\n");
      fflush(fd);
    } else if (COMMAND("QUIT")) {
      fprintf(fd,
              "221 Service closing control connection. Logged out if "
              "appropriate.\n");
      fflush(fd);
      close(data_socket);
      parar = true;
      break;
    } else {
      fprintf(fd, "502 Command not implemented.\n");
      fflush(fd);
      printf("Comando : %s %s\n", command, arg);
      printf("Error interno del servidor\n");
    }
  }

  fclose(fd);

  return;
};
