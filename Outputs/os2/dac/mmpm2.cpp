/* ########################################################################

   DACMixer - Class to allow Muse/2 to output a module to a MMPM/2 DAC.

   Interface to MMPM/2 for the DAC.

   ########################################################################
*/
#ifndef CPPCOMPILE
#include <Sequence.hh>
#include <MMPM2.hh>
#include <DART.hh>
#include <DebugUtils.hh>
#else
#include <IDLSeq.hc>
#include <MMPM2.hc>
#include <DebugUtl.hc>
#include <HandLst.hc>

museDACMMPM2Class *museDACMMPM2::__ClassObject = new museDACMMPM2Class;
#endif

#define INCL_OS2MM
#define INCL_ERRORS
#define INCL_DOS
#include <os2.h>
#include <os2me.h>

#include <io.h>
#include <math.h>

unsigned long DoneSem2 = 0;

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - Constructor (Init the class)
   IDL - somDefaultInit

   ########################################################################
*/
museDACMMPM2::museDACMMPM2()
{
   DBU_FuncTrace("museDACMMPM2","museDACMMPM2",TRACE_SIMPLE);

   BufferBegin = 0;
   BufferEnd = 0;
   BufferCur = 0;
   BlockSize = 16*1024;
   PlayList.construct();
   PlayList.reserve(44100*2*2*3.5/BlockSize*2 + 50);
   Opened = false;
   SamplingRate = 44100;
   Bits = 16;
   Stereo = true;
   DeviceId = 0;
   Playing = 0;
   LastCount = 0;
   LastDelta = 0;
   PlayedBlocks = 0;
   DetectSRate = true;

   if ((DosCreateEventSem(0,(HEV *)(&BlockSem),0,false) != NO_ERROR) ||
      (DosCreateEventSem(0,(HEV *)(&DoneSem),0,false) != NO_ERROR) ||
      (DosCreateEventSem(0,(HEV *)(&DoneSem2),0,false) != NO_ERROR))
   {
      DBU_FuncMessage("Could not allocate semephore, panicing");
      char *Blah = 0;
      *Blah = 0;
   }
}

