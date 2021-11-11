//******************************************************************************
//
// Copyright (c) 2020 Advantech Industrial Automation Group.
//
// Exar with Advantech RS232/422/485 capacities
// 
// This program is free software; you can redistribute it and/or modify it 
// under the terms of the GNU General Public License as published by the Free 
// Software Foundation; either version 2 of the License, or (at your option) 
// any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT 
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for 
// more details.
// 
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 
// Temple Place - Suite 330, Boston, MA  02111-1307, USA.
// 
//
//
//******************************************************************************

//***********************************************************************
// File:        getconfig.c
// Version:     1.01.1
// Author:      Po-Cheng Chen
// Purpose:	Get serial port configuration, such as RS232 or RS422/485
//***********************************************************************
//***********************************************************************
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/ioctls.h>
#include <linux/serial.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define 	ADVANTECH_EXAR_MAIGC            'x'
#define 	ADV_GET_SERIAL_MODE				_IO(ADVANTECH_EXAR_MAIGC, 3)

#define SERIAL_MODE_RS232           1
#define SERIAL_MODE_RS422M          2
#define SERIAL_MODE_RS485ORRS422S   3

#define MaxBoardID                  15

int  GetExarMode(char *ttyname)
{
	int i,k, ttynum, fd;
	//char ttyname[80];
	struct serial_struct serinfo;

	if((fd = open(ttyname, O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK)) == -1)
	{
		//printf("open error,ttyName:%s does not exit.\n",ttyname);
		return -1;
	}

	// test serial port exist or not
	if(write(fd, ttyname, strlen(ttyname)) == -1)
	{
		printf("write to %s failed!!!\n",ttyname);
		return -1;
	}

	// get serial port information
	int runMode;
	int bResult;
	bResult = ioctl(fd, ADV_GET_SERIAL_MODE, (void *)&runMode);

	// get serial port type configuration
	if(runMode == SERIAL_MODE_RS232)
	{
		// RS232
		printf("%s is RS232\n", ttyname);
	}
	else if(runMode == SERIAL_MODE_RS422M)
	{
		//RS422M
		printf("%s is RS422M\n", ttyname);
	}
	else if(runMode == SERIAL_MODE_RS485ORRS422S)
	{
		//RS485ORRS422S
		printf("%s is RS422S/RS485\n", ttyname);
	}
	else
	{
		printf("%s is unknown type!!!\n",ttyname);
	}
	close(fd);

	return 0;
}

int main(int argc, char **argv)
{
// 	int i,k, ttynum, fd;
 	char ttyname[80];
// 	struct serial_struct serinfo;

	if(argc < 3)
	{
		printf("Usage:\n");
		printf("------argc = %d",argc);
		printf("%s TTYNAME TTYNUM", argv[0]);
		return 1;
	}

	int ttynum = atoi(argv[2]);

	if (strcmp(argv[1],"ttyA") == 0)///dev/ttyAPi
	{
		for(int i=0;i<ttynum;i++)
		{	
			sprintf(ttyname, "/dev/%sP%d", argv[1],i);//  /dev/ttyAPi
			GetExarMode(ttyname);
		}
	}
	else///dev/ttyBxxPi
	{
		for(int k = 0;k <= MaxBoardID;++k)
		{
			for(int i=0;i<ttynum;i++)
			{
				sprintf(ttyname, "/dev/%s%02dP%d", argv[1],k, i);//  /dev/ttyBxxPi
				GetExarMode(ttyname);
			}
		}
	}

	return 0;
}
