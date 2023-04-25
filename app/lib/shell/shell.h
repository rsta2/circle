//
// shell.h
//
#ifndef _shell_h
#define _shell_h

#include <circle/types.h>
#include <circle/interrupt.h>
#include <circle/logger.h>
#include <circle/util.h>


#define REBOOT_MAGIC "__reboot__"

typedef void (*SHELL_CALLBACK_T)(void *context);

class CShell
{
public:
	CShell (CDevice *pInputDevice);
	~CShell (void);

	// methods ...
	boolean Initialize();
	
	void Update(void);

	void SetRebootCallback(SHELL_CALLBACK_T pRebootCallback, void *pContext);

private:
	void PeekLine(void);
	void ProcessLine(void);
		

private:
	// members ...
	CDevice 	*m_pInputDevice;
	char		*m_pInputBuffer;
	char		*m_pInputWrite;
	SHELL_CALLBACK_T m_pRebootCallback;
	void 		*m_pRebootCallbackContext;
};

#endif // _shell_h
