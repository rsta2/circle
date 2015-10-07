//
// kernel.cpp
//
// Spectrum screen emulator sample provided by Jose Luis Sanchez
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include "BruceLeeScr.h"
#include <circle/util.h>
#include <assert.h>

static const char FromKernel[] = "kernel";

CKernel::CKernel(void) :
	m_Timer (&m_Interrupt),
	m_Logger (LogDebug, &m_Timer) {

	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel(void) {

}

boolean CKernel::Initialize(void) {

	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		bOK = m_Logger.Initialize (&m_Serial);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_SpectrumScreen.Initialize (BruceLee_scr);
	}

	return bOK;
}

TShutdownMode CKernel::Run(void) {

	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	while (TRUE) {
		m_SpectrumScreen.Update(FALSE);
		m_ActLED.Off();
		// The flash changes his state every 16 screen frames
		m_Timer.usDelay(319488);
		m_SpectrumScreen.Update(TRUE);
		m_ActLED.On();
		m_Timer.usDelay(319488);
	}

	return ShutdownHalt;
}
