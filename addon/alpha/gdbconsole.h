#ifndef CIRCLE_ADDON_ALPHA_GDBCONSOLE_H_
# define CIRCLE_ADDON_ALPHA_GDBCONSOLE_H_

# include <circle/device.h>

///
/// GDB console I/Os as a CDevice, mainly written to be used as a CLogger target
/// to get Circle's logs written into the GDB consle. Once created, this device
/// can be also retrieved using CDeviceNameService under the device name used in
/// the constructor, "gdb" by default. So one can enable it through Circle
/// option `logdev=gdb`.
/// It is based on GDB's File I/O protocole extension implemented by Alpha.
///
class GDBConsole : public CDevice
{
public:
  /// Add the device to the CDeviceNameService.
  /// \param pDeviceName CDeviceNameService registration name.
	GDBConsole (const char* pDeviceName = "gdb");

  /// Write to the GDB console.
  /// \param pBuffer Pointer to data to be sent
	/// \param nCount Number of bytes to be sent
	/// \return Number of bytes successfully sent (< 0 on error)
  int Write (const void *pBuffer, unsigned nCount);

  /// Read from the GDB console.
  /// \param pBuffer Pointer to buffer for received data
	/// \param nCount Maximum number of bytes to be received
	/// \return Number of bytes received (0 no data available, < 0 on error)
	int Read (void *pBuffer, unsigned nCount);
};

#endif /* !CIRCLE_ADDON_ALPHA_GDBCONSOLE_H_ */