museDACMMPM2::~museDACMMPM2()
{
   DBU_FuncTrace("museDACMMPM2","museDACMMPM2",TRACE_SIMPLE);

   DosCloseEventSem(BlockSem);

   SOMFree(BufferBegin);
   PlayList.free();
   CloseMMPM();
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - InitPlay (Initilize the MMPM/2 interface)

   All MCI events from the object window come here.

   The pointer to a string returned in Error is valid for the life of this
   DLL.

   ########################################################################
*/
long museDACMMPM2::InitPlay(string *Error)
{
   long Rc;

   // Init MMPM/2
   if ((Rc = OpenMMPM(Error)) != 0)
      return Rc;

   return 0;
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - SetMixParams (Builds the playlist needed for tracking)

   Builds the playlist and allocates the buffers.

   ########################################################################
*/
long museDACMMPM2::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
{
   DBU_FuncTrace("museDACMMPM2","SetMixParams",TRACE_SIMPLE);

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

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - OpenMMPM (Open the MMPM/2 interface)

   The pointer to a string returned in Error is valid for the life of this
   DLL.

   ########################################################################
*/
long museDACMMPM2::OpenMMPM(string *Error)
{
   if (Opened == true)
      return 0;
   RealTime = false;

   DBU_FuncTrace("museDACMMPM2","OpenMMPM",TRACE_SIMPLE);

   APIRET Ret;

   /* Setup the open structure, pass the playlist and tell MCI_OPEN to
      use it. */
   MCI_OPEN_PARMS OpenData;
   memset(&OpenData,0,sizeof(OpenData));

   OpenData.pszDeviceType = (PSZ)MAKEULONG(MCI_DEVTYPE_WAVEFORM_AUDIO,1);
   OpenData.pszElementName = (PSZ) PlayList.begin();
   OpenData.hwndCallback = 0;
   OpenData.pszAlias = 0;

   // Generic parameters
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

DBU_FuncLineTrace;
   // Send the command
   Ret = mciSendCommand(0,MCI_OPEN,MCI_WAIT | MCI_OPEN_PLAYLIST |
                  MCI_OPEN_TYPE_ID,(PVOID)&OpenData,0);
   if (Ret != 0)
   {
      DBU_FuncLineTrace;
      *Error = "MCI_OPEN Failure";
      return Ret;
   }

   DeviceId = OpenData.usDeviceID;

   Opened = true;
   Playing = 0;
DBU_FuncLineTrace;

   // Configure the waveform description structure.
   MCI_WAVE_SET_PARMS WaveInfo;
   memset(&WaveInfo,0,sizeof(WaveInfo));
   WaveInfo.ulAudio = MCI_SET_AUDIO_ALL;    // Both Channels

   if (DetectSRate == true)
   {
      WaveInfo.ulSamplesPerSec = 44100;
      WaveInfo.usBitsPerSample = 16;
      WaveInfo.usChannels = (Stereo == true?2:1);
   }
   else
   {
      WaveInfo.ulSamplesPerSec = SamplingRate;
      WaveInfo.usBitsPerSample = Bits;
      WaveInfo.usChannels = (Stereo == true?2:1);
   }

   // Set the # of channels
   Ret = mciSendCommand(DeviceId,MCI_SET,MCI_WAIT | MCI_WAVE_SET_CHANNELS,
                       (PVOID) &WaveInfo,0);
   if (Ret != 0)
   {
      *Error = "Mono/Stereo Rejected";
      CloseMMPM();
      return Ret;
   }

   // Set the Sampling rate
   while (1)
   {
      Ret = mciSendCommand(DeviceId,MCI_SET,MCI_WAIT | MCI_WAVE_SET_SAMPLESPERSEC,
                          (PVOID) &WaveInfo,0);
      if (Ret != 0 && DetectSRate == false)
      {
         *Error = "Sampling Rate Rejected";
         CloseMMPM();
         return Ret;
      }

      if (Ret == 0)
         break;

      if (WaveInfo.ulSamplesPerSec == 22050)
      {
         *Error = "Can't find compatible sampling rate";
         CloseMMPM();
         return Ret;
      }

      WaveInfo.ulSamplesPerSec = 22050;
   }

   // Set the bit size
   while (1)
   {
      Ret = mciSendCommand(DeviceId,MCI_SET,MCI_WAIT | MCI_WAVE_SET_BITSPERSAMPLE,
                          (PVOID) &WaveInfo,0);
      if (Ret != 0 && DetectSRate == false)
      {
         *Error = "Bit Rate Rejected";
         CloseMMPM();
         return Ret;
      }

      if (Ret == 0)
         break;

      if (WaveInfo.usBitsPerSample == 8)
      {
         *Error = "Can't find compatible bit rate";
         CloseMMPM();
         return Ret;
      }

      WaveInfo.usBitsPerSample = 8;
   }

   if (DetectSRate == true)
      SetMixParams(WaveInfo.ulSamplesPerSec,WaveInfo.usBitsPerSample,WaveInfo.usChannels-1);

   // Setup the playlist
   unsigned int BufferCount = (SamplingRate*Bits/8*(Stereo == true?2:1)*3.5)/BlockSize;

   SOMFree(BufferBegin);
   BufferBegin = (unsigned char *)SOMMalloc(BufferCount*BlockSize);
   BufferCur = BufferBegin;
   BufferEnd = BufferBegin + BufferCount*BlockSize;

   /* Build the playlist
      Each command is 4*4 bytes long, each block requires 2 commands and
      a jump command at the end.
   */
   PlayList._length = BufferCount;
   for (unsigned int I = 0;I != BufferCount;I++)
   {
      // Play a data block.
      PlayList[I*2].OpCode = DATA_OPERATION;
      PlayList[I*2].Operand1 = (unsigned long)(BufferBegin + I*BlockSize);
      PlayList[I*2].Operand2 = BlockSize;
      PlayList[I*2].Operand3 = 0;

      /* Put a message operation in, the window will recieve a message when
         this command is executed */
      PlayList[I*2+1].OpCode = 10;
      PlayList[I*2+1].Operand1 = BlockSem;
      PlayList[I*2+1].Operand2 = 0;
      PlayList[I*2+1].Operand3 = 0;
   };

   PlayList[2*BufferCount].OpCode = BRANCH_OPERATION;
   PlayList[2*BufferCount].Operand1 = 0;
   PlayList[2*BufferCount].Operand2 = 0;
   PlayList[2*BufferCount].Operand3 = 0;

   return 0;
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - CloseMMPM (Close the MMPM/2 interface)

   Can be called during a failed OpenMMPM2

   ########################################################################
*/
long museDACMMPM2::CloseMMPM()
{
   if (Opened == false)
      return 0;

   DBU_FuncTrace("museDACMMPM2","CloseMMPM",TRACE_SIMPLE);

   APIRET Rc;

   // Generic parameters
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Stop the playback.
   Rc = mciSendCommand(DeviceId,MCI_STOP,MCI_WAIT,(PVOID)&Params,0);

   // Close the device
   Rc = mciSendCommand(DeviceId,MCI_CLOSE,MCI_WAIT,(PVOID)&Params,0);
   if (Rc != 0)
      return Rc;

   Opened = false;
   DeviceId = 0;
   Playing = 0;

   return 0;
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - TestOutput (Play a sine waveform)

   Fills the buffer with a sine wave and plays it.

   ########################################################################
*/
long museDACMMPM2::TestOutput()
{
   // Build the sine info
   if (Stereo == true)
   {
      unsigned int I2 = 0;
      for (unsigned char *Cur = BufferBegin; Cur != BufferEnd; Cur += 2)
      {
         *Cur = ((unsigned char)127+127*sin(((float)I2)/50));
         Cur[1] = ((unsigned char)127+127*sin(((float)I2)/50));
         I2++;
      }
   }
   else
   {
      unsigned int I2 = 0;
      for (unsigned char *Cur = BufferBegin; Cur != BufferEnd; Cur++)
      {
         *Cur = ((unsigned char)127+127*sin(((float)I2)/50));
         I2++;
      }
   }

   // Only 1 pass
   PlayList.back().OpCode = EXIT_OPERATION;

   // Play structure
   MCI_PLAY_PARMS PlayParams;
   PlayParams.hwndCallback = 0;
   PlayParams.ulFrom = 0;
   PlayParams.ulTo = 0;

   // Send the play command
   APIRET Ret = mciSendCommand(DeviceId,MCI_PLAY,MCI_WAIT,(PVOID)&PlayParams,0);
   if (Ret != 0)
      return Ret;

   // Back to normal
   PlayList.back().OpCode = BRANCH_OPERATION;

   return 0;
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - StopPlay (Stops the wav playback.)

   Halts wave playback.

   ########################################################################
*/
long museDACMMPM2::StopPlay()
{
   DBU_FuncTrace("museDACMMPM2","StopPlay",TRACE_SIMPLE);

   if (Opened == false)
      return 0;

   // Generic parameter structure
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Resume paused playback.
   mciSendCommand(DeviceId,MCI_STOP,MCI_WAIT,(PVOID)&Params,0);
   Playing = 4;

   DosPostEventSem(BlockSem);

   CloseMMPM();
   return 0;
}

/* ########################################################################

   Class - museDACMMPM2 (Class to allow Muse/2 to use MMPM/2 for output)
   Member - StopWhenDone (Fills the buffer with the last byte)
   IDL - void StopWhenDone()

   Used to allow the buffer to run out so the song can end on a buffer
   boundry.

   ########################################################################
*/
void museDACMMPM2::StopWhenDone()
{
   DBU_FuncTrace("museDACMMPM2","StopWhenDone",TRACE_SIMPLE);

   if (Playing != 3)
      return;

   museDACMixer::StopWhenDone();

   int Block = (BufferCur - BufferBegin)/BlockSize;

   ULONG Jnk;
   DosResetEventSem((HEV)DoneSem2,&Jnk);
   PlayList[Block*2].OpCode = EXIT_OPERATION;
   DosWaitEventSem((HEV)DoneSem2,-1);
   PlayList[Block*2].OpCode = DATA_OPERATION;
   DosResetEventSem((HEV)DoneSem2,&Jnk);

   CloseMMPM();

   return;
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - PausePlay (Pause the wav playback.)

   Pauses playback.

   ########################################################################
*/
void museDACMMPM2::PausePlay()
{
   DBU_FuncTrace("museDACMMPM2","PausePlay",TRACE_SIMPLE);

   // Generic parameter structure
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Resume paused playback.
//   mciSendCommand(DeviceId,MCI_PAUSE,MCI_WAIT,(PVOID)&Params,0);
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - ResumePlay (Resumes the wav playback.)

   Halts wave playback.

   ########################################################################
*/
void museDACMMPM2::ResumePlay()
{
   DBU_FuncTrace("museDACMMPM2","ResumePlay",TRACE_SIMPLE);

   // Generic parameter structure
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Resume paused playback.
//   mciSendCommand(DeviceId,MCI_RESUME,MCI_WAIT,(PVOID)&Params,0);
}

/* ########################################################################

   Class - museDACMMPM2 (MMPM/2 Interface for the digital mixer)
   Member - GetNextBuffer (GetNextBuffer)

   Returns a new buffer that is clear for writing.

   ########################################################################
*/
void APIENTRY CueThread(unsigned long DeviceId)
{
   // Play structure
   MCI_PLAY_PARMS PlayParams;
   PlayParams.hwndCallback = 0;
   PlayParams.ulFrom = 0;
   PlayParams.ulTo = 0;

   somPrintf("Play\n");
   // Send the play command
   APIRET Ret = 0;
   Ret = mciSendCommand(DeviceId,MCI_PLAY,MCI_WAIT,(PVOID)&PlayParams,0);
   if (Ret != 0)
   {
      somPrintf("PLAY FAILURE!\n");
      return;
   }

   DosPostEventSem(DoneSem2);
}

long museDACMMPM2::GetNextBuffer(unsigned char **Start,unsigned char **Stop)
{
   // Data is loaded, Cue playback and reload the stuff that was cued
   if ((Opened == true) && (Playing == 0))
   {
      if (Bits == 8)
         for (unsigned char *Cur = BufferBegin; Cur != BufferEnd; Cur++) *Cur = 127;

      if (Bits == 16)
         for (unsigned char *Cur = BufferBegin; Cur != BufferEnd; Cur++) *Cur = 0;

      DosResetEventSem((unsigned long)DoneSem,&LastCount);
      DosResetEventSem((unsigned long)DoneSem2,&LastCount);
      DosResetEventSem((unsigned long)BlockSem,&LastCount);
      LastCount = 0;
      BufferCur = BufferBegin;

      MCI_GENERIC_PARMS Params;
      Params.hwndCallback = 0;

      // Send the cue command
      APIRET Ret = 0;
      Ret = mciSendCommand(DeviceId,MCI_CUE,MCI_WAIT | MCI_WAVE_OUTPUT,(PVOID)&Params,0);
      if (Ret != 0)
      {
         somPrintf("CUE FAILURE!\n");
         return 1;
      }

      DosResetEventSem((unsigned long)BlockSem,&LastCount);
      somPrintf("LC! %u ",(unsigned int)LastCount);
      BufferCur += LastCount % ((BufferEnd - BufferBegin)/BlockSize);
      LastCount = ((BufferEnd - BufferBegin)/BlockSize);
      somPrintf(" %u\n",(unsigned int)LastCount);

      Playing = 1;

      if (BufferCur >= BufferEnd)
         BufferCur = BufferBegin;

      *Start = BufferCur;
      BufferCur += LastCount*BlockSize;
      if (BufferCur >= BufferEnd)
      {
         LastCount = (BufferCur - BufferEnd)/BlockSize;
         BufferCur = BufferEnd;
      }
      else
         LastCount = 0;

      *Stop = BufferCur;
   }

   if ((Opened == true) && (Playing == 1))
   {
      TID Tid;
      DosCreateThread(&Tid,&CueThread,DeviceId,CREATE_READY | 2,20000);

      Playing = 3;
   }

   if ((Opened == true) && (Playing == 3))
   {
      if (BufferCur >= BufferEnd)
         BufferCur = BufferBegin;

      if (LastCount == 0)
      {
         DosWaitEventSem((HEV)BlockSem,-1);
         DosResetEventSem((HEV)BlockSem,&LastCount);

         if (Playing == 4)
            return 2;
      }

      *Start = BufferCur;
      BufferCur += LastCount*BlockSize;
      if (BufferCur >= BufferEnd)
      {
         LastCount = (BufferCur - BufferEnd)/BlockSize;
         BufferCur = BufferEnd;
      }
      else
         LastCount = 0;

      *Stop = BufferCur;

      return 0;
   }

   return 1;
}

/* ########################################################################

   Class - museDACMMPM2Class (Class of museDACMMPM2)
   Member - Constructor (Inits the class)

   Simply inits data members.

   ########################################################################
*/
museDACMMPM2Class::museDACMMPM2Class()
{
   DetectStatus = 0;
   DART = false;
   Level = 1;

#ifdef CPPCOMPILE
   MajorVersion = 1;
   MinorVersion = 2;
   museHandlerList::__ClassObject->AddOutput(this);
#endif
}

/* ########################################################################

   Class - museDACMMPM2Class (Class of museDACMMPM2)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns a pointer to a string containing 'Scream Tracker 3 Module'.
   This string is valid while the object exists.

   ########################################################################
*/
string museDACMMPM2Class::GetTypeName()
{
   return "MMPM/2 Wave Output";
}

/* ########################################################################

   Class - museDACMMPM2Class (Class of museDACMMPM2)
   Member - IsSupported (Returns the Support level of this class)
   IDL - octet IsSupported()

   Checks to see if MMPM is installed and a Digital Wave device is openable.

   ########################################################################
*/
octet museDACMMPM2Class::IsSupported()
{
   if (DetectStatus == 1)
      return 0;
   if (DetectStatus == 2)
      return 1;

   /* Setup the open structure, pass the playlist and tell MCI_OPEN to
      use it. */
   MCI_AMP_OPEN_PARMS OpenData;
   memset(&OpenData,0,sizeof(OpenData));

   OpenData.pszDeviceType = (PSZ)MCI_DEVTYPE_AUDIO_AMPMIX;
   OpenData.usDeviceID = (USHORT)0;

   // Send the command
   APIRET Rc;
   Rc = mciSendCommand(0,MCI_OPEN,MCI_WAIT | MCI_OPEN_TYPE_ID,(PVOID)&OpenData,0);

   if (Rc != 0)
   {
      DetectStatus = 1;
      DART = false;
      return 0;
   }
   DetectStatus = 2;

   // Configure the waveform description structure.
   MCI_MIXSETUP_PARMS WaveInfo;
   memset(&WaveInfo,0,sizeof(WaveInfo));
   WaveInfo.ulFormatTag = MCI_WAVE_FORMAT_PCM;
   WaveInfo.ulSamplesPerSec = 22050;
   WaveInfo.ulBitsPerSample = 8;
   WaveInfo.ulChannels = 1;
   WaveInfo.ulFormatMode = MCI_PLAY;
   WaveInfo.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;

   WaveInfo.pmixEvent = 0;

   // Set the device up
   Rc = mciSendCommand(OpenData.usDeviceID,MCI_MIXSETUP,MCI_WAIT | MCI_MIXSETUP_INIT,
                       (PVOID) &WaveInfo,0);
   if ((Rc & 0xFFFF) == MCIERR_UNRECOGNIZED_COMMAND)
      DART = false;
   else
      DART = true;
   somPrintf("Ret - %u",(unsigned long)Rc);

   // Generic parameters
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Close the device
   mciSendCommand(OpenData.usDeviceID,MCI_CLOSE,MCI_WAIT,(PVOID)&Params,0);

   return 1;
}

#ifndef CPPCOMPILE
extern "C"
{
   void QueryOutputHandlers(Sequence<museOutputClass *> *);
}

void QueryOutputHandlers(Sequence<museOutputClass *> *List)
{
   List->construct();
   (*List)[1] = (museOutputClass *)museDACDART::__ClassObject;
   (*List)[0] = (museOutputClass *)museDACMMPM2::__ClassObject;
}
#endif
