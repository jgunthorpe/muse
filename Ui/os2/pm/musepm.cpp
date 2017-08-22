/* ########################################################################

   Muse PM

   ########################################################################
*/
#include <Muse.h>

#define INCL_WIN
#define INCL_GPI
#include <pm.h>

#include "MusePM.rh"
#include "MusePM.h"
#include <iomanip.h>
#include <process.h>

MRESULT EXPENTRY NullWndProc(HWND hwnd,ULONG msg, MPARAM mp1, MPARAM mp2)
{
   return WinDefDlgProc(hwnd,msg,mp1,mp2);
};

static SongListBox *Songs = 0;
static SongListBox *BrowserFiles = 0;
static DirectoryListBox *BrowserDirs = 0;
static HMQ WorkerThread = 0;
static HWND FileWnd = 0;
static PFNWP OldListProc;
static HPOINTER DiskWaitPointer;
static HPOINTER WaitPointer;
static HPOINTER ListPointer = 0;
static SongList::Node *CurHead = 0;

static MRESULT EXPENTRY PointerSubClass(HWND hwnd,ULONG msg, MPARAM mp1, MPARAM mp2)
{
   if (msg == WM_MOUSEMOVE && ListPointer != 0)
   {
      WinSetPointer(HWND_DESKTOP,ListPointer);
      return 0;
   }
   return OldListProc(hwnd,msg,mp1,mp2);
}

static void FileChoozerThread(HMQ hq,ULONG msg,MPARAM mp1, MPARAM mp2)
{
   switch (msg)
   {
      // Request to refresh everything
      case WM_USER+1:
      {
         // Flush
         QMSG qmsg;
         if (WinPeekMsg(hq,&qmsg,0,0,0,0) == TRUE && qmsg.msg >= WM_USER)
            return;

         // Blank them
         SongList::iterator Node = (SongList::Node *)mp1;
         for (SongList::iterator Cur = Node->Child; Cur != 0; Cur++)
            Cur->Refreshed = 0;

         // Fall through and do the refresh
      }

      // Request to refresh the list boxes
      case WM_USER:
      {
         // Flush
         QMSG qmsg;
         if (WinPeekMsg(hq,&qmsg,0,0,0,0) == TRUE && qmsg.msg >= WM_USER)
            return;

         SongList::iterator Node = (SongList::Node *)mp1;

         // Descend past empty directories
         int First = 1;
         do
         {
            // Go Down!
            if (First == 0)
               Node = Node->Child;
            else
               First = 0;

            /* Refresh the directory, don't refresh it if it's an archive
               that has been scanned, logic is archive scanning is slow */
            if (Node->Directory == 1 && (Node->Archive == 0 || Node->Refreshed == 0))
            {
               while (Node != 0)
               {
                  // Change the pointer
                  ListPointer = DiskWaitPointer;
                  WinSetPointer(HWND_DESKTOP,ListPointer);
                  AsyncSetText(FileWnd,ST_CURREFRESH,Node->Name);

                  SongList::iterator Parent = Node;
                  if (App->SngL->Refresh(Node) != 0)
                  {
                     AsyncSetText(FileWnd,ST_CURREFRESH,0);
                     /* Cute, our Node pointer is now invalid, we must find
                        a valid one someplace.. Look upwards. */
                     Node = Parent;
                     cerr << "FAILED" << endl;
                     continue;
                  }
                  AsyncSetText(FileWnd,ST_CURREFRESH,0);
                  break;
               }
            }

            if (Node == 0 || Node->Refreshed == 0)
            {
               cerr << "PANIC!! Refresher broke" << endl;
               break;
            }
         }
         while (Node->Child != 0 && Node->Child->Next == 0 && Node->Child->Directory == 1);

         // Update the listboxes
         if (Node->Directory == 1)
            WinPostMsg(FileWnd,WM_USER+1,Node.Cur,0);

         // Change the pointer
         if (ListPointer == 0)
         {
            ListPointer = DiskWaitPointer;
            WinSetPointer(HWND_DESKTOP,ListPointer);
         }

         // Scan the new node for unrefreshed archives
         int Refresh = 0;
         for (SongList::iterator Cur = Node->Child; Cur != 0; Cur++)
         {
            if (Cur->Refreshed == 0 && Cur->Archive == 1)
            {
               // Abort!!
               if (WinPeekMsg(hq,&qmsg,0,0,0,0) == TRUE && qmsg.msg == WM_USER)
               {
                  Refresh = 0;
                  break;;
               }

               AsyncSetText(FileWnd,ST_CURREFRESH,Cur->Name);
               App->SngL->Refresh(Cur);
               if (Cur->Child != 0 && Cur->Child->Next == 0 && Cur->Child->Directory == 0)
                  Refresh = 1;
            }

            // Abort!!
            if (WinPeekMsg(hq,&qmsg,0,0,0,0) == TRUE && qmsg.msg == WM_USER)
            {
               Refresh = 0;
               break;;
            }
         }
         AsyncSetText(FileWnd,ST_CURREFRESH,0);

         // Refresh again
         if (Refresh == 1)
            WinPostMsg(FileWnd,WM_USER+1,Node.Cur,0);

         // Change the pointer
         ListPointer = 0;
         WinSetPointer(HWND_DESKTOP,WinQuerySysPointer(HWND_DESKTOP,SPTR_ARROW,FALSE));
      };
   }
   return;
}

