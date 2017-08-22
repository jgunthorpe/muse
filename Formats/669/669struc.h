#pragma pack(1)
struct F669Header 
{
   unsigned short Sig;
   char SongMessage[108];
   unsigned char NoOfSamples;
   unsigned char NoOfPatterns;
   unsigned char LoopOrder;
   unsigned char OrderList[128];
   unsigned char TempoList[128];
   unsigned char BreakList[128];
};

struct F669Sample 
{
   char FileName[13];
   unsigned long Length;
   unsigned long LoopBegin;
   unsigned long LoopEnd;
   unsigned char *SampleData;
};

struct _669Row 
{
   unsigned char Data[3];
};

struct F669Pattern 
{
   _669Row Rows[64][8];
};

struct CmdInfo669 
{
   int Flags;
   int Pitch;
   int BasePitch;
   int PortaTarget;
   int Vibrato;
   int PanPos;
   int Volume;
   int Sample;
   int LastCommand;
};

#define EFF_SlideUp     0
#define EFF_SlideDown   1
#define EFF_Porta       2
#define EFF_FreqAdjust  3
#define EFF_Vibrato     4
#define EFF_SetTempo    5

#define MAXNOCHANS 8

#pragma pack()
