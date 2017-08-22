int MyPOpen(char *Program,char *Arg)
{
   char ArgStr[10000];

   sprintf(ArgStr,"%s %s",Program,Arg);
   ArgStr[strlen(Program)] = 0;

   HPIPE hpRead = -1;
   HPIPE hpWrite = -1;

   HFILE Save = -1;
   HFILE New = 0;
   DosDupHandle(0,&Save);                 // Save the 0 handle
   DosCreatePipe(&hpRead,&hpWrite,10);
   DosDupHandle(hpRead,&New);             // Attach the read handle to the 0 handle

   RESULTCODES rec;
   char szFailName[100];
   APIRET Rc = DosExecPgm(szFailName,sizeof(szFailName),EXEC_ASYNC,ArgStr,0,&rec,Program);

   DosClose(hpRead);
   DosDupHandle(Save,&New);

   if (Rc != 0)
   {
      DosClose(hpRead);
      return -1;
   }

   return hpWrite;
}
