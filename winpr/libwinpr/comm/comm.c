/**
 * WinPR: Windows Portable Runtime
 * Serial Communication API
 *
 * Copyright 2011 O.S. Systems Software Ltda.
 * Copyright 2011 Eduardo Fiss Beloni <beloni@ossystems.com.br>
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright 2014 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef _WIN32

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <errno.h>

#include <freerdp/utils/debug.h>

#include <winpr/crt.h>
#include <winpr/comm.h>
#include <winpr/collections.h>
#include <winpr/tchar.h>

#include "comm_ioctl.h"


/**
 * Communication Resources:
 * http://msdn.microsoft.com/en-us/library/windows/desktop/aa363196/
 */

#include "comm.h"

BOOL BuildCommDCBA(LPCSTR lpDef, LPDCB lpDCB)
{
	return TRUE;
}

BOOL BuildCommDCBW(LPCWSTR lpDef, LPDCB lpDCB)
{
	return TRUE;
}

BOOL BuildCommDCBAndTimeoutsA(LPCSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
{
	return TRUE;
}

BOOL BuildCommDCBAndTimeoutsW(LPCWSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts)
{
	return TRUE;
}

BOOL CommConfigDialogA(LPCSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
{
	return TRUE;
}

BOOL CommConfigDialogW(LPCWSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC)
{
	return TRUE;
}

BOOL GetCommConfig(HANDLE hCommDev, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hCommDev;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL SetCommConfig(HANDLE hCommDev, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hCommDev;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL GetCommMask(HANDLE hFile, PDWORD lpEvtMask)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL SetCommMask(HANDLE hFile, DWORD dwEvtMask)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL GetCommModemStatus(HANDLE hFile, PDWORD lpModemStat)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

/**
 * ERRORS:
 *   ERROR_INVALID_HANDLE
 */
BOOL GetCommProperties(HANDLE hFile, LPCOMMPROP lpCommProp)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;
	DWORD bytesReturned;

	if (!pComm || pComm->Type != HANDLE_TYPE_COMM || !pComm->fd )
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_GET_PROPERTIES, NULL, 0, lpCommProp, sizeof(COMMPROP), &bytesReturned, NULL))
	{
		DEBUG_WARN("GetCommProperties failure.");
		return FALSE;
	}

	return TRUE;
}



/**
 * 
 *
 * ERRORS:
 *   ERROR_INVALID_HANDLE
 *   ERROR_INVALID_DATA
 *   ERROR_IO_DEVICE
 *   ERROR_OUTOFMEMORY
 */
BOOL GetCommState(HANDLE hFile, LPDCB lpDCB)
{
	DCB *lpLocalDcb;
	struct termios currentState;
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;
	DWORD bytesReturned;

	if (!pComm || pComm->Type != HANDLE_TYPE_COMM || !pComm->fd )
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (!lpDCB)
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	if (lpDCB->DCBlength < sizeof(DCB))
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	if (tcgetattr(pComm->fd, &currentState) < 0)
	{
		SetLastError(ERROR_IO_DEVICE);
		return FALSE;
	}

	lpLocalDcb = (DCB*)calloc(1, lpDCB->DCBlength);
	if (lpLocalDcb == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return FALSE;
	}

	/* error_handle */
	
	lpLocalDcb->DCBlength = lpDCB->DCBlength;

	SERIAL_BAUD_RATE baudRate;
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_GET_BAUD_RATE, NULL, 0, &baudRate, sizeof(SERIAL_BAUD_RATE), &bytesReturned, NULL))
	{
		DEBUG_WARN("GetCommState failure: could not get the baud rate.");
		goto error_handle;
	}
	lpLocalDcb->BaudRate = baudRate.BaudRate;
		    
	lpLocalDcb->fBinary = (currentState.c_cflag & ICANON) == 0;
	if (!lpLocalDcb->fBinary)
	{
		DEBUG_WARN("Unexpected nonbinary mode, consider to unset the ICANON flag.");
	}

	lpLocalDcb->fParity =  (currentState.c_iflag & INPCK) != 0; 

	SERIAL_HANDFLOW handflow;
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_GET_HANDFLOW, NULL, 0, &handflow, sizeof(SERIAL_HANDFLOW), &bytesReturned, NULL))
	{
		DEBUG_WARN("GetCommState failure: could not get the handflow settings.");
		goto error_handle;
	}

	lpLocalDcb->fOutxCtsFlow = (handflow.ControlHandShake & SERIAL_CTS_HANDSHAKE) != 0;

	lpLocalDcb->fOutxDsrFlow = (handflow.ControlHandShake & SERIAL_DSR_HANDSHAKE) != 0;

	if (handflow.ControlHandShake & SERIAL_DTR_HANDSHAKE)
	{
		lpLocalDcb->fDtrControl = DTR_CONTROL_HANDSHAKE;
	}
	else if (handflow.ControlHandShake & SERIAL_DTR_CONTROL)
	{
		lpLocalDcb->fDtrControl = DTR_CONTROL_ENABLE;
	}
	else
	{
		lpLocalDcb->fDtrControl = DTR_CONTROL_DISABLE;
	}

	lpLocalDcb->fDsrSensitivity = (handflow.ControlHandShake & SERIAL_DSR_SENSITIVITY) != 0;

	lpLocalDcb->fTXContinueOnXoff = (handflow.FlowReplace & SERIAL_XOFF_CONTINUE) != 0; 

	lpLocalDcb->fOutX = (handflow.FlowReplace & SERIAL_AUTO_TRANSMIT) != 0;

	lpLocalDcb->fInX = (handflow.FlowReplace & SERIAL_AUTO_RECEIVE) != 0;

	lpLocalDcb->fErrorChar = (handflow.FlowReplace & SERIAL_ERROR_CHAR) != 0;

	lpLocalDcb->fNull = (handflow.FlowReplace & SERIAL_NULL_STRIPPING) != 0;

	if (handflow.FlowReplace & SERIAL_RTS_HANDSHAKE)
	{
		lpLocalDcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
	}
	else if (handflow.FlowReplace & SERIAL_RTS_CONTROL)
	{
		lpLocalDcb->fRtsControl = RTS_CONTROL_ENABLE;
	}
	else
	{
		lpLocalDcb->fRtsControl = RTS_CONTROL_DISABLE;
	}

	// FIXME: how to get the RTS_CONTROL_TOGGLE state? Does it match the UART 16750's Autoflow Control Enabled bit in its Modem Control Register (MCR)


	lpLocalDcb->fAbortOnError = (handflow.ControlHandShake & SERIAL_ERROR_ABORT) != 0;

	/* lpLocalDcb->fDummy2 not used */

	lpLocalDcb->wReserved = 0; /* must be zero */

	lpLocalDcb->XonLim = handflow.XonLimit;

	lpLocalDcb->XoffLim = handflow.XoffLimit;

	SERIAL_LINE_CONTROL lineControl;
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_GET_LINE_CONTROL, NULL, 0, &lineControl, sizeof(SERIAL_LINE_CONTROL), &bytesReturned, NULL))
	{
		DEBUG_WARN("GetCommState failure: could not get the control settings.");
		goto error_handle;
	}

	lpLocalDcb->ByteSize = lineControl.WordLength;

	lpLocalDcb->Parity = lineControl.Parity;

	lpLocalDcb->StopBits = lineControl.StopBits;


	SERIAL_CHARS serialChars;
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_GET_CHARS, NULL, 0, &serialChars, sizeof(SERIAL_CHARS), &bytesReturned, NULL))
	{
		DEBUG_WARN("GetCommState failure: could not get the serial chars.");
		goto error_handle;
	}

	lpLocalDcb->XonChar = serialChars.XonChar;

	lpLocalDcb->XoffChar = serialChars.XoffChar;

	lpLocalDcb->ErrorChar = serialChars.ErrorChar;

	lpLocalDcb->EofChar = serialChars.EofChar;
	
	lpLocalDcb->EvtChar = serialChars.EventChar;


	memcpy(lpDCB, lpLocalDcb, lpDCB->DCBlength);
	return TRUE;


  error_handle:
	if (lpLocalDcb)
		free(lpLocalDcb);

	return FALSE;
}


