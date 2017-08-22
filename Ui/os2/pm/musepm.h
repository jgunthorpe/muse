/* ########################################################################

   Muse PM

   Class containts current state of all shared objects and hwnd's for all open
   windows. Has methods to allow change notification to occure as well.

   ########################################################################
*/
#ifndef MUSEPM_H
#define MUSEPM_H

#include "SngList.h"

class MusePM
{
   void MakeWindow(int ID,PFNWP Func);

   int DeathCounter;
   public:
   struct WinRec
   {
      HPOINTER Icon;
      HWND hwnd;                   // Subclass of the frame window
   };

   IDLSequence<WinRec> Windows;
   SongList *SngL;
   SongList::iterator Songs;

   HAB AnchorBlock;
   HMQ MsgQueue;

   static void ObjThread(void *ObjFunc);

   typedef void (* QueFunc)(HMQ,ULONG,MPARAM,MPARAM);
   HMQ MakeAsyncThread(QueFunc Func,int Idle = 1);
   void CreateWindows();
   void Go();
   MusePM();
   ~MusePM();
};

class SongListBox
{
   public:

   typedef SongList::iterator iterator;
   SongList *List;
   HWND hwnd;
   iterator Head;
   unsigned char NoSize;

   static MRESULT MeasureItem(HWND Parent,MPARAM mp1,MPARAM mp2);
   MRESULT DrawItem(MPARAM mp1,MPARAM mp2);
   void Refresh(iterator Head);

   SongListBox(HWND Parent,int ID);
};

class DirectoryListBox
{
   public:

   typedef SongList::iterator iterator;
   SongList *List;
   HWND hwnd;
   iterator Head;

   static MRESULT MeasureItem(HWND Parent,MPARAM mp1,MPARAM mp2);
   MRESULT DrawItem(MPARAM mp1,MPARAM mp2);
   void Refresh(iterator Head);

   DirectoryListBox(HWND Parent,int ID);
   ~DirectoryListBox();

   private:

   int JustParents(iterator Current,int &I);
   int AddNode(SongList::Node *N);
   SequenceOctet Depth;
   HPOINTER FolderOpen;
   HPOINTER Folder;
   HPOINTER Archive;
};

extern MusePM *App;

MRESULT EXPENTRY FileChoozer(HWND hwnd,ULONG msg, MPARAM mp1, MPARAM mp2);

void AsyncSetText(HWND wnd,const char *Text);
inline void AsyncSetText(HWND wnd,unsigned int ID, const char *Text)
{
   AsyncSetText(WinWindowFromID(wnd,ID),Text);
}

#endif