MRESULT EXPENTRY FileChoozerWndProc(HWND hwnd,ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg)
   {
      case WM_INITDLG:
         FileWnd = hwnd;
         Songs = new SongListBox(hwnd,LB_SONGLIST);
         BrowserFiles = new SongListBox(hwnd,LB_FILES);
         BrowserDirs = new DirectoryListBox(hwnd,LB_DIRS);

         CurHead = App->SngL->Head;
         Songs->List = App->SngL;
         BrowserFiles->List = App->SngL;
         BrowserDirs->List = App->SngL;

/*         Songs->Refresh(CurHead);
         BrowserFiles->Refresh(CurHea);
         BrowserDirs->Refresh(CurHead);*/

         // Apply pointer subclassing to the 2 listboxes
         OldListProc = WinSubclassWindow(BrowserDirs->hwnd,PointerSubClass);
         WinSubclassWindow(BrowserFiles->hwnd,PointerSubClass);

         // Fire off the helper thread
         WorkerThread = App->MakeAsyncThread(FileChoozerThread);
         DiskWaitPointer = WinLoadPointer(HWND_DESKTOP,0,PTR_DISKWAIT);
         WaitPointer = WinLoadPointer(HWND_DESKTOP,0,PTR_WAIT);

         // Init refresh
         WinPostQueueMsg(WorkerThread,WM_USER,CurHead,0);
         break;

      case WM_MEASUREITEM:
         switch (SHORT1FROMMP(mp1))
         {
            case LB_SONGLIST:
            case LB_FILES:
            case LB_DIRS:
               return SongListBox::MeasureItem(hwnd,mp1,mp2);
         }
         break;

      case WM_DRAWITEM:
         switch (SHORT1FROMMP(mp1))
         {
            case LB_SONGLIST:
               if (Songs == 0) break;
               return Songs->DrawItem(mp1,mp2);
            case LB_FILES:
               if (BrowserFiles == 0) break;
               return BrowserFiles->DrawItem(mp1,mp2);
            case LB_DIRS:
               if (BrowserDirs == 0) break;
               return BrowserDirs->DrawItem(mp1,mp2);
         }
         break;

      case WM_CONTROL:
         if (SHORT1FROMMP(mp1) == LB_FILES || SHORT1FROMMP(mp1) == LB_DIRS)
         {
            if (SHORT2FROMMP(mp1) == LN_ENTER)
            {
               HWND hwnd = LONGFROMMP(mp2);
               short Item = SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYSELECTION,MPFROMSHORT(LIT_CURSOR),0));
               if (Item == LIT_NONE)
                  return 0;

               SongList::Node *Node = (SongList::Node *)WinSendMsg(hwnd,LM_QUERYITEMHANDLE,MPFROMSHORT(Item),0);
               if (Node == 0)
                  return 0;

               if (Node->Directory == 1)
               {
                  /* The worker is busy scanning zips, let the user know we got
                     the click */
                  if (ListPointer != 0)
                  {
                     ListPointer = WaitPointer;
                     WinSetPointer(HWND_DESKTOP,ListPointer);
                  }

                  // Worker thread does all this work
                  WinPostQueueMsg(WorkerThread,WM_USER,Node,0);
               }
               else
               {
                  App->SngL->Link(App->Songs,Node);
                  Songs->Refresh(App->Songs);
               }
            }
            break;
         }
         break;

      case WM_COMMAND:
         cerr << "Control, " << SHORT1FROMMP(mp1) << endl;
         switch (SHORT1FROMMP(mp1))
         {
            // Add a node
            case PB_ADD:
            {
               HWND hwnd = BrowserFiles->hwnd;
               short Item = SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST),0));
               if (Item == LIT_NONE)
                  return 0;

               while (Item != LIT_NONE)
               {
                  SongList::Node *Node = (SongList::Node *)WinSendMsg(hwnd,LM_QUERYITEMHANDLE,MPFROMSHORT(Item),0);
                  if (Node == 0)
                     continue;;

                  App->SngL->Link(App->Songs,Node);
                  Item = SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYSELECTION,MPFROMSHORT(Item),0));
               }
               Songs->Refresh(App->Songs);
               break;
            }

            // Delete a node
            case PB_REMOVE:
            {
               HWND hwnd = Songs->hwnd;
               short Item = SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYSELECTION,MPFROMSHORT(LIT_FIRST),0));
               if (Item == LIT_NONE)
                  return 0;

               while (Item != LIT_NONE)
               {
                  SongList::Node *Node = (SongList::Node *)WinSendMsg(hwnd,LM_QUERYITEMHANDLE,MPFROMSHORT(Item),0);
                  if (Node == 0)
                     continue;
                  App->SngL->Free(Node);
                  Item = SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYSELECTION,MPFROMSHORT(Item),0));
               }
               Songs->Refresh(App->Songs);
               break;
            }

            // Refresh the current dir.
            case PB_REFRESH:
            {
               /* The worker is busy scanning zips, let the user know we got
                  the click */
               if (ListPointer != 0)
               {
                  ListPointer = WaitPointer;
                  WinSetPointer(HWND_DESKTOP,ListPointer);
               }

               // Worker thread does all this work
               WinPostQueueMsg(WorkerThread,WM_USER+1,BrowserDirs->Head,0);
            }
         }
         break;

      // Worker wants us to refresh the listboxes
      case WM_USER+1:
         BrowserDirs->Refresh((SongList::Node *)mp1);
         BrowserFiles->Refresh((SongList::Node *)mp1);
         Songs->Refresh(App->Songs);
         break;

      case WM_CLOSE:
         WinDestroyWindow(hwnd);
         WinPostQueueMsg(WorkerThread,WM_QUIT,0,0);
         WinPostMsg(0,WM_QUIT,0,0);
         break;

      case WM_DESTROY:
         delete Songs;
         Songs = 0;
         delete BrowserFiles;
         BrowserFiles = 0;
         delete BrowserDirs;
         BrowserDirs = 0;
         break;

      default:
         return WinDefDlgProc(hwnd,msg,mp1,mp2);
   }
   return 0;
};

