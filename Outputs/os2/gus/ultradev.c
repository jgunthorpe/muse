#define INCL_DOSDEVICES
#include <os2.h>
#include <string.h>
#include "struct.h"
#include <math.h>

// IOCTL commands
#define GUS_COMMANDS			0x0F
#define UltraDevSetNVoices		0x50
#define UltraDevEnableOutput		0x51
#define UltraDevDisableOutput		0x52
#define UltraDevPokeData		0x53
#define UltraDevPeekData		0x54
#define UltraDevMemAlloc		0x55
#define UltraDevMemFree			0x56
#define UltraDevMemInit			0x57
#define UltraDevStartTimer		0x58
#define UltraDevStopTimer		0x59
#define UltraDevBlockTimerHandler1	0x5A
#define UltraDevBlockTimerHandler2	0x5B
#define UltraDevStartVoice		0x5C
#define UltraDevStopVoice		0x5D
#define UltraDevSetBalance		0x5E
#define UltraDevSetFrequency		0x5F
#define UltraDevVectorLinearVolume	0x60
#define UltraDevPrepare4DMAXfer		0x61
#define UltraDevUnblockAll		0x62
#define UltraDevSetAll			0x63
#define UltraDevGetAccess   	        0x6A
#define UltraDevReleaseAccess  	        0x6B
#define UltraDevSizeDRAM		0x6D
#define UltraDevGetDriverVersion	0x6E

#define UltraDevMODMemAlloc  		0x70
#define UltraDevMODMemFree		0x71
#define UltraDevMODBigMemAlloc		0x72
#define UltraDevSetLoopMode		0x73
#define UltraDevVoiceStopped		0x74

extern HFILE GUSHandle;

#define _min(A,B) ((A) < (B)?(A):(B))
#define _max(A,B) ((A) > (B)?(A):(B))

