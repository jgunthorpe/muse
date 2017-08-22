// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   FARFormat - Muse file format to handle FAR files.
   
   ##################################################################### */
									/*}}}*/
#ifndef FARFORM_H
#define FARFORM_H

#ifndef FORMATBS_H
#include <formatbs.h>
#endif

struct FARHeader;
struct FARPattern;
struct FARSample;
struct FAROrders;

class museFARFormatClass;

class museFARFormat : public museFormatBase
{
   FARHeader *Header;
   FAROrders *Orders;
   FARPattern **Patterns;
   FARSample **Samples;
   unsigned long *SampleMap;
   unsigned char Private;

   unsigned long Glissando ( long Frequency, long FineTune );
   public:
   static museFARFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Play(museOutputBase *Device,musePlayerControl *Control);
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);
   
   virtual void Free();
   long GetMarkers(SequenceLong *List);

   museFARFormat();
};

class museFARFormatClass : public museFormatClass
{
   public:

   virtual const char *GetTypeName();
   virtual museFormatClass *GetClassForFile(const char *FName);

   virtual unsigned long GetScanSize();
   virtual unsigned long Scan(octet *Region, museSongInfo &Info);

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFormatBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museFARFormat;};
   virtual const char *GetName() {return "museFARFormat";};

   museFARFormatClass();
};

inline MetaClass *museFARFormat::GetMetaClass() {return Meta;};

#endif
