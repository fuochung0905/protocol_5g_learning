#ifndef GTPU_H
#define GTPU_H

#define GTPU_PORT 2152
#define SMF_PORT 38412


// USER PLANE
typedef struct {
    unsigned char flags;
    unsigned char msg_type;
    unsigned short lenght;
    unsigned int teid;
} __attribute__((packed)) gtpu_header_t;


//CONTROL PLANE
typedef struct {
    unsigned char msg_type;
    unsigned int session_id;
    unsigned int allocated_id;
}__attribute__((packed)) smf_header;
#endif