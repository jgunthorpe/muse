#pragma pack(1)
struct XMHeader 
{
   char IDText[17];
   char Name[20];
   char EOFMarker;
   char Tracker[20];
   unsigned short Version;
   unsigned long HeaderSize;
   unsigned short SongLength;
   unsigned short RestartPos;
   unsigned short NoOfChannels;
   unsigned short NoOfPatterns;
   unsigned short NoOfInstruments;
   unsigned short Flags;
   unsigned short InitialTempo;
   unsigned short InitialBPM;
   char OrderList[256];
};

struct PatternHeader 
{
   unsigned long HeaderSize;
   unsigned char Type;
   unsigned short NoOfRows;
   unsigned short PatternDataSize;
   unsigned char *PatternData;
};

struct SampleHeader 
{
   unsigned long Length;
   unsigned long LoopStart;
   unsigned long LoopEnd;
   unsigned char Volume;
   signed char FineTune;
   unsigned char Flags;
   unsigned char Pan;
   signed char NoteAdjust;
   unsigned char Reserved;
   char Name[22];
   unsigned char *SampleData;
};

struct InstrumentHeader 
{
   unsigned long Size;
   char Name[22];
   unsigned char Type;
   unsigned short NoOfSamples;
   unsigned long HeaderSize;
   unsigned char NoteMap[96];
   short VolumeEnvelope[24];
   short PanEnvelope[24];
   unsigned char VolumePoints;
   unsigned char PanPoints;
   unsigned char VolumeSustain;
   unsigned char VolumeLoopStart;
   unsigned char VolumeLoopEnd;
   unsigned char PanSustain;
   unsigned char PanLoopStart;
   unsigned char PanLoopEnd;
   unsigned char VolumeFlags;
   unsigned char PanFlags;
   unsigned char VibratoType;
   unsigned char VibratoSweep;
   unsigned char VibratoDepth;
   unsigned char VibratoRate;
   unsigned short FadeOut;
   unsigned short Reserved;
   SampleHeader *Samples;
};

struct CmdInfoXM 
{
   int Note;
   int NoteParm;
   int InstrumentParm;
   int VolumeParm;
   int EffectParm;
   int EffectDataParm;
   int FineTune;
   int Flags;
   int Volume;
   int Period;
   int VolEnvelopePos;
   int PanEnvelopePos;
   int PortaTarget;
   int PeriodShift;
   int VolumeShift;
   int VibratoType;
   int VibratoSize;
   int VibratoPos;
   int VibratoSpeed;
   int TremoloType;
   int TremoloSize;
   int TremoloPos;
   int TremoloSpeed;
   int PanPos;
   int PanShift;
   int ArpeggioCounter;
   int TimeCounter;
   int TimeOff;
   int KeyState;
   int RetrigCounter;
   
   // These define the sample in various ways
   int CurInstrument;            // XM instrument number
   unsigned long Sample;         // Mixer sample Number 
                                 //  SampleMappings[CurInstrument][SubSampleNo]
   unsigned char SubSampleNo;    // XM Sub Sample number 
                                 //  InstData->NoteMap[Note-1]
   SampleHeader *SubSample;      // Sub Sample Pointer 
                                 //  InstData->Sample[SampleNo]
   InstrumentHeader *InstData;   // XM instrument header 
                                 //  InstrumentData[CurInstrument]
};


#define EFF_Arpeggio              0
#define EFF_SlideUp               1
#define EFF_SlideDown             2
#define EFF_Porta                 3
#define EFF_Vibrato               4
#define EFF_DualPorta             5
#define EFF_DualVibrato           6
#define EFF_Tremolo               7
#define EFF_Pan                   8
#define EFF_SetSampleOffset       9
#define EFF_VolumeSlide           'A' - 'A' + 10
#define EFF_SetOrder              'B' - 'A' + 10
#define EFF_SetVolume             'C' - 'A' + 10
#define EFF_PatternBreak          'D' - 'A' + 10
#define EFF_Special               'E' - 'A' + 10
#define EFF_SetSpeed              'F' - 'A' + 10
#define EFF_SetGlobalVolume       'G' - 'A' + 10
#define EFF_GlobalVolumeSlide     'H' - 'A' + 10
#define EFF_KeyOff                'K' - 'A' + 10
#define EFF_SetEnvelopePosition   'L' - 'A' + 10
#define EFF_PanSlide              'P' - 'A' + 10
#define EFF_MultiRetrig           'R' - 'A' + 10
#define EFF_Tremor                'T' - 'A' + 10
#define EFF_ExtraFine             'X' - 'A' + 10

#define EFF_FinePortaUp           1
#define EFF_FinePortaDown         2
#define EFF_SetGlissando          3
#define EFF_SetVibrato            4
#define EFF_SetFineTune           5
#define EFF_Loop                  6
#define EFF_Tremolo               7
#define EFF_Retrig                9
#define EFF_FineVolumeSlideUp     'A' - 'A' + 10
#define EFF_FineVolumeSlideDown   'B' - 'A' + 10
#define EFF_NoteCut               'C' - 'A' + 10
#define EFF_NoteDelay             'D' - 'A' + 10
#define EFF_PatternDelay          'E' - 'A' + 10

#define EFF_ExtraFinePortaUp      1
#define EFF_ExtraFinePortaDown    2

#define VOL_None                  0x0
#define VOL_VolumeSlideDown       0x6
#define VOL_VolumeSlideUp         0x7
#define VOL_FineVolumeSlideDown   0x8
#define VOL_FineVolumeSlideUp     0x9
#define VOL_Vibrato               0xA
#define VOL_SetVibratoSpeed       0xB
#define VOL_SetPanning            0xC
#define VOL_PanSlideLeft          0xD
#define VOL_PanSlideRight         0xE
#define VOL_Portamento            0xF

#define BASEFREQ 844 * 32
#define MAXNOCHANS 32

#pragma pack()
