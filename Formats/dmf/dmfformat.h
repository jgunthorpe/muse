typedef struct museSongSample
{
  octet *Sample;
  unsigned long Flags;
  unsigned long LoopBegin;
  unsigned long LoopEnd;
  unsigned long SampleEnd;
  string Name;
  string SampleName;
  int InstNo;
  unsigned long Center;
  octet Volume;
} museSongSample;
typedef Sequence<museSongSample> SequenceSongSample;

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

class museOutputBase;
class museDMFFormat
{
  public :

     virtual unsigned short GetNumPatterns();
     virtual unsigned short GetNumOrders();
     virtual unsigned short GetRowsAPattern();
     virtual string GetTitle();
     virtual unsigned short GetNumChannels();
     virtual void GetSongSamples(SequenceSongSample* Samples);
     virtual long LoadMemModule(octet* Region, unsigned long Size);
     virtual long Play(museOutputBase *);
     virtual long Glissando (long Frequency,long FineTune);

     museDMFFormat();
     ~museDMFFormat();

  private :
     void Free();

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

};

