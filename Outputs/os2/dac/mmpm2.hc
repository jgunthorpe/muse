/* ########################################################################

   MMPM2 - MMPM2 Wave Output

   ########################################################################
*/
#ifndef MMPM2_HC
#define MMPM2_HC

#include <DAC.hc>

class museDACMMPM2;
class museDACMMPM2Class;

typedef struct
{
  unsigned long OpCode;
  unsigned long Operand1;
  unsigned long Operand2;
  unsigned long Operand3;
} PlayStruct;
typedef IDLSequence<PlayStruct> SeqPlayStruct;

class museDACMMPM2 : public museDACMixer
{
   long ObjectWindow;
   octet *BufferBegin;
   octet *BufferCur;
   octet *BufferEnd;
   unsigned long BlockSize;
   SeqPlayStruct PlayList;
   boolean Opened;
   octet Playing;
   unsigned long LastCount;
   unsigned long BlockSem;
   unsigned long DoneSem;
   unsigned long PlayedBlocks;
   unsigned long SamplingRate;
   octet Bits;
   boolean Stereo;
   unsigned long LastDelta;
   unsigned long DeviceId;
   boolean DetectSRate;

   public :
   static museDACMMPM2Class *__ClassObject;
   inline virtual SOMClass *somGetClass();

   long OpenMMPM(string* Error);
   long CloseMMPM();
   virtual long InitPlay(string* Error);
   virtual long StopPlay();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual long TestOutput();
   virtual long GetNextBuffer(octet** Begin, octet** End);
   virtual void StopWhenDone();
   virtual long SetMixParams(unsigned long SRate, octet Bits,
                             boolean Stereo);
   museDACMMPM2();
   ~museDACMMPM2();
};

class museDACMMPM2Class : public museDACMixerClass
{
   octet DetectStatus;
   public :
   boolean DART;

   virtual string GetTypeName();
   virtual octet IsSupported();

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museDACMMPM2::__ClassObject)
         return TRUE;
      return museDACMixerClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return new museDACMMPM2;};
   virtual string somGetName() {return "museDACMMPM2";};

   museDACMMPM2Class();
};

inline SOMClass *museDACMMPM2::somGetClass() {return __ClassObject;};

#endif
