#pragma pack(1)
struct ITHeader 
{
   unsigned char Sig[4];
   char SongName[26];
   unsigned char Discard1[2];
   unsigned short NoOfOrders;
   unsigned short NoOfInstruments;
   unsigned short NoOfSamples;
   unsigned short NoOfPatterns;
   unsigned short Version;
   unsigned short Compatability;
   unsigned short Flags;
   unsigned char Discard2[2];
   unsigned char GlobalVolume;
   unsigned char MixVolume;
   unsigned char InitialSpeed;
   unsigned char InitialTempo;
   unsigned char InitialPanning;
   unsigned char Discard3[1];
   unsigned short MsgLen;
   unsigned long MsgOffset;
   unsigned char Discard4[4];
   unsigned char ChannelPan[64];
   unsigned char ChannelVolume[64];
};

struct MinITHeader 
{
   unsigned char Sig[4];
   char SongName[26];
   unsigned char Discard1[2];
   unsigned short NoOfOrders;
   unsigned short NoOfInstruments;
   unsigned short NoOfSamples;
   unsigned short NoOfPatterns;
   unsigned short Version;
   unsigned short Compatability;
   unsigned short Flags;
};

struct Envelope 
{
   unsigned char Position;
   unsigned char Value;
};

struct ITInstrument 
{
   unsigned char Sig[4];
   char SampleName[13];
   unsigned char Flags;
   unsigned char VolumeLoopStart;
   unsigned char VolumeLoopEnd;
   unsigned char SustainLoopStart;
   unsigned char SustainLoopEnd;
   unsigned char Discard1[2];
   unsigned short FadeOut;
   unsigned char NewNoteAction;
   unsigned char DuplicateNoteCheck;
   unsigned char Discard2[3];
   unsigned char NoOfSamples;
   char Name[26];
   unsigned char Discard3[6];
   unsigned char Mapping[120][2];
   unsigned char VolumeMap[200];
   Envelope VolumeEnvelope[25];
};

struct Node 
{
    signed char Value;
    unsigned short Position;
};

struct Envelope2 
{
   unsigned char Flags;
   unsigned char Points;
   unsigned char LoopBegin;
   unsigned char LoopEnd;
   unsigned char SustainLoopBegin;
   unsigned char SustainLoopEnd;
   Node Nodes[25];
   unsigned char Discard;
};

struct IT2Instrument 
{
   unsigned char Sig[4];
   char SampleName[13];
   unsigned char NewNoteAction;
   unsigned char DuplicateCheckType;
   unsigned char DuplicateCheckAction;
   unsigned short FadeOut;
   signed char PitchPanSeparation;
   unsigned char PitchPanCentre;
   unsigned char GlobalVolume;
   unsigned char DefaultPan;
   unsigned char RandomVolume;
   unsigned char RandomPan;
   unsigned char Discard2[4];
   char Name[26];
   unsigned char Discard3[6];
   unsigned char Mapping[120][2];
   Envelope2 VolumeEnvelope;
   Envelope2 PanEnvelope;
   Envelope2 PitchEnvelope;
   unsigned char Discard4[7];
};

struct ITSample 
{
   unsigned char Sig[4];
   char SampleName[12];
   unsigned char Discard1[1];
   unsigned char GlobalVolume;
   unsigned char Flags;
   unsigned char Volume;
   char Name[26];
   unsigned short Convert;
   unsigned long Length;
   unsigned long LoopBegin;
   unsigned long LoopEnd;
   unsigned long BaseFrequency;
   unsigned long SustainLoopBegin;
   unsigned long SustainLoopEnd;
   unsigned long SamplePointer;
   unsigned char VibratoSpeed;
   unsigned char VibratoDepth;
   unsigned char VibratoRate;
   unsigned char VibratoType;
};

struct ITPattern 
{
   unsigned short Length;
   unsigned short Rows;
   char Discard[4];
   unsigned char Data;
};

struct Counter 
{
   unsigned char Sample;
   unsigned char Count;
   unsigned char Background;
   signed long MinVol;
   signed short Channel;
};

#define PATTERNCHANS 64

struct CmdInfoIT 
{
   int Note;
   int Period;
   int PeriodShift;
   int PortaTarget;
   int Sample;
   int Instrument;
   int Volume;
   int VolumeShift;
   int PanPos;
   int PanShift;
   int PanbrelloShift;
   int PitchPanPos;
   
   int ChannelVolume;
   int VolEnvelope;
   int VolEnvelopePos;
   int PanEnvelope;
   int PanEnvelopePos;
   int PitchEnvelope;
   int PitchEnvelopePos;
   
   int Flags;
   int KeyState;
   int FadeOut;
   int LastPorta;
   int ArpeggioCounter;
   int RetrigCounter;
   int NewNoteAction;
   
   int Parent;
   int Child;
   
