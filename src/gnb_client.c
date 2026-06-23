#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h> 
#include "gtpu.h"

#define NUM_CLIENT_THREADS 5 

typedef struct {
    int thread_id;
    unsigned int teid;
    struct sockaddr_in upf_addr;
} client_task_t;

void* send_heavy_traffic(void *arg) {
    client_task_t *info = (client_task_t*)arg;
    
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) {
        perror("[gNB Luồng ERROR] Không thể tạo Socket UDP");
        free(info);
        return NULL;
    }

    for (int i = 1; i <= 5; i++) {
        char user_payload[256];
        sprintf(user_payload, "[gNB Thread #%d] - Tháp gNB nã gói dữ liệu lần %d", info->thread_id, i);
        unsigned short data_len = strlen(user_payload);

        gtpu_header_t gtpu_vỏ;
        gtpu_vỏ.flags = 0x30;              
        gtpu_vỏ.msg_type = 255;            
        gtpu_vỏ.lenght = htons(data_len);  
        gtpu_vỏ.teid = htonl(info->teid);    

        char packet_buffer[2048];
        memcpy(packet_buffer, &gtpu_vỏ, sizeof(gtpu_header_t));
        memcpy(packet_buffer + sizeof(gtpu_header_t), user_payload, data_len);
        
        int total_packet_size = sizeof(gtpu_header_t) + data_len;

        sendto(udp_fd, packet_buffer, total_packet_size, 0,
                (struct sockaddr *)&info->upf_addr, sizeof(info->upf_addr));
                
        printf("[gNB Thread #%d] -> Đã bắn gói tin lần %d lên UPF\n", info->thread_id, i);
        
        usleep(5000); 
    }

    close(udp_fd);
    free(info); 
    return NULL;
}

int main()
{
    int tcp_fd;
    struct sockaddr_in smf_addr, upf_addr;
    unsigned int teid = 0;
    
    printf("=====================================================\n");
    printf("=== [gNB CLIENT] TRẠM PHÁT SÓNG 5G ĐANG KHỞI ĐỘNG ===\n");
    printf("=====================================================\n\n");
    
    printf("[Control-Plane] Bước 1: Mở kết nối TCP tới SMF (Port %d)...\n", SMF_PORT);
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) {
        perror("[gNB ERROR] Không thể tạo Socket TCP");
        exit(EXIT_FAILURE);
    }
    smf_addr.sin_family = AF_INET;
    smf_addr.sin_port = htons(SMF_PORT);
    smf_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    if (connect(tcp_fd, (struct sockaddr *)&smf_addr, sizeof(smf_addr)) < 0) {
        perror("[gNB ERROR] Kết nối tới SMF thất bại! Hãy chắc chắn đã bật ./smf_server trước");
        close(tcp_fd);
        exit(EXIT_FAILURE);
    }
    
    smf_header req_msg;
    req_msg.msg_type = 1;       
    req_msg.session_id = 5566;  
    req_msg.allocated_id = 0; 

    printf("[Control-Plane] Bước 2: Gửi yêu cầu cấp cấu hình cho Session ID: %u\n", req_msg.session_id);
    send(tcp_fd, &req_msg, sizeof(smf_header), 0);

    smf_header res_msg;
    int bytes_received = recv(tcp_fd, &res_msg, sizeof(smf_header), 0);
    if (bytes_received > 0 && res_msg.msg_type == 2) { 
        teid = res_msg.allocated_id; 
        printf("[Control-Plane] Bước 3: Đã nhận phản hồi từ SMF thành công!\n");
        printf(" -> Mã định danh tuyến hầm TEID được cấp là: %u\n", teid);
    } else {
        printf("[gNB ERROR] Nhận phản hồi từ SMF thất bại.\n");
        close(tcp_fd);
        exit(EXIT_FAILURE);
    }

    close(tcp_fd);
    printf("[Control-Plane] Hoàn thành Giai đoạn Điều khiển. Đã đóng kết nối TCP.\n\n");

    printf("[User-Plane] Chuẩn bị cấu hình bắn dữ liệu tải cao tới UPF (Port %d)...\n", GTPU_PORT);
    
    upf_addr.sin_family = AF_INET;
    upf_addr.sin_port = htons(GTPU_PORT);
    upf_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    pthread_t client_threads[NUM_CLIENT_THREADS];

    printf("[User-Plane] Kích hoạt đồng loạt %d Luồng Client để nã dữ liệu...\n", NUM_CLIENT_THREADS);
    for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
        client_task_t *data = malloc(sizeof(client_task_t));
        data->thread_id = i;
        data->teid = teid;
        data->upf_addr = upf_addr;

        pthread_create(&client_threads[i], NULL, send_heavy_traffic, (void*)data);
    }

    for (int i = 0; i < NUM_CLIENT_THREADS; i++) {
        pthread_join(client_threads[i], NULL);
    }

    printf("\n=== [gNB CLIENT] Hoàn tất toàn bộ chu trình oanh tạc đa luồng! ===\n");
    return 0;
}