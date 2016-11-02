#ifndef __LIB_MSG_MULTI
#define __LIB_MSG_MULTI

#include <stdint.h>

typedef uint16_t status_type;
typedef uint16_t id_type;

struct msgrecord {
	id_type id;
	status_type status;
};

struct msgpacket {
	uint16_t nstatuses;
	struct msgrecord *statuses;
};

extern msgpacket *lastfull;
extern msgpacket *last;

// UDP port
#define STATE_UDP_PORT 10421

// Number of statuses to store in internal array
#define ALLSTATUS_SIZE 256

// Number of clients in cache
#define MSGMULTI_MAXCLIENTS 16

// Predefined statuses
#define STATE_UNDEFINED 0
#define STATE_EMERGENCYSTOP 1
#define STATE_SILENT 2
#define STATE_WALK 3
#define STATE_PHOTO 4
#define STATE_DEMO 5
#define STATE_PRESENTATION 128

#define MSGMULTI_MASTER 1
#define MSGMULTI_SLAVE 0

// How many times to resend last sent status
#define MSGMULTI_RESEND 200

// Delay (in ms) between sending all available statuses on master
#define MSGMULTI_RESEND_ALL 20000

// Find status in last/lastfull for specific id
status_type check_status(id_type id);

// Send status, 0 is error (last one will be resent periodically)
int send_status(id_type id, status_type status);

// Check UDP for incoming packets
void check_incoming();

// Init stuff if needed
void init_msgmulti(int node_type);
#endif
