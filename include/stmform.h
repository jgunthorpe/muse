// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   STMFormat - Muse file format to handle STM files.
   
   This is a quick STM->ProTracker loader for STM files. Since they are
   so uncommon they don't rate their own player.
   
   ##################################################################### */
									/*}}}*/
#ifndef STMFORM_H
#define STMFORM_H

#ifndef MODGEN_H
#include <modgen.h>
#endif

struct STMHeader;
struct STMInstrument;
struct STMRow;
struct STMPattern;

class museSTMFormatClass;
class museSTMFormat;

class museSTMFormat : public museMODGeneric
{
   STMHeader *Header;
   STMPattern *Patterns;
   octet **Samples;
   unsigned char Private;

   public:
   static museSTMFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);
   
   virtual long GetRowElements(SequenceMODElement *Elements,unsigned long Row,unsigned long Pattern);
   virtual void GetOrderList(unsigned char **List,unsigned long *Count);
   virtual void GetMODPan(unsigned char *Pan);
   virtual octet GetInitValues(octet *Speed,octet *Tempo,octet *GlobalVol);

   virtual void Free();

   museSTMFormat();
};

class museSTMFormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new museSTMFormat;};
   virtual const char *GetName() {return "museSTMFormat";};

   museSTMFormatClass();
};

inline MetaClass *museSTMFormat::GetMetaClass() {return Meta;};

#endif
