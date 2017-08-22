/* ########################################################################

   This file contains the original 32 bit flat mixer

   ########################################################################
*/
#include <muse.h>
#include <m32flat.h>

template class Sequence<museMixerOrg::MixChannel>;

museMixerOrgClass *museMixerOrg::Meta = new museMixerOrgClass;

#ifdef ASM
extern "C"
{
void SYSCALL MixAsm_00_03(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_03_05(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_05_10(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_10_20(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_20_xx(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);

void SYSCALL MixAsm_00_03_R(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_03_05_R(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_05_10_R(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_10_20_R(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
void SYSCALL MixAsm_20_xx_R(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap);
}
#endif

#ifdef ASM
museMix32Elm museMixerOrg::Mixers[MAXMIXERS] =
{
// Lower   Upper  16b            ASM Forward              ASM Reverse            Chip Forward             Chip Reverse
{      0,    -1.0, 1, museMixerOrg::MixCPP16, museMixerOrg::MixCPP16R, museMixerOrg::MixCPP16, museMixerOrg::MixCPP16R},
{      0, 1.0/3.0, 0,           MixAsm_00_03,          MixAsm_00_03_R,           MixAsm_20_xx,   museMixerOrg::MixCPPR},
{1.0/3.0, 1.0/2.0, 0,           MixAsm_03_05,          MixAsm_03_05_R,           MixAsm_20_xx,   museMixerOrg::MixCPPR},
{1.0/2.0,     1.0, 0,           MixAsm_05_10,          MixAsm_05_10_R,           MixAsm_20_xx,   museMixerOrg::MixCPPR},
{    1.0,     2.0, 0,           MixAsm_10_20,          MixAsm_10_20_R,           MixAsm_10_20,   museMixerOrg::MixCPPR},
{    2.0,    -1.0, 0,           MixAsm_20_xx,          MixAsm_20_xx_R,           MixAsm_20_xx,   museMixerOrg::MixCPPR}
};
#else
museMix32Elm museMixerOrg::Mixers[MAXMIXERS] =
{
// Lower   Upper  16b            ASM Forward              ASM Reverse            Chip Forward             Chip Reverse
{      0,    -1.0, 1, museMixerOrg::MixCPP16, museMixerOrg::MixCPP16R, museMixerOrg::MixCPP16, museMixerOrg::MixCPP16R},
{      0,    -1.0, 0,   museMixerOrg::MixCPP,   museMixerOrg::MixCPPR,   museMixerOrg::MixCPP,   museMixerOrg::MixCPPR},
{      0,       0, 0,                      0,                       0,                      0,                       0},
{      0,       0, 0,                      0,                       0,                      0,                       0},
{      0,       0, 0,                      0,                       0,                      0,                       0},
{      0,       0, 0,                      0,                       0,                      0,                       0}
};
#endif

museMixerOrg::museMixerOrg()
{
   ChnsOrg.construct();
};

museMixerOrg::~museMixerOrg()
{
   ChnsOrg.free();
}

void museMixerOrg::Channels(int I)
{
   museMixer32Flat::Channels(I);
   ChnsOrg.free();
   ChnsOrg[I-1].Volumes[0] = 0;
}

void museMixerOrg::Reset()
{
   ChnsOrg.free();
}

void museMixerOrg::ChanVolume(int Chan)
{
   VolMap *V = (VolMap *)&ChnsOrg[Chan].Volumes[8];
   museDACMixer::MixChannel *Cur = &Owner->Chns[Chan];

   if (Owner->MStereo == true)
   {
      if (Cur->Bit16 == true)
      {
         if (Cur->Signed == V[-4].L && Cur->MV[1].L/0x100 == V[-1].L && Cur->MV[1].R/0x100 == V[-1].R)
            return;

         V[-1].L = Cur->MV[1].L/0x100;
         V[-1].R = Cur->MV[1].R/0x100;
         V[-4].L = Cur->Signed;
         return;
      }
      else
      {
         if (Cur->Signed == V[-4].L && Cur->MV[1].L == V[-1].L && Cur->MV[1].R == V[-1].R)
            return;

         V[-1].L = Cur->MV[1].L;
         V[-1].R = Cur->MV[1].R;
         V[-4].L = Cur->Signed;
      }

      long DeltaL = Cur->MV[1].L;
      long DeltaR = Cur->MV[1].R;
      signed long *I = &ChnsOrg[Chan].Volumes[8];
/*      if (Chan == 0)
         printf("%u,%u %u,%u\n",DeltaL,DeltaR,Cur->V[1].L,Cur->V[1].R);*/
      if (Cur->Signed == true || Cur->Bit16 == true)
      {
         long CurL = 1;
         long CurR = 1;
         signed long *IE = I + 128*2;
         for (; I != IE; I += 2, CurL += DeltaL, CurR += DeltaR)
         {
            I[0] = CurL;
            I[1] = CurR;
         }

         CurL = (-128)*DeltaL;
         CurR = (-128)*DeltaR;
         IE = I + 128*2;
         for (; I != IE; I += 2, CurL += DeltaL, CurR += DeltaR)
         {
            I[0] = CurL;
            I[1] = CurR;
         }
      }
      else
      {
         long CurL = (-127)*DeltaL;
         long CurR = (-127)*DeltaR;
         signed long *IE = I + 256*2;
         for (; I != IE; I += 2, CurL += DeltaL, CurR += DeltaR)
         {
            I[0] = CurL;
            I[1] = CurR;
         }
      }
   }
   else
   {
/*      if (Cur->Signed == V[-2].L && Cur->VolMain == V[-1].L)
         return;
      V[-1].L = Cur->VolMain;
      V[-2].L = Cur->Signed;

      if (Cur->Signed == true)
         for (int I = 0; I < 256*2; I++,I2++)
            V[I].L = ((signed char)I2)*(-1)*Cur->VolMain*0x100;
      else
         for (int I = 0; I < 256; I++)
            V[I].L = (I - 128)*Cur->VolMain*0x100;*/
   }
};

void museMixerOrg::StartChan(int Chan)
{
   museMixerOrg::MixChannel &OChn = ChnsOrg[Chan];
   museDACMixer::MixChannel &MChn = Owner->Chns[Chan];
   museMixer32Flat::StartChan(Chan);

   if (MChn.PingPong < 10)
      OChn.Safe = bound(0,(signed long)(MChn.LoopEnd - MChn.PlayPos),0xFFF0);
   else
      OChn.Safe = bound(0,(signed long)(MChn.PlayPos - MChn.LoopEnd),0xFFF0);
   if (MChn.Bit16 == true)
      OChn.Safe /= 2;

   if (OChn.Safe > 0 && OChn.Incr16 != 0)
      OChn.SafeB = (((OChn.Safe << 16) - OChn.CurIncr16 + OChn.Incr16 - 1)/OChn.Incr16)*2;
   else
      OChn.SafeB = 0;
}

void museMixerOrg::StopChan(int Chan)
{
   museMixerOrg::MixChannel &OChn = ChnsOrg[Chan];
   museMixer32Flat::MixChannel &FChn = Chns32[Chan];
   museDACMixer::MixChannel &MChn = Owner->Chns[Chan];

   if (MChn.Playing == 0)
      return;

   if (OChn.PlayLast == 0)
   {
      museMixer32Flat::StopChan(Chan);
      return;
   }

   VolMap *V = (VolMap *)&OChn.Volumes[8];
   if (MChn.Bit16 == false)
   {
      unsigned char Byte = *OChn.PlayLast;
      FChn.LastLeft = V[Byte].L;
      FChn.LastRight = V[Byte].R;
   }
   else
   {
      signed short Byte = *(short *)OChn.PlayLast;
      FChn.LastLeft = V[-1].L*Byte;
      FChn.LastRight = V[-1].R*Byte;
   }

   museMixer32Flat::StopChan(Chan);
}

void museMixerOrg::ChanPitch(int Chan)
{
   museMixerOrg::MixChannel &OChn = ChnsOrg[Chan];
   museDACMixer::MixChannel &MChn = Owner->Chns[Chan];

   // Compute 16 bit incrs
   OChn.Incr16 = (MChn.Add << 16) + (MChn.Incr >> 16);
   OChn.CurIncr16 = MChn.CurIncr >> 16;

   // Compute safe number of bytes to travel
   // Incr16-1 has the effect of ceiling the value
   if (OChn.Safe > 0 && OChn.Incr16 != 0)
      OChn.SafeB = (((OChn.Safe << 16) - OChn.CurIncr16 + OChn.Incr16 - 1)/OChn.Incr16)*2;
   else
      OChn.SafeB = 0;

   float Incr = OChn.Incr16/((float)0xFFFF);
   int I;
   for (I = 0; I != MAXMIXERS; I++)
   {
      if (Mixers[I].Bit16 != MChn.Bit16)
         continue;
      if (Incr < Mixers[I].LowIncr)
         continue;
      if (Incr > Mixers[I].HighIncr && Mixers[I].HighIncr != -1)
         continue;
      OChn.Mixer = I;
      break;
   }
   if (I == MAXMIXERS)
      printf("Splat!\n");
}

void museMixerOrg::ChanInst(int/* Chan*/)
{
}

static unsigned char *Death;

long museMixerOrg::MixStereoInto(long *Start,long *End,unsigned long/*Bytes*/)
{
   museDACMixer::MixChannel *MChn = Owner->Chns.begin();
   museDACMixer::MixChannel *MChnE = Owner->Chns.end();
   museMixer32Flat::MixChannel *FChn = Chns32.begin();
   museMixerOrg::MixChannel *OChn = ChnsOrg.begin();

   // Mix Each channel
   for (; MChn != MChnE; MChn++, FChn++, OChn++)
   {
      if (MChn->Playing == 0)
         continue;

      VolMap *V = (VolMap *)&OChn->Volumes[8];

      // Fill the Mix buffer with the channel
      signed long *MCur;
      for (MCur = Start;MCur < End;)
      {
         // Zero Incr -- Propogate LastByte for the rest of sample
         if (OChn->Incr16 == 0)
         {
            printf("0 incr\n");
            break;
         }

         if (((signed long)OChn->Safe) < 0)
            printf("Woah\n");

         if (OChn->SafeB != 0)
         {
            unsigned long Distance = min(End - MCur,OChn->SafeB);

            if (MChn->PingPong < 10)
            {
//               if (MChn->PlayPos > MChn->LoopEnd || MChn->PlayPos < MChn->PlayStart)
//                  printf("Range: %p,%p,%p\n",MChn->PlayPos,MChn->LoopEnd,MChn->PlayStart);

               // This calls the actual mixer, 'chip' if the sample will end.
               if (V[-1].L != 0 || V[-1].R != 0)
               {
		  Death = MChn->LoopEnd;
                  if (OChn->SafeB > Distance)
                     Mixers[OChn->Mixer].Forward(MCur,MCur + Distance,MChn->Incr,
                                                MChn->CurIncr,MChn->Add,
                                                MChn->PlayPos,V);
                  else
                     Mixers[OChn->Mixer].ChipForward(MCur,MCur + Distance,MChn->Incr,
                                                MChn->CurIncr,MChn->Add,
                                                MChn->PlayPos,V);
               }

               MCur += Distance;

               // Recompute CurIncr and PlayPos
               unsigned long Pos = OChn->CurIncr16 + OChn->Incr16*(Distance/2);
               MChn->CurIncr = Pos << 16;
               OChn->CurIncr16 = Pos & 0xFFFF;
               OChn->SafeB -= Distance;
               OChn->Safe -= Pos >> 16;
	       
               // Change Play Counter
               if (MChn->Bit16 == false)
               {
                  OChn->PlayLast = MChn->PlayPos + ((Pos-OChn->Incr16) >> 16);
                  MChn->PlayPos += (Pos >> 16);
               }
               else
               {
                  OChn->PlayLast = MChn->PlayPos + ((Pos-OChn->Incr16) >> 16)*2;
                  MChn->PlayPos += (Pos >> 16)*2;
               }
            }
            else
            {
//               if (MChn->PlayPos < MChn->LoopEnd || MChn->PlayPos < MChn->PlayStart)
//                  printf("Range: %p,%p,%p\n",MChn->PlayPos,MChn->LoopEnd,MChn->PlayStart);

               // The Generic one
               if (V[-1].L != 0 || V[-1].R != 0)
               {
                  if (OChn->SafeB > Distance)
                     Mixers[OChn->Mixer].Reverse(MCur,MCur + Distance,MChn->Incr,
                                                MChn->CurIncr,MChn->Add,
                                                MChn->PlayPos,V);
                  else
                     Mixers[OChn->Mixer].ChipReverse(MCur,MCur + Distance,MChn->Incr,
                                                MChn->CurIncr,MChn->Add,
                                                MChn->PlayPos,V);
               }

               MCur += Distance;

               // Recompute CurIncr and PlayPos
               unsigned long Pos = OChn->CurIncr16 + OChn->Incr16*(Distance/2);
               MChn->CurIncr = Pos << 16;
               OChn->CurIncr16 = Pos & 0xFFFF;
               OChn->SafeB -= Distance;
               OChn->Safe -= Pos >> 16;

               // Change play counters
               if (MChn->Bit16 == false)
               {
                  OChn->PlayLast = MChn->PlayPos - ((Pos-OChn->Incr16) >> 16);
                  MChn->PlayPos -= (Pos >> 16);
               }
               else
               {
                  OChn->PlayLast = MChn->PlayPos - ((Pos-OChn->Incr16) >> 16)*2;
                  MChn->PlayPos -= (Pos >> 16)*2;
               }
            }
         }

         // Achieved end of sample, loop or deactivate the channel.
         if (OChn->SafeB == 0)
         {
            /* Recompute safe to see if the sample really ended or if we
               just passed 64k */
            if (MChn->PingPong < 10)
               OChn->Safe = bound(0,(signed long)(MChn->LoopEnd - MChn->PlayPos),0xFFF0);
            else
               OChn->Safe = bound(0,(signed long)(MChn->PlayPos - MChn->LoopEnd),0xFFF0);
            if (MChn->Bit16 == true)
               OChn->Safe /= 2;

            if (OChn->Safe > 0)
            {
               OChn->SafeB = (((OChn->Safe << 16) - OChn->CurIncr16 + OChn->Incr16 - 1)/OChn->Incr16)*2;
               continue;
            }

            if (MChn->LoopStart != 0)
            {
               if (MChn->PingPong == 0)
               {
		  // Internal correctness check.
                  if (MChn->PlayPos < MChn->LoopEnd)
		  {
		     /* This is neat, some songs have 16 bit samples with
		        loops that are an odd number of bytes, this triggers
			this but isn't an internal error. I -think- in that
  		        case that it would be proper to let this fall 
		        down, that will realign the loop onto a 2 byte
		        boundry. */
		     if (!(MChn->LoopEnd - MChn->PlayPos == 1 && 
			   MChn->Bit16 == true))
			printf("Loop Failure\n");
		  }
	       
		  MChn->PlayPos = MChn->LoopStart + ((signed long)(MChn->PlayPos - MChn->LoopEnd));
                  OChn->Safe = bound(0,(signed long)(MChn->LoopEnd - MChn->PlayPos),0xFFF0);
               }
               else
               {
		  MChn->PlayPos = MChn->LoopEnd - ((signed long)(MChn->PlayPos - MChn->LoopEnd));

		  while (1)
		  {
		     // Front ping pong hit
		     if (MChn->PingPong == 1)
		     {
			if (MChn->Bit16 == false)
			   MChn->PlayPos--;
			else
			   MChn->PlayPos -= 2;
			
			MChn->LoopEnd = MChn->LoopStart;
			MChn->PingPong = 10;

			OChn->Safe = bound(0,(signed long)(MChn->PlayPos - MChn->LoopEnd),0xFFF0);
			break;
		     }
		     
		     // Reverse was done before the first loop point was hit
		     if (MChn->PingPong == 2)
		     {
			MChn->PingPong = 11;
			MChn->LoopEnd = MChn->LoopStart;
			break;
		     }
		     
		     // Ping pong reverse mixer
		     if (MChn->PingPong == 10)
		     {		     
			MChn->LoopEnd = MChn->EndPos;
			MChn->PingPong = 1;
			
			OChn->Safe = bound(0,(signed long)(MChn->LoopEnd - MChn->PlayPos),0xFFF0);
			break;
		     }
		     
		     // Normal reverse mixing
		     if (MChn->PingPong == 11)
		     {
			MChn->PlayPos = MChn->EndPos;
			OChn->Safe = bound(0,(signed long)(MChn->PlayPos - MChn->LoopEnd),0xFFF0);
			break;
		     }

		     printf("PingPong failed\n");
		     break;
		  }
	       }
	       
               if (MChn->Bit16 == true)
                  OChn->Safe /= 2;

               if (OChn->Safe > 0)
                  OChn->SafeB = (((OChn->Safe << 16) - OChn->CurIncr16 + OChn->Incr16 - 1)/OChn->Incr16)*2;
            }
            else
            {
               StopChan(OChn-ChnsOrg.begin());
               MChn->Playing = 0;
               break;
            }
         }
      }

      /* Fill in any remaining bytes in the buffer by continuing the last
         point in the sample onwards. */
      for (;MCur < End; MCur += 2)
      {
         *MCur += FChn->LastLeft;
         MCur[1] += FChn->LastRight;
      }
   };
   return 0;
}

void SYSCALL museMixerOrg::MixCPP(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,
            unsigned char *PlayPos,VolMap *VMap)
{
   unsigned long MyIncr = (Add << 16) + (Incr >> 16);
   unsigned long MyCIncr = CurIncr >> 16;
   for (; MCur < End; MCur += 2)
   {
      register unsigned char Byte = *(PlayPos + (MyCIncr >> 16));
      MyCIncr += MyIncr;

      *MCur += VMap[Byte].L;
      MCur[1] += VMap[Byte].R;
   }
}

void SYSCALL museMixerOrg::MixCPPR(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,
            unsigned char *PlayPos,VolMap *VMap)
{
   unsigned long MyIncr = (Add << 16) + (Incr >> 16);
   unsigned long MyCIncr = CurIncr >> 16;
   for (; MCur < End; MCur += 2)
   {
      register unsigned char Byte = *(PlayPos - (MyCIncr >> 16));
      MyCIncr += MyIncr;

      *MCur += VMap[Byte].L;
      MCur[1] += VMap[Byte].R;
   }
}

void SYSCALL museMixerOrg::MixCPP16(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,
            unsigned char *PlPos,VolMap *VMap)
{
   unsigned long MyIncr = (Add << 16) + (Incr >> 16);
   unsigned long MyCIncr = CurIncr >> 16;
   signed short *PlayPos = (signed short *)PlPos;
   signed long VolLeft = VMap[-1].L;
   signed long VolRight = VMap[-1].R;

   for (; MCur < End; MCur += 2)
   {
      register signed long ByteS = *(PlayPos + (MyCIncr >> 16));
      MyCIncr += MyIncr;

      *MCur += ByteS*VolLeft;
      MCur[1] += ByteS*VolRight;
   }
}

void SYSCALL museMixerOrg::MixCPP16R(signed long *MCur,signed long *End,unsigned long Incr,
            unsigned long CurIncr,unsigned long Add,
            unsigned char *PlPos,VolMap *VMap)
{
   unsigned long MyIncr = (Add << 16) + (Incr >> 16);
   unsigned long MyCIncr = CurIncr >> 16;
   signed short *PlayPos = (signed short *)PlPos;
   signed long VolLeft = VMap[-1].L;
   signed long VolRight = VMap[-1].R;
   for (; MCur < End; MCur += 2)
   {
      register signed long Byte = *(PlayPos - (MyCIncr >> 16));
      MyCIncr += MyIncr;

      *MCur += Byte*VolLeft;
      MCur[1] += Byte*VolRight;
   }
}

long museMixerOrg::MixMonoInto(long */*Start*/,long */*End*/,unsigned long /*Bytes*/)
{
   return 1;
}

museMixerOrgClass::museMixerOrgClass()
{
   MajorVersion = 1;
   MinorVersion = 0;
}
