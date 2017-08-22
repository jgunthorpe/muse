#pragma pack(1)
struct DMFHeader {
char Sig[4];
unsigned char Version;
char Tracker[8];
char SongName[30];
char Composer[20];
unsigned char Date[3];
};

struct DMFInfo {
char Sig[4];
unsigned long HeaderSize;
};

struct DMFMessage {
char Sig[4];
unsigned long HeaderSize;
unsigned char Filler;
char Message;
};

struct DMFSequence {
char Sig[4];
unsigned long HeaderSize;
unsigned short LoopStart;
unsigned short LoopEnd;
unsigned short Orders;
};

struct DMFPatternHeader {
char Sig[4];
unsigned long HeaderSize;
unsigned short NoOfPatterns;
unsigned char NoOfChannels;
};

struct DMFPattern {
unsigned char Channels;
unsigned char Beat;
unsigned short Rows;
unsigned long HeaderSize;
unsigned char PatternData;
};

struct DMFInstrumentRange {
unsigned char SampleNo;
unsigned char RangeLength;
};

struct DMFInstrument {
char Name[30];
unsigned char Flags;
unsigned char NoOfRanges;
};

struct DMFInstrumentHeader {
char Sig[4];
unsigned long HeaderSize;
unsigned char NoOfInstruments;
};

struct DMFSampleInfo {
unsigned char NameLength;
char Name;
};

struct DMFSampleInfo2 {
unsigned long Length;
unsigned long LoopStart;
unsigned long LoopEnd;
unsigned short Frequency;
unsigned char Volume;
unsigned char Flags;
unsigned short Filler;
unsigned int CRC32;
};

struct DMFSampleInfoHeader {
char Sig[4];
unsigned long HeaderSize;
unsigned char NoOfSamples;
};

struct DMFSample {
unsigned long SampleSize;
unsigned char SampleData;
};

struct DMFSampleHeader {
char Sig[4];
unsigned long HeaderSize;
};

struct DMFEndHeader {
char Sig[4];
};

#define PATTERN_VolumeEffect     0x02
#define PATTERN_NoteEffect       0x04
#define PATTERN_InstrumentEffect 0x08
#define PATTERN_Volume           0x10
#define PATTERN_Note             0x20
#define PATTERN_Instrument       0x40
#define PATTERN_Counter          0x80

#define INST_Sample              0x00
#define INST_MIDI                0x01
#define INST_FM                  0x02
#define INST_None                0x03
#define INST_Attack              0x04
#define INST_Sustain             0x08

#define SMPL_Loop                0x01
#define SMPL_16Bit               0x02
#define SMPL_PackType            0x04
#define SMPL_Unpacked            0x00
#define SMPL_PackType0           0x04
#define SMPL_PackType1           0x08
#define SMPL_PackType2           0x0C
#define SMPL_InDMF               0x80

#define EFF_G_None               0x00
#define EFF_G_SetTickSpeed       0x01
#define EFF_G_SetBPMSpeed        0x02
#define EFF_G_SetBeatSpeed       0x03
#define EFF_G_SetTickDelaySpeed  0x04
#define EFF_G_SetFlag            0x05

#define EFF_I_None               0x00
#define EFF_I_StopSample         0x01
#define EFF_I_StopSampleLoop     0x02
#define EFF_I_RestartSample      0x03
#define EFF_I_TriggerSample      0x04
#define EFF_I_TremoloRetrig      0x05
#define EFF_I_SetSampleOffset    0x06
#define EFF_I_ReverseSample      0x07
#define EFF_I_RewindSample       0x08

#define EFF_N_None               0x00
#define EFF_N_SetFinetune        0x01
#define EFF_N_NoteDelay          0x02
#define EFF_N_Arpeggio           0x03
#define EFF_N_SlideUp            0x04
#define EFF_N_SlideDown          0x05
#define EFF_N_Portamento         0x06
#define EFF_N_ScratchPortamento  0x07
#define EFF_N_VibratoSine        0x08
#define EFF_N_VibratoTriangular  0x09
#define EFF_N_VibratoSquare      0x0A
#define EFF_N_Tremolo            0x0B
#define EFF_N_NoteCut            0x0C

#define EFF_V_None               0x00
#define EFF_V_SlideUp            0x01
#define EFF_V_SlideDown          0x02
#define EFF_V_Tremolo            0x03
#define EFF_V_VibratoSine        0x04
#define EFF_V_VibratoTriangular  0x05
#define EFF_V_VibratoSquare      0x06
#define EFF_V_SetPan             0x07
#define EFF_V_SlidePanLeft       0x08
#define EFF_V_SlidePanRight      0x09
#define EFF_V_PanVibrato         0x0A
#pragma pack()
