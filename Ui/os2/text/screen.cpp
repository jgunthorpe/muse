#pragma off(behaved)
/* ########################################################################

   Screen - Class that represents the screen

   ########################################################################
*/
#ifndef CPPCOMPILE
#ifndef DTS
#include <IDLSequence.xh>
#pragma pack(4)
#include <DAC.xh>
#include <FormatBase.xh>
#include <somcdev.h>
#include <DebugUtils.xh>
#include <EfxFiltr.xh>
#include <BaseSet\SOMDelete.h>
#pragma pack()
#else
#include <IDLSequence.hh>
#include <DAC.hh>
#include <FormatBs.hh>
#include <EfxFiltr.hh>
#include <somcdev.hh>
#include <DebugUtils.hh>
#define SOMDelete(x) delete x;
#endif
#else
#include <IDLSeq.hc>
#include <DAC.hc>
#include <FormatBs.hc>
#include <EfxFiltr.hc>
#include <DebugUtl.hc>
#endif

#include <MinMax.h>
#include <Flags.h>

#define INCL_DOS
#define INCL_VIO
#define INCL_KBD
#define INCL_DOSERRORS
#include <os2.h>

#include "Screen.h"
#include "Displays.h"

#include <iomanip.h>
#include <conio.h>
#include <ctype.h>

// Write debug data to the file
int SOMLINK OutChar(char C)
{
   if (Screen::DebugFile == 0)
   {
//      printf("%c",C);
      return 0;
   }
   fputc(C,Screen::DebugFile);
   return 0;
};

FILE *Screen::DebugFile;

/* ########################################################################

   Class - Screen (The display screen)
   Member - Constructor (Init the class)

   Construct the needed helper classes.

   ########################################################################
*/
Screen::Screen()
{
   SOMOutCharRoutine = &OutChar;
   Songs = new SongList;
   Handlers = new museHandlerList;
   Outputs.construct();
   FileFormats.construct();
   Displays.construct();
   Displays.reserve(10);
   HiPrio = FALSE;
   AllowLoop = TRUE;
   OutputOptions = 0;
   DebugFile = 0;
   OutputClass = 0;
   Output = 0;
   OldScreen = 0;
   Rows = 0;
   Cols = 0;
   OldX = 0;
   OldY = 0;
   BaseStartLine = 0;
   BaseLines = 0;
   Exit = 0;
   BeginPlay = 0;
   DonePlay = 0;
   Changed = 0;
   Playing = 0;
   ScrollLoc = 0;
   UIBase = 200;
   CurrentDisplay = 0;
   LoopStyle = 0;
   PlayTid = 0;
   BaseDisplay = 0;
   Detached = 0;
   CurSong = 0;
   Server = FALSE;
   RealTime = FALSE;
   DeviceDesc = 0;
   ServerWait = 0;
   StdError = 0;
   KeyDelay = 0;
   DefVolume = -1;
   FileName = 0;
   EfxOut = 0;
   SongListLoop = 0;

   // Get the display attributes from the VIO system.
   VIOMODEINFO Mode;
   Mode.cb = sizeof(Mode);
   if (VioGetMode(&Mode,0) == NO_ERROR)
   {
      Rows = Mode.row;
      Cols = Mode.col;
   }
   VioGetCurType(&Cursor,0);
   OAttrib = Cursor.attr;

   if (DosCreateMutexSem(0,&KeyProtect,0L,FALSE) != 0)
      somPrintf("Sem Construct error\n");

   // Spawn the std error thread
   TID Tid;
   SpawnType = 3;
   if (DosCreateThread(&Tid,&SpawnThreads,(unsigned long)this,2,70000) != 0)
   {
      cout << "Thread spawn Error" << endl;
      return;
   }
};

Screen::~Screen()
{
   somPrintf("Destruct\n");
   Exit = 1;
   DosDisConnectNPipe(PipeHandle);

   Cursor.attr = OAttrib;
   VioSetCurType(&Cursor,0);

   // Restore the old screen
   if (OldScreen != 0)
   {
      VioSetCurPos(OldY,OldX,0);
      VioWrtCellStr((char *)OldScreen,Rows*Cols*sizeof(ScreenCell),0,0,0);
      somPrintf("%u,%u\n",(int)Rows,(int)Cols);
      delete [] OldScreen;
   }

   // Nuke all panes
   for (Pane **I = Displays.begin(); I != Displays.end(); I++)
      delete *I;
   Displays.free();

   if (DosCloseEventSem(BeginPlay) != 0)
      somPrintf("BeginPlay Close error\n");
   if (DosCloseEventSem(DonePlay) != 0)
      somPrintf("DonePlay Close error\n");
   if (DosCloseEventSem(ServerWait) != 0)
      somPrintf("Sever Wait Close Error\n");

   delete Songs;
   SOMDelete(Handlers);
   Outputs.free();
   FileFormats.free();
   SOMDelete(EfxOut);
   EfxOut = 0;
   SOMDelete(Output);
   Output = 0;

   if (DeviceDesc != 0)
      cout << DeviceDesc;
   delete [] DeviceDesc;

   if (StdError != 0 && *StdError != 0)
   {
      cout << endl;
      cout << "Errors that occured during exectution" << endl;
      cout << "~~~~~~ ~~~~ ~~~~~~~ ~~~~~~ ~~~~~~~~~~" << endl;
      cout << StdError;
   }

   somPrintf("Destruct Done\n");
   fclose(DebugFile);
   DebugFile = 0;
};

