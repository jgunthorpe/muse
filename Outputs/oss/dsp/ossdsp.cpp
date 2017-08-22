// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   OSSDSP - Driver for the OSS /dev/dsp* devices.

   Just a note on bitness, the OSS guide says 'signed short' should
   not be used to represent 16 bit audio data. Well I use it. :P In this
   entire program char = 8 bit, short = 16, long = 32, long long = 64 and 
   int = pointer size. This is followed religiosly. If ever a compiler
   comes around that uses short = ? then shoot the compiler author and 
   consider globally replacing short with a define that expands to the 
   new type that was made 16 bits. Last I checked their are 4 fundamental 
   C++ types, and I would be amazed if someone decided to eliminate one 
   of the bit sizes (hardware limitation?). In any event so much stuff 
   would break in this program if the bit sizes change.
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#include <muse.h>
#include <ossdsp.h>
#include <sys/ioctl.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/soundcard.h>       // OSS/Free header
#include <cmdline.h>
#include <errno.h>
   									/*}}}*/

/* Ik, 3.50 does not appear to have A_FMT_S16_NE. (Doc error?)
   I do not know when/if they added it, just that 3.50 doesn't have it. */
#if SOUND_VERSION <= 350
# define AFMT_S16_NE AFMT_S16_LE
#endif

museOSSDSPClass *museOSSDSP::Meta = new museOSSDSPClass;

// Play State Variables 						/*{{{*/
#define PLAY_BEGIN 0
#define PLAY_GO 1
#define PLAY_HALT 2
#define PLAY_PAUSE 3
   									/*}}}*/

// OSSDSP::museOSSDSP - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* Initializes the class. By default it will autodetect the sampling rate
   and bitrate. */
museOSSDSP::museOSSDSP()
{
   SamplingRate = 0;
   Bits = 0;
   Stereo = true;
   Opened = false;
   Buffer = 0;
   FD = -1;
   Play = PLAY_HALT;
}
									/*}}}*/
// OSSDSP::~museOSSDSP - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* Cleans up the sound system frees internal buffers etc */
museOSSDSP::~museOSSDSP()
{
   StopPlay();
   Close();  // Just in case
}
									/*}}}*/
// OSSDSP::SetMixParams - Configure the desired output parameters	/*{{{*/
// ---------------------------------------------------------------------
/* Specifing 0 for any parameters will have no effect. Specifing a 
   parameter will force the detector to use that parameter. */
long museOSSDSP::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
{
   long Rc = museDACMixer::SetMixParams(SRate,Bits,Stereo);
   if (Rc != 0) return Rc;

   if (Bits != 0)
      this->Bits = Bits;
   this->Stereo = Stereo;

   if (SRate != 0)
      SamplingRate = SRate;

   return 0;
}
									/*}}}*/
// OSSDSP::InitPlay - Initialize the playback routines.			/*{{{*/
// ---------------------------------------------------------------------
/* Perform sampling rate autodetect, and device/class setup. After this
   call GetNextBuffer can be called. Error is a pointer to a string in 
   which an error string is returned. It should not be freed. */
