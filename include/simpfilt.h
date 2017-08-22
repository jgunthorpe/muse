/* ########################################################################

   SimpleFilters - Simple output filters

   ########################################################################
*/
#ifndef SIMPLEFILTERS_H
#define SIMPLEFILTERS_H

#ifndef MIXER_H
#include <mix.h>
#endif

class museScaleFilterClass;
class museScaleFilter : public museOutputFilterBase
{
   public:

   static museScaleFilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) ;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In);
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In);
};

class museScaleFilterClass : public museOFilterClass
{
   public:

   virtual const char *GetTypeName() {return "Clip/Scale Filter";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museScaleFilter::Meta)
         return true;
      return museOFilterClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museScaleFilter;};
   virtual const char *GetName() {return "museScaleFilter";};

   virtual void Link() {};
   museScaleFilterClass();
};
MetaClass *museScaleFilter::GetMetaClass() {return Meta;};

//----------- Noisy Clip
class museNoisyClipFilterClass;
class museNoisyClipFilter : public museOutputFilterBase
{
   public:

   static museNoisyClipFilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) ;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In);
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In);
};

class museNoisyClipFilterClass : public museOFilterClass
{
   public:

   virtual const char *GetTypeName() {return "Scale Filter";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museNoisyClipFilter::Meta)
         return true;
      return museOFilterClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museNoisyClipFilter;};
   virtual const char *GetName() {return "museNoisyClipFilter";};

   virtual void Link() {};
   museNoisyClipFilterClass();
};
MetaClass *museNoisyClipFilter::GetMetaClass() {return Meta;};

//------------ Light Filter
class museLightFilterClass;
class museLightFilter : public museOutputFilterBase
{
   long FilterLeft;
   long FilterRight;

   public:

   static museLightFilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) ;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In);
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In);
   museLightFilter() : FilterLeft(0), FilterRight(0) {};
};

class museLightFilterClass : public museOFilterClass
{
   public:

   virtual const char *GetTypeName() {return "Light High Reduction 1";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museLightFilter::Meta)
         return true;
      return museOFilterClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museLightFilter;};
   virtual const char *GetName() {return "museLightFilter";};

   virtual void Link() {};
   museLightFilterClass();
};
MetaClass *museLightFilter::GetMetaClass() {return Meta;};

//----------- Light-2 Filter
class museLight2FilterClass;
class museLight2Filter : public museOutputFilterBase
{
   long FilterLeft;
   long FilterRight;
   public:

   static museLight2FilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) ;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In);
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   museLight2Filter() : FilterLeft(0), FilterRight(0) {};
};

class museLight2FilterClass : public museOFilterClass
{
   public:

   virtual const char *GetTypeName() {return "Light High Reduction 2";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museLight2Filter::Meta)
         return true;
      return museOFilterClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museLight2Filter;};
   virtual const char *GetName() {return "museLight2Filter";};

   virtual void Link() {};
   museLight2FilterClass();
};
MetaClass *museLight2Filter::GetMetaClass() {return Meta;};

//-----------Heavy Deep 1
class museHeavyDeepFilterClass;
class museHeavyDeepFilter : public museOutputFilterBase
{
   long FilterLeft;
   long FilterRight;
   long PFilterLeft;
   long PFilterRight;
   public:

   static museHeavyDeepFilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) ;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In);
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   museHeavyDeepFilter() : FilterLeft(0), FilterRight(0), PFilterLeft(0), PFilterRight(0) {};
};

class museHeavyDeepFilterClass : public museOFilterClass
{
   public:

   virtual const char *GetTypeName() {return "Heavy High Reduction 1";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museHeavyDeepFilter::Meta)
         return true;
      return museOFilterClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museHeavyDeepFilter;};
   virtual const char *GetName() {return "museHeavyDeepFilter";};

   virtual void Link() {};
   museHeavyDeepFilterClass();
};
MetaClass *museHeavyDeepFilter::GetMetaClass() {return Meta;};

//---------- Heavy Deep 2
class museHeavyDeep2FilterClass;
class museHeavyDeep2Filter : public museOutputFilterBase
{
   long FilterLeft;
   long FilterRight;
   long PFilterLeft;
   long PFilterRight;
   public:

   static museHeavyDeep2FilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) ;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In);
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   museHeavyDeep2Filter() : FilterLeft(0), FilterRight(0), PFilterLeft(0), PFilterRight(0) {};
};

class museHeavyDeep2FilterClass : public museOFilterClass
{
   public:

   virtual const char *GetTypeName() {return "Heavy High Reduction 2";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museHeavyDeep2Filter::Meta)
         return true;
      return museOFilterClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museHeavyDeep2Filter;};
   virtual const char *GetName() {return "museHeavyDeep2Filter";};

   virtual void Link() {};
   museHeavyDeep2FilterClass();
};
MetaClass *museHeavyDeep2Filter::GetMetaClass() {return Meta;};

//--------- Heavy Deep 3
class museHeavyDeep3FilterClass;
class museHeavyDeep3Filter : public museOutputFilterBase
{
   long FilterLeft;
   long FilterRight;
   long PFilterLeft;
   long PFilterRight;
   long P2FilterLeft;
   long P2FilterRight;
   public:

   static museHeavyDeep3FilterClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   // 32 bit stereo filters
   virtual void Filter16(unsigned short *Out,unsigned short *OutEnd,signed long *In) ;
   virtual void Filter8(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   // 32 bit mono filters
   virtual void Filter16m(unsigned short *Out,unsigned short *OutEnd,signed long *In);
   virtual void Filter8m(unsigned char *Out,unsigned char *OutEnd,signed long *In);

   museHeavyDeep3Filter() : FilterLeft(0), FilterRight(0), PFilterLeft(0), PFilterRight(0), P2FilterLeft(0), P2FilterRight(0) {};
};

class museHeavyDeep3FilterClass : public museOFilterClass
{
   public:

   virtual const char *GetTypeName() {return "Very Heavy High Reduction";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museHeavyDeep3Filter::Meta)
         return true;
      return museOFilterClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museHeavyDeep3Filter;};
   virtual const char *GetName() {return "museHeavyDeep3Filter";};

   virtual void Link() {};
   museHeavyDeep3FilterClass();
};
MetaClass *museHeavyDeep3Filter::GetMetaClass() {return Meta;};

#endif
