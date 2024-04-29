// Class implementation of the FTPServer

// Authors: Daniel Pérez Rodríguez, Saúl Ruiz Fernández
// Date: 29/05/2024

#include "FTPServer.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <list>

#include "ClientConnection.h"
#include "common.h"

// This function creates a TCP socket and binds it to the port passed as
// argument. Bind is used to associate the socket with the port on the local
// machine. The function returns the socket descriptor.
int define_socket_TCP(int port) {
  struct sockaddr_in sin;  // Struct to store the address of the socket
  int s;
  // Create the socket
  // AF_INET: IPv4
  // SOCK_STREAM: TCP
  // 0: Protocol to use, 0 means that the default protocol will be used ->
  // Internet Protocol (IP)
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0) {
    errexit("Socket cannot be created: %s\n", strerror(errno));
  }
  memset(&sin, 0, sizeof(sin));  // Fill the struct with zeros (good practice)
  sin.sin_family = AF_INET;      // IPv4
  sin.sin_addr.s_addr =
      INADDR_ANY;  // Accept connections from any address (0.0.0.0)
  sin.sin_port =
      htons(port);  // htons() -> from host byte order to network byte order

  // Bind the socket to the port
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    errexit("Could not bind socket with the port: %s\n", strerror(errno));
  }
  // Start accepting incoming connections from clients
  // s: socket descriptor
  // 5: maximum number of clients that can be waiting for a connection
  if (listen(s, 5) < 0) {
    errexit("Error listening for incoming connections: %s\n", strerror(errno));
  }
  return s;
}

// This function is executed when the thread is executed.
void *run_client_connection(void *c) {
  ClientConnection *connection = (ClientConnection *)c;
  connection->WaitForRequests();
  return NULL;
}

// Constructor definition
FTPServer::FTPServer(int port) { this->port = port; }

// Server stops
void FTPServer::stop() {
  close(msock);
  shutdown(msock, SHUT_RDWR);
}

// Starting of the server
void FTPServer::run() {
  struct sockaddr_in fsin;
  int ssock;
  socklen_t alen = sizeof(fsin);
  msock = define_socket_TCP(port);
  while (true) {
    pthread_t thread;
    // Accept a connection from a client
    // msock: socket descriptor
    // fsin: struct to store the address of the client
    // alen: size of the struct
    
    ssock = accept(msock, (struct sockaddr *)&fsin, &alen);
    if (ssock < 0) errexit("Error in accept: %s\n", strerror(errno));
    ClientConnection *connection = new ClientConnection(ssock);
    // Here a thread is created in order to process multiple client-requests simultaneously
    pthread_create(&thread, NULL, run_client_connection, (void *)connection);
  }
}
