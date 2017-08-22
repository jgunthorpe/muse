// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   MODFormat - Muse file format to handle MOD files.

   This loads all the known variaties of mod files and pipes them into
   the ModGen player.
   
   WOW files are mearly 8 chan mods that have a M.K. sig and have the .WOW
   extension.
   
   ##################################################################### */
									/*}}}*/
#ifndef MODFORM_H
#define MODFORM_H

#ifndef MODGEN_H
#include <modgen.h>
#endif

struct MODHeader;
struct MODBoundPattern;

class museMODFormatClass;
class museMODFormat : public museMODGeneric
{
   MODHeader *Header;
   MODBoundPattern *Patterns;
   bool Samp15;
   unsigned short MODChans;
   unsigned short PatNum;
   octet **Samples;
   unsigned char Private;
   
   public:
   static museMODFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Keep();
   virtual long GetSongInfo(museSongInfo &Info);

   // Mod Generic stuff
   virtual long GetRowElements(SequenceMODElement *Elements,
			       unsigned long Row,unsigned long Pattern);
   virtual void GetOrderList(unsigned char **List,unsigned long *Count);
   virtual void GetMODPan(unsigned char *Pan);
   virtual octet GetInitValues(octet *Speed,octet *Tempo,octet *GlobalVol);

   virtual void Free();

   museMODFormat();
};

class museMODFormatClass : public museFormatClass
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
   virtual MetaObject *New() {return new museMODFormat;};
   virtual const char *GetName() {return "museMODFormat";};

   museMODFormatClass();
   museMODFormatClass(int) {};
};

inline MetaClass *museMODFormat::GetMetaClass() {return Meta;};

class museWOWFormatClass;
class museWOWFormat : public museMODFormat
{
   public:
   static museWOWFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
};

class museWOWFormatClass : public museMODFormatClass
{
   public:

   virtual const char *GetTypeName();
   virtual museFormatClass *GetClassForFile(const char *FName);

   virtual unsigned long Scan(octet *Region, museSongInfo &Info);

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFormatBase::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museWOWFormat;};
   virtual const char *GetName() {return "museWOWFormat";};

   museWOWFormatClass();
};

inline MetaClass *museWOWFormat::GetMetaClass() {return Meta;};

#endif
