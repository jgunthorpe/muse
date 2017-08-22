/* ########################################################################

   DART - DART Wave Output

   ########################################################################
*/
#ifndef DART_HC
#define DART_HC

#include <DAC.hc>

class museDACDART;
class museDACDARTClass;
typedef struct _MCI_MIX_BUFFER MCI_MIX_BUFFER;
typedef struct _MCI_MIXSETUP_PARMS MCI_MIXSETUP_PARMS;

class museDACDART : public museDACMixer
{
   unsigned long BlockSize;
   unsigned long NumBuffers;
   unsigned long CurBuffer;
   unsigned long CurPlayBuffer;
   unsigned long CurOutBuffer;
   unsigned long Silence;
   boolean Opened;
   octet Playing;
   unsigned long LastCount;
   unsigned long BlockSem;
   unsigned long DoneSem;
   unsigned long SamplingRate;
   octet Bits;
   boolean Stereo;
   boolean Paused;
   unsigned long DeviceId;
   MCI_MIX_BUFFER *Buffers;
   MCI_MIXSETUP_PARMS *Setup;
   boolean DetectSRate;

   unsigned long WaitTime;
   unsigned long SyncSem;

   public :
   static museDACDARTClass *__ClassObject;
   inline virtual SOMClass *somGetClass();

   long OpenMMPM(string* Error);
   long CloseMMPM();
   long EventHandler(unsigned long Status, MCI_MIX_BUFFER* Buffer,
                     unsigned long Flags);
   virtual long InitPlay(string* Error);
   virtual long StopPlay();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual long TestOutput();
   virtual long GetNextBuffer(octet** Begin, octet** End);
   virtual void StopWhenDone();
   virtual long SetMixParams(unsigned long SRate, octet Bits,
                             boolean Stereo);

   virtual unsigned long Sync(unsigned long Time);

   unsigned short MaxBufferLen;
   float TotalBufferTime;
   Attr(TotalBufferTime,float);
   Attr(MaxBufferLen,unsigned short);

   museDACDART();
   ~museDACDART();
};

class museDACDARTClass : public museDACMixerClass
{
   public :
   virtual string GetTypeName();
   virtual octet IsSupported();

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museDACDART::__ClassObject)
         return TRUE;
      return museDACMixerClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return new museDACDART;};
   virtual string somGetName() {return "museDACDART";};

   museDACDARTClass();
};

inline SOMClass *museDACDART::somGetClass() {return __ClassObject;};

#endif
