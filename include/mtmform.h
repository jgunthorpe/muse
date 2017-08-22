// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MTMFormat - Muse file format to handle MTM files.

   The MTM format is basicaly a Super-MOD file. It claims to use exactly
   the same command set, we convert it to Protracker on load/play.
   
   ##################################################################### */
									/*}}}*/
#ifndef MTMFORM_H
#define MTMFORM_H

#ifndef MODGEN_H
#include <modgen.h>
#endif

struct MTMHeader;
struct MTMInstrument;

class museMTMFormat;
class museMTMFormatClass;

class museMTMFormat : public museMODGeneric
{
   MTMHeader *Header;
   MTMInstrument *SampleData;
   octet **Samples;
   octet *TrackData;
   unsigned short *PatternData;
   octet *OrderData;
   char *CommentData;
   unsigned char Private;

   public:
   static museMTMFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);
   
   virtual unsigned short GetRowsAPattern();
   virtual long GetRowElements(SequenceMODElement *Elements,unsigned long Row,unsigned long Pattern);
   virtual void GetOrderList(unsigned char **List,unsigned long *Count);
   virtual void GetMODPan(unsigned char *Pan);
   virtual octet GetInitValues(octet *Speed,octet *Tempo,octet *GlobalVol);

   virtual void Free();

   museMTMFormat();
};

class museMTMFormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new museMTMFormat;};
   virtual const char *GetName() {return "museMTMFormat";};

   museMTMFormatClass();
};

inline MetaClass *museMTMFormat::GetMetaClass() {return Meta;};
#endif
