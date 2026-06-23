#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "gtpu.h"
#include "threadPool.h"

typedef struct {
   char rowData[1024];
   int byteRecived;
   struct sockaddr_in client_info;
} package_data;

void process_gpu_package(void *arg)
{
    package_data *task = (package_data*)arg;
    if (task->byteRecived >= (int)sizeof(gtpu_header_t)) {
        gtpu_header_t *header = (gtpu_header_t *)task->rowData;
        
        unsigned short data_len = ntohs(header->lenght);
        unsigned int dynamic_teid = ntohl(header->teid);
        char *user_data = task->rowData + sizeof(gtpu_header_t);

        printf("[UPF CORE] Nhận gói từ gNB %s:%d | TEID: %u | Nội dung: %.*s\n", 
               inet_ntoa(task->client_info.sin_addr), ntohs(task->client_info.sin_port),
               dynamic_teid, data_len, user_data);
    }
    
    free(task);
}

int main()
{
    int sockfd;
    struct sockaddr_in server_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("[UPF] Lỗi mở socket UDP");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(GTPU_PORT);

    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[UPF] Lỗi bind cổng");
        exit(EXIT_FAILURE);
    }
    thread_pool *pool = thread_pool_create(4, 50);

    printf("=== [UPF SERVER] Sẵn sàng đợi gói dữ liệu tại port %d ===\n\n", GTPU_PORT);

    while (1) {
        package_data *new_packet = malloc(sizeof(package_data));
        socklen_t addr_len = sizeof(new_packet->client_info);
        
        new_packet->byteRecived = recvfrom(sockfd, new_packet->rowData, sizeof(new_packet->rowData), 0, 
                                              (struct sockaddr *)&new_packet->client_info, &addr_len);
        
        if (new_packet->byteRecived > 0) {
            if (thread_pool_add(pool, process_gpu_package, (void*)new_packet) != 0) {
                printf("[UPF WARN] ThreadPool quá tải! Đã hủy gói tin.\n");
                free(new_packet);
            }
        } else {
            free(new_packet);
        }
    }

    thread_pool_destroy(pool);
    close(sockfd);
    return 0;
}
