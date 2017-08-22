/* ########################################################################

   AU - AU Output device

   ########################################################################
*/
#ifndef AU_H
#define AU_H

#include <dac.h>

class museFileAU;
class museFileAUClass;

class museFileAU : public museFileOutput
{
   unsigned long SamplingRate;
   octet Bits;
   bool Stereo;
   bool Opened;
   int Handle;
   octet *Buffer;
   unsigned long BufferSize;
   unsigned long TotalSize;
   octet Play;

   public:
   static museFileAUClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long InitPlay(char **Eror);
   virtual long StopPlay();
   virtual void StopWhenDone();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual long TestOutput();

   virtual long SetMixParams(unsigned long SRate,octet Bits,bool Stereo);
   virtual long GetNextBuffer(octet **Begin,octet **End);

   museFileAU();
   ~museFileAU();
};

class museFileAUClass : public museFileOutputClass
{
   public:

   virtual const char *GetTypeName();
   virtual octet IsSupported();

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFileAU::Meta)
         return true;
      return museFileOutputClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museFileAU;};
   virtual const char *GetName() {return "museFileAU";};

   museFileAUClass();
};

inline MetaClass *museFileAU::GetMetaClass() {return Meta;};

#endif
