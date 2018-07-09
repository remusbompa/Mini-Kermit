/*Bompa Remus 325CB*/
#ifndef LIB
#define LIB

typedef struct
{
  int len;
  char payload[1400];
} msg;

typedef struct
{
  char maxl;
  char time;
  char npad;
  char padc;
  char eol;
  char qctl;
  char qbin;
  char chkt;
  char rept;
  char capa;
  char r;
} __attribute__ ((packed, aligned (1))) data_s;


void init (char *remote, int remote_port);
void set_local_port (int port);
void set_remote (char *ip, int port);
int send_message (const msg * m);
int recv_message (msg * r);
msg *receive_message_timeout (int timeout);	//timeout in milliseconds
unsigned short crc16_ccitt (const void *buf, int len);

msg *receive_message_timeout (int timeout);	//timeout in milliseconds
unsigned short crc16_ccitt (const void *buf, int len);

void
afi_msg (msg * r)
{
  if (r->payload[3] == 'N')
    {
      unsigned char data_len = r->payload[1] - 5;
      printf
	("[./ksender] Primire NAK cu sumele: %hu %hu si cu continutul:\n",
	 crc16_ccitt (r->payload, (int) data_len + 4),
	 *((unsigned short *) (r->payload + 4 + data_len)));
    }
  else if (r->payload[3] == 'Y')
    {
      unsigned char data_len = r->payload[1] - 5;
      printf
	("[./ksender] Primire ACK cu sumele: %hu %hu si cu continutul:\n",
	 crc16_ccitt (r->payload, (int) data_len + 4),
	 *((unsigned short *) (r->payload + 4 + data_len)));
    }
  else if (r->payload[3] == 'E')	//urmatorul pachet primit e ACK
    printf ("[./ksender] Primire eroare cu continutul:\n");
  else
    {
      unsigned char data_len = r->payload[1] - 5;
      printf
	("[./kreceiver] Primire pachet %c cu sumele: %hu %hu si cu continutul:\n ",
	 r->payload[3], crc16_ccitt (r->payload, (int) data_len + 4),
	 *((unsigned short *) (r->payload + 4 + data_len)));
    }
  unsigned char data_len = r->payload[1] - 5;
  if (data_len == sizeof (data_s))
    printf ("%d %d %d %c\t%d %d %d %d %d %d %d %d %d %d %d\t%hu %d\n",
	    r->payload[0], r->payload[1], r->payload[2], r->payload[3],
	    r->payload[4], r->payload[5], r->payload[6], r->payload[7],
	    r->payload[8], r->payload[9], r->payload[10], r->payload[11],
	    r->payload[12], r->payload[13], r->payload[14],
	    *((unsigned short *) (r->payload + 15)), r->payload[17]);
  if (r->payload[3] == 'F' || r->payload[3] == 'D')
    {
      char data[data_len + 1];
      memcpy (data, r->payload + 4, data_len);
      data[data_len] = 0;
      printf ("%d %d %d %c\t%s\t%hu %d\n",
	      r->payload[0], r->payload[1], r->payload[2], r->payload[3],
	      data, *((unsigned short *) (r->payload + 4 + data_len)),
	      r->payload[data_len + 6]);
    }
  if (data_len == 0)
    printf ("%d %d %d %c\t%hu %d\n",
	    r->payload[0], r->payload[1], r->payload[2], r->payload[3],
	    *((unsigned short *) (r->payload + 4)), r->payload[6]);
}

#endif
