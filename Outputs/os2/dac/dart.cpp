/* ########################################################################

   DACDART - Class to allow Muse/2 to output a module to a DART DAC.
             using DART

   Interface to DART for the DAC.

   ########################################################################
*/
#ifndef CPPCOMPILE
#include <Sequence.hh>
#include <DART.hh>
#include <DebugUtils.hh>
#include <MMPM2.hh>
#else
#include <IDLSeq.hc>
#include <DART.hc>
#include <DebugUtl.hc>
#include <HandLst.hc>
#include <MMPM2.hc>

museDACDARTClass *museDACDART::__ClassObject = new museDACDARTClass;
#endif

#define INCL_OS2MM
#define INCL_ERRORS
#define INCL_DOS
#include <os2.h>
#include <os2me.h>

#include <io.h>
#include <math.h>

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - Constructor (Init the class)
   IDL - somDefaultInit

   Initilize variables to non play states.

   ########################################################################
*/
museDACDART::museDACDART()
{
   DBU_FuncTrace("museDACDART","museDACDART",TRACE_SIMPLE);

   BlockSize = 16*1024;
   Opened = false;
   SamplingRate = 22050;
   Bits = 8;
   Stereo = true;
   DeviceId = 0;
   Playing = 0;
   LastCount = 0;
   NumBuffers = 0;
   Buffers = 0;
   CurPlayBuffer = 0;
   RealTime = false;
   Setup = new MCI_MIXSETUP_PARMS;
   DetectSRate = true;
   WaitTime = 0;

   TotalBufferTime = 2;
   MaxBufferLen = 16*1024;
   if ((DosCreateEventSem(0,(HEV *)(&BlockSem),0,false) != NO_ERROR) ||
      (DosCreateEventSem(0,(HEV *)(&SyncSem),0,false) != NO_ERROR) ||
      (DosCreateEventSem(0,(HEV *)(&DoneSem),0,false) != NO_ERROR))
   {
      DBU_FuncMessage("Could not allocate semephore, panicing");
      char *Blah = 0;
      *Blah = 0;
   }
}