   int VibratoType;
   int VibratoPos;
   int VibratoSpeed;
   int VibratoSize;
   int TremoloType;
   int TremoloPos;
   int TremoloSpeed;
   int TremoloSize;
   int PanbrelloType;
   int PanbrelloPos;
   int PanbrelloSpeed;
   int PanbrelloSize;
   int VibratoTotal;
   int VibratoCounter;
   
   int Channel;
   int Muted;
   char Channels[PATTERNCHANS];
};

#define PATTERN_EndOfRow    0x00
#define PATTERN_NewMask     0x80
#define PATTERN_NewNote     0x01
#define PATTERN_NewInst     0x02
#define PATTERN_NewVol      0x04
#define PATTERN_NewCommand  0x08
#define PATTERN_LastNote    0x10
#define PATTERN_LastInst    0x20
#define PATTERN_LastVol     0x40
#define PATTERN_LastCommand 0x80

//           The following effects 'memorise' their previous values:
//            (D/K/L), (E/F/G), H, I, J, N, O, S, U, W

#define EFF_None            0
#define EFF_SetSpeed        ( 'A' - 'A' ) + 1
#define EFF_SetOrder        ( 'B' - 'A' ) + 1
#define EFF_BreakRow        ( 'C' - 'A' ) + 1
#define EFF_VolSlide        ( 'D' - 'A' ) + 1
#define EFF_SlideDown       ( 'E' - 'A' ) + 1
#define EFF_SlideUp         ( 'F' - 'A' ) + 1
#define EFF_Porta           ( 'G' - 'A' ) + 1
#define EFF_Vibrato         ( 'H' - 'A' ) + 1
#define EFF_Tremor          ( 'I' - 'A' ) + 1
#define EFF_Arpeggio        ( 'J' - 'A' ) + 1
#define EFF_DualVibrato     ( 'K' - 'A' ) + 1
#define EFF_DualPorta       ( 'L' - 'A' ) + 1
#define EFF_SetChanVolume   ( 'M' - 'A' ) + 1
#define EFF_SlideChanVolume ( 'N' - 'A' ) + 1
#define EFF_SetSampleOffset ( 'O' - 'A' ) + 1
#define EFF_PanSlide        ( 'P' - 'A' ) + 1
#define EFF_MultiRetrig     ( 'Q' - 'A' ) + 1
#define EFF_Tremolo         ( 'R' - 'A' ) + 1
#define EFF_Special         ( 'S' - 'A' ) + 1
#define EFF_SetTempo        ( 'T' - 'A' ) + 1
#define EFF_FineVibrato     ( 'U' - 'A' ) + 1
#define EFF_SetGlobalVol    ( 'V' - 'A' ) + 1
#define EFF_SlideGlobalVol  ( 'W' - 'A' ) + 1
#define EFF_SetPanPos       ( 'X' - 'A' ) + 1
#define EFF_PanBrello       ( 'Y' - 'A' ) + 1

#define EFF_SPECIAL_SetVibratoWave   3
#define EFF_SPECIAL_SetTremoloWave   4
#define EFF_SPECIAL_SetPanbrelloWave 5
#define EFF_SPECIAL_PastNote         7
#define EFF_SPECIAL_SetPanPos        8
#define EFF_SPECIAL_SetSurround      9
#define EFF_SPECIAL_PatternLoop      0xB
#define EFF_SPECIAL_NoteCut          0xC
#define EFF_SPECIAL_NoteDelay        0xD
#define EFF_SPECIAL_PatternDelay     0xE

#define EFF_SPECIAL_PASTNOTE_NoteCut     0
#define EFF_SPECIAL_PASTNOTE_NoteOff     1
#define EFF_SPECIAL_PASTNOTE_NoteFade    2
#define EFF_SPECIAL_PASTNOTE_NNACut      3
#define EFF_SPECIAL_PASTNOTE_NNACont     4
#define EFF_SPECIAL_PASTNOTE_NNAOff      5
#define EFF_SPECIAL_PASTNOTE_NNAFade     6
#define EFF_SPECIAL_PASTNOTE_VolEnvOff   7
#define EFF_SPECIAL_PASTNOTE_VolEnvOn    8
#define EFF_SPECIAL_PASTNOTE_PanEnvOff   9
#define EFF_SPECIAL_PASTNOTE_PanEnvOn    10
#define EFF_SPECIAL_PASTNOTE_PitchEnvOff 11
#define EFF_SPECIAL_PASTNOTE_PitchEnvOn  12

#define NNA_Cut       0
#define NNA_Continue  1
#define NNA_Noteoff   2
#define NNA_Notefade  3

#define DCT_Off         0
#define DCT_Note        1
#define DCT_Sample      2
#define DCT_Instrument  3

#define DCA_Cut      0
#define DCA_Noteoff  1
#define DCA_Notefade 2

#define CHAN_Surround (1 << 30)

#define MAXNOCHANS 256
#define PATTERNCHANS 64
#define SLIDESPEED 768.0

#pragma pack()