/**
 * @return TRUE on success, FALSE otherwise.
 * 
 * As of today, SetCommState() can fail half-way with some settings
 * applied and some others not. SetCommState() returns on the first
 * failure met. FIXME: or is it correct?
 *
 * ERRORS:
 *   ERROR_INVALID_HANDLE
 *   ERROR_IO_DEVICE
 */
BOOL SetCommState(HANDLE hFile, LPDCB lpDCB)
{
	struct termios upcomingTermios;
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;
	DWORD bytesReturned;

	// TMP: FIXME: validate changes according GetCommProperties

	if (!pComm || pComm->Type != HANDLE_TYPE_COMM || !pComm->fd )
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	if (!lpDCB)
	{
		SetLastError(ERROR_INVALID_DATA);
		return FALSE;
	}

	/* NB: did the choice to call ioctls first when available and
	   then to setup upcomingTermios. Don't mix both stages. */

	/** ioctl calls stage **/

	SERIAL_BAUD_RATE baudRate;
	baudRate.BaudRate = lpDCB->BaudRate;
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_SET_BAUD_RATE, &baudRate, sizeof(SERIAL_BAUD_RATE), NULL, 0, &bytesReturned, NULL))
	{
		DEBUG_WARN("SetCommState failure: could not set the baud rate.");
		return FALSE;
	}

	SERIAL_CHARS serialChars;
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_GET_CHARS, NULL, 0, &serialChars, sizeof(SERIAL_CHARS), &bytesReturned, NULL)) /* as of today, required for BreakChar */
	{
		DEBUG_WARN("SetCommState failure: could not get the initial serial chars.");
		return FALSE;
	}
	serialChars.XonChar = lpDCB->XonChar;
	serialChars.XoffChar = lpDCB->XoffChar;
	serialChars.ErrorChar = lpDCB->ErrorChar;
	serialChars.EofChar = lpDCB->EofChar;
	serialChars.EventChar = lpDCB->EvtChar;	
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_SET_CHARS, &serialChars, sizeof(SERIAL_CHARS), NULL, 0, &bytesReturned, NULL))
	{
		DEBUG_WARN("SetCommState failure: could not set the serial chars.");
		return FALSE;
	}

	SERIAL_LINE_CONTROL lineControl;
	lineControl.StopBits = lpDCB->StopBits;
	lineControl.Parity = lpDCB->Parity;
	lineControl.WordLength = lpDCB->ByteSize;
	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_SET_LINE_CONTROL, &lineControl, sizeof(SERIAL_LINE_CONTROL), NULL, 0, &bytesReturned, NULL))
	{
		DEBUG_WARN("SetCommState failure: could not set the control settings.");
		return FALSE;
	}


	SERIAL_HANDFLOW handflow;
	ZeroMemory(&handflow, sizeof(SERIAL_HANDFLOW));

	if (lpDCB->fOutxCtsFlow)
	{
		handflow.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
	}
	

	if (lpDCB->fOutxDsrFlow)
	{
		handflow.ControlHandShake |= SERIAL_DSR_HANDSHAKE;
	}

	switch (lpDCB->fDtrControl)
	{
		case SERIAL_DTR_HANDSHAKE:
			handflow.ControlHandShake |= DTR_CONTROL_HANDSHAKE;
			break;

		case SERIAL_DTR_CONTROL:
			handflow.ControlHandShake |= DTR_CONTROL_ENABLE;
			break;

		case DTR_CONTROL_DISABLE:
			/* do nothing since handflow is init-zeroed */
			break;

		default:
			DEBUG_WARN("Unexpected fDtrControl value: %d\n", lpDCB->fDtrControl);
			return FALSE;
	}

	if (lpDCB->fDsrSensitivity)
	{
		handflow.ControlHandShake |= SERIAL_DSR_SENSITIVITY;
	}

	if (lpDCB->fTXContinueOnXoff)
	{
		handflow.FlowReplace |= SERIAL_XOFF_CONTINUE;
	}
	
	if (lpDCB->fOutX)
	{
		handflow.FlowReplace |= SERIAL_AUTO_TRANSMIT;
	}

	if (lpDCB->fInX)
	{
		handflow.FlowReplace |= SERIAL_AUTO_RECEIVE;
	}

	if (lpDCB->fErrorChar)
	{
		handflow.FlowReplace |= SERIAL_ERROR_CHAR;
	}

	if (lpDCB->fNull)
	{
		handflow.FlowReplace |= SERIAL_NULL_STRIPPING;
	}

	switch (lpDCB->fRtsControl)
	{
		case RTS_CONTROL_TOGGLE:
			DEBUG_WARN("Unsupported RTS_CONTROL_TOGGLE feature");
			// FIXME: see also GetCommState()
			return FALSE;

		case RTS_CONTROL_HANDSHAKE:
			handflow.FlowReplace |= SERIAL_RTS_HANDSHAKE;
			break;

		case RTS_CONTROL_ENABLE:
			handflow.FlowReplace |= SERIAL_RTS_CONTROL;
			break;

		case RTS_CONTROL_DISABLE:
			/* do nothing since handflow is init-zeroed */
			break;

		default:
			DEBUG_WARN("Unexpected fRtsControl value: %d\n", lpDCB->fRtsControl);
			return FALSE;
	}

	if (lpDCB->fAbortOnError)
	{
		handflow.ControlHandShake |= SERIAL_ERROR_ABORT;
	}

	/* lpDCB->fDummy2 not used */

	/* lpLocalDcb->wReserved  ignored */

	handflow.XonLimit = lpDCB->XonLim;

	handflow.XoffLimit = lpDCB->XoffLim;

	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_SET_HANDFLOW, &handflow, sizeof(SERIAL_HANDFLOW), NULL, 0, &bytesReturned, NULL))
	{
		DEBUG_WARN("SetCommState failure: could not set the handflow settings.");
		return FALSE;
	}


	/** upcomingTermios stage **/


	ZeroMemory(&upcomingTermios, sizeof(struct termios));
	if (tcgetattr(pComm->fd, &upcomingTermios) < 0) /* NB: preserves current settings not directly handled by the Communication Functions */
	{
		SetLastError(ERROR_IO_DEVICE);
		return FALSE;
	}

	if (lpDCB->fBinary)
	{
		upcomingTermios.c_lflag &= ~ICANON;
	}
	else
	{
		upcomingTermios.c_lflag |= ICANON;
		DEBUG_WARN("Unexpected nonbinary mode, consider to unset the ICANON flag.");
	}

	if (lpDCB->fParity)
	{
		upcomingTermios.c_iflag |= INPCK;
	}
	else
	{
		upcomingTermios.c_iflag &= ~INPCK;
	}

	// TMP: TODO:
	// (...)

	/* http://msdn.microsoft.com/en-us/library/windows/desktop/aa363423%28v=vs.85%29.aspx
	 *
	 * The SetCommState function reconfigures the communications
	 * resource, but it does not affect the internal output and
	 * input buffers of the specified driver. The buffers are not
	 * flushed, and pending read and write operations are not
	 * terminated prematurely.
	 *
	 * TCSANOW matches the best this definition
	 */
	
	if (_comm_ioctl_tcsetattr(pComm->fd, TCSANOW, &upcomingTermios) < 0)
	{
		SetLastError(ERROR_IO_DEVICE);
		return FALSE;
	}
	

	return TRUE;
}

