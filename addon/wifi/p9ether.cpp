#include "p9ether.h"
#include <circle/util.h>
#include <assert.h>

Block *allocb (size_t size)
{
	static const size_t maxhdrsize = 64;

	size += sizeof (Block) + maxhdrsize;

	Block *b = (Block *) new uchar[size];
	assert (b != 0);

	b->buf = b->data;

	b->next = 0;
	b->lim = b->buf + size;
	b->wp = b->buf + maxhdrsize;
	b->rp = b->wp;

	return b;
}

void freeb (Block *b)
{
	uchar *p = (uchar *) b;
	delete [] p;
}

Block *padblock (Block *b, int size)
{
	assert (size > 0);

	assert (b != 0);
	assert (b->rp - b->buf >= size);
	b->rp -= size;

	return b;
}

unsigned qlen (Queue *q)
{
	return q->nelem;
}

Block *qget (Queue *q)
{
	Block *b = 0;

	assert (q != 0);
	if (q->first != 0)
	{
		b = q->first;

		q->first = b->next;
		if (q->first == 0)
		{
			assert (q->last == b);
			q->last = 0;
		}

		assert (q->nelem > 0);
		q->nelem--;
	}

	return b;
}

void qpass (Queue *q, Block *b)
{
	assert (b != 0);
	b->next = 0;

	assert (q != 0);
	if (q->first == 0)
	{
		q->first = b;
	}
	else
	{
		assert (q->last != 0);
		assert (q->last->next == 0);
		q->last->next = b;
	}

	q->last = b;

	q->nelem++;
}

ushort nhgets (const void *p)
{
	return be2le16 (*(ushort *) p);
}

uint nhgetl (const void *p)
{
	return be2le32 (*(uint *) p);
}

int parseether (uchar *addr, const char *str)
{
	for (unsigned i = 1; i <= 6; i++)
	{
		char buf[3], *p = buf;

		if (*str == '\0')
		{
			return -1;
		}
		*p++ = *str++;

		if (*str == '\0')
		{
			return -1;
		}
		*p++ = *str++;

		if (i < 6 && *str == ':')
		{
			str++;
		}
		*p = '\0';

		char *end = 0;
		*addr++ = (uchar) strtoul (buf, &end, 16);
		if (end != 0 && *end != '\0')
		{
			return -1;
		}
	}

	return *str == '\0' ? 0 : -1;
}
