#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <math.h>

#include <Muse.h>
#include <HandLst.hc>

#include <DART.hc>
#include <iostream.h>
#include <stdio.h>

#define INCL_DOS
#define INCL_VIO
#define INCL_KBD
#define INCL_DOSERRORS
#include <os2.h>

museDACDART *Output;
int Done = 0;

void APIENTRY Keyboard(unsigned long Data)
{
   while (1)
   {
      KBDKEYINFO info;
      memset(&info,0,sizeof(info));
      if (KbdCharIn(&info,IO_WAIT,0) != 0)
         return;

      switch (info.chChar)
      {
         // Esc
         case 27:
            Done = 1;
            break;
      }
   }
}

void main(char argc,char *argv[])
{
   if (argc <= 1)
   {
      cerr << "Useage: rawplay file.raw speed" << endl;
      return;
   }

   museDACDART::__ClassObject->Link();

   // Detect the output device
   museOutputClass *OutputClass = museDACDART::__ClassObject;

   // Open the output device
   Output = (museDACDART *)(OutputClass->somNew());
   if (Output == 0)
   {
      cout << "Unable to construct output class" << endl;
      return;
   }

   Output->SetMixParams(atoi(argv[2]),8,0);
   Output->RealTime = 1;

   if (argc >= 4)
      Output->MaxBufferLen = atol(argv[3]);

   char *Error = 0;
   if (Output->InitPlay(&Error) != 0)
   {
      cerr << "Error " << Error << endl;
      return;
   }

   if (strcmp(argv[1],"-") == 0)
   {
      cerr << "Ready" << endl;

      TID Tid;
      DosCreateThread(&Tid,&Keyboard,0,2,20000);
      DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,5,Tid);

      while (Done != 1)
      {
         unsigned char *Start;
         unsigned char *End;
         if (Output->GetNextBuffer(&Start,&End) != 0)
         {
            return;
         }

         while (Start != End)
         {
            int Rc = read(0,Start,End-Start);
            if (Rc <= 0)
            {
               cout << "Done" << endl;
               Done = 1;
               break;
            }
            Start += Rc;
         }
      }
   }
   else
   {

      int fd = open(argv[1],O_BINARY | O_RDONLY);
      if (fd <= 0)
      {
         cerr << "Can't open the file" << endl;
         return;
      }

      unsigned char *Buf = new unsigned char[64000];
      long Size = read(fd,Buf,64000);
      if (Size <= 0)
      {
         cerr << "Can't read" << endl;
         return;
      }

      TID Tid;
      DosCreateThread(&Tid,&Keyboard,0,2,20000);
      DosSetPriority(PRTYS_THREAD,PRTYC_REGULAR,5,Tid);

      unsigned char *Cur = Buf;
      unsigned char *CEnd = Buf + Size;
      while (Done != 1)
      {
         unsigned char *Start;
         unsigned char *End;
         if (Output->GetNextBuffer(&Start,&End) != 0)
         {
            cerr << "Done" << endl;
            return;
         }

         cout << End - Start << endl;

         while (Start != End)
         {
            for (;Cur != CEnd && Start != End; Cur++, Start++)
               *Start = *Cur;
            if (Cur == CEnd)
               Cur = Buf;
         }
      }
   }

   Output->StopPlay();
   DosSleep(1000);
}
