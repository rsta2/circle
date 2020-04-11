#ifndef _p9error_h
#define _p9error_h

#include <circle/setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

struct error_stack_t
{
#define ERROR_STACK_SIZE	30
	jmp_buf stack[ERROR_STACK_SIZE];
	volatile unsigned stackptr;
};

struct error_stack_t *get_error_stack (void);

#define error	__p9error
void error (const char *str);
#define Eio		"I/O error"
#define Enomem		"Not enough memory"
#define Enonexist	"File does not exist"

jmp_buf *pusherror (void);
#define waserror()	setjmp (*pusherror ())

void nexterror (void);
void poperror (void);

void okay (int status);

#ifdef __cplusplus
}
#endif

#endif