/* ########################################################################

   Class - MusePM (Manages the program)
   Member - Constructor (Inits)

   Creates the songlist, anchor block and message queue.

   ########################################################################
*/
MusePM::MusePM()
{
   DeathCounter = 0;

   AnchorBlock = WinInitialize(0UL);
   MsgQueue = WinCreateMsgQueue(AnchorBlock,400);
   if (MsgQueue == 0)
      exit(1);

   WinRegisterClass(AnchorBlock,"ObjectWindow",0,0,0);

   cerr << "SongList" << endl;
   SngL = new SongList;
   SngL->Refresh(SngL->Head);
   Songs = SngL->New();
   Songs->UnSorted = 1;
   cerr << "SongList End" << endl;

   Windows.construct();
}


MusePM::~MusePM()
{
   for (WinRec *I = Windows.begin(); I != Windows.end(); I++)
   {
      WinDestroyWindow(I->hwnd);
      WinDestroyPointer(I->Icon);
   }

   // Allow object windows to shutdown;
   for (int I2 = 0; I2 != 10 && DeathCounter != 0; I2++)
      DosSleep(100);

   delete SngL;
   WinDestroyMsgQueue(MsgQueue);
   WinTerminate(AnchorBlock);
}

/* ########################################################################

   Class - MusePM (Manages the program)
   Member - CreateWindows (Builds each muse window)

   Creates all main muse windows.

   ########################################################################
*/
void MusePM::CreateWindows()
{
   MakeWindow(DLG_FILECHOOZER,FileChoozerWndProc);
}