/******************************************************************************/
/******************************************************************************/
APIRET UltraSetNVoices(numvoices)
int numvoices;
{
ULONG ParmLength=0, DataLength=4;

return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevSetNVoices, NULL, 0, &ParmLength,
		 &numvoices, 4, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraGetAccess()
{
ULONG DataLength = 0, ParmLength = 0;

return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevGetAccess, NULL, 0,
		 &ParmLength, NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraReleaseAccess()
{
ULONG DataLength = 0, ParmLength = 0;

return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevReleaseAccess, NULL, 0,
		 &ParmLength, NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraEnableOutput()
{
ULONG DataLength = 0, ParmLength = 0;

return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevEnableOutput, NULL, 0,
		 &ParmLength, NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraDisableOutput()
{
ULONG DataLength = 0, ParmLength = 0;

return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevDisableOutput, NULL, 0,
		 &ParmLength, NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraBlockTimerHandler1()
{
ULONG ParmLength = 0, DataLength = 0;
// block until timer interrupt
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevBlockTimerHandler1, NULL, 0, &ParmLength,
        	    NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraBlockTimerHandler2()
{
ULONG ParmLength = 0, DataLength = 0;
// block until timer interrupt
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevBlockTimerHandler2, NULL, 0, &ParmLength,
         	    NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraStartTimer(timer, time)
int timer;
int time;
{
TimerStruct tbuffer;
ULONG ParmLength, DataLength;

ParmLength = 0;
DataLength = sizeof(TimerStruct);
tbuffer.timer = (short int)timer;
tbuffer.time = (unsigned char)time;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevStartTimer, NULL, 0, &ParmLength,
	    &tbuffer, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraStopTimer(timer)
int timer;
{
ULONG ParmLength, DataLength;

ParmLength = 0;
DataLength = 4;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevStopTimer, NULL, 0, &ParmLength,
		 &timer, 4, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraSetBalance(voice, data)
int voice;
int data;
{
ULONG ParmLength = 0, DataLength;
BalanceStruct bbuffer;

DataLength = sizeof(BalanceStruct);
bbuffer.voice = (short int)voice;
bbuffer.data = (unsigned char)data;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevSetBalance, NULL, 0,
	    &ParmLength, &bbuffer, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraSetFrequency(voice, freq)
int voice;
int freq;
{
ULONG ParmLength = 0, DataLength;
FreqStruct fbuffer;

DataLength = sizeof(FreqStruct);
fbuffer.voice = (short int)voice;
fbuffer.speed_khz = freq;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevSetFrequency, NULL, 0,
	    &ParmLength, &fbuffer, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltiMODMemFree()
{
ULONG ParmLength = 0, DataLength = 0;
//just free it all
  return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevMODMemFree, NULL,
	  	      0, &ParmLength, NULL, 0, &DataLength)) ;
}
/******************************************************************************/
/******************************************************************************/
APIRET UltiMODMemAlloc(size, location)
unsigned long size;
unsigned long *location;
{
ULONG ParmLength = 0, DataLength;
AllocStruct abuffer;
APIRET rc;

DataLength = sizeof(AllocStruct);
abuffer.size = size;
abuffer.location = *location;

if(size >= 256*1024)
//alloc more than 256 kb (xm) NO 16 BIT SAMPLES!
	rc = DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevMODBigMemAlloc, NULL,
			 0, &ParmLength, &abuffer, DataLength, &DataLength);
else	rc = DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevMODMemAlloc, NULL,
		         0, &ParmLength, &abuffer, DataLength, &DataLength);

*location = abuffer.location; /* location in DRAM GUS */
return(rc);
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraMemFree(int size, int location)
{
ULONG ParmLength = 0, DataLength = sizeof(AllocStruct);
AllocStruct abuffer;

 abuffer.size = size;
 abuffer.location = location;
 return(DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevMemFree, NULL,
	    0, &ParmLength, &abuffer, DataLength, &DataLength)) ;
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraMemAlloc(size, location)
unsigned long size;
unsigned long *location;
{
ULONG ParmLength = 0, DataLength;
AllocStruct abuffer;
APIRET rc;

DataLength = sizeof(AllocStruct);
abuffer.size = size;
abuffer.location = *location;

rc = DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevMemAlloc, NULL,
	       0, &ParmLength, &abuffer, DataLength, &DataLength);

*location = abuffer.location; /* location in DRAM GUS */
return(rc);
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraMemInit()
{
ULONG ParmLength = 0, DataLength = 0;

 return(DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevMemAlloc, NULL,
	            0, &ParmLength, NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
APIRET UltraDownload(dataptr, control, dram_loc, size, wait)
char *dataptr;
unsigned char control;
unsigned long dram_loc;
unsigned int size;
int wait;
{
ULONG ParmLength = 0, DataLength;
APIRET rc;
ULONG written;
XferStruct xbuffer;
char *Buffer64k;

 /* structure voor UltraPrepare4DMAXfer IOCtl call */
 DataLength = sizeof(XferStruct);
 xbuffer.control = control;	 //control byte (data width, signed/unsigned)
 xbuffer.dram_loc = dram_loc;

 // NEED to allocate a buffer to transfer samples > 64 kb !!!
 rc = DosAllocMem((PPVOID)(&Buffer64k), 64*1024, PAG_COMMIT | PAG_WRITE);
 if(rc) return(TRUE);

 while(TRUE) {
 	if(size > 64000) {//16 bit segments in a 32 bit world 8(
		memcpy(Buffer64k, dataptr, 64000);

	 	rc = DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevPrepare4DMAXfer, NULL,
			       0, &ParmLength, &xbuffer, DataLength, &DataLength);
		if(rc != 0) 	    goto download_err;

		rc = DosWrite(GUSHandle, Buffer64k, 64000, &written);
		if(rc != 0)	    goto download_err;

		dram_loc += 64000;
		xbuffer.dram_loc = dram_loc;
		xbuffer.control = control;
		dataptr += 64000;
		size -= 64000;
	 }
	 else 	break;
 }
 if(size > 0) {
 	 memcpy(Buffer64k, dataptr, size);
	 rc = DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevPrepare4DMAXfer, NULL,
	 	       0, &ParmLength, &xbuffer, DataLength, &DataLength);
	 if(rc != 0) 	    goto download_err;

	 rc =  DosWrite(GUSHandle, Buffer64k, size, &written); //last transfer
	 if(rc) goto download_err;
 }

 DosFreeMem(Buffer64k);
 return(0);

download_err:
 DosFreeMem(Buffer64k);
 return(rc);
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraPokeData(address, data)
long address;
int data;
{
ULONG ParmLength = 0, DataLength;
PokeStruct pbuffer;

pbuffer.address = address;
pbuffer.data = (unsigned char)data;
DataLength = sizeof(pbuffer);
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevPokeData, NULL, 0,
	    &ParmLength, &pbuffer, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraPeekData(int *address)
{
ULONG ParmLength = 0, DataLength;

DataLength = sizeof(int);
return(DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevPeekData, NULL, 0, &ParmLength,
                   address, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraUnblockAll()
{
ULONG ParmLength = 0, DataLength = 0;

return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevUnblockAll, NULL, 0, &ParmLength,
		  NULL, 0, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraStopVoice(int c)
{
ULONG ParmLength = 0, DataLength = 2;
short int voice;

voice = (short int)c;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevStopVoice, NULL, 0, &ParmLength,
		    &voice, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraVectorLinearVolume(voice, end_idx, rate, mode)
int voice;
unsigned int end_idx;		/* end location in ultra DRAM */
unsigned char rate;			/* 0 to 63 */
unsigned char mode;			/* mode to run the volume ramp in ... */
{
ULONG ParmLength, DataLength;
VolumeStruct vbuffer;

vbuffer.voice = (short int)voice;
vbuffer.end_idx = (short int)end_idx;
vbuffer.rate = rate;
vbuffer.mode = mode;
DataLength = sizeof(VolumeStruct);
ParmLength = 0;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevVectorLinearVolume,
		 NULL, 0, &ParmLength, &vbuffer, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraSetAll(voice, balance, freq, volume, rate, mode)
int voice;
char balance;
int freq, volume;
unsigned char rate;			/* 0 to 63 */
unsigned char mode;			/* mode to run the volume ramp in ... */
{
ULONG ParmLength = 0, DataLength;
AllStruct abuffer;

DataLength = sizeof(AllStruct);
abuffer.voice = (short int)voice;
abuffer.balance = (unsigned char)balance;
abuffer.freq = freq;
abuffer.volume = (unsigned short int)volume;
abuffer.rate = rate;
abuffer.mode = mode;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevSetAll, NULL, 0,
		    &ParmLength, &abuffer, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraStartVoice(gusvoice, begin, start, end, mode)
int gusvoice;		 /* voice to start */
unsigned int begin;	 /* start location in ultra DRAM */
unsigned int start;	 /* start loop location in ultra DRAM */
unsigned int end;	 /* end location in ultra DRAM */
unsigned char mode;	 /* mode to run the voice (loop etc) */
{
ULONG ParmLength = 0, DataLength;
VoiceStruct voice;

voice.voice = (short int)gusvoice;
voice.begin = begin; // start in DRAM
voice.start = start; //start loop
voice.end = end;
voice.mode = mode;
DataLength = sizeof(VoiceStruct);
return(	DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevStartVoice, NULL, 0,
		    &ParmLength, &voice, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraSizeDram(int *size)
{
ULONG ParmLength = 0, DataLength;

  DataLength = sizeof(int);
  return(DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevSizeDRAM, NULL, 0, &ParmLength,
                     size, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraGetDriverVersion(int *version)
{
ULONG ParmLength = 0, DataLength;
//High word contains major version, low word minor version

  DataLength = sizeof(int);
  return(DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevGetDriverVersion, NULL, 0, &ParmLength,
                     version, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraSetLoopMode(int voice, int mode)
{
ULONG ParmLength = 0, DataLength;
BalanceStruct bbuffer;

DataLength = sizeof(BalanceStruct);
bbuffer.voice = (short int)voice;
bbuffer.data = (unsigned char)mode;
return( DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevSetLoopMode, NULL, 0,
	    &ParmLength, &bbuffer, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraVoiceStopped(int *voice)
{
ULONG ParmLength = 0, DataLength;

  DataLength = sizeof(int);
  return(DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevVoiceStopped, NULL, 0, &ParmLength,
                     voice, DataLength, &DataLength));
}
/******************************************************************************/
/******************************************************************************/
APIRET UltraDownload2(dtaptr, control, dram_loc, size, wait)
void *dtaptr;
unsigned char control;
unsigned long dram_loc;
unsigned int size;
int wait;
{
   ULONG ParmLength = 0, DataLength;
   APIRET rc;
   ULONG written;
   XferStruct xbuffer;
   signed int ISize = size;
   char *dataptr = (char *)dtaptr;
   char *Buf;
   long BSize;

   DosAllocMem(&((void *)Buf), 30*1024, PAG_COMMIT | PAG_WRITE);

   /* structure voor UltraPrepare4DMAXfer IOCtl call */
   DataLength = sizeof(XferStruct);
   xbuffer.control = control;	 //control byte (data width, signed/unsigned)
   xbuffer.dram_loc = dram_loc;

   while (ISize > 0)
   {
  	   rc = DosDevIOCtl(GUSHandle, GUS_COMMANDS, UltraDevPrepare4DMAXfer, NULL,
     	                 0, &ParmLength, &xbuffer, DataLength, &DataLength);

      if (rc != 0)
      {
         DosFreeMem(Buf);
         return(rc);
      }

      BSize = _min(30*1024,ISize);

      // Woops, crossed a 256k boundry
      if (((unsigned long)(dram_loc)/(256*1024)) != (unsigned long)((BSize + dram_loc)/(256*1024)))
      {
         BSize = ceil(((float)dram_loc)/(256*1024.0))*(256*1024) - dram_loc;

         if (BSize == 0)
            BSize = _min(30*1024,size);
      }

      // Copy to temp buffer.
      memcpy(Buf,dataptr,BSize);
      rc = DosWrite(GUSHandle, Buf, BSize, &written);
      if (rc != 0)
      {
         DosFreeMem(Buf);
         return(rc);
      }

      dram_loc += BSize;
      xbuffer.dram_loc = dram_loc;
      dataptr += BSize;
      ISize -= BSize;
   }
   DosFreeMem(Buf);
   return 0;
}

