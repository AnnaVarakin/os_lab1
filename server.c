#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 256

static volatile sig_atomic_t running = 1;
static volatile sig_atomic_t sighup_received = 0;

void signal_handler(int sig) {
    if (sig == SIGHUP) {
        sighup_received = 1;
    }
    else if (sig == SIGINT) {
        running = 0;
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server started on port %d\n", PORT);
    printf("PID: %d\n", getpid());
    printf("Ctrl+C to stop the server\n");

    struct sigaction sa_hup = { 0 };
    struct sigaction sa_int = { 0 };

    sa_hup.sa_handler = signal_handler;
    sa_hup.sa_flags = 0;  
    sigemptyset(&sa_hup.sa_mask);
    sigaddset(&sa_hup.sa_mask, SIGINT);
    if (sigaction(SIGHUP, &sa_hup, NULL) < 0) {
        perror("sigaction SIGHUP failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    sa_int.sa_handler = signal_handler;
    sa_int.sa_flags = 0; 
    sigemptyset(&sa_int.sa_mask);

    sigaddset(&sa_int.sa_mask, SIGHUP);
    if (sigaction(SIGINT, &sa_int, NULL) < 0) {
        perror("sigaction SIGINT failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    sigset_t block_mask, original_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGHUP);
    sigaddset(&block_mask, SIGINT);

    if (sigprocmask(SIG_BLOCK, &block_mask, &original_mask) < 0) {
        perror("sigprocmask failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    int client_socket = -1;
    int max_fd;
    fd_set read_fds;

    while (running) {
        if (sighup_received) {
            printf("SIGHUP signal received\n");
            sighup_received = 0;
        }

        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;

        if (client_socket != -1) {
            FD_SET(client_socket, &read_fds);
            if (client_socket > max_fd) {
                max_fd = client_socket;
            }
        }

        int ready = pselect(max_fd + 1, &read_fds, NULL, NULL, NULL, &original_mask);

        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            else {
                perror("pselect error");
                break;
            }
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int new_client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (new_client < 0) {
                perror("accept error");
                continue;
            }

            if (client_socket == -1) {
                client_socket = new_client;
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                printf("New client connected from %s:%d\n",
                    client_ip,
                    ntohs(client_addr.sin_port));
            }
            else {
                close(new_client);
            }
        }

        if (client_socket != -1 && FD_ISSET(client_socket, &read_fds)) {
            char buffer[BUFFER_SIZE];
            ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                printf("Received %zd bytes from client\n", bytes_received);
            }
            else if (bytes_received == 0) {
                printf("Client disconnected\n");
                close(client_socket);
                client_socket = -1;
            }
            else {
                perror("recv error");
                close(client_socket);
                client_socket = -1;
            }
        }
    }

    if (client_socket != -1) {
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        printf("Client connection closed\n");
    }

    shutdown(server_fd, SHUT_RDWR);
    close(server_fd);
    printf("Server stopped\n");

    return 0;
}