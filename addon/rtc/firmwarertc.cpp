//
// firmwarertc.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2024  R. Stange <rsta2@o2online.de>
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
#include <rtc/firmwarertc.h>
#include <circle/bcmpropertytags.h>
#include <circle/machineinfo.h>
#include <circle/logger.h>
#include <assert.h>

LOGMODULE ("fwrtc");

boolean CFirmwareRTC::Initialize (void)
{
	const CDeviceTreeBlob *pDTB = CMachineInfo::Get ()->GetDTB ();
	const TDeviceTreeNode *pRTCNode;
	const TDeviceTreeProperty *pChargeMicroVolt;
	if (   !pDTB
	    || !(pRTCNode = pDTB->FindNode ("/soc/rpi_rtc"))
	    || !(pChargeMicroVolt = pDTB->FindProperty (pRTCNode, "trickle-charge-microvolt"))
	    || pDTB->GetPropertyValueLength (pChargeMicroVolt) != 4)
	{
		return FALSE;
	}

	u32 nChargeMicroVolt = pDTB->GetPropertyValueWord (pChargeMicroVolt, 0);

	CBcmPropertyTags Tags;
	TPropertyTagRTCRegister RTCReg;
	RTCReg.nRegNum = RTC_REGISTER_BBAT_CHG_VOLTS;
	RTCReg.nValue = nChargeMicroVolt;
	if (!Tags.GetTag (PROPTAG_SET_RTC_REG, &RTCReg, sizeof RTCReg, 8))
	{
		return FALSE;
	}

	if (nChargeMicroVolt)
	{
		LOGNOTE ("Charging enabled at %.1fV", nChargeMicroVolt / 1000000.0);
	}

#ifndef NDEBUG
	//DumpStatus ();
#endif

	return TRUE;
}

boolean CFirmwareRTC::Get (CTime *pUTCTime)
{
	assert (pUTCTime);

	CBcmPropertyTags Tags;
	TPropertyTagRTCRegister RTCReg;
	RTCReg.nRegNum = RTC_REGISTER_TIME;
	if (!Tags.GetTag (PROPTAG_GET_RTC_REG, &RTCReg, sizeof RTCReg, 4))
	{
		return FALSE;
	}

	pUTCTime->Set (RTCReg.nValue);

	return TRUE;
}

boolean CFirmwareRTC::Set (const CTime &UTCTime)
{
	CBcmPropertyTags Tags;
	TPropertyTagRTCRegister RTCReg;
	RTCReg.nRegNum = RTC_REGISTER_TIME;
	RTCReg.nValue = UTCTime.Get ();
	return Tags.GetTag (PROPTAG_SET_RTC_REG, &RTCReg, sizeof RTCReg, 8);
}

#ifndef NDEBUG

void CFirmwareRTC::DumpStatus (void)
{
	static const char *Name[] =
	{
		"TIME",
		"ALARM",
		"ALARM_PENDING",
		"ALARM_ENABLE",
		"BBAT_CHG_VOLTS",
		"BBAT_CHG_VOLTS_MIN",
		"BBAT_CHG_VOLTS_MAX",
		"BBAT_VOLTS"
	};

	for (unsigned i = 0; i < RTC_REGISTER_COUNT; i++)
	{
		CBcmPropertyTags Tags;
		TPropertyTagRTCRegister RTCReg;
		RTCReg.nRegNum = i;
		if (Tags.GetTag (PROPTAG_GET_RTC_REG, &RTCReg, sizeof RTCReg, 4))
		{
			LOGNOTE ("%10u %s", RTCReg.nValue, Name[i]);
		}
	}
}

#endif
