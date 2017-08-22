/* ########################################################################

   32-Bit Flat mixer

   ########################################################################
*/
#ifndef M32Flat_H
#define M32Flat_H

#ifndef MIX_H
#include <mix.h>
#endif

// 32 bit mix buffer, flat mixing style.
class museMixer32Flat : public museMixerBase
{
   protected:

   struct MixChannel
   {
      signed long LastMain;
      signed long LastRight;
      signed long LastLeft;
   };

   Sequence<MixChannel> Chns32;

   signed long LastMain;
   signed long LastRight;
   signed long LastLeft;

   signed long *MixBuffer;
   unsigned long MixBSize;

   public:

   virtual long DoMix(unsigned long Len);
   virtual void Reset();
   virtual void StopChan(int Chan);
   virtual void StartChan(int Chan);
   virtual void Channels(int I) ;
   virtual signed long ComputeSConst(unsigned long Percent,unsigned long Chans);

   virtual long MixStereoInto(long *Start,long *End,unsigned long Bytes) = 0;
   virtual long MixMonoInto(long *Start,long *End,unsigned long Bytes) = 0;
   void FillDeadSt(signed long *MStart, signed long *MEnd);
   void FillDeadMono(signed long *MStart, signed long *MEnd);
   virtual void DumpBuffer();

   museMixer32Flat();
   ~museMixer32Flat();
};

// Original muse mixer
#ifdef __WATCOMC__
#define SYSCALL _System
#else
#define SYSCALL
#endif

typedef struct
{
   signed long L;
   signed long R;
} VolMap;

typedef void (SYSCALL *museMixFunc)(signed long *MCur,signed long *End,
                                    unsigned long Incr,unsigned long CurIncr,
                                    unsigned long Add,unsigned char *PlayPos,
                                    VolMap *VMap);

// Defines a set of asm mixing routines for a specified incr range
#define MAXMIXERS 6
struct museMix32Elm
{
   float LowIncr;
   float HighIncr;
   int Bit16;

   museMixFunc Forward;
   museMixFunc Reverse;
   museMixFunc ChipForward;
   museMixFunc ChipReverse;
};

class museMixerOrgClass;
class museMixerOrg : public museMixer32Flat
{
   protected:
   struct MixChannel
   {
      unsigned long Safe;
      unsigned long SafeB;
      unsigned char *PlayLast;

      unsigned long CurIncr16;
      unsigned long Incr16;
      int Mixer;

      long Volumes[256*2+8];
   };
   Sequence<MixChannel> ChnsOrg;

   static void SYSCALL MixCPP(signed long *MCur,signed long *End,
                              unsigned long Incr,unsigned long CurIncr,
                              unsigned long Add,unsigned char *PlayPos,
                              VolMap *VMap);
   static void SYSCALL MixCPP16(signed long *MCur,signed long *End,
                              unsigned long Incr,unsigned long CurIncr,
                              unsigned long Add,unsigned char *PlayPos,
                              VolMap *VMap);
   static void SYSCALL MixCPPR(signed long *MCur,signed long *End,
                              unsigned long Incr,unsigned long CurIncr,
                              unsigned long Add,unsigned char *PlayPos,
                              VolMap *VMap);
   static void SYSCALL MixCPP16R(signed long *MCur,signed long *End,
                              unsigned long Incr,unsigned long CurIncr,
                              unsigned long Add,unsigned char *PlayPos,
                              VolMap *VMap);
   static museMix32Elm Mixers[MAXMIXERS];

   public:
   static museMixerOrgClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual void ChanVolume(int Chan);
   virtual void ChanPitch(int Chan);
   virtual void ChanInst(int Chan);
   virtual void StartChan(int Chan);
   virtual void StopChan(int Chan);
   virtual void Channels(int I);
   virtual void Reset();

   virtual long MixStereoInto(long *Start,long *End,unsigned long Bytes);
   virtual long MixMonoInto(long *Start,long *End,unsigned long Bytes);

   museMixerOrg();
   ~museMixerOrg();
};

class museMixerOrgClass : public museMixerClass
{
   public:

   virtual const char *GetTypeName() {return "32 bit Mixer";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museMixerOrg::Meta)
         return true;
      return museMixerClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museMixerOrg;};
   virtual const char *GetName() {return "museMixerOrg";};

   virtual void Link() {};
   museMixerOrgClass();
};
MetaClass *museMixerOrg::GetMetaClass() {return Meta;};

typedef void (SYSCALL *museIntMixFunc)(signed long *MCur,signed long *End,
                                    unsigned long Incr,unsigned long CurIncr,
                                    unsigned long Add,unsigned char *PlayPos,
                                    VolMap *VMap,
                       signed long First,signed long Prev,unsigned char *Last);

class museMixerIntClass;
class museMixerInt : public museMixerOrg
{
   protected:
   struct MixChannel
   {
      museIntMixFunc Foreward;
      museIntMixFunc Reverse;
      museIntMixFunc Cur;
      unsigned long LastPlayPos;
   };
   Sequence<MixChannel> ChnsInt;

   public:
   static museMixerIntClass *Meta;
   inline virtual MetaClass *GetMetaClass();

   virtual void StartChan(int Chan);
   virtual void StopChan(int Chan);
   virtual void Channels(int I);
   virtual void Reset();

   virtual long MixStereoInto(long *Start,long *End,unsigned long Bytes);
   virtual long MixMonoInto(long *Start,long *End,unsigned long Bytes);

   museMixerInt();
   ~museMixerInt();
};

class museMixerIntClass : public museMixerClass
{
   public:

   virtual const char *GetTypeName() {return "32 bit Interpolated Mixer";};

   virtual bool DescendedFrom(MetaClass *Base)
   {
      if (Base == museMixerInt::Meta)
         return true;
      return MetaClass::DescendedFrom(Base);
   }
   virtual MetaObject *New() {return new museMixerInt;};
   virtual const char *GetName() {return "museMixerInt";};

   virtual void Link() {};
   museMixerIntClass();
};

MetaClass *museMixerInt::GetMetaClass() {return Meta;};

#endif