/**
 * ERRORS:
 *   ERROR_INVALID_HANDLE
 */
BOOL GetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;
	DWORD bytesReturned;

	if (!pComm || pComm->Type != HANDLE_TYPE_COMM || !pComm->fd )
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	/* as of today, SERIAL_TIMEOUTS and COMMTIMEOUTS structures are identical */

	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_GET_TIMEOUTS, NULL, 0, lpCommTimeouts, sizeof(COMMTIMEOUTS), &bytesReturned, NULL))
	{
		DEBUG_WARN("GetCommTimeouts failure.");
		return FALSE;
	}

	return TRUE;
}


/**
 * ERRORS:
 *   ERROR_INVALID_HANDLE
 */
BOOL SetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;
	DWORD bytesReturned;

	if (!pComm || pComm->Type != HANDLE_TYPE_COMM || !pComm->fd )
	{
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}

	/* as of today, SERIAL_TIMEOUTS and COMMTIMEOUTS structures are identical */

	if (!CommDeviceIoControl(pComm, IOCTL_SERIAL_SET_TIMEOUTS, lpCommTimeouts, sizeof(COMMTIMEOUTS), NULL, 0, &bytesReturned, NULL))
	{
		DEBUG_WARN("SetCommTimeouts failure.");
		return FALSE;
	}

	return TRUE;
}

