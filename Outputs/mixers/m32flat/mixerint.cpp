/* ########################################################################

    This file contains the interpolated 32 bit flat mixer

   ########################################################################
*/
#include <Muse.h>
#include <M32Flat.hc>

extern "C"
{
void SYSCALL MixIntAsmR(signed long *MCur,signed long *End,unsigned long Incr,
                        unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap,
                        signed long First,signed long Prev,unsigned char *Last);
void SYSCALL MixIntAsm(signed long *MCur,signed long *End,unsigned long Incr,
                       unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap,
                       signed long First,signed long Prev,unsigned char *Last);

void SYSCALL MixIntAsmR_Si(signed long *MCur,signed long *End,unsigned long Incr,
                           unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap,
                           signed long First,signed long Prev,unsigned char *Last);
void SYSCALL MixIntAsm_Si(signed long *MCur,signed long *End,unsigned long Incr,
                          unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap,
                          signed long First,signed long Prev,unsigned char *Last);

void SYSCALL MixIntAsmR_16(signed long *MCur,signed long *End,unsigned long Incr,
                           unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap,
                           signed long First,signed long Prev,unsigned char *Last);
void SYSCALL MixIntAsm_16(signed long *MCur,signed long *End,unsigned long Incr,
                          unsigned long CurIncr,unsigned long Add,unsigned char *PlPos,VolMap *VMap,
                          signed long First,signed long Prev,unsigned char *Last);
}

museMixerInt::museMixerInt()
{
   ChnsInt.construct();
};

museMixerInt::~museMixerInt()
{
   ChnsInt.free();
}

void museMixerInt::Channels(int I)
{
   museMixerOrg::Channels(I);
   ChnsInt.free();
   ChnsInt[I-1].Cur = 0;
}

void museMixerInt::Reset()
{
   ChnsInt.free();
}

void museMixerInt::StartChan(int Chan)
{
   museMixerOrg::StartChan(Chan);

   museMixerInt::MixChannel &IChn = ChnsInt[Chan];
   museDACMixer::MixChannel &MChn = Owner->Chns[Chan];

   // Select a mixer
   if (MChn.Bit16 == true)
   {
      IChn.Reverse = MixIntAsmR_16;
      IChn.Foreward = MixIntAsm_16;
   }
   else
   {
      if (MChn.Signed == true)
      {
         IChn.Reverse = MixIntAsmR_Si;
         IChn.Foreward = MixIntAsm_Si;
      }
      else
      {
         IChn.Reverse = MixIntAsmR;
         IChn.Foreward = MixIntAsm;
      }
   }

   IChn.Cur = IChn.Foreward;
}

void museMixerInt::StopChan(int Chan)
{
   museMixerOrg::StopChan(Chan);
}

