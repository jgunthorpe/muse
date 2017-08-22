// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   ULTFormat - Muse file format to handle ULT files.
   
   ##################################################################### */
									/*}}}*/
#ifndef ULTFORM_H
#define ULTFORM_H

#ifndef FORMATBS_H
#include <formatbs.h>
#endif

struct ULTHeader;
struct ULTPatternHeader;
struct ULTSample;
struct ULTData;

class museULTFormatClass;

class museULTFormat : public museFormatBase
{
   ULTHeader *Header;
   ULTPatternHeader *PatHeader;
   ULTSample *Samples;
   ULTData *HeaderData;
   octet **SampleData;
   octet **Patterns;
   char *Comment;
   long UltType;
   unsigned char Private;

   public:
   static museULTFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Play(museOutputBase *Device,musePlayerControl *Control);
   virtual void Free();
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);

   museULTFormat();
};

class museULTFormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new museULTFormat;};
   virtual const char *GetName() {return "museULTFormat";};

   museULTFormatClass();
};

inline MetaClass *museULTFormat::GetMetaClass() {return Meta;};

#endif