BOOL GetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	return TRUE;
}

BOOL GetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize)
{
	return TRUE;
}

BOOL SetDefaultCommConfigA(LPCSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	return TRUE;
}

BOOL SetDefaultCommConfigW(LPCWSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize)
{
	return TRUE;
}

BOOL SetCommBreak(HANDLE hFile)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL ClearCommBreak(HANDLE hFile)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL ClearCommError(HANDLE hFile, PDWORD lpErrors, LPCOMSTAT lpStat)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL PurgeComm(HANDLE hFile, DWORD dwFlags)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL SetupComm(HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

/* http://msdn.microsoft.com/en-us/library/windows/desktop/aa363423%28v=vs.85%29.aspx */
/* A process reinitializes a communications resource by using the SetupComm function, which performs the following tasks: */

/*     Terminates pending read and write operations, even if they have not been completed. */
/*     Discards unread characters and frees the internal output and input buffers of the driver associated with the specified resource. */
/*     Reallocates the internal output and input buffers. */

/* A process is not required to call SetupComm. If it does not, the resource's driver initializes the device with the default settings the first time that the communications resource handle is used. */

	return TRUE;
}

BOOL EscapeCommFunction(HANDLE hFile, DWORD dwFunc)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL TransmitCommChar(HANDLE hFile, char cChar)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}

