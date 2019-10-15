// This C program does nothing important yet.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "../include/socket.h"

static const unsigned int PORT = 8080;
static const unsigned int BUFFER_LEN = 4096;

int main() {
  initializeUdpServer(PORT);
  printf("Socket available on port %d.\n", 8080);

  printf("On the host computer, run the following command and type in a message:\n");
  printf("  $ netcat -u 192.168.7.2 8080\n");

  printf("Waiting for message from host...\n");
  char buffer[BUFFER_LEN];
  struct sockaddr_in sktReturn;
  sktReturn = receiveThroughUdpServer(buffer, BUFFER_LEN);

  printf("Received message: %s\n", buffer);

  // Return the same content
  char *message = "Your message was: ";
  char *messageEnd = "Terminating server.\n";
  char returnBuffer[strlen(message) + BUFFER_LEN + strlen(messageEnd)];
  strcpy(returnBuffer, message);
  strcat(returnBuffer, buffer);
  strcat(returnBuffer, messageEnd);
  sendThroughUdpServer(sktReturn, returnBuffer);

  closeUdpServer();
}

