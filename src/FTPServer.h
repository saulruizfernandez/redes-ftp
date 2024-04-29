// Class definition for FTPServer

// Authors: Daniel Pérez Rodríguez, Saúl Ruiz Fernández
// Date: 29/05/2024

#if !defined FTPServer_H
#define FTPServer_H

#include <list>

#include "ClientConnection.h"

int define_socket_TCP(int port);
void* run_client_connection(void *c);

class FTPServer {
public:
  FTPServer(int port = 21); // Constructor
  void run(); // Start the server
  void stop(); // Stop the server

private:
  int port; // Port where the server will be listening
  int msock; // Socket descriptor
  std::list<ClientConnection*> connection_list; // List of connections
};

#endif