static char Error[300];  // We use a static buffer for the error code :<
long museOSSDSP::InitPlay(char **Error)
{
   if (museDACMixer::InitPlay(Error) != 0)
      return 1;
   
   *Error = 0;

   if (Opened == true)
      StopPlay();

   Opened = false;
   Play = PLAY_BEGIN;

   // Open the DSP device
   const char *Device = GetIniOptions()->GetArg("oss-dsp","/dev/dsp");
   if ((FD = open(Device,O_WRONLY,0)) == -1)
   {
      sprintf(::Error,"Can't open %s -- %s (%i)",Device,strerror(errno),errno);
      *Error = ::Error;
      return 1;
   }
   
   /* We use AFMT_S16_NE for all 16 bit data. Muse internally computes
      all the output so it will always be in the edianness of the 
      processor */
   int Format = AFMT_S16_NE;

   // Muse uses unsigned 8 bit samples in 8 bit mode.
   if (Bits == 8)
      Format = AFMT_U8;
   
   // Set the bit rate trying different values if permitted
   do
   {
      if (ioctl(FD,SNDCTL_DSP_SETFMT,&Format)==-1)
      {
	 sprintf(::Error,"%s: Setting bit rate -- %s (%i)",Device,strerror(errno),errno);
	 *Error = ::Error;
	 close(FD);
	 FD = -1;
	 return 1;
      }

      // It picked 16 bit.
      if (Format == AFMT_S16_NE && (Bits != 8))
      {
	 Bits = 16;
	 break;
      }

      // It picked 8 bit.
      if (Format == AFMT_U8 && (Bits != 16))
      {
	 Bits = 8;
	 break;
      }
   
      // User specified a bit rate, do not bother autodetecting
      if (Bits != 0)
      {
	 *Error = "Device rejected the bitrate specified";
	 return 1;
      }

      // We tried 16 bits, fall back to 8 bit.
      if ((Bits == 0) && (Format == AFMT_S16_NE))
      {
	 Format = AFMT_U8;
	 continue;
      }
      
      // Autodetect failed.
      if (Bits == 0)
      {
	 *Error = "Autodetect failed, 8 and 16 bits were rejected";
	 return 1;
      }      
   }
   while (Bits == 0);
   
   int Chans = (Stereo == true?1:0);     /* 0=mono, 1=stereo */
   if (ioctl(FD,SNDCTL_DSP_STEREO,&Chans) == -1)
   {
      sprintf(::Error,"%s: Setting channels -- %s (%i)",
	      Device,strerror(errno),errno);
      *Error = ::Error;
      close(FD);
      FD = -1;
      return 1;
   }

   // Hmm.. Should we go into mono mode?
   if (Chans == 0 && Stereo == true)
   {
      *Error = "Device is not capable of stereo";
      close(FD);
      FD = -1;
      return 1;
   }

   TraceC(TRACE_INFO);
   DebugMessage("SRate = %lu %lu\n",SRate,SamplingRate);
   
   // Try 2GHz, the device will return the actual max.
   if (SamplingRate == 0)
   {
      int Rate = 2000000;
      
      // Try to set the speed.
      if (ioctl(FD,SNDCTL_DSP_SPEED,&Rate) == -1)
      {
	 sprintf(::Error,"%s: Setting speed -- %s (%i)",
		 Device,strerror(errno),errno);
	 *Error = ::Error;
	 close(FD);
	 FD = -1;
	 return 1;
      }

      SamplingRate = Rate;
   }
   else
   {
      int Rate = SRate;
      // Try to set the speed.
      if (ioctl(FD,SNDCTL_DSP_SPEED,&Rate) == -1)
      {
	 sprintf(::Error,"%s: Setting speed -- %s (%i)",
		 Device,strerror(errno),errno);
	 *Error = ::Error;
	 close(FD);
	 FD = -1;
	 return 1;
      }

      // Check for a wide varaince
      if (abs(Rate - SRate) > 5000)
      {
	 *Error = "Device is not capable of the requested sampling rate";
	 close(FD);
         FD = -1;
	 return 1;
      }

      // Use the actual returned sampling rate.
      SamplingRate = Rate;
   }

   // The device is fully configured now.
   // Let the Digital Mixer know of the new state
   SetMixParams(SamplingRate,Bits,Stereo);
   
   // Setup the buffer.
   // Try to set the speed.
   BlockSize = 0;
   if (ioctl(FD,SNDCTL_DSP_GETBLKSIZE,&BlockSize) == -1)
   {
      sprintf(::Error,"%s: Getting frag size -- %s (%i)",
	      Device,strerror(errno),errno);
      *Error = ::Error;
      close(FD);
      FD = -1;
      return 1;
   }

   // Use 4k if the driver barfed.
   if (BlockSize == 0)
      BlockSize = 4096;
   
   // Allocate our internal buffers.
   Blocks = (int)ceil(SamplingRate*Bits/8*(Stereo == true?2:1)/BlockSize);
   delete [] Buffer;
   ReadPos = 0;
   WritePos = 0;
   BufSize = BlockSize*Blocks;
   Buffer = new octet[BufSize];
   Opened = true;
  
   TraceC(TRACE_INFO);
   DebugMessage("Done: Decided on %i bs, %u bits, %lu speed, "
                "%u chans. Allocated %lu\n",BlockSize,Bits,SRate,
		Stereo,BufSize);
   return 0;
}
									/*}}}*/
