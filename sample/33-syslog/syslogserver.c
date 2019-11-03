/*
 * syslogserver.c
 *
 * Based on example from the Linux getaddrinfo(3) man page, which is:
 *
 * Copyright (c) 2007, 2008 Michael Kerrisk <mtk.manpages@gmail.com>
 * and Copyright (c) 2006 Ulrich Drepper <drepper@redhat.com>
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE (1000+1)

int main (int argc, char *argv[])
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	ssize_t nread;
	char buf[BUF_SIZE];

	if (argc != 2)
	{
		fprintf (stderr, "Usage: %s port\n", argv[0]);

		return EXIT_FAILURE;
	}

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_INET;	/* Allow IPv4 only */
	hints.ai_socktype = SOCK_DGRAM;	/* Datagram socket */
	hints.ai_flags = AI_PASSIVE;	/* For wildcard IP address */
	hints.ai_protocol = 0;		/* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo (NULL, argv[1], &hints, &result);
	if (s != 0)
	{
		fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));

		return EXIT_FAILURE;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		{
			continue;
		}

		if (bind (sfd, rp->ai_addr, rp->ai_addrlen) == 0)
		{
			break;		/* Success */
		}

		close (sfd);
	}

	if (rp == NULL)			/* No address succeeded */
	{
		fprintf (stderr, "Could not bind to port: %s\n", argv[1]);

		return EXIT_FAILURE;
	}

	freeaddrinfo (result);		/* No longer needed */

	fprintf (stderr, "Press ^C to terminate\n");

	for (;;)
	{
		peer_addr_len = sizeof (struct sockaddr_storage);
		nread = recvfrom (sfd, buf, BUF_SIZE-1, 0,
				  (struct sockaddr *) &peer_addr, &peer_addr_len);
		if (nread == -1)
		{
			continue;	/* Ignore failed request */
		}

		buf[nread] = '\0';

		puts (buf);
	}
}
