/* ########################################################################

   Echos

   ######################################################################## */

#ifndef ECHO_HC
#define ECHO_HC

#ifndef MIXER_HC
#include <Mixer.hc>
#endif

struct InputEcho
{
double Delay;
double Scale, Pan;
unsigned long Flags;
};

struct Echo
{
unsigned long Delay;
signed long ScaleL, ScaleR;
unsigned long Flags;
};

#define ECHO_Normal   0
#define ECHO_Flipped  1
#define ECHO_Surround 4
#define ECHO_Feedback 8

class museEchoFilterClass;
class museEchoFilter : public museFilterBase
{
   public:
   static museEchoFilterClass *__ClassObject;
   inline virtual SOMClass *somGetClass();

   // 32 bit stereo filters
   virtual void SetupEcho(int NoOfEchos,InputEcho *Echos,float Allow);
   virtual void Filter(signed long *Out,signed long *OutEnd);
   virtual void Filterm(signed long *Out,signed long *OutEnd);

   museEchoFilter();
   ~museEchoFilter();

   private:

   signed long *Buffer, *FeedbackBuffer;
   unsigned long BufferLength, FeedbackLength, BufferPos, FeedbackPos;
   int NoOfEchos, NoOfFeedbacks;
   Echo *Echos;
   int Allowance;
   unsigned long Mutex;
};

class museEchoFilterClass : public museFilterClass
{
   public:

   virtual string GetTypeName() {return "Echo Effect Processor";};

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museFilterBase::__ClassObject)
         return TRUE;
      return SOMClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return new museEchoFilter;};
   virtual string somGetName() {return "museEchoFilter";};

   virtual void Link() {};
   museEchoFilterClass();
};
SOMClass *museEchoFilter::somGetClass() {return __ClassObject;};

#endif
