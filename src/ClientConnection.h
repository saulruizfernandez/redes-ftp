// Class definition of ClientConnection, that manages the information between
// the server and each client.

// Authors: Daniel Pérez Rodríguez, Saúl Ruiz Fernández
// Date: 29/05/2024

#if !defined ClientConnection_H
#define ClientConnection_H

#include <pthread.h>

#include <cstdint>
#include <cstdio>
#include <string>

const int MAX_BUFF = 1000;  // Maximum buffer size of the data sent

class ClientConnection {
 public:
  ClientConnection(int s);  // Constructor
  ~ClientConnection();      // Destructor

  void WaitForRequests();
  void stop();

 private:
  bool ok;  // This variable is a flag that avoids that the
            // server listens if initialization errors occured.

  FILE *fd;  // C file descriptor. We use it to buffer the
             // control connection of the socket and it allows to
             // manage it as a C file using fprintf, fscanf, etc.

  char command[MAX_BUFF];  // Buffer for saving the FTP control command.
  char arg[MAX_BUFF];      // Buffer for saving the arguments.

  int data_socket;     // Data socket descriptor;
  int control_socket;  // Control socket descriptor;
  bool parar;          // Flag to stop the connection.
};

#endif
