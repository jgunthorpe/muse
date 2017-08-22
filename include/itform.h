// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   ITFormat - Muse file format to handle IT files.
   
   ##################################################################### */
									/*}}}*/
#ifndef ITFORM_H
#define ITFORM_H

#ifndef FORMATBS_H
#include <formatbs.h>
#endif

struct ITHeader;
struct ITInstrument;
struct IT2Instrument;
struct ITSample;
struct ITPattern;

class museITFormatClass;

class museITFormat : public museFormatBase
{
   octet  *FileLoc;
   unsigned long FileSize;
   ITHeader *Header;
   octet *Orders;
   ITInstrument **Instruments;
   IT2Instrument **Instruments2;
   ITSample **Samples;
   ITPattern **Patterns;
   octet *TempPatMem;
   long MaxNoOfChannels;
   bool Private;
   
   octet ChannelMask[64];

   unsigned long Glissando (long Frequency, long FineTune);
   public:
   
   static museITFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Play(museOutputBase *Device,musePlayerControl *);
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);
   
   virtual void Free();

   museITFormat();
};

class museITFormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new museITFormat;};
   virtual const char *GetName() {return "museITFormat";};

   museITFormatClass();
};

inline MetaClass *museITFormat::GetMetaClass() {return Meta;};

#endif