/* ########################################################################

   Class - Screen (The display screen)
   Member - Init (Init the output engine)

   Init the output engine and display a few stats

   ########################################################################
*/
int Screen::Init()
{
   DBU_FuncTrace("Screen","Init",TRACE_SIMPLE);

   // Place a nice blank line after songlist load errors
   if (StdError != 0 && *StdError != 0)
      cerr << endl;

   // Construct Sems
   if (DosCreateEventSem(0,&BeginPlay,0,FALSE) != 0 ||
       DosCreateEventSem(0,&DonePlay,0,FALSE) != 0 ||
       DosCreateEventSem(0,&ServerWait,0,FALSE) != 0)
   {
      somPrintf("Semaphore creation error\n");
      cout << "Semaphore creation error" << endl;
      return 1;
   }

   // Check if autodetect is needed (no -O)
   if (OutputClass == 0)
   {
      cout << "Auto Detecting Output Method";
      OutputClass = Handlers->GetBestOutput(0);
      cout << '.' << endl;
      if (OutputClass == 0)
      {
         somPrintf("Sorry, couldn't detect a sound device, try the -O option.\n");
         cout << "Sorry, couldn't detect a sound device, try the -O option." << endl;
         return 1;
      }
   }

   // Construct
   Output = (museOutputBase *)(OutputClass->somNew());
   if (Output == 0)
   {
      somPrintf("Unable to construct output class\n");
      cout << "Unable to construct output class" << endl;
      return 1;
   }

   // Special stuf for song to file output
   #ifndef DTS
      if (OutputClass->somDescendedFrom(_museFileOutput) == TRUE)
   #else
      if (OutputClass->somDescendedFrom(museFileOutput::__ClassObject) == TRUE)
   #endif
   {
      if (FileName == 0 && Songs->List.size() != 0)
      {
         char *N = new char [300];
         char *I = Songs->List[0].Name;
         for (I += strlen(I); I != Songs->List[0].Name && *I != '\\' && *I != '/' && *I != ':'; I--);
         if (I != Songs->List[0].Name)
            I++;
         sprintf(N,"%s%s",I,((museFileOutput *)Output)->_get_Extension());
         FileName = N;
      }
   }

   // Set the filename
   if (FileName != 0)
   {
      char *C = (char *)SOMMalloc(strlen(FileName) + 1);
      strcpy(C,FileName);
      ((museFileOutput *)Output)->_set_FileName(C);
   }

   // Get the file name and show it.
   #ifndef DTS
      if (OutputClass->somDescendedFrom(_museFileOutput) == TRUE)
   #else
      if (OutputClass->somDescendedFrom(museFileOutput::__ClassObject) == TRUE)
   #endif
   {
      cout << "Writing song to file " << ((museFileOutput *)Output)->_get_FileName() << endl;
   }

   Output->_set_RealTime(RealTime);
   // Configue the output class.
   if (OutputOptions != 0)
      Output->SetOptions(OutputOptions);

   // Construct thread
   TID Tid;
   SpawnType = 0;
   if (DosCreateThread(&Tid,&SpawnThreads,(unsigned long)this,2,70000) != 0)
   {
      somPrintf("Thread spawn Error\n");
      cout << "Thread spawn Error" << endl;
      return 1;
   }
   DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,15,Tid);

   Cursor.attr = -1;
   VioSetCurType(&Cursor,0);

   OldScreen = new ScreenCell[Cols*Rows];
   unsigned short Len = Cols*Rows*sizeof(ScreenCell);
   if (VioReadCellStr((char *)OldScreen,&Len,0,0,0) == ERROR_VIO_DETACHED)
      Detached = 1;
//Detached = 1;
   VioGetCurPos(&OldY,&OldX,0);
   VioSetCurPos(Rows-1,0,0);
   if (Detached == 1)
   {
      delete [] OldScreen;
      OldScreen = 0;
   }

   // If not detached load the displays.
   somPrintf("Detached - %u\n",(unsigned long)Detached);
   if (Detached == 0)
   {
      // Add the displays
      switch (UIBase)
      {
         case 100:
           Displays.push_backv(BaseDisplay = new ::BaseDisplay(this));
           Displays.push_backv(new InstScreen(this));
           Displays.push_backv(new StatScreen(this));
           Displays.push_backv(new HelpScreen(this));
           Displays.push_backv(new AnvilScreen(this));
           break;

         case 200:
           Displays.push_backv(BaseDisplay = new ::BaseDisplay(this));
           Displays.push_backv(new WizStatScreen(this));
           Displays.push_backv(new InstScreen(this));
           Displays.push_backv(new HelpScreen(this));

        // Add the extra dac screen
        #ifndef DTS
           if (OutputClass->somDescendedFrom(_museDACMixer) == TRUE)
        #else
           if (OutputClass->somDescendedFrom(museDACMixer::__ClassObject) == TRUE)
        #endif
              Displays.push_backv(new DACScreen(this));
           break;
      }
      if (BaseDisplay != 0)
      {
         Display(BaseDisplay);
         Display(Displays[1]);
      }
   }

   // Initialize output
   char *Error;
   if (BaseDisplay != 0)
      BaseDisplay->DisplayStatus("Initializing...");
   if (Output->InitPlay(&Error) != 0)
   {
      somPrintf("Error init");
      // Restore the original screeen
      VioSetCurPos(OldY,OldX,0);
      VioWrtCellStr((char *)OldScreen,Rows*Cols*sizeof(ScreenCell),0,0,0);
      delete [] OldScreen;
      OldScreen = 0;

      somPrintf("Couldn't Initialize device. Error String is %s\n",Error);
      cout << "Couldn't Initialize device. Error String is '" << Error << "'" << endl;
      return 1;
   }

   /* Wait for playback thread to initilize and then set the prio to time
      critical if the device is realtime.
   */