// OSSDSP::Close - Closes the device.       				/*{{{*/
// ---------------------------------------------------------------------
/* This safely closes the device. We aquire the mutex to prevent things
   from going nutz when we discard the buffer. */
void museOSSDSP::Close()
{
   Opened = false;

   close(FD);
   FD = -1;
   Play = PLAY_HALT;
   delete [] Buffer;
   Buffer = 0;
   BlockSize = 0;
}
									/*}}}*/
// OSSDSP::StopPlay - Halts playback instantly				/*{{{*/
// ---------------------------------------------------------------------
/* This ceases all playback instantly. */
long museOSSDSP::StopPlay()
{
   if (Opened == false)
      return 0;

   // Request sync with the device. 
   Play = PLAY_HALT;
   if (ioctl(FD,SNDCTL_DSP_RESET,0) == -1)
      ErrNo("IOCTL SNDCTL_DSP_RESET");
   
   Close();
   return 0;
}
									/*}}}*/
// OSSDSP::StopWhenDone - Stops once the current data has been dequeued	/*{{{*/
// ---------------------------------------------------------------------
/* This will stop playback once all the currently buffered data has been
   played. The parent is called to fill the last buffer with silence data. 
   Stop implies that the device is closed and that init must be called. */
void museOSSDSP::StopWhenDone()
{
   if (Opened == false)
      return;

   // Fill the final buffer, GetNextBuffer is NOT called.
   museDACMixer::StopWhenDone();

   // This commits the current buffer without getting a new one.
   GetNextBuffer(0,0);
		 
   // Request sync with the device.
   Play = PLAY_HALT;
   if (ioctl(FD,SNDCTL_DSP_SYNC,0) == -1)
      ErrNo("IOCTL SNDCTL_DSP_SYNC");

   Close();
}
									/*}}}*/
// OSSDSP::PausePlay - Performs a hardware level pause			/*{{{*/
// ---------------------------------------------------------------------
/* OSS does not support pause. We fake it by telling the driver that
   there is about to be an interruption and then ceasing to send data
   to it. */
void museOSSDSP::PausePlay()
{
   if (ioctl(FD,SNDCTL_DSP_POST,0) == -1)
   {
      ErrNo("IOCTL SNDCTL_DSP_POST");
      return;
   }
   Play = PLAY_PAUSE; 
}
									/*}}}*/
// OSSDSP::ResumePlay - Undoes PausePlay				/*{{{*/
// ---------------------------------------------------------------------
/* */
void museOSSDSP::ResumePlay()
{
   Play = PLAY_GO;
}
									/*}}}*/
// OSSDSP::GetNextBuffer - Returns a empty block of data		/*{{{*/
// ---------------------------------------------------------------------
/* Calling this function implies that the last buffer used has been fully
   written and should be played. GetNextBuffer should block if it cannot
   allocate a buffer. The player constantly calls this function to 
   write data to the sound device. */
