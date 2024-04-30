// Main program for the FTP server

// Authors: Daniel Pérez Rodríguez, Saúl Ruiz Fernández
// Date: 29/05/2024

#include <signal.h>

#include <iostream>

#include "FTPServer.h"

FTPServer *server;

// Signal handler definition
extern "C" void sighandler(int signal, siginfo_t *info, void *ptr) {
  std::cout << "Trigger sigaction" << std::endl;
  server->stop();
  exit(-1);
}

// Function to stop the server
void exit_handler() { server->stop(); }

int main(int argc, char **argv) {
  // Signal handler
  struct sigaction action;
  // sa_sigaction: pointer to the signal-catching function
  action.sa_sigaction = sighandler;
  // sa_flags: special flags
  action.sa_flags = SA_SIGINFO;
  // sigaction: allows to change the action taken by a process on receipt of a
  // specific signal
  sigaction(SIGINT, &action, NULL);
  // Create the server
  server = new FTPServer(2121);
  // Register the exit handler
  atexit(exit_handler);
  // Start the server
  server->run();
}