BOOL WaitCommEvent(HANDLE hFile, PDWORD lpEvtMask, LPOVERLAPPED lpOverlapped)
{
	WINPR_COMM* pComm = (WINPR_COMM*) hFile;

	if (!pComm)
		return FALSE;

	return TRUE;
}


/* Extended API */

/* FIXME: DefineCommDevice / QueryCommDevice look over complicated for
 * just a couple of strings, should be simplified.
 * 
 * TODO: what about libwinpr-io.so?
 */
static wHashTable *_CommDevices = NULL;

static HANDLE_CREATOR *_CommHandleCreator = NULL;

static int deviceNameCmp(void* pointer1, void* pointer2)
{
	return _tcscmp(pointer1, pointer2);
}


static int devicePathCmp(void* pointer1, void* pointer2)
{
	return _tcscmp(pointer1, pointer2);
}

/* copied from HashTable.c */
static unsigned long HashTable_StringHashFunctionA(void* key)
{
	int c;
	unsigned long hash = 5381;
	unsigned char* str = (unsigned char*) key;

	/* djb2 algorithm */
	while ((c = *str++) != '\0')
		hash = (hash * 33) + c;

	return hash;
}


static void _CommDevicesInit()
{
	/* 
	 * TMP: FIXME: What kind of mutex should be used here?
	 * better have to let DefineCommDevice() and QueryCommDevice() thread unsafe ?
	 * use of a module_init() ?
	 */ 

	if (_CommDevices == NULL)
	{
		_CommDevices = HashTable_New(TRUE);
		_CommDevices->keycmp = deviceNameCmp;
		_CommDevices->valuecmp = devicePathCmp;
		_CommDevices->hashFunction = HashTable_StringHashFunctionA; /* TMP: FIXME: need of a HashTable_StringHashFunctionW */
		_CommDevices->keyDeallocator = free;
		_CommDevices->valueDeallocator = free;

		_CommHandleCreator = (HANDLE_CREATOR*)malloc(sizeof(HANDLE_CREATOR));
		_CommHandleCreator->IsHandled = IsCommDevice;
		_CommHandleCreator->CreateFileA = CommCreateFileA;

		RegisterHandleCreator(_CommHandleCreator);
	}
}


