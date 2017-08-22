// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   669Format - Muse file format to handle 669 files.

   ##################################################################### */
									/*}}}*/
#ifndef F669FORM_H
#define F669FORM_H

#ifndef FORMATBS_H
#include <formatbs.hc>
#endif

struct F669Header;
struct F669Pattern;
struct F669Sample;

class muse669FormatClass;

class muse669Format : public museFormatBase
{
   F669Header *Header;
   F669Pattern **PatternData;
   F669Sample *SampleData;
   unsigned long Glissando (long Frequency,long FineTune);
   unsigned char Private;

   public:
   static muse669FormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Play(museOutputBase *Device,musePlayerControl *Control);
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);
   
   virtual void Free();

   muse669Format();
};

class muse669FormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new muse669Format;};
   virtual const char *GetName() {return "muse669Format";};

   muse669FormatClass();
};

inline MetaClass *muse669Format::GetMetaClass() {return Meta;};

#endif
