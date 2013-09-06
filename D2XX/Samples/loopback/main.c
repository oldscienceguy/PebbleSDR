/*
 * Writes a known sequence of bytes then expects to read them back.
 * Run this with a loopback device fitted to one of FTDI's USB-RS232 
 * converter cables.
 * A loopback device has:
 *   1.  Receive Data    connected to    Transmit Data
 *   2.  Data Set Ready  connected to    Data Terminal Ready
 *   3.  Ready To Send   connected to    Clear To Send
 *
 * Build with:
 *     gcc main.c -o loopback -Wall -Wextra -lftd2xx -Wl,-rpath /usr/local/lib
 * 
 * Run with:
 *     sudo ./loopback
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../ftd2xx.h"



#define UNUSED_PARAMETER(x) (void)(x)

#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

// Number of bytes to write (and hopefully read)
#define BUF_SIZE 293



static void dumpBuffer(unsigned char *buffer, int elements)
{
	int j;

	for (j = 0; j < elements; j++)
	{
		if (j % 8 == 0)
		{
			if (j % 16 == 0)
				printf("\n%p: ", &buffer[j]);
			else
				printf("   "); // Separate two columns of eight bytes
		}
		printf("%02X ", (unsigned int)buffer[j]);
	}
	printf("\n\n");
}



int main(int argc, char *argv[])
{
	int             retCode = -1; // Assume failure
	int             f = 0;
	DWORD           driverVersion = 0;
	FT_STATUS       ftStatus = FT_OK;
	FT_HANDLE       ftHandle = NULL;
	int             portNum = 0; // First port
	DWORD           bytesToWrite = 0;
	DWORD           bytesWritten = 0;
	DWORD           bytesReceived = 0;
	DWORD           bytesRead = 0;
	unsigned char 	writeBuffer[BUF_SIZE];
	unsigned char  *readBuffer = NULL;
	int             queueChecks = 0;
	ULONG           rates[] = {300, 600, 1200, 2400, 4800, 9600,
	                           19200, 38400, 57600, 115200, 
	                           230400, 460800, 576000, 921600};
	
	UNUSED_PARAMETER(argc);
	UNUSED_PARAMETER(argv);
	
	(void)FT_GetDriverVersion(NULL, &driverVersion);
	printf("Using D2XX version %08x\n", driverVersion);

	// Fill write buffer with consecutive values
	bytesToWrite = ARRAY_SIZE(writeBuffer);
	for (f = 0; f < (int)bytesToWrite; f++) 
	{
		writeBuffer[f] = (unsigned char)f;
	}
	
	printf("Opening FTDI device %d.\n", portNum);
	
	ftStatus = FT_Open(portNum, &ftHandle);
	if (ftStatus != FT_OK) 
	{
		printf("FT_Open(%d) failed, with error %d.\n", portNum, (int)ftStatus);
		printf("Use lsmod to check if ftdi_sio (and usbserial) are present.\n");
		printf("If so, unload them using rmmod, as they conflict with ftd2xx.\n");
		goto exit;
	}

	assert(ftHandle != NULL);

	ftStatus = FT_ResetDevice(ftHandle);
	if (ftStatus != FT_OK) 
	{
		printf("Failure.  FT_ResetDevice returned %d.\n", (int)ftStatus);
		goto exit;
	}
	
	
	ftStatus = FT_SetDataCharacteristics(ftHandle, 
	                                     FT_BITS_8,
	                                     FT_STOP_BITS_1,
	                                     FT_PARITY_NONE);
	if (ftStatus != FT_OK) 
	{
		printf("Failure.  FT_SetDataCharacteristics returned %d.\n",
		       (int)ftStatus);
		goto exit;
	}
	
	for (f = 4; f < (int)ARRAY_SIZE(rates); f++)
	{
		ftStatus = FT_SetBaudRate(ftHandle, rates[f]);
		if (ftStatus != FT_OK) 
		{
			printf("Failure.  FT_SetBaudRate(%d) returned %d.\n", 
				   (int)rates[f],
				   (int)ftStatus);
			goto exit;
		}
		
		printf("\nBaud rate %d.  Writing %d bytes to loopback device.\n", 
		       (int)rates[f],
		       (int)bytesToWrite);

		ftStatus = FT_Write(ftHandle, 
		                    writeBuffer,
		                    bytesToWrite, 
		                    &bytesWritten);
		if (ftStatus != FT_OK) 
		{
			printf("Failure.  FT_Write returned %d\n", (int)ftStatus);
			goto exit;
		}
		
		if (bytesWritten != bytesToWrite)
		{
			printf("Failure.  FT_Write wrote %d bytes instead of %d.\n",
				   (int)bytesWritten,
				   (int)bytesToWrite);
			goto exit;
		}

		// Keep checking queue until D2XX has received all the bytes we wrote
		printf("D2XX receive-queue has ");
		bytesReceived = 0;
		for (queueChecks = 0; queueChecks < 64000; queueChecks++)
		{
			if (queueChecks % 128 == 0)
			{
				// Periodically display number of bytes D2XX has received
				printf("%s%d", 
				       queueChecks == 0 ? " " : ", ",
					   (int)bytesReceived);
			}

			ftStatus = FT_GetQueueStatus(ftHandle, &bytesReceived);
			if (ftStatus != FT_OK)
			{
				printf("\nFailure.  FT_GetQueueStatus returned %d\n",
				       (int)ftStatus);
				goto exit;
			}
		    
		    if (bytesReceived >= bytesWritten)
				break;
		}

		printf("\nGot %d (of %d) bytes\n", (int)bytesReceived, (int)bytesWritten);

		// Even if D2XX has the wrong number of bytes, create our
		// own buffer so we can read and display them.
		free(readBuffer); // Free previous iteration's buffer.
		readBuffer = calloc(bytesReceived, sizeof(unsigned char));
		if (readBuffer == NULL)
		{
			printf("Failed to allocate %d bytes\n", bytesReceived);
			goto exit;
		}

		// Then copy D2XX's buffer to ours.
		ftStatus = FT_Read(ftHandle, readBuffer, bytesReceived, &bytesRead);
		if (ftStatus != FT_OK)
		{
			printf("Failure.  FT_Read returned %d\n", (int)ftStatus);
			goto exit;
		}

		if (bytesRead != bytesReceived)
		{
			printf("Failure.  FT_Read only read %d (of %d) bytes\n",
			       (int)bytesRead,
			       (int)bytesReceived);
			goto exit;
		}
		
		if (0 != memcmp(writeBuffer, readBuffer, bytesRead))
		{
			printf("Failure.  Read-buffer does not match write-buffer\n");
			dumpBuffer(readBuffer, bytesReceived);
			break;
		}

		// Fail if D2XX's queue lacked (or had surplus) bytes.
		if (bytesReceived != bytesWritten)
		{
			printf("Failure.  D2XX received %d bytes but we expected %d\n",
				   (int)bytesReceived,
				   (int)bytesWritten);
			dumpBuffer(readBuffer, bytesReceived);
			goto exit;
		}

		// Check that queue hasn't gathered any additional unexpected bytes
		bytesReceived = 4242; // deliberately junk
		ftStatus = FT_GetQueueStatus(ftHandle, &bytesReceived);
		if (ftStatus != FT_OK)
		{
			printf("Failure.  FT_GetQueueStatus returned %d\n",
			       (int)ftStatus);
			goto exit;
		}

		if (bytesReceived != 0)
		{
			printf("Failure.  %d bytes in input queue -- expected none\n",
				   (int)bytesReceived);
			goto exit;
		}
	}

	// Success
	printf("\nTest PASSED.\n");
	retCode = 0;

exit:
	free(readBuffer);

	if (ftHandle != NULL)
		FT_Close(ftHandle);

	return retCode;
}
