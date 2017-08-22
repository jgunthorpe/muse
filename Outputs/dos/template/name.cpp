/* ########################################################################

   DACNAME - Generic Template for a digital stream device

   Search and replace NAME with the device name in all file.

   Quikie Description of how the OS/2 DART Driver works,
     DART is pretty much as close as you can get to a DOS driver...

  Two Parts:
     EventProc -- Acts like an interrupt, is triggered by the device driver
     DART Class -- Interfaces with Muse

    Since OS/2 is multitasking the dart driver implements a variable size
    buffer. A minimume of 2 buffer is required, 3 is prefered and when
    runing at application priority 9 are used. Two counters are used to
    track the current playback block, 1 repesents the block that is being
    played, the other is the block that is being filled, when the two
    are equal the EventProc sends out a silence block (this doesn't work
    with less than 3 blocks). This allows the player to be interrupted
    without having that charateristic stuttering. For simplicity the two
    numbers are never wrapped but % (modulus) is used to find the buffer
    index. GetNextBuffer simply blocks on the Event Semephore (Proxy, Message
    etc). The nature of Os/2 sem's allows them to be triggered many times
    and the # of times they have been triggered is returned when they are
    blocked on. This tells GetNextBuffer how many buffers it can write.

    Hope that makes sense. Oh, and to make things work easially in /2, and
    I assume NT, Thread.h defines the following functions:

// Mutual Exclusion semaphore (Traditional Sense)
int DosReleaseMutexSem(long Sem);
int DosRequestMutexSem(long Sem,long TimeOut);
int DosCreateMutexSem(long,unsigned long *Sem,long,long);
int DosCloseMutexSem(long Sem);
   - Note DosCreateMutexSem's other parameters should be set to 0.
    Funcs that want to be protected look like:
    void Blah()
    {
      DosRequestMutexSem(Sem,SEM_INDEFINITE_WAIT);
      // Stuf
      DosReleaseMutexSem(Sem);
    }
// Sleeps for about time millisecons (Accuracy isn't too important)
int DosSleep(long Time);

// Signalling Semaphores (Proxies, Messages etc)
int DosCreateEventSem(long,unsigned long *Sem,long,long);
int DosCloseEventSem(long Sem);
int DosPostEventSem(long Sem);
int DosWaitEventSem(long Sem,long TimeOut);
int DosResetEventSem(long Sem,unsigned long *Count);
   Looks like this:
      void SomeThreadOrInt()
      {
         DosPosEventSem(Sem);
      }

      void SomePlaceElese()
      {
         unsigned long Count = 0;
         DosWaitEventSem();
         DosResetEventSem(Sem,&Count);
         // We are here, meaning SomeOtherThreadOrInt has executed Count times.
      }
   These two primitive provide sufficent control for the player, they
   are not particularly elagent, but are very simple. I think NT has
   nearly identical functions, A quick grep through the NT headers
   seems to confirm this..
   ########################################################################
*/
#include <IDLSeq.hc>
#include <NAME.hc>
#include <DebugUtl.hc>
#include <HandLst.hc>
#include <Thread.h>

museDACNAMEClass *museDACNAME::__ClassObject = new museDACNAMEClass;

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - Constructor (Init the class)
   IDL - somDefaultInit

   Initilize variables to non play states.

   ########################################################################
*/
museDACNAME::museDACNAME()
{
   DBU_FuncTrace("museDACNAME","museDACNAME",TRACE_SIMPLE);

   SamplingRate = 0;
   Bits = 16;
   Stereo = true;
   Opened = false;
   DetectSRate = true;
}

