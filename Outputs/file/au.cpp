/* ########################################################################

   FileAU - Class to allow Muse/2 to output a module to a AU file

   Write RAW mix data to a AU file.

   ########################################################################
*/
#include <muse.h>
#include <au.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

museFileAUClass *museFileAU::Meta = new museFileAUClass;

#ifdef __unix__
# define O_BINARY 0
#endif

/* ########################################################################

   Class - museFileAU (AU File Interface for the digital mixer)
   Member - Constructor (Init the class)
   IDL - somDefaultInit

   Initilize variables to non play states.

   ########################################################################
*/
museFileAU::museFileAU()
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
   Extension = ".AU";
}

museFileAU::~museFileAU()
{
   StopPlay();
   delete [] Buffer;
}

/* ########################################################################

   Class - museFileAU (AU File Interface for the digital mixer)
   Member - SetMixParams (Builds the playlist needed for tracking)

   Store playback type for initilization.

   ########################################################################
*/
long museFileAU::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
{
   long Rc = museDACMixer::SetMixParams(SRate,Bits,Stereo);
   if (Rc != 0) return Rc;

   SamplingRate = SRate;
   this->Bits = Bits;
   this->Stereo = Stereo;
   return 0;
}

/* ########################################################################

   Class - museFileAU (AU File Interface for the digital mixer)
   Member - InitPlay (Initilize the AU File interface)

   Open the file and write the header.

   ########################################################################
*/
long museFileAU::InitPlay(char **Error)
{
   *Error = 0;

   if (Opened == true)
      StopPlay();

   if (SamplingRate == 0)
      SetMixParams(8012,16,false);

   if (FileName == 0)
   {
      const char *C = "module.AU";
      FileName = new char[strlen(C)+1];
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
   const char *Comment = "Created by Muse/2";
   unsigned long Size = 24 + strlen(Comment) + 1;
   write(Handle,".snd",4);
   write(Handle,&Size,4);
   write(Handle,&Size,4);        // Data Size

   Size = 1; // ULAW encoding
   write(Handle,&Size,4);
   Size = 8012; // Smapling Rate
   write(Handle,&Size,4);
   Size = 1; // Channels
   write(Handle,&Size,4);

   if (write(Handle,Comment,strlen(Comment) + 1) != (int)(strlen(Comment) + 1))
   {
      *Error = "Write Error";
      close(Handle);
      Handle = -1;
      Opened = false;
      return 1;
   }

   return 0;
}

/* ########################################################################

   Class - museFileAU (AU File Interface for the digital mixer)
   Member - TestOutput (Play a sine AUeform)

   Fills the buffer with a sine AUe and plays it.

   ########################################################################
*/
long museFileAU::TestOutput()
{
   return 0;
}

/* ########################################################################

   Class - museFileAU (AU File Interface for the digital mixer)
   Member - StopPlay (Stops the AU playback.)

   Closes the file.

   ########################################################################
*/
static unsigned long Res;
unsigned long *ByteFlip(unsigned long Byte)
{
   unsigned char A = Byte & 0x000000FF;
   unsigned char B = (Byte & 0x0000FF00) >> 8;
   unsigned char C = (Byte & 0x00FF0000) >> 16;
   unsigned char D = (Byte & 0xFF000000) >> 24;
   Res = (A << 24) + (B << 16) + (C << 8) + D;
   return &Res;
}
long museFileAU::StopPlay()
{
   if (Opened == false)
      return 0;

   Opened = false;
   Play = 0;

   // Re-Write the header
   lseek(Handle,0,SEEK_SET);

   // Write the header
   const char *Comment = "Created by Muse/2";
   write(Handle,".snd",4);
   write(Handle,ByteFlip(24 + strlen(Comment) + 1),4); // Header Len
   write(Handle,ByteFlip(TotalSize),4);     // Data Size
   write(Handle,ByteFlip(1),4);     // ULAW Encode
   write(Handle,ByteFlip(8012),4);  // Sampling Rate
   write(Handle,ByteFlip(1),4);     // Channels

   close(Handle);
   Handle = -1;
   return 0;
}

/* ########################################################################

   Class - museFileAU (Class to allow Muse/2 to use AU File for output)
   Member - StopWhenDone (Fills the buffer with the last byte)
   IDL - void StopWhenDone()

   Used to allow the buffer to run out so the song can end on a buffer
   boundry.

   ########################################################################
*/
void museFileAU::StopWhenDone()
{
   museDACMixer::StopWhenDone();

   if (Opened == false)
      return;

   octet *A;
   GetNextBuffer(&A,&A);
   StopPlay();
   return;
}

/* ########################################################################

   Class - museFileAU (AU File Interface for the digital mixer)
   Member - PausePlay (Pause the AU playback.)

   Pauses writing.

   ########################################################################
*/
void museFileAU::PausePlay()
{
   Play = 2;
}

/* ########################################################################

   Class - museFileAU (AU File Interface for the digital mixer)
   Member - ResumePlay (Resumes the AU playback.)

   Resumes writing.

   ########################################################################
*/
void museFileAU::ResumePlay()
{
   Play = 1;
}

// Taken from libst.c

/*
** This routine converts from linear to ulaw.
**
** Craig Reese: IDA/Supercomputing Research Center
** Joe Campbell: Department of Defense
** 29 September 1989
**
** References:
** 1) CCITT Recommendation G.711  (very difficult to follow)
** 2) "A New Digital Technique for Implementation of Any
**     Continuous PCM Companding Law," Villeret, Michel,
**     et al. 1973 IEEE Int. Conf. on Communications, Vol 1,
**     1973, pg. 11.12-11.17
** 3) MIL-STD-188-113,"Interoperability and Performance Standards
**     for Analog-to_Digital Conversion Techniques,"
**     17 February 1987
**
** Input: Signed 16 bit linear sample
** Output: 8 bit ulaw sample
*/

#define ZEROTRAP    /* turn on the trap as per the MIL-STD */
#define BIAS 0x84   /* define the add-in bias for 16 bit samples */
#define CLIP 32635

unsigned char st_linear_to_ulaw(short sample)
{
    static short exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                               4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
    short sign, exponent, mantissa;
    unsigned char ulawbyte;

    /* Get the sample into sign-magnitude. */
    sign = (sample >> 8) & 0x80;		/* set aside the sign */
    if ( sign != 0 ) sample = -sample;		/* get magnitude */
    if ( sample > CLIP ) sample = CLIP;		/* clip the magnitude */

    /* Convert from 16 bit linear to ulaw. */
    sample = sample + BIAS;
    exponent = exp_lut[( sample >> 7 ) & 0xFF];
    mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F;
    ulawbyte = ~ ( sign | ( exponent << 4 ) | mantissa );
#ifdef ZEROTRAP
    if ( ulawbyte == 0 ) ulawbyte = 0x02;	/* optional CCITT trap */
#endif

    return ulawbyte;
}

long museFileAU::GetNextBuffer(unsigned char **Start,unsigned char **Stop)
{
   while (Play == 2)
      threadSleep(100);

   if (Play != 0)
   {
      octet *S = Buffer;
      octet *E = Buffer + BufferSize;
      for (octet *C = Buffer; S != E; S += 2,C++)
         *C = st_linear_to_ulaw(*((signed short *)S));

      if (write(Handle,Buffer,BufferSize/2) != (int)(BufferSize/2))
         return 3;
      TotalSize += BufferSize/2;
   }
   Play = 1;
   *Start = Buffer;
   *Stop = Buffer + BufferSize;
   return 0;
}

/* ########################################################################

   Class - museFileAUClass (Class of museFileAU)
   Member - Constructor (Inits the class)

   Simply inits data members.

   ########################################################################
*/
museFileAUClass::museFileAUClass()
{
   Level = 0xFF;

   MajorVersion = 1;
   MinorVersion = 0;
}

/* ########################################################################

   Class - museFileAUClass (Class of museFileAU)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns a pointer to a string containing 'Scream Tracker 3 Module'.
   This string is valid while the object exists.

   ########################################################################
*/
const char *museFileAUClass::GetTypeName()
{
   return ".AU File Ouput";
}

/* ########################################################################

   Class - museFileAUClass (Class of museDACMMPM2)
   Member - IsSupported (Returns the Support level of this class)
   IDL - octet IsSupported()

   Checks to see if DART is installed and a Digital AUe device is openable.

   ########################################################################
*/
octet museFileAUClass::IsSupported()
{
   return 0xFF;
}
