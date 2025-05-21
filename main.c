#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#include "globmatch.h"
#include "network.h"
#include "osc_config.h"
#include "tinyosc.h"

// debug send wrapper

size_t send_wrapper(connectionT *conn, const void *buf, size_t len) {
  ssize_t sent = sendto(conn->con.fd, buf, len, 0,
                        (struct sockaddr *)&conn->con.addr, conn->con.addr_len);

  printf("SENDING: ");

  for (int i = 0; i < len; i++) {
    char c = ((char *)buf)[i];
    if (isprint(c)) {
      printf("%c", c);
    } else {
      printf("(%02X)", c);
    }
  }
  printf("\n");

  if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    fd_set wfds;
    for (;;) {
      FD_ZERO(&wfds);
      FD_SET(conn->con.fd, &wfds);
      int r = select(conn->con.fd + 1, NULL, &wfds, NULL, NULL);
      if (r < 0) {
        if (errno == EINTR)
          continue;
        perror("select");
        return -1;
      }
      if (FD_ISSET(conn->con.fd, &wfds)) {
        sent = sendto(conn->con.fd, buf, len, 0,
                      (struct sockaddr *)&conn->con.addr, conn->con.addr_len);
        if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
          continue;
        break;
      }
    }
  } else if (sent < 0) {
    perror("sendto");
  }

  return (size_t)sent;
}

// main loop

bool keepRunning = true;

static void sigintHandler(int x) {
  (void)x;
  keepRunning = false;
}

int main(int argc, char *argv[]) {
  char buffer[2048];
  connectionT conn = {0};
  conn.send = send_wrapper;

  signal(SIGINT, sigintHandler);

  conn.con.fd = socket(AF_INET, SOCK_DGRAM, 0);
  fcntl(conn.con.fd, F_SETFL, O_NONBLOCK);

  struct sockaddr_in sin = {0};
  sin.sin_family = AF_INET;
  sin.sin_port = htons(9000);
  sin.sin_addr.s_addr = INADDR_ANY;
  bind(conn.con.fd, (struct sockaddr *)&sin, sizeof(sin));

  printf("tinyosc is now listening on port 9000.\n");
  printf("Press Ctrl+C to stop.\n");

  while (keepRunning) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(conn.con.fd, &readSet);
    struct timeval timeout = {1, 0};
    if (select(conn.con.fd + 1, &readSet, NULL, NULL, &timeout) > 0) {
      int len;
      conn.con.addr_len = sizeof(conn.con.addr);
      while ((len = (int)recvfrom(conn.con.fd, buffer, sizeof(buffer), 0,
                                  (struct sockaddr *)&conn.con.addr,
                                  &conn.con.addr_len)) > 0) {
        printf("RECEIVED [%s]\n", buffer);
        if (tosc_isBundle(buffer)) {
          tosc_bundle bundle;
          tosc_parseBundle(&bundle, buffer, len);
          uint64_t timetag = tosc_getTimetag(&bundle);
          printf("Timetag: %llu\n", (unsigned long long)timetag);
          tosc_message osc;
          while (tosc_getNextMessage(&bundle, &osc)) {
            dispatch_message(&osc, &conn);
          }
        } else {
          tosc_message osc;
          tosc_parseMessage(&osc, buffer, len);
          tosc_printOscBuffer(buffer, len);
          dispatch_message(&osc, &conn);
        }
      }
    }
  }

  close(conn.con.fd);
  return 0;
}
