/* ########################################################################

   Options - Handles command line options

   ########################################################################
*/
#ifndef CPPCOMPILE
#ifndef DTS
#include <SOMUtil\IDLSequence.xh>
#pragma pack(4)
#include <DAC.xh>
#include <FormatBs.xh>
#include <somcdev.h>
#include <DebugUtils.xh>
#pragma pack()
#else
#include <IDLSequence.hh>
#include <DAC.hh>
#include <FormatBs.hh>
#include <somcdev.hh>
#include <DebugUtils.hh>
#endif
#else
#include <IDLSeq.hc>
#include <DAC.hc>
#include <FormatBs.hc>
#include <DebugUtl.hc>
#endif

#include <MinMax.h>

#include <iostream.h>

#define INCL_KBD
#define INCL_VIO
#include <os2.h>

#include "Screen.h"
#include "SongList.h"

#include <fstream.h>

/* ########################################################################

   Class - Screen (The display screen)
   Member - AddDllsInDir (Checks a DLL for SOM objects)

   This function is used to get the file formats or output methods from
   a DLL. It was taken from BaseSet and changed to draw a dot after each
   load.

   ########################################################################
*/
long Screen::AddDllsInDir(string Dir, string Pattern)
{
   DBU_FuncTrace("museHandlerList","AddDllsInDir",TRACE_SIMPLE);

   char Name[300];
   strcpy(Name,Dir);

   // Add a trailing slash
   int Len = strlen(Name);
   if (Name[Len - 1] != '\\')
   {
      Name[Len] = '\\';
      Name[Len + 1] = 0;
   }

   // Make a copy of the directory
   char Name2[300];
   strcpy(Name2,Name);

   char *InsertP = Name2 + strlen(Name2);
   strcat(Name,Pattern);

   somPrintf(" S %s\n",Name);

   HDIR hdirFindHandle = HDIR_SYSTEM;
   FILEFINDBUF3 FindBuffer = {0};      //Returned from FindFirst/Next
   ULONG ulResultBufLen = sizeof(FILEFINDBUF3);
   ULONG ulFindCount = 1;              // Look for 1 file at a time
   APIRET rc = 0;                      // Return code

   rc = DosFindFirst( Name,                 // File pattern - all files
                      &hdirFindHandle,      // Directory search handle
                      FILE_NORMAL,          // Search attribute
                      &FindBuffer,          // Result buffer
                      ulResultBufLen,       // Result buffer length
                      &ulFindCount,         // Number of entries to find
                      FIL_STANDARD);        // Return level 1 file info

   // Keep finding the next file until there are no more files
   while (rc == 0)
   {
      strcpy(InsertP,FindBuffer.achName);

      Handlers->AddDll(Name2);
      cout << '.';
      ulFindCount = 1;                      // Reset find count.

      rc = DosFindNext(hdirFindHandle,      // Directory handle
                       &FindBuffer,         // Result buffer
                       ulResultBufLen,      // Result buffer length
                       &ulFindCount);       // Number of entries to find
   }

   rc = DosFindClose(hdirFindHandle);    // Close our directory handle
   return 0;
}