long museMixerInt::MixStereoInto(long *Start,long *End,unsigned long Bytes)
{
   museDACMixer::MixChannel *MChn = Owner->Chns.begin();
   museDACMixer::MixChannel *MChnE = Owner->Chns.end();
   museMixer32Flat::MixChannel *FChn = Chns32.begin();
   museMixerOrg::MixChannel *OChn = ChnsOrg.begin();
   museMixerInt::MixChannel *IChn = ChnsInt.begin();

   // Mix Each channel
   for (; MChn != MChnE; MChn++, FChn++, OChn++, IChn++)
   {
      if (MChn->Playing == 0)
         continue;

       VolMap *V = (VolMap *)&OChn->Volumes[8];

      // Fill the Mix buffer with the channel
      for (signed long *MCur = Start;MCur < End;)
      {
         // Zero Incr -- Propogate LastByte for the rest of sample
         if (OChn->Incr16 == 0)
         {
            printf("0 incr\n");
            break;
         }

         if (((signed long)OChn->Safe) < 0)
            printf("Woah\n");

         unsigned long Distance = min(End - MCur,OChn->SafeB);

         // Record these things for after loops have been calculated
         museIntMixFunc Cur = IChn->Cur;
         unsigned char *PlayPos = MChn->PlayPos;
         unsigned long CurIncr = MChn->CurIncr;

         if (OChn->SafeB != 0)
         {
            // Recompute CurIncr and PlayPos
            unsigned long Pos = OChn->CurIncr16 + OChn->Incr16*(Distance/2);
            MChn->CurIncr = Pos << 16;
            OChn->CurIncr16 = Pos & 0xFFFF;
            OChn->SafeB -= Distance;
            OChn->Safe -= Pos >> 16;

            // Forward
            if (MChn->PingPong < 10)
            {
               if (MChn->PlayPos > MChn->LoopEnd || MChn->PlayPos < MChn->PlayStart)
                  printf("Range: %u,%u,%u\n",MChn->PlayPos,MChn->LoopEnd,MChn->PlayStart);

               // Change Play Counter
               if (MChn->Bit16 == false)
               {
                  OChn->PlayLast = MChn->PlayPos + ((Pos-OChn->Incr16) >> 16);
                  IChn->LastPlayPos = (unsigned long) (Pos-OChn->Incr16);
                  MChn->PlayPos += (Pos >> 16);
               }
               else
               {
                  OChn->PlayLast = MChn->PlayPos + ((Pos-OChn->Incr16) >> 16)*2;
                  IChn->LastPlayPos = (unsigned long) (Pos-OChn->Incr16) * 2;
                  MChn->PlayPos += (Pos >> 16)*2;
               }
            }
            else
            {
               // Reverse
               if (MChn->PlayPos < MChn->LoopEnd || MChn->PlayPos < MChn->PlayStart)
                  printf("Range: %u,%u,%u\n",MChn->PlayPos,MChn->LoopEnd,MChn->PlayStart);

               // Change play counters
               if (MChn->Bit16 == false)
               {
                  OChn->PlayLast = MChn->PlayPos - ((Pos-OChn->Incr16) >> 16);
                  IChn->LastPlayPos = (unsigned long) -(Pos-OChn->Incr16);
                  MChn->PlayPos -= (Pos >> 16);
               }
               else
               {
                  OChn->PlayLast = MChn->PlayPos - ((Pos-OChn->Incr16) >> 16)*2;
                  IChn->LastPlayPos = (unsigned long) -(Pos-OChn->Incr16) * 2;
                  MChn->PlayPos -= (Pos >> 16)*2;
               }
            }
         }

         // 16/8 bit steping
         int Size;
         if (MChn->Bit16 == true)
            Size = 2;
         else
            Size = 1;

         unsigned char *Last = OChn->PlayLast + Size;

         // Readjust Last to compensate for loop hits
//         if (MChn->LoopStart != 0 && OChn->SafeB == 0)
         {
            if (MChn->PingPong == 0)
            {
               if (MChn->LoopStart && Last >= MChn->LoopEnd && MChn->LoopEnd + 1 >= MChn->EndPos)
                   Last = MChn->LoopStart;
            }

            if (MChn->PingPong == 1)
            {
                if (Last >= MChn->EndPos && MChn->LoopEnd + 1 >= MChn->EndPos)
                   Last = MChn->EndPos - Size;
            }
            if (MChn->PingPong == 2)
            {
               Last = OChn->PlayLast - Size;
               if (Last <= MChn->LoopEnd && MChn->LoopEnd - 1 <= 0)
                  Last = MChn->LoopEnd;
            }
            if (MChn->PingPong == 3)
            {
               Last = OChn->PlayLast - Size;
               if (Last <= MChn->LoopEnd && MChn->LoopEnd - 1 <= 0)
                  Last = MChn->EndPos;
            }
         }

/*         if (MChn->PingPong != 0)
            printf("%u,%u,%u,%u,%u,%u\n",MChn->PingPong,Last,OChn->PlayLast,
                                MChn->PlayPos,MChn->LoopStart,MChn->EndPos);*/
         // Achieved end of sample, loop or deactivate the channel.
         if (OChn->SafeB == 0)
         {
            /* Recompute safe to see if the sample really ended or if we
               just passed 64k */
            if (MChn->PingPong < 2)
               OChn->Safe = bound(0,(signed long)(MChn->LoopEnd - MChn->PlayPos),0xFFF0);
            else
               OChn->Safe = bound(0,(signed long)(MChn->PlayPos - MChn->LoopEnd),0xFFF0);
            if (MChn->Bit16 == true)
               OChn->Safe /= 2;

            if (OChn->Safe > 0)
            {
               OChn->SafeB = (((OChn->Safe << 16) - OChn->CurIncr16 + OChn->Incr16 - 1)/OChn->Incr16)*2;
               goto DoMix;
            }

            if (MChn->LoopStart != 0)
            {
               if (MChn->PingPong == 0)
               {
                  if (MChn->PlayPos < MChn->LoopEnd)
                  {
                     MChn->PlayPos = MChn->LoopStart;
                     printf("##\n");
                  }
                  else
                     MChn->PlayPos = MChn->LoopStart + ((signed long)(MChn->PlayPos - MChn->LoopEnd));
                  OChn->Safe = bound(0,(signed long)(MChn->LoopEnd - MChn->PlayPos),0xFFF0);
               }
               else
               {
                  MChn->PlayPos = MChn->LoopEnd - ((signed long)(MChn->PlayPos - MChn->LoopEnd));
                  if (MChn->PingPong == 1)
                  {
                     if (MChn->Bit16 == false)
                        MChn->PlayPos--;
                     else
                        MChn->PlayPos -= 2;
                     MChn->LoopEnd = MChn->LoopStart;
                     MChn->PingPong = 2;

                     IChn->Cur = IChn->Reverse;
                     OChn->Safe = bound(0,(signed long)(MChn->PlayPos - MChn->LoopEnd),0xFFF0);
                  }

                  if (MChn->PingPong == 2)
                  {
                     MChn->LoopEnd = MChn->EndPos;
                     MChn->PingPong = 1;

                     IChn->Cur = IChn->Foreward;
                     OChn->Safe = bound(0,(signed long)(MChn->LoopEnd - MChn->PlayPos),0xFFF0);
                  }

                  if (MChn->PingPong == 3)
                  {
                     MChn->PlayPos = MChn->EndPos;
                     OChn->Safe = bound(0,(signed long)(MChn->PlayPos - MChn->LoopEnd),0xFFF0);
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
            }
         }
         if (MChn->PingPong == 3)
            IChn->Cur = IChn->Reverse;

         // The Generic one
DoMix:
         if ((V[-1].L != 0 || V[-1].R != 0) && Distance != 0)
         {
             // End of mix volume
            if (MCur + Distance >= End)
            {
               if (MChn->Bit16 == true)
               {
                  V[-2].L = MChn->MV[2].L/0x100;
                  V[-2].R = MChn->MV[2].R/0x100;
               }
               else
               {
                  V[-2].L = MChn->MV[2].L;
                  V[-2].R = MChn->MV[2].R;
               }
            }
            else
               V[-2] = V[-1];

            // Last mix volume
            if (MCur == Start)
            {
               if (MChn->Bit16 == true)
               {
                  V[-3].L = MChn->MV[0].L/0x100;
                  V[-3].R = MChn->MV[0].R/0x100;
               }
               else
               {
                  V[-3].L = MChn->MV[0].L;
                  V[-3].R = MChn->MV[0].R;
               }
            }
            else
              V[-3] = V[-1];

            if (MChn->LoopStart && Last >= MChn->LoopEnd && MChn->LoopEnd + 1 >= MChn->EndPos)
               Last = MChn->LoopStart;

//            printf ( "%x,%x %x,%x\n",OChn->PlayLast,Last,MChn->LoopStart,MChn->LoopEnd);
            if (MChn->Playing == 0)
            {
               if ( OChn->PlayLast == 0 ) printf ( "Uh-oh 1...\n" );
               Cur(MCur,MCur + Distance,MChn->Incr,CurIncr,MChn->Add,PlayPos,V,(0xFFFF-CurIncr)/OChn->Incr16,((IChn->LastPlayPos)&65535)/OChn->Incr16,OChn->PlayLast);
            } else {                                                                                // /OChn->Incr16
               if ( Last == 0 ) printf ( "Uh-oh 2...\n" );
               Cur(MCur,MCur + Distance,MChn->Incr,CurIncr,MChn->Add,PlayPos,V,(0xFFFF-CurIncr)/OChn->Incr16,((IChn->LastPlayPos)&65535)/OChn->Incr16,Last);
            }
         }
         MCur += Distance;

         // Cut it
         if (MChn->Playing == 0)
            break;
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

long museMixerInt::MixMonoInto(long *Start,long *End,unsigned long Bytes)
{
   return 1;
}

#ifdef CPPCOMPILE
#include <HandLst.hc>
museMixerIntClass *museMixerInt::Meta = new museMixerIntClass;
museMixerIntClass::museMixerIntClass()
{
   MajorVersion = 1;
   MinorVersion = 0;

   museHandlerList::Meta->AddMixer(this);
}
#endif
