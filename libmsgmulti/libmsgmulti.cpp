// Library to send status'es through UDP broadcast
#include <libmsgmulti.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// Print debug messages to Serial
// #define DEBUG

//typedef uint16_t status_type;
// typedef uint16_t id_type;

// struct msgrecord {
//	id_type id;
//	status_type status;
//}

//struct msgpacket {
//	uint16_t nstatuses;
//	struct msgrecord statuses[];
//}

int numstatuses = 0;
struct msgrecord allstatuses[ALLSTATUS_SIZE];
struct {
  IPAddress client;
  int16_t expire;
} clients[MSGMULTI_MAXCLIENTS];

int num_sent = 0;
struct {
  id_type id;
  uint8_t expire;
} sent_statuses[MSGMULTI_MAXCLIENTS];

msgpacket *lastfull;
msgpacket *last;

int my_node_type;

WiFiUDP udp;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];

IPAddress addr_broadcast, addr_local;

// Find status position in statuses array, return -1 if not found
int find_status(id_type id) {
  int i;

  for (i = 0; i < numstatuses; i++)
    if (allstatuses[i].id == id)
      return i;

  return -1;
} // int find_status(id_type id)

// Find status in last/lastfull for specific id
status_type check_status(id_type id) {
  status_type temp;
  int i;

  // We use default "master" status if not find specific one
  temp = allstatuses[0].status;
  if ((i = find_status(id)) >= 0)
    temp = allstatuses[i].status;
  return temp;
} // status_type check_status(id_type id) 

// Set all statuses to specific status
void set_all_statuses(status_type status) {
  int i;

  for (i = 0; i < numstatuses; i++)
    allstatuses[i].status = status;
} // void set_all_statuses(status status_type status)

// Send status packet
int send_packet(char *packet, int size, IPAddress exclude) {
  int i;

  if (my_node_type == MSGMULTI_MASTER) {
    for (i = 0; i < MSGMULTI_MAXCLIENTS; i++) {
      if ((clients[i].expire > 0) && (clients[i].client != exclude)) {
	#ifdef DEBUG
	Serial.print("Sending packet to ");
	Serial.println(clients[i].client);
	#endif
        clients[i].expire--;
        udp.beginPacket(clients[i].client, STATE_UDP_PORT);
        udp.write(packet, size);
        return udp.endPacket();
      }
    }
  } else {
    udp.beginPacket(WiFi.gatewayIP(), STATE_UDP_PORT);
    udp.write(packet, size);
    return udp.endPacket();
  }
} // void send_packet(IPaddress dest, char *packet, int size)

// Send status (slave->master, master->slaves), 0 is error (last one will be resent periodically)
int send_status(id_type id, status_type status) {
  struct {
    uint16_t nstatuses;
    struct msgrecord record;
  } sendPacket;
  int empty, ind, i;

  sendPacket.nstatuses = 1;
  sendPacket.record.id = id;
  sendPacket.record.status = status;
  // Update allstatuses table
  ind = find_status(id);
  if (ind < 0) {
    ind = numstatuses;
    numstatuses++;
    allstatuses[ind].id = id;
  }
  allstatuses[ind].status = status;

  // Update sent_statuses table
  ind = -1;
  empty = -1;
  for (i = 0; i < num_sent; i++) {
    if (sent_statuses[i].expire > 0) {
      ind = i;
      break;
    } else
      empty = i;
  }
  if (ind < 0) {
    if (empty < 0) {
      empty = num_sent;
      num_sent++;
    }  
    ind = empty;
  }
  sent_statuses[ind].id = id;
  sent_statuses[ind].expire = MSGMULTI_RESEND; 
  if (id == 1) {
    set_all_statuses(status);
    sent_statuses[ind].expire = 5;
  }

  #ifdef DEBUG
  Serial.print("Sending status, id: ");
  Serial.print(id);
  Serial.print(", status: ");
  Serial.println(status);
  #endif
  return send_packet((char *) &sendPacket, sizeof(sendPacket), addr_local);
} // int send_status(id_type id, status_type status)

unsigned long timer_resend = 0;
unsigned long timer_resend_all = 0;

