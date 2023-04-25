//
// shell.cpp
//
#include <shell/shell.h>


LOGMODULE ("shell");


#define SHELL_LINE_BUFFER_LEN  512



//
// Class
//

CShell::CShell (CDevice *pInputDevice)
: m_pInputDevice (pInputDevice),
  m_pInputBuffer(0),
  m_pInputWrite(0),
  m_pRebootCallback(0),
  m_pRebootCallbackContext(0)
{
  
}

CShell::~CShell (void)
{
  	//
}

boolean CShell::Initialize ()
{ 
  LOGNOTE("Initializing Shell");

  // Allocate Shell buffers
  m_pInputBuffer = new char[SHELL_LINE_BUFFER_LEN];  

  // Clear the line buffer
  memset(m_pInputBuffer, 0, SHELL_LINE_BUFFER_LEN);

  // Set the initial write pointer
  m_pInputWrite = m_pInputBuffer;
  
	return TRUE;
}


void CShell::Update()
{
  // LOGDBG("Updating Shell");

	int readCount = 0;
  int maxReadCount = SHELL_LINE_BUFFER_LEN - 1;
  maxReadCount = maxReadCount - (m_pInputWrite - m_pInputBuffer);

  // Read from the input device
	readCount = m_pInputDevice->Read(m_pInputWrite, maxReadCount);

  // If read anything, copy it to the line buffer
	if (readCount > 0) {
    m_pInputWrite += readCount;

    // Check if we have a line (either return, or max line length)
    int totalReadCount = (m_pInputWrite - m_pInputBuffer);
    boolean haveEOL = FALSE;
    char* pLineData = m_pInputBuffer;

    // Peek the line (e.g. for the reboot magic)
    PeekLine();

    // Look for newline to signify end of line
    while (pLineData < m_pInputWrite) {
      if (*pLineData == '\n' || *pLineData == '\r') {
        haveEOL = TRUE;
        break;
      }
      pLineData++;
    }

    if (haveEOL || totalReadCount == maxReadCount) {
      // Ensure terminator at end of line
      *m_pInputWrite = '\0';

      // Process the line
      ProcessLine();

      // Get ready for the next line            
      memset(m_pInputBuffer, 0, SHELL_LINE_BUFFER_LEN);  // Clear the line buffer  
      m_pInputWrite = m_pInputBuffer; // Set the initial write pointer
    }
	}
}

void CShell::SetRebootCallback(SHELL_CALLBACK_T pRebootCallback, void *pContext) {
    m_pRebootCallback = pRebootCallback;
    m_pRebootCallbackContext = pContext;
}

void CShell::PeekLine() {
  int maxReadCount = SHELL_LINE_BUFFER_LEN - 1;

  // Check for reboot magic
  int rebootMagicLen = strlen(REBOOT_MAGIC);

  for (int i = 0; i < maxReadCount; i++) {
    char* pLineData = &m_pInputBuffer[i];
    if (strncmp(REBOOT_MAGIC, pLineData, rebootMagicLen) == 0) {
      if (m_pRebootCallback != nullptr) {
        m_pRebootCallback(m_pRebootCallbackContext);
      }
    };
  }
}


void CShell::ProcessLine() {
  LOGDBG("%s", m_pInputBuffer);
}


