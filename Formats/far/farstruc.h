#pragma pack(1)
struct FARHeader 
{
   char Sig[4];
   char Name[40];
   char Majic[3];
   unsigned short HeaderSize;
   unsigned char Version;
   unsigned char ChannelMap[16];
   unsigned char EditInfo[9];
   unsigned char Tempo;
   unsigned char Pan[16];
   unsigned char EditInfo2[4];
   unsigned short TextLength;
};

struct FAROrders 
{
   unsigned char Orders[256];
   unsigned char NoOfPatterns;
   unsigned char NoOfOrders;
   unsigned char LoopLocation;
   unsigned short PatternLength[256];
};

struct FARRow 
{
   unsigned char Note;
   unsigned char Instrument;
   unsigned char Volume;
   unsigned char Effect;
};

struct FARPattern 
{
   unsigned char Rows;
   unsigned char TempoUnused;
   FARRow Data;
};

struct FARSample 
{
   char Name[32];
   unsigned long Length;
   unsigned char FineTune;
   unsigned char Volume;
   unsigned long LoopBegin;
   unsigned long LoopEnd;
   unsigned char Type;
   unsigned char LoopMode;
};

struct CmdInfoFAR 
{
   int Note;
   int Flags;
   int Sample;
   int Pitch;
   int Volume;
   int PortaTarget;
   int PortaSpeed;
   int RetrigCount;
};

#define EFF_None            0
#define EFF_SlideUp         0x1
#define EFF_SlideDown       0x2
#define EFF_Porta           0x3
#define EFF_MultiRetrig     0x4
#define EFF_FineTempoUp     0xE
#define EFF_FineTempoDown   0xD
#define EFF_Tempo           0xF

#define MAXNOCHANS 16

#pragma pack()