/*   if (Output->_get_RealTime() == TRUE)
   {
      for (;PlayTid == 0;DosSleep(200));
      DosSetPriority(PRTYS_THREAD,PRTYC_TIMECRITICAL,31,PlayTid);
   }
*/
   char *Desc = Output->GetCurOptionDesc();
   somPrintf("%s\n",Desc);

   DeviceDesc = new char[300];
   sprintf(DeviceDesc,"Using %s %s\n",OutputClass->GetTypeName(),Desc);

   SOMFree(Desc);

   EfxOut = new museEffectFilter;
   EfxOut->_set_NextLink(Output);
   if (DefVolume != -1)
      EfxOut->_set_Volume(DefVolume);
   EfxOut->SetChanged(EFX_Volume);
   DefVolume = EfxOut->_get_Volume();

   for (Pane **I = Displays.begin(); I != Displays.end(); I++)
   {
      (*I)->SetPan(EfxOut);
      (*I)->SetParms(EfxOut);
   }
   return 0;
}

void APIENTRY Screen::SpawnThreads(unsigned long Data)
{
   Screen *Scr = (Screen *)Data;
   char Spawn = Scr->SpawnType;
   if (Spawn < 2)
   {
      Scr->SpawnType++;
      TID Tid;
      if (DosCreateThread(&Tid,&SpawnThreads,Data,2,70000) != 0)
         somPrintf("Thread spawn Error\n");

      // Change the prio of the playback thread
      if (Spawn == 0)
      {
         if (Scr->HiPrio == TRUE)
           DosSetPriority(PRTYS_THREAD,PRTYC_TIMECRITICAL,0,Tid);
         else
           DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,1,Tid);
         Scr->PlayTid = Tid;
      }
   }

   switch (Spawn)
   {
      case 0:
         Scr->Keyboard();
         break;

      case 1:
         Scr->PlayThread();
         break;

      case 2:
         Scr->PipeThread();
         break;

      // This thread is spawned by setting Spawn to 3 before creating this
      case 3:
         Scr->StdErrThread();
         break;
   }
}

/* ########################################################################

   Class - Screen (The display screen)
   Member - Keyboard (Handles key presses)

   Keyboard Handler thread

   ########################################################################
*/

