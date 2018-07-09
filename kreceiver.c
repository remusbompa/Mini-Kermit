/*Bompa Remus 325CB*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

#define MAXL 250
#define TIME 5
#define MARK 0x0d

	void
	construire_confirmare_S (msg * t, char seq, void *data)
	{
	  t->payload[0] = 0x01;		//soh
	  t->payload[1] = 2 + sizeof (data_s) + 3;	//len
	  t->payload[2] = seq;		//seq
	  t->payload[3] = 'Y';		//type
	  memcpy (t->payload + 4, data, sizeof (data_s));	//data
	  unsigned short crc = crc16_ccitt (t->payload, 4 + sizeof (data_s));
	  memcpy (t->payload + 4 + sizeof (data_s), &crc, 2);	//check
	  t->payload[4 + sizeof (data_s) + 2] = MARK;
	  t->len = t->payload[1] + 2;

	}

	void
	construire_msg (msg * t, char seq, char tip)
	{
	  t->payload[0] = 0x01;		//soh
	  t->payload[1] = 2 + 3;	//len
	  t->payload[2] = seq;		//seq
	  t->payload[3] = tip;		//type
	  unsigned short crc = crc16_ccitt (t->payload, 4);
	  memcpy (t->payload + 4, &crc, 2);	//check
	  t->payload[6] = MARK;
	  t->len = t->payload[1] + 2;
	}


	int
	primire_pachet (msg * t, char *seq, msg ** r, int asteaptaS)
	{
	  int i = 0;
	  while (i < 3)
	    {
	      *r = receive_message_timeout (TIME * 1000);
	      if (*r == NULL || (*r)->payload[2] != ((*seq) + 1) % 64)
		{
		  if(!asteaptaS) send_message(t);               
		  i++;
		  continue;
		}
	     // afi_msg(*r);
	      unsigned char data_len = (*r)->payload[1] - 5;	//lungimea campului data al mesajului primit
	      *seq = (*seq + 1) % 64;
	      if (crc16_ccitt ((*r)->payload, (int) data_len + 4) !=
		  *((unsigned short *) ((*r)->payload + 4 + data_len)))
		{
		  //se emite NAK  
		  i = 0;
		  construire_msg (t, *seq, 'N');
		  send_message (t);
		  continue;
		}
	      break;
	    }
	  //se verifica daca ultimul ACK/NAK a fost retrimis de trei ori si nu s-a primit un nou mesaj
	  if (*r == NULL)
	    {
	      printf
		("[./kreceiver] S-a asteptat de trei ori si nu s-a primit un nou mesaj\n");
	      construire_msg (t, *seq, 'E');
	      send_message (t);
	      return 1;
	    }
	  if ((*r)->payload[3] == 'E')
	    {
	      printf ("[./kreceiver] A fost primit pachet E\n");
	      return 1;
	    }
	  if (asteaptaS)
	    return 0;			//se iese pentru a se trimite un ACK special
	  construire_msg (t, *seq, 'Y');
	  send_message (t);
	  return 0;
	}



	int
	main (int argc, char **argv)
	{
	  msg *r, t;
	  init (HOST, PORT);
	  char seq = -1;
	  //primire pachet S
	  if (primire_pachet (&t, &seq, &r, 1))
	    return 1;
	  //salvare informatii despre emitor
	  data_s info_emitor;
	  memcpy (&info_emitor, r->payload + 4, sizeof (data_s));
	  //trimitere ACK pentru S
	  data_s info_receptor = { MAXL, TIME, 0, 0, MARK, 0, 0, 0, 0, 0, 0 };
	  construire_confirmare_S (&t, seq, &info_receptor);
	  send_message (&t);
	  //scriere in fisiere
	  while (1)
	    {
	      //primire pachet F
	      if (primire_pachet (&t, &seq, &r, 0))
		return 1;
	      if (r->payload[3] == 'B')
		break;
	      //salvare informatii despre pachetul F
	      unsigned char name_len = r->payload[1] - 5;
	      char nume[5 + name_len + 1];
	      sprintf (nume, "recv_");
	      memcpy (nume + 5, r->payload + 4, name_len);
	      nume[5 + name_len] = 0;
	      int file_out = open (nume, O_WRONLY | O_CREAT, 0644);
	      if (file_out < 0)
		{
		  printf ("[%s] Eroare creare fisier %s\n", argv[0], nume);
		  construire_msg (&t, seq, 'E');
		  send_message (&t);
		  return 2;
		}
	      //primire pachete D de la un fisier
	      while (1)
		{
		  if (primire_pachet (&t, &seq, &r, 0))
		    return 1;
		  if (r->payload[3] == 'Z')
		    {
		      close (file_out);
		      break;
		    }
		  int nr =
		    write (file_out, r->payload + 4,
			   ((unsigned char) (r->payload[1] - 5)));
		  if (nr < 0)
		    {
		      printf ("[%s] Eroare scriere in fisier\n", argv[0]);
		      construire_msg (&t, seq, 'E');
		      send_message (&t);
		      return 2;
		    }
		}
	    }
  return 0;
}
