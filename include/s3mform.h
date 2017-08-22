// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
/* ######################################################################

   S3MFormat - Muse file format to handle S3M files.
   
   ##################################################################### */
									/*}}}*/
#ifndef S3MFORMAT_H
#define S3MFORMAT_H

#ifndef FORMATBS_H
#include <formatbs.h>
#endif

struct S3MHeader;
struct S3MBoundInst;
struct S3MBoundPattern;

class museS3MFormat;
class museS3MFormatClass;

class museS3MFormat : public museFormatBase
{
   S3MHeader *Header;                  // Header block
   S3MBoundInst *Instruments;          // Pointer to array of instruments
   S3MBoundPattern *Patterns;          // Array of patterns
   octet *Orders;                      // Array of orders
   octet ChannelPan[32];               // Channel pan positions
   octet ChannelMask[32];              // Prescanned channel mask

   unsigned long PlayedBlocks;         // # of blocks played
   unsigned short GlobalVol;           // Global volume

   unsigned char Private;

   public:
   static museS3MFormatClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual long Keep();
   virtual void GetSongSamples(SequenceSample &Samples);
   virtual long Play(museOutputBase *Device,musePlayerControl *Control);
   virtual void Free();
   virtual long GetSongInfo(museSongInfo &Info);
   
   museS3MFormat();
};

class museS3MFormatClass : public museFormatClass
{
   public:

   virtual const char *GetTypeName();
   virtual museFormatClass *GetClassForFile(const char *FName);

   virtual unsigned long GetScanSize();
   virtual unsigned long Scan(octet *Region,museSongInfo &Info);

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museFormatBase::Meta)
         return true;
      return museFormatClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museS3MFormat;};
   virtual const char *GetName() {return "museS3MFormat";};

   museS3MFormatClass();
};

inline MetaClass *museS3MFormat::GetMetaClass() {return Meta;};

#endif
