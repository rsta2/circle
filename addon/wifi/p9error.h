#ifndef _p9error_h
#define _p9error_h

#ifdef __cplusplus
extern "C" {
#endif

void error (const char *str);
#define Eio		"I/O error"
#define Enomem		"Not enough memory"
#define Enonexist	"File does not exist"

int waserror (void);
void nexterror (void);
void poperror (void);

void okay (int status);

#ifdef __cplusplus
}
#endif

#endif
