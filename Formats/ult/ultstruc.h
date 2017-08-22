#pragma pack(1)
struct ULTHeader 
{
   char IDText[15];
   char Name[32];
   char CommentLength;
};

struct ULTData 
{
   unsigned char NoOfSamples;
};

struct ULTSample 
{
   char Name[32];
   char SampleName[12];
   unsigned long LoopStart;
   unsigned long LoopEnd;
   unsigned long SizeStart;
   unsigned long SizeEnd;
   unsigned char Volume;
   unsigned char Flags;
   signed short FineTune;
   unsigned short BaseFrequency;
};

struct OldULTSample 
{
   char Name[32];
   char SampleName[12];
   unsigned long LoopStart;
   unsigned long LoopEnd;
   unsigned long SizeStart;
   unsigned long SizeEnd;
   unsigned char Volume;
   unsigned char Flags;
   signed short FineTune;
};

struct ULTPatternHeader 
{
   unsigned char Orders[256];
   unsigned char NoOfChannels;
   unsigned char NoOfPatterns;
   unsigned char PanPosTable;
};

struct CmdInfoULT 
{
   int Note;
   int Sample;
   int Volume;
   int VolumeShift;
   int StartVolume;
   int PanPos;
   int Period;
   int PeriodShift;
   int PortaTarget;
       
   int NoteParm;
   int InstrumentParm;
   int Effect1;
   int Effect2;
   int EffectParm1;
   int EffectParm2;
   int Flags;
       
   int VibratoType;
   int VibratoPos;
   int TremoloType;
   int TremoloPos;
   int VibratoSpeed;
   int VibratoSize;
   int TremoloSpeed;
   int TremoloSize;
   int ArpeggioCounter;
};

struct LArgType
{
   unsigned char L[32][36];
};

typedef struct
{
   unsigned char C[32][5];
} CPatType;

#define FLAG_16Bit 0x04
#define FLAG_Loop 0x08
#define FLAG_Bidir 0x10


#define EFF_None                 0x00
#define EFF_SlideUp              0x01
#define EFF_SlideDown            0x02
#define EFF_Porta                0x03
#define EFF_Vibrato              0x04
#define EFF_SpecialA             0x05
#define EFF_Tremolo              0x07
#define EFF_SetSampleOffset      0x09
#define EFF_SetFineSampleOffset  0x99
#define EFF_VolumeSlide          0x0A
#define EFF_SetPanPos            0x0B
#define EFF_SetVolume            0x0C
#define EFF_PatternBreak         0x0D
#define EFF_Special              0x0E
#define EFF_SetSpeed             0x0F

#define EFF_NoCommand 0
#define EFF_NoLoopForwards 1
#define EFF_NoLoopBackwards 2
#define EFF_StopLoop 0xC

#define EFF_SetVibratoValue 0
#define EFF_FineSlideUp 1
#define EFF_FineSlideDown 2
#define EFF_PatternDelay 8
#define EFF_Retrig 9
#define EFF_FineVolumeSlideUp 'A' - 'A' + 10
#define EFF_FineVolumeSlideDown 'B' - 'A' + 10
#define EFF_NoteCut 'C' - 'A' + 10
#define EFF_NoteDelay 'D' - 'A' + 10

#define MAXNOCHANS 32

#pragma pack()
