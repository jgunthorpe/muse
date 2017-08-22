/* ########################################################################

   NAME - NAME Output device

   ########################################################################
*/
#ifndef NAME_HC
#define NAME_HC

#include <SOM.hc>

#include <DAC.hc>

class museDACNAMEClass;

class museDACNAME : public museDACMixer
{
   unsigned long SamplingRate;
   octet Bits;
   boolean Stereo;
   boolean Opened;
   octet Play;

   public:
   static museDACNAMEClass *__ClassObject;
   inline virtual SOMClass *somGetClass();

   virtual long InitPlay(string *Error);
   virtual long StopPlay();
   virtual void StopWhenDone();
   virtual void PausePlay();
   virtual void ResumePlay();
   virtual long TestOutput();

   virtual long SetMixParams(unsigned long SRate,octet Bits,boolean Stereo);
   virtual long GetNextBuffer(octet **Begin,octet **End);

   museDACNAME();
   ~museDACNAME();
};

class museDACNAMEClass : public museDACMixerClass
{
   public:

   virtual string GetTypeName();
   virtual octet IsSupported();

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museDACNAME::__ClassObject)
         return TRUE;
      return museDACMixerClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return new museDACNAME;};
   virtual string somGetName() {return "museDACNAME";};

   museDACNAMEClass();
};

inline SOMClass *museDACNAME::somGetClass() {return __ClassObject;};

#endif
