/* ########################################################################

   FileRAW - Class to allow Muse/2 to output a module to a RAW file

   Write RAW mix data to a RAW file.

   ########################################################################
*/
#include <muse.h>
#include <raw.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

museFileRAWClass *museFileRAW::Meta = new museFileRAWClass;

#ifdef __unix__
# define O_BINARY 0
#endif

/* ########################################################################

   Class - museFileRAW (RAW File Interface for the digital mixer)
   Member - Constructor (Init the class)
   IDL - somDefaultInit

   Initilize variables to non play states.

   ########################################################################
*/
museFileRAW::museFileRAW()
{
   SamplingRate = 0;
   Bits = 16;
   Stereo = true;
   Opened = false;
   FileName = 0;
   Handle = -1;
   BufferSize = 63*1024;
   Buffer = new octet[BufferSize];
   Play = 0;
   Extension = ".RAW";
}

museFileRAW::~museFileRAW()
{
   StopPlay();
   delete [] Buffer;
}

/* ########################################################################

   Class - museFileRAW (RAW File Interface for the digital mixer)
   Member - SetMixParams (Builds the playlist needed for tracking)

   Store playback type for initilization.

   ########################################################################
*/
long museFileRAW::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
{
   long Rc = museDACMixer::SetMixParams(SRate,Bits,Stereo);
   if (Rc != 0) return Rc;

   SamplingRate = SRate;
   this->Bits = Bits;
   this->Stereo = Stereo;
   return 0;
}

/* ########################################################################

   Class - museFileRAW (RAW File Interface for the digital mixer)
   Member - InitPlay (Initilize the RAW File interface)

   Open the file and write the header.

   ########################################################################
*/
long museFileRAW::InitPlay(char **Error)
{
   *Error = 0;

   if (Opened == true)
      StopPlay();

   if (SamplingRate == 0)
      SetMixParams(44100,16,true);

   if (FileName == 0)
   {
      const char *C = "module.raw";
      FileName = new char [strlen(C) + 1];
      strcpy(FileName,C);
   }

   Handle = open(FileName,O_WRONLY | O_BINARY | O_CREAT | O_TRUNC,S_IREAD | S_IWRITE);
   if (Handle == -1)
   {
      *Error = "File Open Error";
      return 1;
   }

   TotalSize = 0;
   Opened = true;
   RealTime = false;
   Play = 0;

   return 0;
}

/* ########################################################################

   Class - museFileRAW (RAW File Interface for the digital mixer)
   Member - TestOutput (Play a sine RAWeform)

   Fills the buffer with a sine RAWe and plays it.

   ########################################################################
*/
long museFileRAW::TestOutput()
{
   return 0;
}

/* ########################################################################

   Class - museFileRAW (RAW File Interface for the digital mixer)
   Member - StopPlay (Stops the RAW playback.)

   Closes the file.

   ########################################################################
*/
long museFileRAW::StopPlay()
{
   if (Opened == false)
      return 0;

   Opened = false;
   Play = 0;

   close(Handle);
   Handle = -1;
   return 0;
}

/* ########################################################################

   Class - museFileRAW (Class to allow Muse/2 to use RAW File for output)
   Member - StopWhenDone (Fills the buffer with the last byte)
   IDL - void StopWhenDone()

   Used to allow the buffer to run out so the song can end on a buffer
   boundry.

   ########################################################################
*/
void museFileRAW::StopWhenDone()
{
   museDACMixer::StopWhenDone();

   if (Opened == false)
      return;

   write(Handle,Buffer,BufferSize);
   StopPlay();
   return;
}

/* ########################################################################

   Class - museFileRAW (RAW File Interface for the digital mixer)
   Member - PausePlay (Pause the RAW playback.)

   Pauses writing.

   ########################################################################
*/
void museFileRAW::PausePlay()
{
   Play = 2;
}

/* ########################################################################

   Class - museFileRAW (RAW File Interface for the digital mixer)
   Member - ResumePlay (Resumes the RAW playback.)

   Resumes writing.

   ########################################################################
*/
void museFileRAW::ResumePlay()
{
   Play = 1;
}

long museFileRAW::GetNextBuffer(unsigned char **Start,unsigned char **Stop)
{
   while (Play == 2)
      threadSleep(100);

   if (Play != 0)
   {
      if (write(Handle,Buffer,BufferSize) != (int)BufferSize)
         return 3;
      TotalSize += BufferSize;
   }
   Play = 1;
   *Start = Buffer;
   *Stop = Buffer + BufferSize;
   return 0;
}

/* ########################################################################

   Class - museFileRAWClass (Class of museFileRAW)
   Member - Constructor (Inits the class)

   Simply inits data members.

   ########################################################################
*/
museFileRAWClass::museFileRAWClass()
{
   Level = 0xFF;

   MajorVersion = 1;
   MinorVersion = 0;
}

/* ########################################################################

   Class - museFileRAWClass (Class of museFileRAW)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns a pointer to a string containing 'Scream Tracker 3 Module'.
   This string is valid while the object exists.

   ########################################################################
*/
const char *museFileRAWClass::GetTypeName()
{
   return ".RAW File Ouput";
}

/* ########################################################################

   Class - museFileRAWClass (Class of museDACMMPM2)
   Member - IsSupported (Returns the Support level of this class)
   IDL - octet IsSupported()

   Checks to see if DART is installed and a Digital RAWe device is openable.

   ########################################################################
*/
octet museFileRAWClass::IsSupported()
{
   return 0xFF;
}
