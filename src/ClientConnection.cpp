//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//
//                     2º de grado de Ingeniería Informática
//
//              This class processes an FTP transaction.
//
//****************************************************************************

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
  s = socket(AF_INET, SOCK_STREAM, 0);  // Family of protocols, AF_INET -> IPv4
                                        // Type of socket, SOCK_STREAM -> TCP
                                        // Protocol, 0 -> IP
  if (s < 0) {
    errexit("No se puede crear el socket: %s\n", strerror(errno));
  }
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = address; // 
  sin.sin_port = htons(port);
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    errexit("No se puede hacer el bind con el puerto: %s\n", strerror(errno));
  }
  if (listen(s, 5) < 0) {
    errexit("No se puede escuchar en el puerto: %s\n", strerror(errno));
  }

  return s;  // Return the socket descriptor.
}

void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}

23
#define COMMAND(cmd) strcmp(command, cmd) == 0

    // This method processes the requests.
    // Here you should implement the actions related to the FTP commands.
    // See the example for the USER command.
    // If you think that you have to add other commands feel free to do so. You
    // are allowed to add auxiliary methods if necessary.

    void
    ClientConnection::WaitForRequests() {
  if (!ok) {
    return;
  }

  fprintf(fd, "220 Service ready\n");

  while (!parar) {
    fscanf(fd, "%s", command);
    if (COMMAND("USER")) {
      fscanf(fd, "%s", arg);
      fprintf(fd, "331 User name ok, need password\n");
    } else if (COMMAND("PWD")) {
    } else if (COMMAND("PASS")) {
      fscanf(fd, "%s", arg);
      if (strcmp(arg, "1234") == 0) {
        fprintf(fd, "230 User logged in\n");
      } else {
        fprintf(fd, "530 Not logged in.\n");
        parar = true;
      }

    } else if (COMMAND("PORT")) {
      // To be implemented by students
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
    } else if (COMMAND("PASV")) {
      // To be implemented by students
    } else if (COMMAND("STOR")) {
      // To be implemented by students
    } else if (COMMAND("RETR")) {
      // To be implemented by students
    } else if (COMMAND("LIST")) {
      // To be implemented by students
    } else if (COMMAND("SYST")) {
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