/* ########################################################################

   Class - Screen (The display screen)
   Member - HandleCommandLine (Handle all things on the command line)

   Do command line stuff. First thing done is to load the required dlls.

   ########################################################################
*/
int Screen::HandleCommandLine(char argc,char *argv[])
{
   DBU_FuncTrace("Screen","HandleCommandLine",TRACE_SIMPLE);

   // Remove the exe name
   char Dir[300];
   strcpy(Dir,argv[0]);
   for (char *C = Dir + strlen(Dir);*C != '\\' && C != Dir; C--);
   if (C != Dir)
      *(C+1) = 0;

   // Begin loading the various parts of the player
   cout << "Loading Components";
//   DebugFile = stderr;
   AddDllsInDir(Dir,"mus2*.dll");
   DebugFile = 0;

   // Quickly scan for the -C option
   for (unsigned int I = 1; I < argc; I++)
   {
      // Is an option
      if ((argv[I][0] == '-') || (argv[I][0] == '/'))
      {
         if ((argv[I][1] == 'o') || (argv[I][1] == 'O') || (argv[I][1] == 'S') ||
              argv[I][1] == 'U' || argv[I][1] == 'L' || argv[I][1] == 'f')
         {
            if (argv[I][2] == 0)
               I++;
            continue;
         }

         // -C?
         if (argv[I][1] == 'C')
         {
            // Fname is with the next argument
            if (argv[I][2] == 0)
            {
               I++;
               strcpy(Dir,argv[I]);
            }
            else
               strcpy(Dir,&argv[I][2]);

            // Split into path and file
            char *Path = argv[I] + strlen(argv[I]);
            for (char *C = Dir + strlen(Dir);*C != '\\' && C != Dir; C--,Path--);
            *(C+1) = 0;

            if (C == Dir)
               *C = '.';

            // Add the Dlls in
            AddDllsInDir(Dir,Path);
         }
      }
   }
   cout << endl;

   // Get a copy of the type lists
   Handlers->GetOutputs(&Outputs);
   Handlers->GetFormats(&FileFormats);
   Songs->Handlers = Handlers;

   // Process command line params
   char Help = 1;
   char Randomize = 0;  // 0 none, 1 rand, 2 sort
   char Write = 0;
   int SongCount = 0;
   for (I = 1; I < argc; I++)
   {
      // Is an option
      if ((argv[I][0] == '-') || (argv[I][0] == '/'))
      {
         switch (argv[I][1])
         {
            // Display Help by forcing files to zero
            case '?':
            case 'H':
            case 'h':
               Help = 2;
               I = argc;
               break;

            // Already handled
            case 'C':
               // Path with next argument
               if (argv[I][2] == 0)
                  I++;
               break;

            case 'D':
               // Setup debuging
               cout << "Debugging" << endl;
               DebugFile = fopen("Muse2Txt.dmp","wtu");
               fprintf(DebugFile,"Hello\n");
               #ifndef CPPCOMPILE
               SOM_TraceLevel = 6;
               SOM_WarnLevel = 2;
               #endif
               break;

            case 'P':
               HiPrio = 1;
               cout << "Using Higher Priority" << endl;
               break;

            case 'T':
               RealTime = TRUE;
               break;

            case 'r':
               Randomize = 1;
               break;

            case 'R':
               Randomize = 2;
               break;

            case 'l':
               if (argv[I][2] == 's')
               {
                  SongListLoop = 1;
                  break;
               }
               AllowLoop = FALSE;
               LoopStyle = 1;
               break;

            // Server mode
            case 'B':
               Server = TRUE;
               break;

            case 's':
               SongCount = 1;
               break;

            case 'W':
               Write = 1;
               break;

            // Song list limit
            case 'S':
            {
               char *Count;

               // Count is with next arg
               if (argv[I][2] == 0)
               {
                  I++;
                  if (I >= argc)
                     break;
                  Count = argv[I];
               }
               else
                  Count = &argv[I][2];
               SongCount = atoi(Count);
               break;
            }

            // Different UI's
            case 'U':
            {
               char *Count;

               // Count is with next arg
               if (argv[I][2] == 0)
               {
                  I++;
                  if (I >= argc)
                     break;
                  Count = argv[I];
               }
               else
                  Count = &argv[I][2];
               int UI = atoi(Count);
               if (UI == 1)
                  UIBase = 200;
               if (UI == 2)
                  UIBase = 100;
               break;
            }

            // Different Loop Styles
            case 'L':
            {
               char *Count;

               // Count is with next arg
               if (argv[I][2] == 0)
               {
                  I++;
                  if (I >= argc)
                     break;
                  Count = argv[I];
               }
               else
                  Count = &argv[I][2];
               int L = atoi(Count);
               LoopStyle = max(0,(int)min(L,2));
               if (LoopStyle == 1)
                  AllowLoop = FALSE;
               else
                  AllowLoop = TRUE;

               break;
            }

            // Volume control
            case 'v':
            {
            printf("Wank\n");
               char *Vol;

               // Count is with next arg
               if (argv[I][2] == 0)
               {
                  I++;
            printf("Wank4\n");
                  if (I >= argc)
                     break;
            printf("Wank3\n");
                  Vol = argv[I];
               }
               else
                  Vol = &argv[I][2];
               int V = atoi(Vol);
               DefVolume = max(0,(int)min(V,64));
            printf("Wank2\n");
               break;
            }

            // Version information
            case 'V':
            {
               cout << "Version Information:" << endl;

               long MajorV;
               long MinorV;
               #ifdef DTS
               char *Name = museDACMixer::__ClassObject->somGetName();
               museDACMixer::__ClassObject->somGetVersionNumbers(&MajorV,&MinorV);
               #else
               char *Name = _museDACMixerClass->somGetName();
               _museDACMixerClass->somGetVersionNumbers(&MajorV,&MinorV);
               #endif

               cout << "   " << "Muse/2 Digital Mixer" << " (" <<
                    Name << ") is Version " << MajorV <<
                    '.' << MinorV << endl;

               // Display file formats methoods
               cout << "  File Formats:" << endl;

               for (SequenceFormatClass::iterator I = FileFormats.begin(); I != FileFormats.end(); I++)
               {
                  long MajorV;
                  long MinorV;
                  (*I)->somGetVersionNumbers(&MajorV,&MinorV);
                  cout << "   " << (*I)->GetTypeName() << " (" <<
                       (*I)->somGetName() << ") is Version " << MajorV <<
                       '.' << MinorV << endl;
               }

               // Display output methoods
               cout << "  Output Methods:" << endl;

               for (SequenceOutputClass::iterator I2 = Outputs.begin(); I2 != Outputs.end(); I2++)
               {
                  long MajorV;
                  long MinorV;
                  (*I2)->somGetVersionNumbers(&MajorV,&MinorV);
                  cout << "   " << (*I2)->GetTypeName() << " (" <<
                       (*I2)->somGetName() << ") is Version " << MajorV << '.'
                       << MinorV;

                  int I3 = (*I2)->IsSupported();
                  if (I3 == 0xFF)
                  {
                     cout << endl;
                     continue;
                  }

                  if (I3 == 0)
                     cout << " (Not Detected)" << endl;
                  else
                     cout << " (Detected, Level=" << I3 << ')' << endl;
               }
               return 1;
            }

            case 'o':
               // Opts are with the next argument
               if (argv[I][2] == 0)
               {
                  I++;
                  if (I >= argc)
                     break;
                  OutputOptions = argv[I];
               }
               else
                  OutputOptions = &argv[I][2];
               break;

            case 'O':
            {
               char *Name;

               // Type is with next argument
               if (argv[I][2] == 0)
               {
                  I++;
                  if (I >= argc)
                     break;
                  Name = argv[I];
               }
               else
                  Name = &argv[I][2];

               // Find a class that has a matching type name (subset type match)
               for (SequenceOutputClass::iterator I2 = Outputs.begin(); I2 != Outputs.end(); I2++)
               {
                  if (stristr((*I2)->somGetName(),Name) != 0)
                  {
                     OutputClass = *I2;
                     break;
                  }
               }

               // Nope, didn't get one.
               if (OutputClass != *I2)
                  cout << "Could not find a match for '" << Name << "'" << endl;
               break;
            }

            case 'f':
            {
               if (OutputClass == 0)
               {
                  // Find a class that has a matching type name (subset type match)
                  for (SequenceOutputClass::iterator I2 = Outputs.begin(); I2 != Outputs.end(); I2++)
                  {
                     #ifndef DTS
                        if ((*I2)->somDescendedFrom(_museFileOutput) == TRUE)
                           break;
                     #else
                        if ((*I2)->somDescendedFrom(museFileOutput::__ClassObject) == TRUE)
                           break;
                     #endif
                  }

                  if (I2 != Outputs.end())
                     OutputClass = *I2;

                  if (OutputClass == 0)
                  {
                     cout << "Couldn't locate any File output devices" << endl;
                     return 1;
                  }
               }

               #ifndef DTS
                  if (OutputClass->somDescendedFrom(_museFileOutput) == FALSE)
               #else
                  if (OutputClass->somDescendedFrom(museFileOutput::__ClassObject) == FALSE)
               #endif
               {
                  cout << "Can't use file output with this device" << endl;
                  break;
               }
               // Opts are with the next argument
               if (argv[I][2] == 0)
               {
                  I++;
                  if (I >= argc)
                     break;
                  FileName = argv[I];
               }
               else
                  FileName = &argv[I][2];
               break;
            }

            default:
               cout << "Option " << argv[I][1] << " is not recognized! (-? for help)" << endl;
               break;
         }
      }
      else
      {
         if (Help != 2)
            Help = 0;

         // Must be a file, add to the list
         if (argv[I][0] == '@')
         {
            Songs->AddListFile(&argv[I][1]);
         }
         else
         {
            Songs->Add(argv[I]);
         }
      }
   };

   // Show the help
   if (Help != 0 && Server == FALSE)
      return this->Help();

   // No songs yet files were given.
   if (Songs->List.size() == 0 && Server == FALSE)
   {
      cout << "Sorry, couldn't find any songs to play (" << FileFormats.size() << " File formats loaded)" << endl;
      return 1;
   }

   // Display # of songs
   cout << "Song list Contains " << Songs->List.size() << " songs";

   // Ramdomize the list
   if (Randomize == 1)
   {
      cout << " Randomizing";
      Songs->Randomize();
   }

   // Sort the list
   if (Randomize == 2)
   {
      cout << " Sorting";
      Songs->Sort();
   }

   unsigned long Size = 0;
   for (DirItem *Cur = Songs->List.begin();Cur != Songs->List.end();Cur++)
      Size += Cur->Size;
   if (Size > 1024*1024*10)
      cout << ". " << (Size/(1024*1024)) << "Meg of songs in the list." << endl;
   else
      if (Size > 1024*10)
         cout << ". " << (Size/1024) << "K of songs in the list." << endl;
      else
         cout << ". " << Size << "bytes of songs in the list." << endl;

   // Play only the first xx songs
   if (SongCount != 0)
   {
      if (SongCount > Songs->List.size())
         SongCount = Songs->List.size();

      // Free the file names
      for (DirItem *Cur = Songs->List.begin() + SongCount;Cur != Songs->List.end();Cur++)
         Cur->Free();

      Songs->List._length = SongCount;
   }

   // Make a song list file
   if (Write == 1)
   {
      ofstream F("Songs.Lst");
      cout << "Writing Songs.Lst" << endl;

      // Free the file names
      for (Cur = Songs->List.begin();Cur != Songs->List.end();Cur++)
         F << Cur->Name << endl;

      return 1;
   }
   return 0;
};