static BOOL _IsReservedCommDeviceName(LPCTSTR lpName)
{
	int i;

	/* Serial ports, COM1-9 */
	for (i=1; i<10; i++)
	{
		TCHAR genericName[5];
		if (_stprintf_s(genericName, 5, "COM%d", i) < 0)
		{
			return FALSE;
		}

		if (_tcscmp(genericName, lpName) == 0)
			return TRUE;
	}

	/* Parallel ports, LPT1-9 */
	for (i=1; i<10; i++)
	{
		TCHAR genericName[5];
		if (_stprintf_s(genericName, 5, "LPT%d", i) < 0)
		{
			return FALSE;
		}

		if (_tcscmp(genericName, lpName) == 0)
			return TRUE;
	}

	/* TMP: TODO: PRN ? */

	return FALSE;
}


/**
 * Returns TRUE on success, FALSE otherwise. To get extended error
 * information, call GetLastError.
 * 
 * ERRORS:
 *   ERROR_OUTOFMEMORY was not possible to get mappings.
 *   ERROR_INVALID_DATA was not possible to add the device.
 */
BOOL DefineCommDevice(/* DWORD dwFlags,*/ LPCTSTR lpDeviceName, LPCTSTR lpTargetPath)
{
	LPTSTR storedDeviceName = NULL;
	LPTSTR storedTargetPath = NULL;

	_CommDevicesInit();
	if (_CommDevices == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		goto error_handle;
	}

	if (_tcsncmp(lpDeviceName, _T("\\\\.\\"), 4) != 0)
	{
		if (!_IsReservedCommDeviceName(lpDeviceName))
		{
			SetLastError(ERROR_INVALID_DATA);
			goto error_handle;
		}
	}

	storedDeviceName = _tcsdup(lpDeviceName);
	if (storedDeviceName == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		goto error_handle;
	}

	storedTargetPath = _tcsdup(lpTargetPath);
	if (storedTargetPath == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		goto error_handle;
	}

	if (HashTable_Add(_CommDevices, storedDeviceName, storedTargetPath) < 0)
	{
		SetLastError(ERROR_INVALID_DATA);
		goto error_handle;
	}

	return TRUE;


  error_handle:
	if (storedDeviceName != NULL)
		free(storedDeviceName);

	if (storedTargetPath != NULL)
		free(storedTargetPath);

	return FALSE;
}


/**
 * Returns the number of target paths in the buffer pointed to by
 * lpTargetPath. 
 *
 * The current implementation returns in any case 0 and 1 target
 * path. A NULL lpDeviceName is not supported yet to get all the
 * paths.
 *  
 * ERRORS:
 *   ERROR_SUCCESS
 *   ERROR_OUTOFMEMORY was not possible to get mappings.
 *   ERROR_NOT_SUPPORTED equivalent QueryDosDevice feature not supported.
 *   ERROR_INVALID_DATA was not possible to retrieve any device information.
 *   ERROR_INSUFFICIENT_BUFFER too small lpTargetPath
 */
