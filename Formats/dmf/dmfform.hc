/* ########################################################################

   DMF - DMF File Format

   ########################################################################
*/
#ifndef DMFFORMAT_HC
#define DMFFORMAT_HC

#ifndef SOM_HC
#include <SOM.hc>
#endif

#ifndef FORMATBS_HC
#include <FormatBs.hc>
#endif

struct DMFHeader;
struct DMFInfo;
struct DMFMessage;
struct DMFSequence;
struct DMFPatternHeader;
struct DMFPattern;
struct DMFInstrumentRange;
struct DMFInstrument;
struct DMFInstrumentHeader;
struct DMFSampleInfo;
struct DMFSampleInfo2;
struct DMFSampleInfoHeader;
struct DMFSample;
struct DMFSampleHeader;
struct DMFEndHeader;

class museDMFFormatClass;
class museDMFFormat : public museFormatBase
{
   DMFHeader *Header;
   DMFInfo *Info;
   DMFMessage *Message;
   DMFSequence *Sequence;
   DMFPatternHeader *PatternHeader;
   DMFPattern **Patterns;
   DMFInstrumentHeader *InstrumentHeader;
   DMFInstrument **Instruments;
   DMFInstrumentRange ***Ranges;
   DMFSampleInfoHeader *SampleInfoHeader;
   DMFSampleInfo2 **SampleInfos2;
   DMFSampleInfo **SampleInfos;
   DMFSampleHeader *SampleHeader;
   DMFSample **Samples;
   unsigned char *TempPatternData;
   unsigned char **TempSamples;

   unsigned char Private;
   
   unsigned char *DecompressADPCM(unsigned char *Data,int Length);
   long Glissando(long Frequency, long BaseNote);

   public:
   static museDMFFormatClass *__ClassObject;
   inline virtual SOMClass *somGetClass();

   virtual long LoadMemModule(octet *Region,unsigned long Size);
   virtual long LoadMemModuleKeep(octet *Region,unsigned long Size);

   virtual unsigned short GetNumPatterns();
   virtual unsigned short GetNumOrders();
   virtual unsigned short GetRowsAPattern();
   virtual string GetTitle();
   virtual unsigned short GetNumChannels();
   virtual void GetSongSamples(SequenceSample &Samples);

   virtual long Play(museOutputBase *Device,musePlayerControl *Control);
   virtual string GetTypeName();

   virtual void Free();
   long GetMarkers(SequenceLong *List);

   museDMFFormat();
};

class museDMFFormatClass : public museFormatClass
{
   public:

   virtual string GetTypeName();
   virtual museFormatClass *GetClassForFile(string FName);

   virtual unsigned long GetScanSize();
   virtual unsigned long Scan(octet *Region, museSongInfo *Info);

   virtual boolean somDescendedFrom(SOMClass *Base)
   {
      if (Base == museFormatBase::__ClassObject)
         return TRUE;
      return SOMClass::somDescendedFrom(Base);
   }
   virtual SOMObject *somNew() {return new museDMFFormat;};
   virtual string somGetName() {return "museDMFFormat";};

   museDMFFormatClass();
};

inline SOMClass *museDMFFormat::somGetClass() {return __ClassObject;};

#endif
