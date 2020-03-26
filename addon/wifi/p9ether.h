#ifndef _p9ether_h
#define _p9ether_h

#include "p9util.h"
#include "p9proc.h"
#include "etherevent.h"

#ifdef __cplusplus
extern "C" {
#endif

#define Eaddrlen	6
#define ETHERHDRSIZE	(Eaddrlen+Eaddrlen+2)

typedef struct Block
{
	struct Block *next;
	uchar *buf;
	uchar *lim;
	uchar *wp;
	uchar *rp;
	uchar data[0];
#define BLEN(b)		((b)->wp - (b)->rp)
}
Block;

Block *allocb (size_t size);
void freeb (Block *b);
Block *padblock (Block *b, int size);

typedef struct
{
	Block *first;
	Block *last;
	unsigned nelem;
}
Queue;

unsigned qlen (Queue *q);
Block *qget (Queue *q);
void qpass (Queue *q, Block *b);

typedef struct Ether
{
	void *ctlr;
	void *arg;
	uchar ea[Eaddrlen];
	uchar addr[Eaddrlen];
	Queue *oq;

	void (*attach) (struct Ether *edev);
	void (*transmit) (struct Ether *edev);
	long (*ifstat) (struct Ether *edev, void *buf, long size, ulong offset);
	long (*ctl) (struct Ether *edev, const void *buf, long n);
	void (*getbssid) (struct Ether *edev, void *bssid);
	void (*scanbs) (void *arg, uint secs);
	void (*setevhndlr) (struct Ether *edev, ether_event_handler_t *hndlr, void *context);
	void (*shutdown) (struct Ether *edev);
}
Ether;

#define nhgets	__p9nhgets
#define nhgetl	__p9nhgetl
ushort nhgets (const void *p);
uint nhgetl (const void *p);
int parseether (uchar *addr, const char *str);

void etheriq (Ether *ether, Block *block, unsigned flag);
void etherscanresult (Ether *ether, const void *buf, long len);

typedef int ether_pnp_t (Ether *edev);
void addethercard (const char *name, ether_pnp_t *ether_pnp);

#ifdef __cplusplus
}
#endif

#endif
