#ifndef _p9ether_h
#define _p9ether_h

#include "p9util.h"
#include "p9proc.h"

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
#define BLEN(b)		((b)->wp - (b)->rp)
}
Block;

Block *allocb (size_t size);
void freeb (Block *b);
Block *copyblock (Block *b, size_t size);
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
void qwrite (Queue *q, unsigned, unsigned);

typedef struct
{
	Queue *in;
	int inuse;
	ushort type;
	int scan;
}
Netfile;

typedef struct Ether
{
	void *ctlr;
	void *arg;
	uchar ea[Eaddrlen];
	uchar addr[Eaddrlen];
	Queue *oq;
	int scan;

	unsigned nfile;
#define Ntypes		10
	Netfile *f[Ntypes];

	void (*attach) (struct Ether *edev);
	void (*transmit) (struct Ether *edev);
	long (*ifstat) (struct Ether *edev, void *buf, long size, ulong offset);
	long (*ctl) (struct Ether *edev, const void *buf, long n);
	void (*scanbs) (void *arg, uint secs);
	void (*shutdown) (struct Ether *edev);
}
Ether;

ushort nhgets (const void *p);
uint nhgetl (const void *p);
int parseether (uchar *addr, const char *str);

void etheriq (Ether *ether, Block *block, unsigned flag);

typedef int ether_pnp_t (Ether *edev);
void addethercard (const char *name, ether_pnp_t *ether_pnp);

#ifdef __cplusplus
}
#endif

#endif
