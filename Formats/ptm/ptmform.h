// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   PTMFormat - Muse file format to handle PTM files.
   
   ##################################################################### */
									/*}}}*/
#ifndef PTMFORM_H
#define PTMFORM_H

#ifndef FORMATBS_H
#include <formatbs.h>
#endif

struct PTMHeader;
struct PTMInstrument;

class musePTMFormatClass;

class musePTMFormat : public museFormatBase
{
   PTMHeader *Header;
   PTMInstrument **Instruments;
   unsigned char **Patterns;
   unsigned char *FileLoc;
   bool Private;
   
   long musePTMFormat::Glissando(long Frequency, long FineTune);
   
   public:
   static musePTMFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);

   virtual long Play(museOutputBase *Device,musePlayerControl *Control);

   virtual void Free();

   musePTMFormat();
};

class musePTMFormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new musePTMFormat;};
   virtual const char *GetName() {return "musePTMFormat";};

   musePTMFormatClass();
};

inline MetaClass *musePTMFormat::GetMetaClass() {return Meta;};

#endif