DWORD QueryCommDevice(LPCTSTR lpDeviceName, LPTSTR lpTargetPath, DWORD ucchMax)
{
	LPTSTR storedTargetPath;

	SetLastError(ERROR_SUCCESS);

	_CommDevicesInit();
	if (_CommDevices == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return 0;
	}

	if (lpDeviceName == NULL || lpTargetPath == NULL)
	{
		SetLastError(ERROR_NOT_SUPPORTED);
		return 0;
	}

	storedTargetPath = HashTable_GetItemValue(_CommDevices, (void*)lpDeviceName);
	if (storedTargetPath == NULL)
	{
		SetLastError(ERROR_INVALID_DATA);
		return 0;
	}

	if (_tcslen(storedTargetPath) + 2 > ucchMax)
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}

	_tcscpy(lpTargetPath, storedTargetPath);
	_tcscat(lpTargetPath, ""); /* 2nd final '\0' */

	return _tcslen(lpTargetPath) + 2;
}

/**
 * Checks whether lpDeviceName is a valid and registered Communication device. 
 */
BOOL IsCommDevice(LPCTSTR lpDeviceName)
{
	TCHAR lpTargetPath[MAX_PATH];

	if (QueryCommDevice(lpDeviceName, lpTargetPath, MAX_PATH) > 0)
	{
		return TRUE;
	}

	return FALSE;
}


/**
 * Sets 
 */
void _comm_setRemoteSerialDriver(HANDLE hComm, REMOTE_SERIAL_DRIVER_ID driverId)
{
	ULONG Type;
	PVOID Object;
	WINPR_COMM* pComm;

	if (!winpr_Handle_GetInfo(hComm, &Type, &Object))
	{
		DEBUG_WARN("_comm_setRemoteSerialDriver failure");
		return;
	}

	pComm = (WINPR_COMM*)Object;
	pComm->remoteSerialDriverId = driverId;
}


/**
 * http://msdn.microsoft.com/en-us/library/windows/desktop/aa363198%28v=vs.85%29.aspx
 *
 * @param lpDeviceName e.g. COM1, \\.\COM1, ...
 *
 * @param dwDesiredAccess expects GENERIC_READ | GENERIC_WRITE, a
 * warning message is printed otherwise. TODO: better support.
 *
 * @param dwShareMode must be zero, INVALID_HANDLE_VALUE is returned
 * otherwise and GetLastError() should return ERROR_SHARING_VIOLATION.
 * 
 * @param lpSecurityAttributes NULL expected, a warning message is printed
 * otherwise. TODO: better support.
 * 
 * @param dwCreationDisposition must be OPEN_EXISTING. If the
 * communication device doesn't exist INVALID_HANDLE_VALUE is returned
 * and GetLastError() returns ERROR_FILE_NOT_FOUND.
 *
 * @param dwFlagsAndAttributes zero expected, a warning message is
 * printed otherwise.
 *
 * @param hTemplateFile must be NULL.
 *
 * @return INVALID_HANDLE_VALUE on error. 
 */
