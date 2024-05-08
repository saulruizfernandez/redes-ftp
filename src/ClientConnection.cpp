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
  struct hostent *hent;
  int s;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  if (hent == gethostbyaddr((const void *)&address, sizeof(address), AF_INET)) {
    memcpy(&sin.sin_addr, hent->h_addr, hent->h_length);
  }
  else if (sin.sin_addr.s_addr == address)
    errexit("No puedo resolver el nombre \"%s\"\n", address);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) errexit("No se puede crear el socket: %s\n", strerror(errno));

  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    struct in_addr ip_addr;
    ip_addr.s_addr = address;
    errexit("No se puede conectar con %s: %s\n", inet_ntoa(ip_addr), strerror(errno));
  }
  return s;
}

// Close client connection
void ClientConnection::stop() {
  close(data_socket);
  close(control_socket);
  parar = true;
}

// Check if the command is the one passed as argument
#define COMMAND(cmd) strcmp(command, cmd) == 0

// This method processes the requests.
// Implements the actions related to the FTP commands.
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
    } else if (COMMAND("PWD")) {  // Print Working Directory of the server
      char cwd[MAX_BUFF];         // Current Working Directory
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
    }  else if (COMMAND(
                   "PORT")) {  // The IP and the port are from the client, the
                               // server is at IP_ANY(0.0.0.0/0) and port 2121
      //fscanf(fd, "%s", arg);
      //fflush(fd);
      int a1, a2, a3, a4, p1, p2;  // ai -> address components, pi -> port components
      fscanf(fd, "%d,%d,%d,%d,%d,%d", &a1, &a2, &a3, &a4, &p1, &p2);
      uint16_t port = (p1 << 8 | p2);
      uint32_t ip = (a1 << 24 | a2 << 16 | a3 << 8 | a4);
      // In active mode, the server connects to the client for transferring data
      data_socket = connect_TCP(ip, port);
      if (data_socket < 0) {
        errexit("Error connecting to the client: %s\n", strerror(errno));
      }
      else {
        fprintf(fd, "200 Command okay.\n");
      }
    } else if (COMMAND("PASV")) {    // The server opens a port and listens for
                                     // the client to connect to it
      int s = define_socket_TCP(0);  // Passive listening in port 0
      struct sockaddr_in fsin;       // Address of the client
      socklen_t len = sizeof(fsin);  // Size of the address
      // Get the address of the server and save it to the client
      getsockname(s, (struct sockaddr *)&fsin, &len);
      uint16_t port = fsin.sin_port;  // Get the port of the server
      uint32_t address = fsin.sin_addr.s_addr;  // Get the IP of the server
      int p2 = (port >> 8) & 0xFF;
      int p1 = port & 0xFF;
      fprintf(fd, "227 Entering Passive Mode (127,0,0,1,%d,%d).\n", p1, p2);
      fflush(fd);
      // Accept a new connection form the client for data transferring
      // based on the same address family and type protocol as the socket
      // which binded to the port 0 to listen the incoming connections
      data_socket = accept(s, (struct sockaddr *)&fsin, &len);
      //fprintf(fd, "200 Command okay.\n");
      //fflush(fd);
      close(s);
    } else if (COMMAND("STOR")) {  // Uploads a copy of a file to the server,
                                   // replacing it if it exists (put)
      fscanf(fd, "%s", arg);
      //fflush(fd);
      // Open the file in write mode
      FILE *f = fopen(arg, "w");
      if (f == NULL) {
        fprintf(fd, "550 File not found.\n");
        fflush(fd);
      } 
      else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);
        char buffer[MAX_BUFF];
        int n;
        while ((n = recv(data_socket, buffer, sizeof(buffer), 0)) > 0) {
          if (fwrite(buffer, 1, n, f) < n) {
            fprintf(fd, "426 Connection closed; transfer aborted.\n");
            fflush(fd);
            break;
          }
        }
        fclose(f);
        close(data_socket);
        fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
        fflush(fd);
      }
    } else if (COMMAND("RETR")) {  // Hace que el servidor transfiera una copia
                                   // del archivo, especificado en el argumento,
                                   // al cliente.
      fscanf(fd, "%s", arg);
      //fflush(fd);
      // Abrir el fichero en modo lectura binario
      FILE *f = fopen(arg, "r");
      if (f == NULL) {
        fprintf(fd, "550 File not found.\n");
        fflush(fd);
        //continue;
      } 
      else {
        fprintf(fd, "150 File status okay; about to open data connection.\n");
        fflush(fd);
        char buffer[MAX_BUFF];
        int n;
        while ((n = fread(buffer, 1, sizeof(buffer), f)) > 0) {
          if (send(data_socket, buffer, n, 0) < 0) {
            fprintf(fd, "426 Connection closed; transfer aborted.\n");
            fflush(fd);
            break;
          }
        }
        fclose(f);
        close(data_socket);
        fprintf(fd, "226 Closing data connection. Requested file action successful.\n");
        fflush(fd);
      }
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
        send(data_socket, " ", 1, 0); // Print a space after each directory entry
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
      printf("Command : %s %s\n", command, arg);
      printf("Server internal error\n");
    }
  }

  fclose(fd);

  return;
};
