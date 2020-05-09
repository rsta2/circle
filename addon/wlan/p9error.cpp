#include "p9error.h"
#include "p9proc.h"
#include "p9util.h"
#include <assert.h>

void error (const char *str)
{
	print ("%s\n", str);

	up->errstr = str;

	error_stack_t *s = get_error_stack ();
	assert (s != 0);

	assert (s->stackptr < ERROR_STACK_SIZE);
	s->stackptr++;
	longjmp (s->stack[s->stackptr-1], 1);
}

jmp_buf *pusherror (void)
{
	error_stack_t *s = get_error_stack ();
	assert (s != 0);

	return &s->stack[--s->stackptr];
}

void nexterror (void)
{
	error_stack_t *s = get_error_stack ();
	assert (s != 0);

	assert (s->stackptr < ERROR_STACK_SIZE);
	s->stackptr++;
	longjmp (s->stack[s->stackptr-1], 1);
}

void poperror (void)
{
	error_stack_t *s = get_error_stack ();
	assert (s != 0);

	assert (s->stackptr < ERROR_STACK_SIZE);
	s->stackptr++;
}

void okay (int status)
{
}
