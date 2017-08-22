/* ########################################################################

   Pipe - Manages the control Pipe

   ########################################################################
*/
#ifndef CPPCOMPILE
#ifndef DTS
#include <IDLSequence.xh>
#pragma pack(4)
#include <OutputBs.xh>
#include <FormatBs.xh>
#include <somcdev.h>
#include <EfxFiltr.xh>
#pragma pack()
#else
#include <IDLSequence.hh>
#include <OutputBs.hh>
#include <FormatBs.hh>
#include <EfxFiltr.hh>
#include <somcdev.hh>
#endif
#else
#include <IDLSeq.hc>
#include <OutputBs.hc>
#include <FormatBs.hc>
#include <EfxFiltr.hc>
#endif

#include <Flags.h>

#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_KBD
#define INCL_VIO
#include <os2.h>

#include <io.h>
#include <errno.h>
#include "Screen.h"
#include "Displays.h"

#define endl '\n'
class writer
{
   public:
   char S[10];
   int Error;
   int Handle;

   write(void *Data,unsigned long len)
   {
      ULONG Len = 0;
      Error = DosWrite(Handle,Data,len,&Len);
   }
//   writer &operator <<(const char *S) {write(S,strlen(S)); return *this;};
   writer &operator <<(char *S) {write(S,strlen(S)); return *this;};
   writer &operator <<(char S) {write(&S,1); return *this;};
   writer &operator <<(unsigned char S) {write(&S,1); return *this;};
   writer &operator <<(signed char S) {write(&S,1); return *this;};
   writer &operator <<(float a)
   {
      sprintf(S,"%.2f",a);
      (*this) << S;
      return *this;
   }
   writer &operator <<(unsigned long a)
   {
      sprintf(S,"%u",a);
      (*this) << S;
      return *this;
   }
   writer &operator <<(long a)
   {
      sprintf(S,"%u",a);
      (*this) << S;
      return *this;
   }
   writer &operator <<(unsigned int a)
   {
      sprintf(S,"%u",a);
      (*this) << S;
      return *this;
   }
   writer &operator <<(int a)
   {
      sprintf(S,"%u",a);
      (*this) << S;
      return *this;
   }
   writer &operator <<(short a)
   {
      sprintf(S,"%u",a);
      (*this) << S;
      return *this;
   }
   writer &operator <<(unsigned short a)
   {
      sprintf(S,"%u",a);
      (*this) << S;
      return *this;
   }
   void getline(char *S,int Max)
   {
      char *I = S;
      do
      {
         ULONG Len = 0;
         Error = DosRead(Handle,I,1,&Len);

         if (Error != 0 || Len != 1)
            break;

         if (*I != '\r')
            I++;
      }
      while (I == S || (I[-1] != '\n' && I[-1] != '\r'));
      *I = 0;
   }
};