museDACDART::~museDACDART()
{
   DBU_FuncTrace("museDACDART","museDACDART",TRACE_SIMPLE);

   DosCloseEventSem(DoneSem);
   DosCloseEventSem(BlockSem);

   delete Setup;
   delete [] Buffers;
   Buffers = 0;

   CloseMMPM();
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - SetMixParams (Builds the playlist needed for tracking)

   Store playback type for initilization.

   ########################################################################
*/
long museDACDART::SetMixParams(unsigned long SRate, octet Bits, bool Stereo)
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

   Class - museDACDART (DART Interface for the digital mixer)
   Member - InitPlay (Initilize the DART interface)

   Initilize the DART system (Done through OpenMMPM).

   ########################################################################
*/
long museDACDART::InitPlay(string *Error)
{
   *Error = 0;

   long Rc;

   // Init MMPM/2
   if ((Rc = OpenMMPM(Error)) != 0)
      return Rc;

   return 0;
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - EventHandler (Handle DART Events)

   Event handling

   ########################################################################
*/
museDACDART *CurPlayClass = 0;
LONG APIENTRY DARTEventHandler(ULONG Status, PMCI_MIX_BUFFER Buffer,ULONG Flags)
{
   return CurPlayClass->EventHandler(Status,Buffer,Flags);
};

long museDACDART::EventHandler(unsigned long Status, MCI_MIX_BUFFER *Buffer, unsigned long Flags)
{
   switch (Flags)
   {
      case MIX_STREAM_ERROR | MIX_WRITE_COMPLETE:
         somPrintf("Stream Error\n");
         Buffers[NumBuffers].ulUserParm = 0xFEAE;   // Silence key
         Setup->pmixWrite(Setup->ulMixHandle,&Buffers[NumBuffers],1);
         Buffers[NumBuffers].ulUserParm = 0xFEAE;   // Silence key
         Setup->pmixWrite(Setup->ulMixHandle,&Buffers[NumBuffers],1);

         /* Let non silence block through so that the filler can fill as
            many as possible */
         if (Buffer->ulUserParm != 0xFEAE)
         {
            DosPostEventSem(BlockSem);
            CurOutBuffer++;
         }
         else
            Silence++;
         break;

      case MIX_WRITE_COMPLETE:
         /* Let non silence block through so that the filler can fill as
            many as possible */
         if (Buffer->ulUserParm != 0xFEAE)
         {
            DosPostEventSem(BlockSem);

            CurOutBuffer++;
            if (WaitTime != 0xFFFFFFFF)
               if (CurOutBuffer >= WaitTime)
               {
                  WaitTime = 0xFFFFFFFF;
                  DosPostEventSem(SyncSem);
               }
         }
         else
            Silence++;

         // 4 means play has been terminated
         if (Playing == 4)
         {
            if (CurOutBuffer + 1 == CurPlayBuffer)
            {
               DosPostEventSem(DoneSem);
               somPrintf("Done\n");
               break;
            }
            break;
         }

         if (CurPlayBuffer >= CurBuffer - 1)
         {
            // End output
            if (Playing == 3)
            {
               Playing = 4;
               Buffers[CurPlayBuffer % NumBuffers].ulFlags = MIX_BUFFER_EOS;
               somPrintf("Exiting\n");
            }
            else
            {
               // Silence block
               Playing = 5;
            }
         }

         // Data underrun, play a empty block until we are recovered.
         if (Playing == 5)
         {
            if (NumBuffers > 2 + (long)(NumBuffers/4))
            {
               if (CurBuffer - CurPlayBuffer > NumBuffers - 2 - NumBuffers/4)
                  Playing = 2;
            }
            else
            {
               if (NumBuffers == 2)
               {
                  if (CurBuffer > CurPlayBuffer)
                     Playing = 2;
               }
               else
               {
                  if (CurBuffer - CurPlayBuffer > NumBuffers - 1)
                     Playing = 2;
               }
            }

            if (Playing == 5)
            {
               Buffers[NumBuffers].ulUserParm = 0xFEAE;   // Silence key
               Setup->pmixWrite(Setup->ulMixHandle,&Buffers[NumBuffers],1);
               break;
            }
         }

         Setup->pmixWrite(Setup->ulMixHandle,&Buffers[CurPlayBuffer % NumBuffers],1);
         CurPlayBuffer++;
         break;

      default:
         somPrintf("Cool! %u\n",(unsigned long)Flags);
         break;
   };
   return 0;
};

unsigned long museDACDART::Sync(unsigned long Time)
{
   unsigned long BSize = BlockSize/(Bits/8)/(Stereo == true?2:1);
   unsigned long Block = Time/BSize;
   if (Block >= CurOutBuffer)
   {
      WaitTime = Block;
      ULONG Jnk;
      DosWaitEventSem((HEV)SyncSem,-1);
      DosResetEventSem((HEV)SyncSem,&Jnk);
   }
   return (CurOutBuffer)*BSize;
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - OpenMMPM (Open the DART interface)

   The pointer to a string returned in Error is valid for the life of this
   DLL.

   ########################################################################
*/
long museDACDART::OpenMMPM(string *Error)
{
   *Error = 0;
   if (Opened == true)
      return 0;

   DBU_FuncTrace("museDACDART","OpenMMPM",TRACE_SIMPLE);

   APIRET Ret;

   /* Setup the open structure, pass the playlist and tell MCI_OPEN to
      use it. */
   MCI_AMP_OPEN_PARMS OpenData;
   memset(&OpenData,0,sizeof(OpenData));

   OpenData.pszDeviceType = (PSZ)MCI_DEVTYPE_AUDIO_AMPMIX;
   OpenData.usDeviceID = (USHORT)0;

DBU_FuncLineTrace;
   // Send the command
   Ret = mciSendCommand(0,MCI_OPEN,MCI_WAIT | /*MCI_OPEN_SHAREABLE |*/
                  MCI_OPEN_TYPE_ID,(PVOID)&OpenData,0);
   if (Ret != 0)
   {
      DBU_FuncLineTrace;
      *Error = "MCI_OPEN Failure";
      return Ret;
   }

   DeviceId = OpenData.usDeviceID;

   MCI_SET_PARMS msp;
   msp.ulLevel = 100;
   msp.ulAudio = MCI_SET_AUDIO_ALL;
   Ret = mciSendCommand(DeviceId,MCI_SET,MCI_WAIT | MCI_SET_AUDIO |
                  MCI_SET_VOLUME,(PVOID)&msp,0);
   if (Ret != 0)
   {
      somPrintf("Set Volume Fail %u\n",Ret);
   }


   Opened = true;
   Playing = 0;
DBU_FuncLineTrace;

   // Configure the waveform description structure.
   MCI_MIXSETUP_PARMS &WaveInfo = *Setup;
   memset(Setup,0,sizeof(*Setup));
   WaveInfo.ulFormatTag = MCI_WAVE_FORMAT_PCM;
   WaveInfo.ulFormatMode = MCI_PLAY;
   WaveInfo.ulDeviceType = MCI_DEVTYPE_WAVEFORM_AUDIO;
   WaveInfo.pmixEvent = DARTEventHandler;

   if (DetectSRate == true)
   {
      WaveInfo.ulSamplesPerSec = 44100;
      WaveInfo.ulBitsPerSample = 16;
      WaveInfo.ulChannels = (Stereo == true?2:1);
   }
   else
   {
      WaveInfo.ulSamplesPerSec = SamplingRate;
      WaveInfo.ulBitsPerSample = Bits;
      WaveInfo.ulChannels = (Stereo == true?2:1);
   }

   // Detect the sampling rate etc
   int Phase = 0;
   while (1)
   {
      // Set the device up
      Ret = mciSendCommand(DeviceId,MCI_MIXSETUP,MCI_WAIT | MCI_MIXSETUP_INIT,
                          (PVOID) &WaveInfo,0);

      if ((Ret & 0xFFFF) == MCIERR_UNRECOGNIZED_COMMAND)
      {
         *Error = "Device configuration error (Do you have DART installed?)";
         CloseMMPM();
         return Ret;
      }

      if (Ret == 0)
         break;

      if (DetectSRate == false)
      {
         *Error = "Device rejected the Sampling Rate or Bit Rate.";
         CloseMMPM();
         return Ret;
      }

      Phase++;
      switch (Phase)
      {
         case 1:
            WaveInfo.ulSamplesPerSec = 44100;
            WaveInfo.ulBitsPerSample = 8;
            break;

         case 2:
            WaveInfo.ulSamplesPerSec = 22050;
            WaveInfo.ulBitsPerSample = 16;
            break;

         case 3:
            WaveInfo.ulSamplesPerSec = 22050;
            WaveInfo.ulBitsPerSample = 8;
            break;

         case 4:
            *Error = "Unable to find a compatible Sampling Rate or Bit Rate. A stereo card is REQUIRED.";
            CloseMMPM();
            return Ret;
      }
   }

   // Let the Digital Mixer know of the srate
   if (DetectSRate == true)
      SetMixParams(WaveInfo.ulSamplesPerSec,WaveInfo.ulBitsPerSample,WaveInfo.ulChannels-1);

   if (MaxBufferLen > 63*1024)
      MaxBufferLen = 63*1024;

   if (RealTime == true)
   {
      if (MaxBufferLen == 0)
         BlockSize = 16*1024;
      else
         BlockSize = MaxBufferLen;

//      BlockSize = 16*1024;
      NumBuffers = 3;
   }
   else
   {
      if (MaxBufferLen == 0)
         BlockSize = WaveInfo.ulBufferSize;
      else
         BlockSize = MaxBufferLen;

      if (TotalBufferTime == 0)
         TotalBufferTime = 3.5;
      NumBuffers = (SamplingRate*Bits/8*(Stereo == true?2:1)*TotalBufferTime)/BlockSize + 1;
      somPrintf("%uHz, %uB\n",SamplingRate,Bits);
      somPrintf("%u buffers,%u Block, %f\n",NumBuffers,BlockSize,TotalBufferTime);
      if (NumBuffers < 6)
         NumBuffers = 6;
      TotalBufferTime = (float)(NumBuffers - 1)*(float)BlockSize/((float)(SamplingRate*Bits/8*(Stereo == true?2:1)));
      somPrintf("Time - %f\n",TotalBufferTime);
   }

   delete [] Buffers;
   Buffers = new MCI_MIX_BUFFER[NumBuffers];
   memset(Buffers,0,sizeof(*Buffers)*NumBuffers);

   MCI_BUFFER_PARMS BufferParms;
   memset(&BufferParms,0,sizeof(BufferParms));
   BufferParms.ulNumBuffers = NumBuffers;
   BufferParms.ulBufferSize = BlockSize;
   BufferParms.pBufList = Buffers;
   somPrintf("%u buffers,%u bytes\n",NumBuffers,BlockSize);

DBU_FuncLineTrace;
   // Allocate buffers
   Ret = mciSendCommand(DeviceId,MCI_BUFFER,MCI_WAIT | MCI_ALLOCATE_MEMORY,
                       (PVOID) &BufferParms,0);
   if (Ret != 0)
   {
      *Error = "Buffer Allocation Error";
      CloseMMPM();
      return Ret;
   }
DBU_FuncLineTrace;

   NumBuffers = BufferParms.ulNumBuffers - 1;

   for (unsigned int I = 0; I != (NumBuffers + 1); I++)
   {
      Buffers[I].ulBufferLength = BlockSize;
      Buffers[I].ulFlags = 0;
      Buffers[I].ulUserParm = 0;
      somPrintf("Buffer %u, Addr %x\n",(unsigned long)I,(unsigned long)Buffers[I].pBuffer);
   }

   // Generate an empty buffer
   if (Bits == 8)
      memset(Buffers[NumBuffers].pBuffer,127,BlockSize);

   if (Bits == 16)
      memset(Buffers[NumBuffers].pBuffer,0,BlockSize);

   CurBuffer = 0;
   CurPlayBuffer = 0;
   Paused = false;
   return 0;
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - CloseMMPM (Close the DART interface)

   Can be called during a failed OpenMMPM2

   ########################################################################
*/
long museDACDART::CloseMMPM()
{
   if (Opened == false)
      return 0;

   DBU_FuncTrace("museDACDART","CloseMMPM",TRACE_SIMPLE);

   APIRET Rc;

   // Generic parameters
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Stop the playback.
   Rc = mciSendCommand(DeviceId,MCI_STOP,MCI_WAIT,(PVOID)&Params,0);

   MCI_BUFFER_PARMS BufferParms;
   memset(&BufferParms,0,sizeof(BufferParms));
   BufferParms.ulNumBuffers = NumBuffers + 1;
   BufferParms.ulBufferSize = BlockSize;
   BufferParms.pBufList = Buffers;
   Rc = mciSendCommand(DeviceId, MCI_BUFFER,MCI_WAIT | MCI_DEALLOCATE_MEMORY,(PVOID)&BufferParms,0);
   somPrintf("RC Free -- %u\n",Rc);
   Rc = mciSendCommand(DeviceId, MCI_MIXSETUP,MCI_WAIT | MCI_MIXSETUP_DEINIT,(PVOID)Setup,0);
   somPrintf("RC DeInit -- %u\n",Rc);

   // Close the device
   Rc = mciSendCommand(DeviceId,MCI_CLOSE,MCI_WAIT,(PVOID)&Params,0);
   if (Rc != 0)
      return Rc;

   Opened = false;
   DeviceId = 0;
   Playing = 0;
   CurPlayClass = 0;
   Paused = false;

   return 0;
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - TestOutput (Play a sine waveform)

   Fills the buffer with a sine wave and plays it.

   ########################################################################
*/
long museDACDART::TestOutput()
{
   return 0;
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - StopPlay (Stops the wav playback.)

   Halts wave playback.

   ########################################################################
*/
long museDACDART::StopPlay()
{
   DBU_FuncTrace("museDACDART","StopPlay",TRACE_SIMPLE);

   if (Opened == false)
      return 0;

   Paused = false;

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

   Class - museDACDART (Class to allow Muse/2 to use DART for output)
   Member - StopWhenDone (Fills the buffer with the last byte)
   IDL - void StopWhenDone()

   Used to allow the buffer to run out so the song can end on a buffer
   boundry.

   ########################################################################
*/
void museDACDART::StopWhenDone()
{
   DBU_FuncTrace("museDACDART","StopWhenDone",TRACE_SIMPLE);

   if (Playing != 2)
      return;

   museDACMixer::StopWhenDone();

   // Comit the final buffer.
   CurBuffer++;

   ULONG Jnk;
   DosResetEventSem((HEV)DoneSem,&Jnk);
   Playing = 3;
   DosWaitEventSem((HEV)DoneSem,10000);
   DosResetEventSem((HEV)DoneSem,&Jnk);
   CloseMMPM();

   return;
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - PausePlay (Pause the wav playback.)

   Pauses playback.

   ########################################################################
*/
void museDACDART::PausePlay()
{
   DBU_FuncTrace("museDACDART","PausePlay",TRACE_SIMPLE);

   // Generic parameter structure
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Resume paused playback.
   mciSendCommand(DeviceId,MCI_PAUSE,MCI_WAIT,(PVOID)&Params,0);
   Paused = true;
}

/* ########################################################################

   Class - museDACDART (DART Interface for the digital mixer)
   Member - ResumePlay (Resumes the wav playback.)

   Resumes wave playback.

   ########################################################################
*/
void museDACDART::ResumePlay()
{
   DBU_FuncTrace("museDACDART","ResumePlay",TRACE_SIMPLE);

   // Generic parameter structure
   MCI_GENERIC_PARMS Params;
   Params.hwndCallback = 0;

   // Resume paused playback.
   mciSendCommand(DeviceId,MCI_RESUME,MCI_WAIT,(PVOID)&Params,0);
   Paused = false;
}

long museDACDART::GetNextBuffer(unsigned char **Start,unsigned char **Stop)
{
   // First Call
   if ((Opened == true) && (Playing == 0))
   {
      if (CurPlayClass == 0)
         CurPlayClass = this;
      else
         return 14;

      CurBuffer = 0;
      CurPlayBuffer = 0;
      Playing = 1;
   }

   // Preload data
   if ((Opened == true) && (Playing == 1))
   {
      unsigned long End = NumBuffers;
/*      if (End > 3)
         End -= 2;*/

      if (CurBuffer < End && CurBuffer < NumBuffers)
      {
         *Start = (unsigned char *)Buffers[CurBuffer % NumBuffers].pBuffer;
         *Stop = *Start + BlockSize;
         CurBuffer++;
         return 0;
      }

      DosResetEventSem((unsigned long)DoneSem,&LastCount);
      DosResetEventSem((unsigned long)BlockSem,&LastCount);
      LastCount = 0;
      Silence = 0;
      somPrintf("Play\n");
      CurPlayBuffer += 2;
      CurOutBuffer = 0;
      Setup->pmixWrite(Setup->ulMixHandle,Buffers,2);
      somPrintf("Play!\n");
      Playing = 2;
   }

   // Normal action to be taken.
   if ((Opened == true) && ((Playing == 2) || (Playing == 5)))
   {
      *Start = (unsigned char *)Buffers[CurBuffer % NumBuffers].pBuffer;
      *Stop = *Start + BlockSize;
      CurBuffer++;

      if (LastCount == 0)
      {
         unsigned long OldSilence = Silence;
         int DeathCount = 0;
         while (1)
         {
            if (Paused == true || RealTime == true)
            {
               if (DosWaitEventSem((HEV)BlockSem,-1) == 0)
                  break;
            }
            else
            {
               if (DosWaitEventSem((HEV)BlockSem,(TotalBufferTime*1000)/NumBuffers+500) == 0)
                  break;
            }

            if ((Silence == OldSilence || CurOutBuffer < 5) && (Paused == false))
            {
               DeathCount++;
               if (DeathCount != 2)
                  continue;
               DosBeep(100,100);
               DosBeep(100,100);
               DosBeep(100,100);
               DosBeep(100,100);
               DosBeep(100,100);
               somPrintf("DART ABORT\n");
               return 10;
            }
            OldSilence = Silence;
         }
         DosResetEventSem((HEV)BlockSem,&LastCount);
      }
      if (LastCount != 0)
         LastCount--;
      return 0;
   }

   return 1;
}

/* ########################################################################

   Class - museDACDARTClass (Class of museDACDART)
   Member - Constructor (Inits the class)

   Simply inits data members.

   ########################################################################
*/
museDACDARTClass::museDACDARTClass()
{
   Level = 2;

#ifdef CPPCOMPILE
   MajorVersion = 1;
   MinorVersion = 5;
   museHandlerList::__ClassObject->AddOutput(this);
#endif
}

/* ########################################################################

   Class - museDACDARTClass (Class of museDACDART)
   Member - GetTypeName (Returns the name of this class)
   IDL - string GetTypeName

   Returns a pointer to a string containing 'Scream Tracker 3 Module'.
   This string is valid while the object exists.

   ########################################################################
*/
string museDACDARTClass::GetTypeName()
{
   return "DART Wave Output";
}

/* ########################################################################

   Class - museDACDARTClass (Class of museDACMMPM2)
   Member - IsSupported (Returns the Support level of this class)
   IDL - octet IsSupported()

   Checks to see if DART is installed and a Digital Wave device is openable.

   ########################################################################
*/
octet museDACDARTClass::IsSupported()
{
   museDACMMPM2Class *MMPM = (museDACMMPM2Class *)museDACMMPM2::__ClassObject;

   MMPM->IsSupported();

   if (MMPM->DART == true)
      return 2;
   else
      return 0;
}
