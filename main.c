/**
 * \file main.c
 * Main program.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "main.h"
#include "message.h"
#include "smtp.h"
#include "local.h"


int verbose = 0;

FILE *log_fp = NULL;


int main (int argc, char **argv)
{
	int c;
	int ret;
	enum notify_flags notify = Notify_NOTSET;
	char *from = NULL;
	message_t *message;
	int local, remote;
	int parse_headers;
	
	/* Modes of operation. */
	enum {
		ENQUEUE,		/* delivery mode */
		NEWALIAS,		/* initialize alias database */
		MAILQ,			/* list mail queue */
		FLUSHQ			/* flush the mail queue */
	} mode;
	
	identities_init();

	/* Parse the rc file. */
	parse_rcfile();

	/* Set the default mode of operation. */
	if (strcmp(argv[0], "mailq") == 0) {
		mode = MAILQ;
	} else if (strcmp(argv[0], "newaliases") == 0) {
		mode = NEWALIAS;
	} else {
		mode = ENQUEUE;
	}

	while ((c = getopt (argc, argv,
						"A:B:b:C:cd:e:F:f:Gh:IiL:M:mN:nO:o:p:q:R:r:sTtV:vX:")) != EOF)
		switch (c)
		{
			case 'A':
				/* Use alternate sendmail/submit.cf */
				break;

			case 'B':
				/* Body type */
				break;

			case 'C':
				/* Select configuration file */
				rcfile = optarg;
				break;

			case 'F':
				/* Set full name */
				break;

			case 'G':
				/* Relay (gateway) submission */
				break;

			case 'I':
				/* Initialize alias database */
				mode = NEWALIAS;
				break;

			case 'L':
				/* Program label */
				break;

			case 'M':
				/* Define macro */
				break;

			case 'N':
				/* Delivery status notifications */
				if (strcmp (optarg, "never") == 0)
					notify = Notify_NEVER;
				else
				{
					if (strstr (optarg, "failure"))
						notify |= Notify_FAILURE;
					if (strstr (optarg, "delay"))
						notify |= Notify_DELAY;
					if (strstr (optarg, "success"))
						notify |= Notify_SUCCESS;
				}
				break;

			case 'R':
				/* What to return */
				break;

			case 'T':
				/* Set timeout interval */
				break;

			case 'X':
				/* Traffic log file */
				if (log_fp)
					fclose(log_fp);
				log_fp = fopen(optarg, "a");
				break;

			case 'V':
				/* Set original envelope id */
				break;

			case 'b':
				/* Operations mode */
				c = (optarg == NULL) ? ' ' : *optarg;
				switch (c)
				{
					case 'm':
						/* Deliver mail in the usual way */
						mode = ENQUEUE;
						break;

					case 'i':
						/* Initialize the alias database */
						mode = NEWALIAS;
						break;

					case 'p':
						/* Print a listing of the queue(s) */
						mode = MAILQ;
						break;
						
					case 'a':
						/* Go into ARPANET mode */
					case 'd':
						/* Run as a daemon */
					case 'D':
						/* Run as a daemon in foreground */
					case 'h':
						/* Print the persistent host status database */
					case 'H':
						/* Purge expired entries from the persistent host
						 * status database */
					case 'P':
						/* Print number of entries in the queue(s) */
					case 's':
						/* Use the SMTP protocol as described in RFC821
						 * on standard input and output */
					case 't':
						/* Run in address test mode */
					case 'v':
						/* Verify names only */
						fprintf (stderr, "Unsupported operation mode %c\n", c);
						exit (EX_USAGE);
						break;

					default:
						fprintf (stderr, "Invalid operation mode %c\n", c);
						exit (EX_USAGE);
						break;
				 }
				 break;

			case 'c':
				/* Connect to non-local mailers */
				break;

			case 'd':
				/* Debugging */
				break;

			case 'e':
				/* Error message disposition */
				break;

			case 'f':
				/* From address */
			case 'r':
				/* Obsolete -f flag */
				from = optarg;
				break;

			case 'h':
				/* Hop count */
				break;

			case 'i':
				/* Don't let dot stop me */
				break;

			case 'm':
				/* Send to me too */
				break;

			case 'n':
				/* don't alias */
				break;

			case 'o':
				/* Set option */
				break;

			case 'p':
				/* Set protocol */
				break;

			case 'q':
				/* Run queue files at intervals */
				mode = FLUSHQ;
				if (optarg[0] == '!')
				{
					/* Negate the meaning of pattern match */
					optarg++;
				}

				switch (optarg[0])
				{
					case 'G':
						/* Limit by queue group name */
						break;

					case 'I':
						/* Limit by ID */
						break;

					case 'R':
						/* Limit by recipient */
						break;

					case 'S':
						/* Limit by sender */
						break;

					case 'f':
						/* Foreground queue run */
						break;

					case 'p':
						/* Persistent queue */
						if (optarg[1] == '\0')
							break;
						++optarg;

					default:
						break;
				}
				break;

			case 's':
				/* Save From lines in headers */
				break;

			case 't':
				/* Read recipients from message */
				parse_headers = 1;
				break;

			case 'v':
				/* Verbose */
				verbose = 1;
				break;

			default:
				fprintf (stderr, "Invalid option %c\n", c);
				exit (EX_USAGE);
			}

	switch (mode)
	{
		case ENQUEUE:
			break;
		
		case MAILQ:
			printf ("Mail queue is empty\n");
		case NEWALIAS:
		case FLUSHQ:
			exit (0);
	}

	/* At least one more argument is needed. */
	if (optind > argc - 1 && !parse_headers)
	{
		fprintf(stderr, "Recipient names must be specified\n");
		exit (EX_USAGE);
	}

	if(!(message = message_new()))
		goto error;

	/** Parse the envelope headers */
	if(parse_headers)
	{
		if(!message_parse_headers(message))
		{
			fprintf(stderr, "Failed to parse headers\n");
			exit(EX_DATAERR);
		}
		
		if(!remote && !local)
		{
			fprintf(stderr, "No recipients found\n");
			exit(EX_DATAERR);
		}
	}

	/* Set the reverse path for the mail envelope.  */
	if(from && !message_set_reverse_path (message, from))
		goto error;

	/* Add remaining program arguments as message recipients. */
	while (optind < argc) {
		if(!message_add_recipient(message, argv[optind++]))
			goto error;
	}

	local = !list_empty(&message->local_recipients);
	remote = !list_empty(&message->remote_recipients);
	
	if(remote && !local)
		ret = smtp_send(message);
	else if(!remote && local)
	{
		if(!local_init(message))
			goto error;

		if(!local_flush(message))
			goto error;
			
		local_cleanup();

		ret = 0;
	}
	else
	{
		if(!local_init(message))
			goto error;

		ret = smtp_send(message);

		if(ferror(mda_fp))
			goto error;

		if(!message_eof(message) && !local_flush(message))
			goto error;
			
		local_cleanup();
	}
	
	message_free(message);

	return ret;

error:
	perror(NULL);
	exit(255);
}