/* ########################################################################

   Class - Screen (The display screen)
   Member - PipeThread (Manages the control pipe)

   Handles a global pipe, \pipe\ModulePlayer which allows other processes
   to control the player.

   ########################################################################
*/
void Screen::PipeThread()
{
   if (DosCreateNPipe("\\PIPE\\ModulePlayer",&PipeHandle,
       NP_ACCESS_DUPLEX | NP_NOINHERIT,1,512,512,0) != NO_ERROR)
   {
      somPrintf("Pipe open Error!");
      return;
   }
   unsigned long Pipe = PipeHandle;

   writer F;
   F.Handle = Pipe;
   while (1)
   {
      F.Error = 0;
      // Force the client off (if there is one) and re listen for a connect
      DosDisConnectNPipe(Pipe);

      if (DosConnectNPipe(Pipe) != 0)
         break;

      // Send connect message
      F << "Module Player Pipe Interface V1.0" << endl;
      F << "Muse/2 Text UI" << endl;

      while (F.Error == 0)
      {
         // Get the string and strip any junk
         char S[301];
         F.getline(S,sizeof(S) - 2);
         S[300] = 0;
         for (char *C = S; *C != 0; C++)
         {
            if (*C == '\n' || *C == '\r')
            {
               *C = 0;
               C++;
            }
         }

         // Hangup
         if (F.Error == 1) break;

         // Parse into 3 parts
         char *Command = strtok(S," ");
         char *SubCommand = strtok(0," ");
         char *Argument = 0;
         if (SubCommand != 0)
            Argument = strlen(SubCommand) + SubCommand + 1;
         char Okay = 1;
         while (Command != 0)
         {
            // Command is Exit
            if (stricmp(Command,"Exit") == 0)
            {
               Exit = 1;
               SongChange();
               break;
            }

            // Command is Query
            if (stricmp(Command,"Query") == 0 && SubCommand != 0)
            {
               if (stricmp(SubCommand,"Song") == 0)
               {
                  if (Song.Name == 0 || Module == 0)
                  {
                     F << "!! None Playing" << endl;
                     Okay = 0;
                     break;
                  }

                  // Write the song info
                  F << Song.Name << endl;
                  F << CurSong << endl;
                  F << Module->GetTitle() << endl;
                  break;
               }

               if (stricmp(SubCommand,"NextSong") == 0)
               {
                  if (Songs->List.size() == 0)
                  {
                     F << "!! None Remain" << endl;
                     Okay = 0;
                     break;
                  }

                  // Get the pointer to the next song
                  int Next = NextSong;
                  if (Changed == 0)
                     Next++;
                  if (Next >= Songs->List.size())
                     Next = Songs->List.size() - 1;

                  if (Songs->List[Next].Name == 0)
                  {
                     F << "!! None Remain" << endl;
                     Okay = 0;
                     break;
                  }

                  F << Songs->List[Next].Name << endl;
                  F << Next << endl;
                  break;
               }

               // Dump the song list
               if (stricmp(SubCommand,"SongList") == 0)
               {
                  if (Songs->List.size() == 0)
                  {
                     F << "!! Empty" << endl;
                     Okay = 0;
                     break;
                  }

                  F << Songs->List.size() << endl;
                  for (DirItem *I = Songs->List.begin(); I != Songs->List.end(); I++)
                     F << I->Name << endl;
                  break;
               }

               // Write the loopstyle
               if (stricmp(SubCommand,"LoopStyle") == 0)
               {
                  F << LoopStyle << endl;
                  break;
               }

               // Write if we are in server mode
               if (stricmp(SubCommand,"Server") == 0)
               {
                  if (Server == 1)
                     F << "TRUE" << endl;
                  else
                     F << "FALSE" << endl;
                  break;
               }

               // Write the Pitch Speed and Volume
               if (stricmp(SubCommand,"Levels") == 0)
               {
                  F << EfxOut->_get_Pitch() << endl;
                  F << EfxOut->_get_Speed() << endl;
                  F << (unsigned long)EfxOut->_get_Volume() << endl;
                  break;
               }

               F << "!! Bad Query Type" << endl;
               Okay = 0;
               break;
            }

            // Command is Set
            if (stricmp(Command,"Set") == 0 && SubCommand != 0 && Argument != 0)
            {
               // Set the loopstyle
               if (stricmp(SubCommand,"LoopStyle") == 0)
               {
                  int L = atoi(Argument);
                  if (L >= 0 && L <= 2)
                     LoopStyle = L;
                  else
                  {
                     F << "!! Out of Range" << endl;
                     Okay = 0;
                     break;
                  }

                  switch (LoopStyle)
                  {
                     // Normal, do nothing special
                     case 0:
                        AllowLoop = TRUE;
                        Module->_set_AllowLoop(AllowLoop);
                        break;

                     // Play once, no looping
                     case 1:
                        AllowLoop = FALSE;
                        Module->_set_AllowLoop(AllowLoop);
                        break;

                     // Infinitly self loop, Disable song breaking too
                     case 2:
                        AllowLoop = TRUE;
                        Module->_set_AllowLoop(AllowLoop);
                        break;
                  }
                  for (Pane **I = Displays.begin(); I != Displays.end(); I++)
                     (*I)->LoopStyle(LoopStyle);

                  break;
               }

               // Write if we are in server mode
               if (stricmp(SubCommand,"Server") == 0)
               {
                  int L = atoi(Argument);
                  if (L >= 0 && L <= 1 && Argument[1] == 0)
                     Server = L;
                  else
                  {
                     if (stricmp(Argument,"TRUE") == 0)
                     {
                        Server = TRUE;
                        break;
                     }

                     if (stricmp(Argument,"FALSE") == 0)
                     {
                        Server = FALSE;
                        break;
                     }

                     F << "!! Out of Range" << endl;
                     Okay = 0;
                     break;
                  }
                  break;
               }

               // Set the Pitch Speed and Volume, -1 is ignore
               if (stricmp(SubCommand,"Levels") == 0)
               {
                  int Pitch = -1;
                  int Speed = -1;
                  int Volume = -1;
                  int I2 = 0;
                  for (char *A = strtok(Argument," "); A != 0; A = strtok(0," "))
                  {
                     switch(I2)
                     {
                        case 0:
                           Pitch = atoi(A);
                           break;

                        case 1:
                           Speed= atoi(A);
                           break;

                        case 2:
                           Volume = atoi(A);
                           break;
                     }
                     I2++;
                  }
                  if (Pitch != -1)
                     EfxOut->_set_Pitch(Pitch);

                  if (Speed != -1)
                     EfxOut->_set_Speed(Speed);

                  if (Volume != -1)
                     EfxOut->_set_Volume(Volume);
                  EfxOut->SetChanged(EFX_Speed | EFX_Pitch | EFX_Volume);

                  for (Pane **I3 = Displays.begin(); I3 != Displays.end(); I3++)
                     (*I3)->SetParms(EfxOut);
                  break;
               }

               F << "!! Bad Set Command" << endl;
               Okay = 0;
               break;
            }

            // Command is Play
            if (stricmp(Command,"Play") == 0 && SubCommand != 0 )
            {
               // Pause
               if (stricmp(SubCommand,"Pause") == 0)
               {
                  if (BaseDisplay != 0)
                     BaseDisplay->DisplayStatus("Paused");
                  Output->PausePlay();
                  break;
               }

               // Resume
               if (stricmp(SubCommand,"Resume") == 0)
               {
                  if (BaseDisplay != 0)
                     BaseDisplay->DisplayStatus("Playing");
                  Output->ResumePlay();
                  break;
               }

               // Advance to the next song
               if (stricmp(SubCommand,"Next") == 0)
               {
                  SongChange();
                  break;
               }

               // Play last song
               if (stricmp(SubCommand,"Last") == 0)
               {
                  if (CurSong > 0)
                  {
                     NextSong = CurSong - 1;
                     Changed = 1;
                     SongChange();
                     break;
                  }
                  F << "!! First Song" << endl;
                  Okay = 0;
                  break;
               }

               // Advance to the given index
               if (stricmp(SubCommand,"Index") == 0 && Argument != 0)
               {
                  int Num = atoi(Argument);
                  if (Num < 0 || Num >= Songs->List.size())
                  {
                     F << "!! Bad Index" << endl;
                     Okay = 0;
                     break;
                  }
                  NextSong = Num;
                  Changed = 1;
                  SongChange();
                  break;
               }

               // Advance to the given song (in the list)
               if (stricmp(SubCommand,"Song") == 0 && Argument != 0)
               {
                  // Search for a match
                  for (DirItem *I = Songs->List.begin(); I != Songs->List.end(); I++)
                  {
                     if (I->Name != 0 && stristr(I->Name,Argument) != 0)
                     {
                        NextSong = I - Songs->List.begin();
                        Changed = 1;
                        F << NextSong << endl;
                        F << I->Name << endl;
                     }
                  }

                  if (Changed == 0)
                  {
                     F << "!! No Match" << endl;
                     Okay = 0;
                     break;
                  }

                  SongChange();
                  break;
               }

               F << "!! Bad Play Command" << endl;
               Okay = 0;
               break;
            }

            // Command is SongList
            if (stricmp(Command,"SongList") == 0 && SubCommand != 0 )
            {
               // Add to the list
               if (stricmp(SubCommand,"Add") == 0 && Argument != 0)
               {
                  for (char *A = strtok(Argument," "); A != 0; A = strtok(0," "))
                  {
                     if (A[0] == '@')
                        Songs->AddListFile(&A[1]);
                     else
                        Songs->Add(A);
                  }
                  F << Songs->List.size() << endl;
                  break;
               }

               // Erase the list
               if (stricmp(SubCommand,"Erase") == 0)
               {
                  for (DirItem *I = Songs->List.begin(); I != Songs->List.end(); I++)
                     I->Free();

                  Songs->List.erasefrom(Songs->List.begin());
                  NextSong = 0;
                  CurSong = -1;
                  Changed = 1;
                  break;
               }

               // Erase an item
               if (stricmp(SubCommand,"EraseIndx") == 0 && Argument != 0)
               {
                  int Num = atoi(Argument);
                  if (Num < 0 || Num >= Songs->List.size())
                  {
                     F << "!! Bad Index" << endl;
                     Okay = 0;
                     break;
                  }
                  Songs->List[Num].Free();

                  Songs->List.erase(Songs->List.begin() + Num);

                  // Re-sync the next song pointer
                  if (Changed == 1)
                  {
                     if (Num < NextSong)
                        NextSong--;
                  }
                  else
                  {
                     if (CurSong >= Num)
                     {
                        CurSong--;
                        NextSong--;
                     }
                  }
                  break;
               }
            }

            F << "!! Bad Command" << endl;
            Okay = 0;
            break;
         }

         if (Command == 0)
            F << "!! Nul Command" << endl;
         else
            if (Okay == 1)
               F << "!! OK" << endl;
      }
   }
   DosClose(Pipe);
}