void Screen::Keyboard()
{
   KBDINFO kinfo;
   kinfo.cb = sizeof(kinfo);
   kinfo.fsMask = KEYBOARD_ECHO_OFF | KEYBOARD_BINARY_MODE;
   KbdSetStatus(&kinfo,0);

   while (1)
   {
      DosReleaseMutexSem(KeyProtect);

      /* This is weird, stupid and silly, but hey, it works! Trouble is, when
         unzip is spawned and a zip error occures unzip doesn't end correctly
         if this thread is blocked on KbdCharIn. Actually, it doesn't end at
         all. So by adding this small delay it will allow the user to unblock
         things and fix it, they must strike a key however.

         It seems to be gone, I piped stderr and it disappeared! */
      if (KeyDelay == 1)
         DosSleep(500);

      KBDKEYINFO info;
      memset(&info,0,sizeof(info));
      if (KbdCharIn(&info,IO_WAIT,0) != 0)
         return;

      DosRequestMutexSem(KeyProtect,-1);

      if (Exit == 1 && info.chChar != 27)
         continue;

      switch (info.chChar)
      {
         // Esc
         case 27:
            Exit = 1;
            if (Output != 0)
               SongChange();
            break;

         // Next song
         case ']':
            if (NextSong + 1 >= Songs->List.size())
               break;
            NextSong++;
            Changed = 1;
            if (Playing == 1)
               SongChange();
            BaseDisplay->SetFileName(Songs->List[NextSong],0);
            break;

         // Last song
         case '[':
            if (NextSong <= 0)
               break;

            NextSong--;
            Changed = 1;
            if (Playing == 1)
               SongChange();
            BaseDisplay->SetFileName(Songs->List[NextSong],0);
            break;

         // Random Song
         case '\\':
         {
            if (Songs->List.size() <= 1)
               break;
            for (int NSong = NextSong; NSong == NextSong;NSong = rand() % (Songs->List.size() - 1));
            NextSong = NSong;
            Changed = 1;
            if (Playing == 1)
               SongChange();
            BaseDisplay->SetFileName(Songs->List[NextSong],0);
            break;
         }

         // Switch to the given song
         case '1': case '2': case '3': case '4': case '5': case '6':
         case '7': case '8': case '9':
         {
            if (Songs->List.size() == 0)
               break;

            int Temp = info.chChar - '1';
            if (Temp >= Songs->List.size())
               Temp = Songs->List.size() - 1;

            if (Temp != NextSong)
            {
               NextSong = Temp;
               Changed = 1;
               if (Playing == 1)
                  SongChange();
               BaseDisplay->SetFileName(Songs->List[NextSong],0);
            }
            break;
         }

         // Pause
         case 'P':
         case 'p':
            if (BaseDisplay != 0)
               BaseDisplay->DisplayStatus("Paused");
            Output->PausePlay();
            break;

         // Resume
         case 'R':
         case 'r':
            if (BaseDisplay != 0)
               BaseDisplay->DisplayStatus("Playing");
            Output->ResumePlay();
            break;

         // Loop Style
         case 'l':
         case 'L':
         {
            if (LoopStyle >= 2)
               LoopStyle = 0;
            else
               LoopStyle++;
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

         // Volume Control
         case '+':
         case '-':
         {
            if (info.chChar == '+')
               EfxOut->_set_Volume(EfxOut->_get_Volume()+1);
            if (info.chChar == '-')
               EfxOut->_set_Volume(EfxOut->_get_Volume()-1);
            EfxOut->SetChanged(EFX_Volume);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetParms(EfxOut);
            break;
         }

         // Pan Control
         case ';':
         case '\'':
         {
            // Depth
            int P = EfxOut->_get_PanDepth();
            if (info.chChar == ';')
               P -= PanStep;
            if (info.chChar == '\'')
               P += PanStep;
            EfxOut->_set_PanDepth(P);
            EfxOut->SetChanged(EFX_Pan);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetPan(EfxOut);
            break;
         }

         case ',':
         case '.':
         {
            // Balance
            int P = EfxOut->_get_PanBalance();
            if (info.chChar == ',')
               P -= PanStep;
            if (info.chChar == '.')
               P += PanStep;
            EfxOut->_set_PanBalance(P);
            EfxOut->SetChanged(EFX_Pan);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetPan(EfxOut);
            break;
         }

         case '<':
         case '>':
         {
            // Center
            int P = EfxOut->_get_PanCenter();
            if (info.chChar == '<')
               P -= PanStep;
            if (info.chChar == '>')
               P += PanStep;
            EfxOut->_set_PanCenter(P);
            EfxOut->SetChanged(EFX_Pan);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetPan(EfxOut);
            break;
         }

         // Reset settings
         case '=':
         {
            EfxOut->Reset();
            EfxOut->_set_Volume(DefVolume);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
            {
               (*I)->SetParms(EfxOut);
               (*I)->SetPan(EfxOut);
            }
            break;
         }

         // Help
         case '?':
            info.chChar  = 'H';

         default:
         {
            // See if a switch is needed
            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               if (toupper((*I)->Key) == toupper(info.chChar))
                  Display(*I);
            if (CurrentDisplay != 0)
               CurrentDisplay->KeyPress(info);
            break;
         }
      };

      switch (info.chScan)
      {
         // Pitch Control
         case 59: // F1 (down)
         case 60: // F2 (up)
         {
            if (info.chScan == 60)
               EfxOut->_set_Pitch(EfxOut->_get_Pitch()+1);
            if (info.chScan == 59)
               EfxOut->_set_Pitch(EfxOut->_get_Pitch()-1);
            EfxOut->SetChanged(EFX_Pitch);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetParms(EfxOut);
            break;
         }

         // Speed Control
         case 61: // F3 (down)
         case 62: // F4 (up)
         {
            if (info.chScan == 62)
               EfxOut->_set_Speed(EfxOut->_get_Speed()+1);
            if (info.chScan == 61)
               EfxOut->_set_Speed(EfxOut->_get_Speed()-1);
            EfxOut->SetChanged(EFX_Speed);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetParms(EfxOut);
            break;
         }

         // Speed + Pitch Control
         case 63: // F5 (down)
         case 64: // F6 (up)
         {
            if (info.chScan == 64)
            {
               EfxOut->_set_Speed(EfxOut->_get_Speed()+1);
               EfxOut->_set_Pitch(EfxOut->_get_Pitch()+1);
            }
            if (info.chScan == 63)
            {
               EfxOut->_set_Speed(EfxOut->_get_Speed()-1);
               EfxOut->_set_Pitch(EfxOut->_get_Pitch()-1);
            }
            EfxOut->SetChanged(EFX_Speed | EFX_Pitch);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetParms(EfxOut);
            break;
         }

         // Speed + Pitch + Volume Control
         case 65: // F7 (down)
         case 66: // F8 (up)
         {
            if (info.chScan == 66)
            {
               EfxOut->_set_Speed(EfxOut->_get_Speed()+1);
               EfxOut->_set_Pitch(EfxOut->_get_Pitch()+1);
            }
            if (info.chScan == 65)
            {
               EfxOut->_set_Speed(EfxOut->_get_Speed()-1);
               EfxOut->_set_Pitch(EfxOut->_get_Pitch()-1);
            }
            EfxOut->_set_Volume(EfxOut->_get_Speed());
            EfxOut->SetChanged(EFX_Speed | EFX_Pitch | EFX_Volume);

            for (Pane **I = Displays.begin(); I != Displays.end(); I++)
               (*I)->SetParms(EfxOut);
            break;
         }

         // Down
         case 80:
            if (CurrentDisplay != 0 && ScrollLoc + BaseLines - CurrentDisplay->Bottom < CurrentDisplay->Lines)
            {
               BYTE C[2] = {0,0};
               C[1] = (BaseDisplay->GetBuffer() + BaseStartLine*80)->Colour;
               VioScrollUp(BaseStartLine + 1,0,BaseStartLine + BaseLines - 1 - CurrentDisplay->Bottom,79,1,C,0);
               VioWrtCellStr((char *)(CurrentDisplay->GetBuffer()+(BaseLines + ScrollLoc - CurrentDisplay->Bottom)*80),80*sizeof(ScreenCell),BaseStartLine + BaseLines - 1 - CurrentDisplay->Bottom,0,0);
               ScrollLoc++;
            }
            break;

         // Up
         case 72:
            if (CurrentDisplay != 0 && ScrollLoc != 0)
            {
               BYTE C[2] = {0,0};
               C[1] = (BaseDisplay->GetBuffer() + BaseStartLine*80)->Colour;
               VioScrollDn(BaseStartLine + 1,0,BaseStartLine + BaseLines - 1 - CurrentDisplay->Bottom,79,1,C,0);
               VioWrtCellStr((char *)(CurrentDisplay->GetBuffer()+(ScrollLoc)*80),80*sizeof(ScreenCell),BaseStartLine + 1,0,0);
               ScrollLoc--;
            }
            break;

         // Pg Down
         case 81:
            if (CurrentDisplay != 0 && ScrollLoc + BaseLines - CurrentDisplay->Bottom < CurrentDisplay->Lines)
            {
               ScrollLoc = min(ScrollLoc + BaseLines - 2 - CurrentDisplay->Bottom,CurrentDisplay->Lines - BaseLines + CurrentDisplay->Bottom);

               // Some ppl run with larger screens, deal with it.
               if (Cols != 80)
               {
                  for(signed int I = BaseLines - 1 - CurrentDisplay->Bottom;I >= 1;I--)
                     VioWrtCellStr((char *)(CurrentDisplay->GetBuffer()+(I+ScrollLoc)*80),
                                   80*sizeof(ScreenCell),
                                   BaseStartLine + I  ,0,0);
               }
               else
                  VioWrtCellStr((char *)(CurrentDisplay->GetBuffer() + (ScrollLoc+1)*80),
                                (BaseLines - 1 - CurrentDisplay->Bottom)*80*sizeof(ScreenCell),
                                BaseStartLine + 1,0,0);
            }
            break;

         // Pg Up
         case 73:
            if (CurrentDisplay != 0 && ScrollLoc != 0)
            {
               ScrollLoc = max((int)(ScrollLoc - BaseLines + 2 + CurrentDisplay->Bottom),(int)0);

               // Some ppl run with larger screens, deal with it.
               if (Cols != 80)
               {
                  for(signed int I = BaseLines - 1 - CurrentDisplay->Bottom;I >= 1;I--)
                     VioWrtCellStr((char *)(CurrentDisplay->GetBuffer()+(I+ScrollLoc)*80),
                                   80*sizeof(ScreenCell),
                                   BaseStartLine + I ,0,0);
               }
               else
                  VioWrtCellStr((char *)(CurrentDisplay->GetBuffer() + (ScrollLoc+1)*80),
                                (BaseLines - 1 - CurrentDisplay->Bottom)*80*sizeof(ScreenCell),
                                BaseStartLine + 1,0,0);
            }
            break;

         // Home
         case 71:
            if (CurrentDisplay != 0 && ScrollLoc != 0)
            {
               ScrollLoc = 0;

               // Some ppl run with larger screens, deal with it.
               if (Cols != 80)
               {
                  for(signed int I = BaseLines - 1 - CurrentDisplay->Bottom;I >= 1;I--)
                     VioWrtCellStr((char *)(CurrentDisplay->GetBuffer()+(I+ScrollLoc)*80),
                                   80*sizeof(ScreenCell),
                                   BaseStartLine + I,0,0);
               }
               else
                  VioWrtCellStr((char *)(CurrentDisplay->GetBuffer() + (ScrollLoc+1)*80),
                                (BaseLines - 1 - CurrentDisplay->Bottom)*80*sizeof(ScreenCell),
                                BaseStartLine + 1,0,0);
            }
            break;

         // End
         case 79:
            if (CurrentDisplay != 0 && ScrollLoc + BaseLines - CurrentDisplay->Bottom < CurrentDisplay->Lines)
            {
               ScrollLoc = CurrentDisplay->Lines - BaseLines + CurrentDisplay->Bottom;

               // Some ppl run with larger screens, deal with it.
               if (Cols != 80)
               {
                  for(signed int I = BaseLines - 1 - CurrentDisplay->Bottom;I >= 1;I--)
                     VioWrtCellStr((char *)(CurrentDisplay->GetBuffer()+(I+ScrollLoc)*80),
                                   80*sizeof(ScreenCell),
                                   BaseStartLine + I,0,0);
               }
               else
                  VioWrtCellStr((char *)(CurrentDisplay->GetBuffer() + (ScrollLoc+1)*80),
                                (BaseLines - 1 - CurrentDisplay->Bottom)*80*sizeof(ScreenCell),
                                BaseStartLine + 1,0,0);
            }
            break;
      };
   }
}

/* ########################################################################

   Class - Screen (The display screen)
   Member - Play (Plays songs)

   Waits for the song Ready Sem and then begins playback and triggers the
   song done sem when finished.

   ########################################################################
*/
void Screen::PlayThread()
{
   while (1)
   {
      unsigned long Count = 0;

      // Wait for go play signal
      somPrintf("PlayWait\n");
      DosWaitEventSem(BeginPlay,-1);
      DosResetEventSem(BeginPlay,&Count);
      if (Exit == 1)
      {
         DosPostEventSem(DonePlay);
         return;
      }

      if (BaseDisplay != 0)
         BaseDisplay->DisplayStatus("Playing");

      int Res = 0;
      do
      {
         Playing = 1;
         somPrintf("Playing\n");
         Res = Module->Play(EfxOut,0);
         somPrintf("Done\n");
         Output->StopNotes();
         Playing = 0;
      }
      while (Res == 0 && LoopStyle == 2 && Exit == 0 && Changed == 0);

      int OrgSong = CurSong;
      int Next = NextSong;
      if (Res == 1)
      {
         do
         {
            Next = NextSong;
            DosSleep(300);
         }
         while (Next != NextSong);

         // Same song, just loop it.
         if (NextSong == OrgSong && Exit != 1)
         {
            DosPostEventSem(BeginPlay);
            continue;
         }
      }

      if (Res > 1)
      {
         cerr << "Playback Error code #" << Res << endl;
         Exit = 1;
      }

      if (BaseDisplay != 0)
         BaseDisplay->DisplayStatus("Stopped");

      // Post done play
      DosPostEventSem(DonePlay);
      if (Exit == 1)
         return;
   }
}

/* ########################################################################

   Class - Screen (The display screen)
   Member - SongChange (Change songs)

   Calls ForceCompError to change songs.

   ########################################################################
*/
void Screen::SongChange()
{
   Output->ForceCompError();
   DosPostEventSem(ServerWait);
}

/* ########################################################################

   Class - Screen (The display screen)
   Member - Play (Plays the songs)

   Play the songs

   ########################################################################
*/
void Screen::Play()
{
   DBU_FuncTrace("Screen","Play",TRACE_SIMPLE);
   do
   {
      for (int CurSong = 0;Exit == 0 && CurSong < Songs->List.size();)
      {
         DBU_FuncMessage("Loop Top");
         NextSong = CurSong;
         Song = Songs->List[CurSong];
         this->CurSong = CurSong;

         Changed = 0;

         if (Exit == 1)
            return;

         // Load the song
         if (BaseDisplay != 0)
            BaseDisplay->DisplayStatus("Loading...");

         // Get the class for the song.
         somPrintf("Checking %s\n",Song.Name);
         museFormatClass *Class = Handlers->GetClassForFile(Song.Name);
         if (Class == 0)
         {
            if (NextSong < 0)
               NextSong = 0;

            if (Changed == 0)
               CurSong = NextSong + 1;
            else
               CurSong = NextSong;
            continue;
         }

         if (BaseDisplay != 0)
            BaseDisplay->SetFileName(Song,Class->GetTypeName());

         // Load the song
         Module = (museFormatBase *)(Class->somNew());
         somPrintf("Loading %s\n",Song.Name);

         // Load the song
         KeyDelay = 1;
         unsigned char *File = Songs->LoadFile(Song);
         if (File == 0)
         {
            if (Module->LoadModule(Song.Name) != 0)
            {
               SOMDelete(Module);
               Module = 0;
               cerr << "Could not load " << Song.Name << endl;

               if (NextSong < 0)
                  NextSong = 0;

               if (Changed == 0)
                  CurSong = NextSong + 1;
               else
                  CurSong = NextSong;
               continue;
            }
         }
         else
         {
            // Load the module from the memory block (unzip'd)
            if (Module->LoadMemModule(File,Song.Size) != 0)
            {
               SOMDelete(Module);
               Module = 0;
               delete [] File;
               cerr << "Could not load " << Song.Name << endl;

               if (NextSong < 0)
                  NextSong = 0;

               if (Changed == 0)
                  CurSong = NextSong + 1;
               else
                  CurSong = NextSong;
               continue;
            }
         }
         KeyDelay = 0;
         Module->_set_AllowLoop(AllowLoop);
         somPrintf("Loop -%u\n",(unsigned int)AllowLoop);

         DBU_FuncMessage("Title");
         if (Song.Title == 0)
         {
            SOMFree(Song.Title);
            SOMFree(Songs->List[CurSong].Title);
            Song.Title = 0;
            Songs->List[CurSong].Title = 0;
         }

         Songs->List[CurSong].Title = Module->GetTitle();
         Song.Title = Module->GetTitle();

         // Setup the screens.
         for (Pane **I = Displays.begin(); I != Displays.end(); I++)
         {
            (*I)->SetModule(Module);
            (*I)->LoopStyle(LoopStyle);
            (*I)->SetParms(EfxOut);
            if (*I != BaseDisplay)
               (*I)->SetFileName(Song,Module->GetTypeName());
         }

         // Verify a song switch hasn't happened during load
         DBU_FuncMessage("Song Switch");
         if (Changed == 0)
         {
            // Begin playback
            DosPostEventSem(BeginPlay);

            // Wait for go play signal
            unsigned long Count = 0;
            DosWaitEventSem(DonePlay,-1);
            DosResetEventSem(DonePlay,&Count);
         }

         // Free
         DBU_FuncMessage("Free");
         SOMDelete(Module);
         Module = 0;
         delete [] File;

         if (Exit == 1)
         {
            Output->StopPlay();
            return;
         }

         if (NextSong < 0)
            NextSong = 0;

         if (Changed == 0)
            CurSong = NextSong + 1;
         else
            CurSong = NextSong;
      }

      // SongListLoop
      if (SongListLoop == 1)
         continue;

      somPrintf("Stop When Done \n");
      Output->StopWhenDone();

      // Hm
      if (Server == TRUE && Exit == 0)
      {
         // Wait for go play signal
         unsigned long Count = 0;
         DosResetEventSem(ServerWait,&Count);
         DosWaitEventSem(ServerWait,-1);
         if (Exit == 0)
         {
            char *Error = 0;
            // Reinit the sound device.
            if (Output->InitPlay(&Error) != 0)
            {
               Exit = 1;
            }
         }
      }
      else
         Exit = 1;
   }
   while (Exit == 0);

   // Playback thread
   DosPostEventSem(BeginPlay);
   unsigned long Count = 0;
   DosWaitEventSem(DonePlay,-1);
   DosResetEventSem(DonePlay,&Count);
   somPrintf("Exit Play\n");
};

/* ########################################################################

   Class - Screen (The display screen)
   Member - Display (Displays a display on the screen)

   Simply copy it to vid ram

   ########################################################################
*/
void Screen::Display(Pane *Display)
{
   // Draw the entire screen
   if (Display == BaseDisplay)
   {
      // Some ppl run with larger screens, deal with it.
      if (Cols != 80)
      {
         for(signed int I = Display->GetSize()/80 - 1;I >= 0;I--)
            VioWrtCellStr((char *)(Display->GetBuffer()+I*80),80*sizeof(ScreenCell),I,0,0);
      }
      else
         VioWrtCellStr((char *)Display->GetBuffer(),Display->GetSize()*sizeof(ScreenCell),0,0,0);
      return;
   }

   DosRequestMutexSem(Display->Protect,-1);
   CurrentDisplay = Display;

   // Screen is scrollable
   if (Display->Lines + Display->Bottom > BaseLines)
   {
      // Some ppl run with larger screens, deal with it.
      if (Cols != 80)
      {
         for(signed int I = BaseLines - 1 - Display->Bottom;I >= 0;I--)
            VioWrtCellStr((char *)(Display->GetBuffer()+I*80),80*sizeof(ScreenCell),BaseStartLine + I,0,0);
      }
      else
         VioWrtCellStr((char *)Display->GetBuffer(),(BaseLines - Display->Bottom)*80*sizeof(ScreenCell),BaseStartLine,0,0);

      if (Display->Bottom == 1)
         VioWrtCellStr((char *)(Display->GetBuffer()+(Display->Lines + 1)*80),80*sizeof(ScreenCell),BaseStartLine + BaseLines - 1,0,0);
   }
   else
   {
      // Some ppl run with larger screens, deal with it.
      if (Cols != 80)
      {
         for(signed int I = Display->Lines - 1;I >= 0;I--)
            VioWrtCellStr((char *)(Display->GetBuffer()+I*80),80*sizeof(ScreenCell),BaseStartLine + I,0,0);
      }
      else
         VioWrtCellStr((char *)Display->GetBuffer(),Display->GetSize()*sizeof(ScreenCell),BaseStartLine,0,0);

      if (Display->Bottom == 1)
         VioWrtCellStr((char *)(Display->GetBuffer()+(Display->Lines + 1)*80),80*sizeof(ScreenCell),BaseStartLine + Display->Lines,0,0);

      // Some ppl run with larger screens, deal with it.
      if (Cols != 80)
      {
         for(signed int I = BaseLines - 1;I >= Display->Lines + Display->Bottom;I--)
            VioWrtCellStr((char *)(BaseDisplay->GetBuffer()+I*80),80*sizeof(ScreenCell),BaseStartLine + I,0,0);
      }
      else
      {
         if (BaseLines + 1 > Display->Lines + Display->Bottom)
            VioWrtCellStr(
            (char *)(BaseDisplay->GetBuffer() + (Display->Lines+BaseStartLine+Display->Bottom)*80),
            (BaseLines - Display->Lines + 1 - Display->Bottom)*80*sizeof(ScreenCell),
            BaseStartLine + Display->Lines + Display->Bottom,0,0);
      }
   }
   ScrollLoc = 0;
   DosReleaseMutexSem(Display->Protect);
}

/* ########################################################################

   Class - Screen (The display screen)
   Member - DrawString (If the display is active the string is drawn)

   Check for currancy then draw the string.

   ########################################################################
*/
void Screen::DrawString(int X,int Y,int Length,Pane *Display)
{
   if (Exit == 1)
      return;

   if (Length <= 0)
      return;

   if (Length > 80)
      Length = 80;

   if (Display == BaseDisplay)
   {
      VioWrtCellStr((char *)(Display->GetBuffer() + X + 80*Y),
                    Length*sizeof(ScreenCell),Y,X,0);
      return;
   }

   if (Display != CurrentDisplay)
      return;

   if (!(Y < ScrollLoc || Y > BaseLines + ScrollLoc - 1))
      VioWrtCellStr((char *)(Display->GetBuffer() + X + 80*Y),
                    Length*sizeof(ScreenCell),Y - ScrollLoc + BaseStartLine,X,0);
   return;
}

/* ########################################################################

   Class - Screen (The display screen)
   Member - Help (Displays the help text with paging)

   Paginates the help text

   ########################################################################
*/
int Screen::Help()
{
   PVOID Res;
   APIRET Rc;
   unsigned long Size;

   // Read resource
   Rc = DosGetResource(0,1,99,&Res);
   DosQueryResourceSize(0,1,99,&Size);

   if (Rc != 0)
      return 1;

   // Display the text
   int Start = 0;
   int Left = Size;
   for (int Lines = 0;; Lines++)
   {
      int Amount = 0;
      for (;Left != 0 && ((char *)Res)[Amount+Start] != '\r' && ((char *)Res)[Amount+Start] != '\n';Amount++,Left--);

      cout.write(((char *)Res) + Start,Amount);
      cout << endl;

      Amount++;
      Left--;
      if (((char *)Res)[Amount+Start] == '\n')
      {
         Amount++;
         Left--;
      }

      Start += Amount;

      if (Left == 0)
         break;

      if (Lines == 23)
      {
         cout << "--more--";
         getch();
         cout << endl;
         Lines = 0;
      }
   }
   DosFreeResource(Res);

   // Display output system help
   SequenceString Seq;
   Seq.construct();
   Seq.reserve(100);
   for (int I = 0; I != Outputs.size(); I++)
   {
      Seq.erasefrom(Seq.begin());
      Outputs[I]->GetOptionHelp(&Seq);

      if (Seq.size() != 0)
      {
         cout << "Options for " << Outputs[I]->GetTypeName() << endl << " -o";
         Lines++;
         for (char **Cur = Seq.begin(); Cur != Seq.end(); Cur++)
         {
            char *C = *Cur;
            while (*C != 0)
            {
               char *Start = C;
               for (; *C != 0 && *C != '\n'; C++);
               if (*C != 0)
                  C++;
               cout.write(Start,C-Start);
               Lines++;

               if (Lines == 23)
               {
                  cout << "--more--";
                  getch();
                  cout << endl;
                  Lines = 0;
               }
            }
         }
         cout << endl;
      }
   }
   Seq.free();
   return 1;
};

/* ########################################################################

   Class - Screen (The display screen)
   Member - StdErrThread (Captures all output to stderr)

   Used to prevent Unzip and whatnot from spewing junk onto the screen, it's
   captured and spit out on program exit.

   ########################################################################
*/
void Screen::StdErrThread()
{
   // Dup stderr.
   HPIPE hpRead = -1;
   HPIPE hpWrite = -1;
   HFILE New = 2;
   DosCreatePipe(&hpRead,&hpWrite,4096);
   DosDupHandle(hpWrite,&New);
//   DosClose(hpWrite);

   StdError = new char [10000];
   char *Cur = StdError;
   char *End = Cur + 10000;
   *Cur = 0;

   // Fill the buffer with text
   while (Cur < End)
   {
      unsigned long R2 = 0;
      int Rc = DosRead(hpRead,Cur,End - Cur,&R2);

      if (Rc != 0)
         return;
      Cur += R2;
      *Cur = 0;
   }
}

/* ########################################################################

   Class - Pane (A display screen)
   Member - Constructor (Loads the screen from the resource)

   Gets the specified id form the resource block and redies it, ID = 0
   means do not load.

   ########################################################################
*/
Pane::Pane(Screen *Scr,unsigned long ID) : Scr(Scr)
{
   Data = 0;
   Size = 0;
   Lines = 0;
   Protect = 0;
   Bottom = 0;
   this->ID = ID + Scr->UIBase;

   if (ID == 0)
      return;

   // Load the Screen
   if (DosGetResource(0,1,this->ID,(void **)(&Data)) != 0)
      return;
   DosQueryResourceSize(0,1,this->ID,&Size);

   // Count the # of lines.
   for (ScreenCell *Ptr = Data + Size/sizeof(*Data) - 1;
        Ptr->Character == '#' && Ptr > (ScreenCell *)Data; Ptr--);
   Lines = (Ptr - (ScreenCell *)Data + 1)/80;
   Size /= sizeof(ScreenCell);

   // No custom ram block so use this one
   Resource = Data;
   Key = -1;
   if (DosCreateMutexSem(0,&Protect,0L,FALSE) != 0)
      somPrintf("Sem Construct error\n");
}

Pane::~Pane()
{
   if (Data != Resource)
      SOMFree(Data);
   if (ID != 0)
      DosFreeResource(Resource);
   DosCloseMutexSem(Protect);
}

// Make a copy of the screen so we can change it.
void Pane::Dup()
{
   Data = (ScreenCell *)SOMCalloc(Size,sizeof(ScreenCell));
   memcpy(Data,Resource,Size*sizeof(ScreenCell));
}

/* ########################################################################

   Class - Pane (A display screen)
   Member - DrawString (Draws a string)

   If this pane is not active the string is drawn to the buffer, otherwise
   it is drawn to the buffer and the display.

   ########################################################################
*/
void Pane::DrawString(const char *Text,int X,int Y,int Length)
{
   DosRequestMutexSem(Protect,-1);
   if (Data == Resource)
      Dup();

   char Center = 0;

   // Don't draw
   if (X < 0)
   {
      DosReleaseMutexSem(Protect);
      return;
   }

   if (X >= 100)
   {
      X -= 100;
      Center = 1;
   }

   ScreenCell *Start = Data + X + Y*80;
   ScreenCell *End = Start + Length;
   ScreenCell *DrawStart = Start;
   ScreenCell *DrawEnd = End;
   if (Center == 1)
   {
      if (Length > strlen(Text))
         DrawStart = (Length - strlen(Text))/2 + Start;
      else
      {
         const char *C = Text;
         const char *LastPos = Text;
         for (;*C != 0 && C - Text < Length; C++)
            if (*C == ' ')
               LastPos = C;
         if (LastPos != Text)
         {
            DrawStart = (Length - (LastPos - Text))/2 + Start;
            DrawEnd = DrawStart + (LastPos - Text);
         }
      }
   }

   for (;Start < End; Start++)
   {
      if (Text == 0 || *Text == 0 || Start < DrawStart || Start > DrawEnd)
         Start->Character = ' ';
      else
         Start->Character = *Text++;
   }

   if (Y < Lines)
      Scr->DrawString(X,Y,Length,this);
   DosReleaseMutexSem(Protect);
}

/* ########################################################################

   Class - Pane (A display screen)
   Member - DrawString (Draws a string)

   If this pane is not active the string is drawn to the buffer, otherwise
   it is drawn to the buffer and the display.

   ########################################################################
*/
void Pane::DrawBuffer(ScreenCell *B,int XLen, int YLen,int X,int Y)
{
   DosRequestMutexSem(Protect,-1);
   if (Data == Resource)
      Dup();

   for (int I = 0; I != YLen; I++)
   {
      memcpy(Data + 80*(Y+I) + X,B+I*XLen,sizeof(ScreenCell)*XLen);
      Scr->DrawString(X,Y + I,XLen,this);
   }
   DosReleaseMutexSem(Protect);
}
