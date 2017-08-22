// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   XMFormat - Muse file format to handle XM files.

   ##################################################################### */
									/*}}}*/
#ifndef XMFORM_H
#define XMFORM_H

#ifndef FORMATBS_H
#include <formatbs.h>
#endif

struct XMHeader;
struct PatternHeader;
struct InstrumentHeader;

class museXMFormatClass;
class museXMFormat : public museFormatBase
{
   XMHeader *Header;                   // Header block
   InstrumentHeader *InstrumentData;   // Pointer to array of instruments
   PatternHeader *PatternData;         // Array of patterns
   unsigned long NoOfSamples;
   unsigned short **SampleMappings;

   bool Private;
   
   unsigned long museXMFormat::Glissando(long Frequency,long FineTune);
   
   public:
   static museXMFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Play(museOutputBase *Device,musePlayerControl *Control);
   virtual void Free();
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);

   museXMFormat();
};

class museXMFormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new museXMFormat;};
   virtual const char *GetName() {return "museXMFormat";};

   museXMFormatClass();
};

inline MetaClass *museXMFormat::GetMetaClass() {return Meta;};

#endif
