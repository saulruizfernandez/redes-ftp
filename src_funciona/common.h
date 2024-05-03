// Common functions

// Authors: Daniel Pérez Rodríguez, Saúl Ruiz Fernández
// Date: 29/05/2024

#ifndef COMMON_H
#define COMMON_H

#include <cstdlib>

inline void errexit(const char *format, ...)

{
  va_list args;                    // Variable argument list
  va_start(args, format);          // Initialize the variable argument list
  vfprintf(stderr, format, args);  // Print the error message
  va_end(args);                    // End using the variable argument list
  exit(1);                         // Exit the program
}

#endif