museDACNAME::~museDACNAME()
{
   StopPlay();
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - SetMixParams (Builds the playlist needed for tracking)

   A non 0 Sampling rate should choose the highest, a non 0 bit/stereo
   setting will force the detect routine to only choose a smapling rate.

   ########################################################################
*/
long museDACNAME::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
{
   DBU_FuncTrace("museDACNAME","SetMixParams",TRACE_SIMPLE);

   long Rc = museDACMixer::SetMixParams(SRate,Bits,Stereo);
   if (Rc != 0) return Rc;

   if (Bits != 0)
      this->Bits = Bits;
   this->Stereo = Stereo;

   if (SRate != 0)
   {
      SamplingRate = SRate;
      DetectSRate = false;
   }

   return 0;
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - InitPlay (Initilize the devcie)

   Perform sampling rate autodetect, and device/class setup. After this
   call GetNextBuffer can be called.

   ########################################################################
*/
long museDACNAME::InitPlay(string *Error)
{
   *Error = 0;

   if (Opened == true)
      StopPlay();

   Opened = false;
   Play = 0;

   // Majic Autodetect code
   if (DetectSRate == true)
   {
   }

   return 0;
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - TestOutput (Play a sine waveform)

   Not needed, will likely move this to the base class in future

   ########################################################################
*/
long museDACNAME::TestOutput()
{
   return 0;
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - StopPlay (Stops the playback.)

   Stops playback instantly, closes the device.

   ########################################################################
*/
long museDACNAME::StopPlay()
{
   DBU_FuncTrace("museDACNAME","StopPlay",TRACE_SIMPLE);

   if (Opened == false)
      return 0;

   Opened = false;
   Play = 0;

   return 0;
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - StopWhenDone (Fills the buffer with the last byte)
   IDL - void StopWhenDone()

   Allows the mixer to fill the remainder of the buffer with silence data
   and then plays the block and waits for all blocks to be played.

   ########################################################################
*/
void museDACNAME::StopWhenDone()
{
   DBU_FuncTrace("museDACNAME","StopWhenDone",TRACE_SIMPLE);

   if (Opened == false)
      return;

   // Fill the final buffer, GetNextBuffer is NOT called.
   museDACMixer::StopWhenDone();

   return;
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - PausePlay (Pause the playback.)

   Pauses playback

   ########################################################################
*/
void museDACNAME::PausePlay()
{
   DBU_FuncTrace("museDACNAME","PausePlay",TRACE_SIMPLE);
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - ResumePlay (Resumes the playback.)

   Resumes playback.

   ########################################################################
*/
void museDACNAME::ResumePlay()
{
   DBU_FuncTrace("museDACNAME","ResumePlay",TRACE_SIMPLE);
}

/* ########################################################################

   Class - museDACNAME (NAME Interface for the digital mixer)
   Member - GetNextBuffer (Returns a buffer for the mixer to use)

   Calling this function implies that the last buffer used has been fully
   written and should be played. GetNextBuffer should block if it cannot
   allocate a buffer.

   ########################################################################
*/
long museDACNAME::GetNextBuffer(unsigned char **Start,unsigned char **Stop)
{
   return 0;
}

/* ########################################################################

   Class - museDACNAMEClass (Class of museDACNAME)
   Member - Constructor (Inits the class)

   Simply inits data members.

   ########################################################################
*/
museDACNAMEClass::museDACNAMEClass()
{
   Level = 0xFF;

   // Set versioning and link the class to the handler array
   MajorVersion = 1;
   MinorVersion = 0;
   museHandlerList::__ClassObject->AddOutput(this);
}

/* ########################################################################

   Class - museDACNAMEClass (Class of museDACNAME)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns the name of the device, should be as specific as possible.

   ########################################################################
*/
string museDACNAMEClass::GetTypeName()
{
   return "NAME";
}

/* ########################################################################

   Class - museDACNAMEClass (Class of museDACNAME)
   Member - IsSupported (Returns the Support level of this class)
   IDL - octet IsSupported()

   Check to see if the device is present in the system.
   Returns a # indicating it's relative ranking in the device tree, GUS
   is considered a 5, 0 is valid. (Set Level to the # that will be
   returned by this function)

   ########################################################################
*/
octet museDACNAMEClass::IsSupported()
{
   return 0xFF;
}