// Periodically resend statuses
void resend_status() {
  int i, ind;
  char *p;
  uint16_t n;
  struct msgrecord tmp;

  #ifdef DEBUG
  Serial.print("resend_status, timer_resend: ");
  Serial.print(timer_resend);
  Serial.print(", timer_resend_all: ");
  Serial.print(timer_resend_all);
  Serial.print(", millis(): ");
  Serial.println(millis());
  #endif

  // Check if we have to resend (every 200 milliseconds)
  if ((millis() - timer_resend) > 200) {
    timer_resend = millis();

    // Construct packet from sent_statuses array
    n = 0;
    p = packetBuffer + sizeof(uint16_t);
    for (i = 0; i < num_sent; i++) {
      if (sent_statuses[i].expire > 0) {
        sent_statuses[i].expire--;
        tmp.id = sent_statuses[i].id;
        ind = find_status(tmp.id);
        if (ind >= 0) {
          tmp.status = allstatuses[ind].status;
          memcpy(p, (char *) &tmp, sizeof(struct msgrecord));
          p += sizeof(struct msgrecord);
          n++;
        }
      }
    }
    if (n > 0) {
      memcpy(packetBuffer, &n, sizeof(uint16_t));
      send_packet(packetBuffer, p - packetBuffer, addr_local);
    }
  }

  // Check if we are master and have to resend all available statuses
  if ((my_node_type == MSGMULTI_MASTER) && ((millis() - timer_resend_all) > MSGMULTI_RESEND_ALL)) {
    timer_resend_all = millis();
    n = 0;
    p = packetBuffer + sizeof(uint16_t);
    for (i = 0; i < numstatuses; i++) {
      // Check if buffer is enough for next one
      if ((p - packetBuffer) > (sizeof(packetBuffer) - sizeof(struct msgrecord)))
        break;
      memcpy(p, &allstatuses[i], sizeof(struct msgrecord));
      p += sizeof(struct msgrecord);
      n++;
    }
    #ifdef DEBUG
    Serial.print("n: ");
    Serial.print(n);
    Serial.print(", numstatuses: ");
    Serial.println(numstatuses);
    #endif
    if (n > 0) {
      memcpy(packetBuffer, &n, sizeof(uint16_t));
      send_packet(packetBuffer, p - packetBuffer, addr_local);
    }
  }
} // void resend_status()

// Process incoming status record (update status table and remove from sent_statuses)
void receive_status(struct msgrecord *in) {
  int i;

  #ifdef DEBUG
  Serial.println("Status:");
  Serial.print(in->id);
  Serial.print(" ");
  Serial.println(in->status);
  #endif
  i = find_status(in->id);
  if (i < 0) {
    if (numstatuses >= ALLSTATUS_SIZE)
      numstatuses = ALLSTATUS_SIZE - 1;
    i = numstatuses;
    allstatuses[i].id = in->id;
    numstatuses++;
  }
  allstatuses[i].status = in->status;

  // Update sent_statuses table so we won't send obsolete status
  for (i = 0; i < num_sent; i++) {
    if (sent_statuses[i].expire > 0) {
      if (sent_statuses[i].id == in->id) {
        sent_statuses[i].expire = 0;
        break;
      }
    }
  }
  if (in->id == 1)
    set_all_statuses(in->status);
} // void receive_status(struct msgrecord in)

// Check UDP for incoming packets
void check_incoming() {
  int packetSize;
  int i;
  int empty = 0;
  int found = -1;

  packetSize = udp.parsePacket();
  if (packetSize) {
    IPAddress remote = udp.remoteIP();
    // read the packet into packetBufffer
    udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    #ifdef DEBUG
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    for (i = 0; i < 4; i++) {
      Serial.print(remote[i], DEC);
      if (i < 3) {
        Serial.print(".");
      }
    }
    Serial.print(", port ");
    Serial.println(udp.remotePort());
    Serial.print("Dump: ");
    for (i = 0; i < packetSize / sizeof(uint16_t); i++) {
      Serial.print(*((uint16_t *) (packetBuffer + i * sizeof(uint16_t))));
      Serial.print(" ");
    }
    Serial.println();
    #endif

    // Master sends all packets to its clients and updates cache
    if (my_node_type == MSGMULTI_MASTER) {
      found = -1;
      empty = 0;
      send_packet(packetBuffer, packetSize, remote);
      for (i = 0; i < MSGMULTI_MAXCLIENTS; i++) {
        if (clients[i].client == remote) {
          found = i;
          break;
        }
        if (clients[i].expire <= 0)
          empty = i;
      }
      if (found >= 0)
        clients[found].expire = 1024;
      else {
        clients[empty].client = remote;
        clients[empty].expire = 1024;
      }
    }

    // Process incoming statuses
    for (i = 0; i < *((uint16_t *) packetBuffer); i++) {
      receive_status((struct msgrecord *) (packetBuffer + i * sizeof(struct msgrecord) + sizeof(uint16_t)));
    }

    // Repeat incoming check to process all packets in queue
    check_incoming();
  }

  // Check if we have to repeat last sent statuses
  resend_status();
} // void check_incoming()

// Init stuff if needed
void init_msgmulti(int node_type) {
  int i;

  // Check if have to clear our array
  if (!numstatuses) {
    for (i = 1; i < ALLSTATUS_SIZE; i++) {
      allstatuses[i].id = -1;
      allstatuses[i].status = STATE_UNDEFINED;
    }
    allstatuses[0].id = 0;
    allstatuses[0].status = STATE_EMERGENCYSTOP;
    numstatuses = 1;
  }
  udp.begin(STATE_UDP_PORT);
  addr_local = WiFi.localIP();
  addr_broadcast = ~WiFi.subnetMask() | WiFi.gatewayIP();
  my_node_type = node_type;
  if (node_type == MSGMULTI_MASTER) {
    for (i = 0; i < MSGMULTI_MAXCLIENTS; i++)
      clients[i].expire = -1;
  }
  for (i = 0; i < MSGMULTI_MAXCLIENTS; i++)
    sent_statuses[i].expire = 0;
} // void init_msgmulti()
