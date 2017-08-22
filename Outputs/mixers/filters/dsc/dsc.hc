/* ########################################################################

   Dynamic Scaling Control

   ########################################################################
*/
#ifndef DSC_HC
#define DSC_HC

#ifndef MIXER_HC
#include <Mixer.hc>
#endif

class museDSCFilterClass;
class museDSCFilter : public museFilterBase
{
   public:
   static museDSCFilterClass *__ClassObject;
   inline virtual SOMClass *somGetClass();

   int AmpType;
   unsigned long AmpLowest;
   unsigned long AmpFactor;
   unsigned long SpikeCounter;
   unsigned long ComputedSP;
   unsigned long CountDown;

   // 32 bit stereo filters
   virtual void Filter(signed long *Out,signed long *OutEnd);
   virtual void Filterm(signed long *Out,signed long *OutEnd);

   museDSCFilter();
};

class museDSCFilterClass : public museFilterClass
{
   public:

   virtual string GetTypeName() {return "Dynamic Scaling Control";};

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museFilterBase::__ClassObject)
         return TRUE;
      return SOMClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return new museDSCFilter;};
   virtual string somGetName() {return "museDSCFilter";};

   virtual void Link() {};
   museDSCFilterClass();
};
SOMClass *museDSCFilter::somGetClass() {return __ClassObject;};

#endif