long museOSSDSP::GetNextBuffer(unsigned char **Start,unsigned char **Stop)
{
   // Abort
   if (Play == PLAY_HALT || Opened == false || FD == -1)
      return 1;

   // Pause the output stream
   while (Play == PLAY_PAUSE)
      threadSleep(500);

   // We preload our internal buffer with data before even starting.
   if (Play == PLAY_BEGIN)
   {
      *Start = Buffer;
      *Stop = Buffer + BufSize;
      Play = PLAY_GO;
      WritePos = BufSize;
      ReadPos = 0;
      return 0;
   }

   // This loop is used when our buffers are filled and osses are too.   
   while (1)
   {
      // Write as many blocks as possible.
      audio_buf_info State;
      if (ioctl(FD,SNDCTL_DSP_GETOSPACE,&State) == -1)
      {
	 ErrNo("IOCTL SNDCTL_DSP_GETOSPACE");
	 return 9;
      }
      
      /* Compute the amount of space that is between the write point and
       read point */
      int Space = min(WritePos - ReadPos,State.bytes);
      while (Space != 0)
      {
	 // Compute the next bit to write
	 int BPos = ReadPos%BufSize;
	 
	 // Write it, check for error and then use the actual size to incr
	 int Res = write(FD,Buffer+BPos,min(BufSize-BPos,Space));
	 if (Res == -1)
	 {
	    Error("Write Failed");
	    return 9;
	 }
	 
	 // Update the counters
	 Space -= Res;
	 ReadPos += Res;
      }
      
      // Compute the new output buffer positions.
      if (Start != 0 && Stop != 0)
      {
	 *Start = Buffer + (WritePos%BufSize);
	 *Stop = *Start + min(BufSize - (WritePos - ReadPos),BufSize - (WritePos%BufSize));
	 WritePos += *Stop - *Start;
	 
	 if (*Stop != *Start)
	    break;
      }
      else
	 if (ReadPos == WritePos)
	    break;
      
      // Compute the next bit to write
      int BPos = ReadPos%BufSize;
      
      // Write it, check for error and then use the actual size to incr
      int Res = write(FD,Buffer+BPos,BlockSize);
      if (Res == -1)
      {
	 Error("Write Failed");
	 return 9;
      }

      // Update the counters
      ReadPos += Res;
   }

   return 0;
}
									/*}}}*/
// OSSDSP::Sync - Syncronize with the output.				/*{{{*/
// ---------------------------------------------------------------------
/* This call will wait for the specified time and then return the actual
   time the highest possible precision. */
unsigned long museOSSDSP::Sync(unsigned long Time)
{
   if (Opened == false || Play == PLAY_HALT)
      return 0;

   while (1)
   {
      count_info info;
      if (ioctl(FD,SNDCTL_DSP_GETOPTR,&info) == -1)
      {
	 ErrNo("IOCTL SNDCTL_DSP_GETOPTR");
	 return 0;
      }

      // Determine the number of samples we are at. 
      unsigned long Now = info.bytes/(Bits/8*(Stereo == true?2:1));
      if (Now > Time)
	 return Now;

      // Sleep for the approx time needed and repeat.
      threadSleep((Time - Now)*1000/SRate);      
   }
   
   return 0;
}
									/*}}}*/

// OSSDSPClass::museOSSDSPClass - Constructor for the MetaClass		/*{{{*/
// ---------------------------------------------------------------------
/* Sets the output preference level and the classes version. */
museOSSDSPClass::museOSSDSPClass()
{
   Level = 1;

   // Set versioning and link the class to the handler array
   MajorVersion = 1;
   MinorVersion = 0;
   
   // Add Our command line params
   GetIniOptions()->AddOption("oss-dsp",0,0,false,0,"/dev/dsp");
}
									/*}}}*/
// OSSDSPClass::GetTypeName - Returns a string describing this device	/*{{{*/
// ---------------------------------------------------------------------
/* */
const char *museOSSDSPClass::GetTypeName()
{
   return "OSS-DSP";
}
									/*}}}*/
// OSSDSPClass::IsSupported - Return the ranking of the device		/*{{{*/
// ---------------------------------------------------------------------
/* Check to see if the device is present in the system.
   Returns a # indicating it's relative ranking in the device tree, GUS
   is considered a 5, 0 is valid. (Set Level to the # that will be
   returned by this function) */
octet museOSSDSPClass::IsSupported()
{
   int FD = open(GetIniOptions()->GetArg("oss-dsp","/dev/dsp"),O_WRONLY);
   if (FD == -1)
      return 0;
   else
      close(FD);
   return Level;
}
									/*}}}*/
