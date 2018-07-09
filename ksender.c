/*Bompa Remus 325CB*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

#define MAXL 250
#define TIME 5
#define MARK 0x0d

	void
	construire_msg (msg * t, void *data, unsigned char data_len, char seq,
			char tip)
	{
	  t->payload[0] = 0x01;		//soh
	  t->payload[1] = 2 + data_len + 3;	//len
	  t->payload[2] = seq;		//seq
	  t->payload[3] = tip;		//type
	  memcpy (t->payload + 4, data, data_len);	//data
	  unsigned short crc = crc16_ccitt (t->payload, (int) data_len + 4);
	  memcpy (t->payload + 4 + data_len, &crc, 2);	//check
	  t->payload[4 + data_len + 2] = MARK;
	  t->len = t->payload[1] + 2;
	}

	int
	trimitere_msg (msg * t, char *seq, msg ** r)
	{
	  int i = 0;
	  int ok = 0;
	  while (i < 3)
	    {
	      //trimitere pachet 
	      if (ok)
		{
		  t->payload[2] = *seq;	//actualizare numar secventa
		  unsigned char data_len = t->payload[1] - 5;
		  unsigned short crc = crc16_ccitt (t->payload, (int) data_len + 4);
		  memcpy (t->payload + 4 + data_len, &crc, 2);	//actualizare check
		}
	      send_message (t);
	      //primire ACK/NAK
	      *r = receive_message_timeout (TIME * 1000);
	      if (*r == NULL || (*r)->payload[2] != *seq)
		{
		  i++;
		  ok = 0;
		  continue;
		}			//daca nu se primeste nimic in TIME secunde
	      (*seq) = ((*seq) + 1) % 64;
	      //afi_msg(*r);
	      if ((*r)->payload[3] == 'N')
		{			//urmatorul pachet primit e NAK
		  ok = 1;
		  i = 0;
		  continue;
		}
	      if ((*r)->payload[3] == 'Y')
		{			//urmatorul pachet primit e ACK
		  return 0;
		}
	      if ((*r)->payload[3] == 'E')
		{			//urmatorul pachet primit e eroare
		  return 1;
		}
	    }

	  printf
	    ("[./ksender Mesajul a fost retrimis de trei ori si nu s-a primit ACK\n");
	  return 1;
	}

	int
	main (int argc, char **argv)
	{
	  msg t, *r;
	  init (HOST, PORT);
	  char seq = 0;
	  //construire pachet S--------------------------
	  data_s info_emitor = { MAXL, TIME, 0, 0, MARK, 0, 0, 0, 0, 0, 0 };
	  construire_msg (&t, &info_emitor, sizeof (data_s), seq, 'S');
	  //trimitere pachet S
	  if (trimitere_msg (&t, &seq, &r))
	    return 1;
	  //salvare informatii despre receptor
	  data_s info_receptor;
	  memcpy (&info_receptor, r->payload + 4, sizeof (data_s));
	  //trimitere fisiere
	  for (int i = 1; i <= argc - 1; i++)
	    {
	      //construire pachet F--------------------------
	      construire_msg (&t, argv[i], strlen (argv[i]), seq, 'F');
	      //trimitere pachet F
	      if (trimitere_msg (&t, &seq, &r))
		return 1;

	      //construire pachet D--------------------------
	      int file_in = open (argv[i], O_RDONLY);
	      if (file_in < 0)
		{
		  printf ("[%s] Eroare deschidere fisier %s\n", argv[0], argv[i]);
		  construire_msg (&t, NULL, 0, seq, 'E');
		  if (trimitere_msg (&t, &seq, &r))
		    return 2;
		  return 2;
		}
	      char data[MAXL];
	      while (1)
		{
		  int nr = read (file_in, data, MAXL);
		  if (nr < 0)
		    {
		      printf ("[%s] Eroare citire din fisier\n", argv[0]);
		      construire_msg (&t, NULL, 0, seq, 'E');
		      if (trimitere_msg (&t, &seq, &r))
			return 2;
		      return 2;
		    }
		  printf ("[%s] Au fost cititi %d bytes din fisier\n", argv[0], nr);
		  construire_msg (&t, data, nr, seq, 'D');
		  if (trimitere_msg (&t, &seq, &r))
		    return 1;
		  if (nr < MAXL)
		    break;
		}
	      //construire pachet Z---------------------------
	      construire_msg (&t, NULL, 0, seq, 'Z');
	      close (file_in);
	      if (trimitere_msg (&t, &seq, &r))
		return 1;
	    }
	  //constructie pachet B-------------------------
	  construire_msg (&t, NULL, 0, seq, 'B');
	  if (trimitere_msg (&t, &seq, &r))
	    return 1;
	  return 0;
	}
