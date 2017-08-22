/* ########################################################################

   FileWAV - Class to allow Muse/2 to output a module to a WAV file

   Write RAW mix data to a WAV file.

   ########################################################################
*/
#include <muse.h>
#include <thread.h>
#include <wav.h>

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

museFileWAVClass *museFileWAV::Meta = new museFileWAVClass;
#ifdef __unix__
# define O_BINARY 0
#endif

/* ########################################################################

   Class - museFileWAV (WAV File Interface for the digital mixer)
   Member - Constructor (Init the class)
   IDL - somDefaultInit

   Initilize variables to non play states.

   ########################################################################
*/
museFileWAV::museFileWAV()
{
   SamplingRate = 0;
   Bits = 16;
   Stereo = true;
   Opened = false;
   FileName = 0;
   Handle = -1;
   BufferSize = 128*1024;
   Buffer = new octet[BufferSize];
   Play = 0;
   Extension = ".wav";
}

museFileWAV::~museFileWAV()
{
   StopPlay();
   delete [] Buffer;
}

/* ########################################################################

   Class - museFileWAV (WAV File Interface for the digital mixer)
   Member - SetMixParams (Builds the playlist needed for tracking)

   Store playback type for initilization.

   ########################################################################
*/
long museFileWAV::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
{
   long Rc = museDACMixer::SetMixParams(SRate,Bits,Stereo);
   if (Rc != 0) return Rc;

   SamplingRate = SRate;
   this->Bits = Bits;
   this->Stereo = Stereo;
   return 0;
}

/* ########################################################################

   Class - museFileWAV (WAV File Interface for the digital mixer)
   Member - InitPlay (Initilize the WAV File interface)

   Open the file and write the header.

   ########################################################################
*/
long museFileWAV::InitPlay(char **Error)
{
   *Error = 0;

   if (Opened == true)
      StopPlay();

   if (SamplingRate == 0)
      SetMixParams(44100,16,true);

   if (FileName == 0)
   {
      const char *C = "module.wav";
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

   // Write the header
   unsigned long Size = 0;
   write(Handle,"RIFF",4);
   write(Handle,&Size,4);

   write(Handle,"WAVE",4);

   // Write the format header
   write(Handle,"fmt ",4);
   Size = 16;
   write(Handle,&Size,4);

   unsigned short Value = 1;
   write(Handle,&Value,2);

   Value = (Stereo == true?2:1);
   write(Handle,&Value,2);

   Size = SamplingRate;
   write(Handle,&Size,4);

   Size = (unsigned long)ceil((float)(Value*Bits*SamplingRate)*0.125);
   write(Handle,&Size,4);

   Value = (unsigned long)ceil((float)(Value*Bits)*0.125);
   write(Handle,&Value,2);

   Value = Bits;
   write(Handle,&Value,2);
   write(Handle,"data",4);
   if (write(Handle,&Size,4) != 4)
   {
      *Error = "Write Error";
      close(Handle);
      Opened = false;
      return 1;
   }

   return 0;
}

/* ########################################################################

   Class - museFileWAV (WAV File Interface for the digital mixer)
   Member - TestOutput (Play a sine waveform)

   Fills the buffer with a sine wave and plays it.

   ########################################################################
*/
long museFileWAV::TestOutput()
{
   return 0;
}

/* ########################################################################

   Class - museFileWAV (WAV File Interface for the digital mixer)
   Member - StopPlay (Stops the wav playback.)

   Closes the file.

   ########################################################################
*/
long museFileWAV::StopPlay()
{
   if (Opened == false)
      return 0;

   Opened = false;
   Play = 0;

   // Re-Write the header
   lseek(Handle,0,SEEK_SET);

   unsigned long Size = TotalSize + 8+16+12;
   write(Handle,"RIFF",4);
   write(Handle,&Size,4);

   write(Handle,"WAVE",4);

   // Write the format header
   write(Handle,"fmt ",4);
   Size = 16;
   write(Handle,&Size,4);

   unsigned short Value = 1;
   write(Handle,&Value,2);

   Value = (Stereo == true?2:1);
   write(Handle,&Value,2);

   Size = SamplingRate;
   write(Handle,&Size,4);

   Size = (unsigned long)ceil((float)(Value*Bits*SamplingRate)*0.125);
   write(Handle,&Size,4);

   Value = (unsigned long)ceil((float)(Value*Bits)*0.125);
   write(Handle,&Value,2);

   Value = Bits;
   write(Handle,&Value,2);
   write(Handle,"data",4);
   write(Handle,&TotalSize,4);
   close(Handle);
   Handle = -1;
   return 0;
}

/* ########################################################################

   Class - museFileWAV (Class to allow Muse/2 to use WAV File for output)
   Member - StopWhenDone (Fills the buffer with the last byte)
   IDL - void StopWhenDone()

   Used to allow the buffer to run out so the song can end on a buffer
   boundry.

   ########################################################################
*/
void museFileWAV::StopWhenDone()
{
   museDACMixer::StopWhenDone();

   if (Opened == false)
      return;

   write(Handle,Buffer,BufferSize);
   TotalSize += BufferSize;
   StopPlay();
   return;
}

/* ########################################################################

   Class - museFileWAV (WAV File Interface for the digital mixer)
   Member - PausePlay (Pause the wav playback.)

   Pauses writing.

   ########################################################################
*/
void museFileWAV::PausePlay()
{
   Play = 2;
}

/* ########################################################################

   Class - museFileWAV (WAV File Interface for the digital mixer)
   Member - ResumePlay (Resumes the wav playback.)

   Resumes writing.

   ########################################################################
*/
void museFileWAV::ResumePlay()
{
   Play = 1;
}

long museFileWAV::GetNextBuffer(unsigned char **Start,unsigned char **Stop)
{
   while (Play == 2)
      threadSleep(100);

   if (Play != 0)
   {
      long Size = write(Handle,Buffer,BufferSize);
      if (Size < 0)
         return 3;
      TotalSize += BufferSize;
      if (Size != (int)BufferSize)
         return 4;
   }
   Play = 1;
   *Start = Buffer;
   *Stop = Buffer + BufferSize;
   return 0;
}

/* ########################################################################

   Class - museFileWAVClass (Class of museFileWAV)
   Member - Constructor (Inits the class)

   Simply inits data members.

   ########################################################################
*/
museFileWAVClass::museFileWAVClass()
{
   Level = 0xFF;

   MajorVersion = 1;
   MinorVersion = 0;
}

/* ########################################################################

   Class - museFileWAVClass (Class of museFileWAV)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns a pointer to a string containing 'Scream Tracker 3 Module'.
   This string is valid while the object exists.

   ########################################################################
*/
const char *museFileWAVClass::GetTypeName()
{
   return ".WAV File Ouput";
}

/* ########################################################################

   Class - museFileWAVClass (Class of museDACMMPM2)
   Member - IsSupported (Returns the Support level of this class)
   IDL - octet IsSupported()

   Checks to see if DART is installed and a Digital Wave device is openable.

   ########################################################################
*/
octet museFileWAVClass::IsSupported()
{
   return 0xFF;
}
