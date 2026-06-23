#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include <arpa/inet.h>
#include <unistd.h>
#include "gtpu.h"
#include <asm-generic/socket.h>

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    unsigned int global_teid_counter =  8001;
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SMF_PORT);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("======đANG ĐỢI KẾT NỐI TẠI PORT %d======\n", SMF_PORT);
    while(1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        smf_header request;
        recv(new_socket, &request, sizeof(request), 0);
        if(request.msg_type == 0x01) {
            printf("Received Create Session Request\n");
            smf_header response;
            response.msg_type = 0x02; 
            response.session_id = request.session_id;
            response.allocated_id = global_teid_counter++;
            send(new_socket, &response, sizeof(response), 0);
            printf("Sent Create Session Response with allocated ID: %u\n", response.allocated_id);
        }
        close(new_socket);
    }
    close(server_fd);
    return 0;
}