/* ########################################################################

   Class - MusePM (Manages the program)
   Member - Go (Message Loop)

   ########################################################################
*/
void MusePM::Go()
{
   QMSG  qmsg;
   while (WinGetMsg(AnchorBlock, &qmsg, (HWND)NULL, 0UL, 0UL))
      WinDispatchMsg(AnchorBlock, &qmsg);
}

/* ########################################################################

   Class - MusePM (Manages the program)
   Member - MakeWindow (Helper to build a main window dialog from a resource)

   Loads up the icon and attaches it to the frame.

   Note, The dialog loader subclasses the frame window and assigns that to
   the dialog procedure.. Very neat -- there is no client window.

   ########################################################################
*/
void MusePM::MakeWindow(int ID,PFNWP Func)
{
   HWND hwnd = Windows[ID].hwnd = WinLoadDlg(HWND_DESKTOP,HWND_DESKTOP,
                                        Func,0,ID,0);
   // Frame window can't get icon ID from the Resource, interven.
   if ((Windows[ID].Icon = WinLoadPointer(HWND_DESKTOP,0,ID)) != 0)
      WinSendMsg(hwnd,WM_SETICON,(MPARAM)Windows[ID].Icon,0);
   else
      cerr << "No Icon" << endl;

   // Don't ask me why, this makes it works.. (Forces the frame to relayout clients?)
   SWP P;
   WinQueryWindowPos(hwnd,&P);
   WinSetWindowPos(P.hwnd,0,P.x,P.y,P.cx,P.cy + 1,SWP_SIZE);
}

/* ########################################################################

   Class - MusePM (Manages the program)
   Member - ObjThread (Manages object window threads)

   Creates an object window and manages it's message loop.

   ########################################################################
*/
struct CreateComm
{
   MusePM::QueFunc Func;
   HMQ hq;
   HEV Sem;
   int *DeathCounter;
};

void MusePM::ObjThread(void *ObjFunc)
{
   CreateComm &Com = *((CreateComm *)ObjFunc);
   HAB AnchorBlock = WinInitialize(0UL);
   HMQ MsgQueue = WinCreateMsgQueue(AnchorBlock, 40L);
   int *DeathCounter = Com.DeathCounter;
   *DeathCounter += 1;
   QueFunc Func = Com.Func;
   Com.hq = MsgQueue;

   DosPostEventSem(Com.Sem);

   // Our message loop.
   QMSG  qmsg;
   while (WinGetMsg(AnchorBlock, &qmsg, (HWND)NULL, 0UL, 0UL))
      Func(MsgQueue,qmsg.msg,qmsg.mp1,qmsg.mp2);

   *DeathCounter -= 1;
   WinDestroyMsgQueue(MsgQueue);
   WinTerminate(AnchorBlock);
}

HMQ MusePM::MakeAsyncThread(MusePM::QueFunc Func,int Idle)
{
   CreateComm Com;
   Com.Func = Func;
   Com.DeathCounter = &DeathCounter;
   DosCreateEventSem(0,&Com.Sem,0,0);

   TID Tid = _beginthread(ObjThread,0,40000,&Com);

   if (Idle == 1)
      DosSetPriority(PRTYS_THREAD,PRTYC_IDLETIME,0x1F,Tid);

   DosWaitEventSem(Com.Sem,-1);
   DosCloseEventSem(Com.Sem);
   return Com.hq;
}

// Sets control text with PostMsg.
void AsyncSetText(HWND wnd,const char *Text)
{
   WNDPARAMS p;
   memset(&p,0,sizeof(p));

   if (Text == 0)
      Text = "";

   p.fsStatus = WPM_TEXT;
   p.cchText = strlen(Text);
   p.pszText = (char *)Text;

   while (WinPostMsg(wnd,WM_SETWINDOWPARAMS,&p,0) != TRUE);
}

MusePM *App = 0;
void main(char argc, char *argv[])
{
   cerr << "Start" << endl;
   App = new MusePM;
   App->CreateWindows();
   App->Go();
   delete App;
}
