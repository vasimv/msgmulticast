#ifndef __LIB_MSG_MULTI
#define __LIB_MSG_MULTI

typedef uint16_t status_type;
typedef uint16_t id_type;

struct msgrecord {
	id_type id;
	status_type status;
}

struct msgpacket {
	uint16_t nstatuses;
	struct msgrecord *statuses;
}

extern msgpacket *lastfull;
extern msgpacket *last;

// Find status in last/lastfull for specific id
status_type check_status(id_type id);

// Send status, 0 is error (last one will be resent periodically)
int send_status(id_type id, status_type status);

// Check UDP for incoming packets
void check_incoming();
#endif
