#pragma pack(1)

struct PTMHeader 
{
   char Name[28];
   char EOFMarker;
   unsigned short Version;
   unsigned char Reserved;
   unsigned short NoOfOrders;
   unsigned short NoOfInstruments;
   unsigned short NoOfPatterns;
   unsigned short NoOfChannels;
   unsigned short FileFlags;
   unsigned short Reserved2;
   char Sig[4];
   unsigned char Reserved3[16];
   unsigned char Panning[32];
   unsigned char Orders[256];
   unsigned short Patterns[128];
};

struct MinPTMHeader 
{
   char Name[28];
   char EOFMarker;
   unsigned short Version;
   unsigned char Reserved;
   unsigned short NoOfOrders;
   unsigned short NoOfInstruments;
   unsigned short NoOfPatterns;
   unsigned short NoOfChannels;
   unsigned short FileFlags;
   unsigned short Reserved2;
   char Sig[4];
};

struct PTMInstrument 
{
   unsigned char Flags;
   char FileName[12];
   unsigned char Volume;
   unsigned short BaseFrequency;
   unsigned short Internal1;
   unsigned long SampleOffset;
   unsigned long Length;
   unsigned long LoopBegin;
   unsigned long LoopEnd;
   unsigned long Internal2;
   unsigned long Internal3;
   unsigned long Internal4;
   unsigned char Internal5;
   unsigned char Reserved;
   char Name[28];
   unsigned long Sig;
};

struct CmdInfoPTM 
{
   int NoteParm;
   int InstrumentParm;
   int VolumeParm;
   int EffectParm;
   int EffectDataParm;

   int Note;
   int Period;
   int PeriodShift;
   int Instrument;
   int Volume;
   int VolumeShift;
   int PanPos;
   int Flags;
   int ArpeggioCounter;
   int RetrigCounter;
   int LastPorta;
   int PortaTarget;
   int BaseFrequency;

   int VibratoType;
   int VibratoPos;
   int VibratoSpeed;
   int VibratoSize;
   int TremoloType;
   int TremoloPos;
   int TremoloSpeed;
   int TremoloSize;
};

#define PATTERN_EndOfRow    0x00
#define PATTERN_Note        0x20
#define PATTERN_Effect      0x40
#define PATTERN_Volume      0x80

#define EFF_Arpeggio        0
#define EFF_SlideDown       1
#define EFF_SlideUp         2
#define EFF_Porta           3
#define EFF_Vibrato         4
#define EFF_DualPorta       5
#define EFF_DualVibrato     6
#define EFF_Tremolo         7
#define EFF_SetSampleOffset 9
#define EFF_VolumeSlide     ( 'A' - 'A' + 10 )
#define EFF_SetOrder        ( 'B' - 'A' + 10 )
#define EFF_SetVolume       ( 'C' - 'A' + 10 )
#define EFF_BreakRow        ( 'D' - 'A' + 10 )
#define EFF_Special         ( 'E' - 'A' + 10 )
#define EFF_SetSpeed        ( 'F' - 'A' + 10 )
#define EFF_SetGlobalVolume ( 'G' - 'A' + 10 )
#define EFF_MultiRetrig     ( 'H' - 'A' + 10 )
#define EFF_FineVibrato     ( 'I' - 'A' + 10 )
#define EFF_NoteSlideDown   ( 'J' - 'A' + 10 )
#define EFF_NoteSlideUp     ( 'K' - 'A' + 10 )
#define EFF_NoteSlideDownR  ( 'L' - 'A' + 10 )
#define EFF_NoteSlideUpR    ( 'M' - 'A' + 10 )

#define EFF_Special_FineSlideDown       1
#define EFF_Special_FineSlideUp         2
#define EFF_Special_SetVibratoWave      4
#define EFF_Special_SetFinetune         5
#define EFF_Special_PatternLoop         6
#define EFF_Special_SetTremoloWave      7
#define EFF_Special_SetPanPosition      8
#define EFF_Special_RetriggerNote       9
#define EFF_Special_FineVolumeSlideUp   0xA
#define EFF_Special_FineVolumeSlideDown 0xB
#define EFF_Special_NoteCut             0xC
#define EFF_Special_NoteDelay           0xD
#define EFF_Special_PatternDelay        0xE

const unsigned short FineTuneTable[16] = { 7895, 7941, 7985, 8046, 8107, 8169, 8232, 8280,
                                           8363, 8413, 8463, 8529, 8581, 8651, 8723, 8757 };

const unsigned char EffectMap[32] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1 };

#define BASEFREQ 856 * 32
#define MAXNOCHANS 32

#pragma pack()