HANDLE CommCreateFileA(LPCSTR lpDeviceName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		       DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	CHAR devicePath[MAX_PATH];
	struct stat deviceStat;
	WINPR_COMM* pComm = NULL;
	struct termios upcomingTermios;
	
	if (dwDesiredAccess != (GENERIC_READ | GENERIC_WRITE))
	{
		DEBUG_WARN("unexpected access to the device: 0x%x", dwDesiredAccess);
	}

	if (dwShareMode != 0)
	{
		SetLastError(ERROR_SHARING_VIOLATION);
		return INVALID_HANDLE_VALUE;
	}

	/* TODO: Prevents other processes from opening a file or
         * device if they request delete, read, or write access. */

	if (lpSecurityAttributes != NULL)
	{
		DEBUG_WARN("unexpected security attributes: 0x%x", lpSecurityAttributes);
	}

	if (dwCreationDisposition != OPEN_EXISTING)
	{
		SetLastError(ERROR_FILE_NOT_FOUND); /* FIXME: ERROR_NOT_SUPPORTED better? */
		return INVALID_HANDLE_VALUE;
	}
	
	if (QueryCommDevice(lpDeviceName, devicePath, MAX_PATH) <= 0)
	{
		/* SetLastError(GetLastError()); */
		return INVALID_HANDLE_VALUE;
	}

	if (stat(devicePath, &deviceStat) < 0)
	{
		DEBUG_WARN("device not found %s", devicePath);
		SetLastError(ERROR_FILE_NOT_FOUND);
		return INVALID_HANDLE_VALUE;
	}

	if (!S_ISCHR(deviceStat.st_mode))
	{
		DEBUG_WARN("bad device %d", devicePath);
		SetLastError(ERROR_BAD_DEVICE);
		return INVALID_HANDLE_VALUE;
	}

	if (dwFlagsAndAttributes != 0)
	{
		DEBUG_WARN("unexpected flags and attributes: 0x%x", dwFlagsAndAttributes);
	}

	if (hTemplateFile != NULL)
	{
		SetLastError(ERROR_NOT_SUPPORTED); /* FIXME: other proper error? */
		return INVALID_HANDLE_VALUE;
	}

	pComm = (WINPR_COMM*) calloc(1, sizeof(WINPR_COMM));
	if (pComm == NULL)
	{
		SetLastError(ERROR_OUTOFMEMORY);
		return INVALID_HANDLE_VALUE;
	}

	WINPR_HANDLE_SET_TYPE(pComm, HANDLE_TYPE_COMM);

	/* error_handle */

	pComm->fd = open(devicePath, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (pComm->fd < 0)
	{
		DEBUG_WARN("failed to open device %s", devicePath);
		SetLastError(ERROR_BAD_DEVICE);
		goto error_handle;
	}

	/* Restore the blocking mode for upcoming read/write operations */
	if (fcntl(pComm->fd, F_SETFL, fcntl(pComm->fd, F_GETFL) & ~O_NONBLOCK) < 0)
	{
		DEBUG_WARN("failed to open device %s, could not restore the O_NONBLOCK flag", devicePath);
		SetLastError(ERROR_BAD_DEVICE);
		goto error_handle;
	}


	/* TMP: TODO: FIXME: this information is at least needed for
	 * get/set baud functions. Is it possible to pull this
	 * information? Could be a command line argument.
	 */
	pComm->remoteSerialDriverId = RemoteSerialDriverUnknown;


	/* The binary/raw mode is required for the redirection but
	 * only flags that are not handle somewhere-else, except
	 * ICANON, are forced here. */

	ZeroMemory(&upcomingTermios, sizeof(struct termios));
	if (tcgetattr(pComm->fd, &upcomingTermios) < 0)
	{
		SetLastError(ERROR_IO_DEVICE);
		goto error_handle;
	}

	upcomingTermios.c_iflag &= ~(/*IGNBRK |*/ BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL /*| IXON*/);
	upcomingTermios.c_oflag = 0; /* <=> &= ~OPOST */
	upcomingTermios.c_lflag = 0; /* <=> &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN); */
	/* upcomingTermios.c_cflag &= ~(CSIZE | PARENB); */
	/* upcomingTermios.c_cflag |= CS8; */

	/* About missing missing flags recommended by termios(3)
	 *
	 *   IGNBRK and IXON, see: IOCTL_SERIAL_SET_HANDFLOW
	 *   CSIZE, PARENB and CS8, see: IOCTL_SERIAL_SET_LINE_CONTROL
	 */
		
	/* a few more settings required for the redirection */
	upcomingTermios.c_cflag |= CLOCAL | CREAD;

	if (_comm_ioctl_tcsetattr(pComm->fd, TCSANOW, &upcomingTermios) < 0)
	{
		SetLastError(ERROR_IO_DEVICE);
		goto error_handle;
	}

	return (HANDLE)pComm;


  error_handle:
	if (pComm != NULL)
	{
		CloseHandle(pComm);
	}


	return INVALID_HANDLE_VALUE;
}

#endif /* _WIN32 */
