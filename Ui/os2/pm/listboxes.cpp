/* ########################################################################

   Muse PM -- List Box Code

800x600 - 13
1280x1024 - 16
1152x882 - 16

   ########################################################################
*/
#include <Muse.h>

#define INCL_WIN
#define INCL_GPI
#include <pm.h>

#include "MusePM.rh"
#include "MusePM.h"
#include <iomanip.h>

/* ########################################################################

   Class - SongListBox (Manages an Owner Draw List box containing a songlist)
   Member - Constructor (Simple init)

   Inits the class, finds the listbox HWND

   ########################################################################
*/
static int LongNum = 0;
SongListBox::SongListBox(HWND Parent,int ID)
{
   hwnd = WinWindowFromID(Parent,ID);
};

/* ########################################################################

   Class - SongListBox (Manages an Owner Draw List box containing a songlist)
   Member - MeasureItem (Returns the item size)

   Sizes the item.

   ########################################################################
*/
MRESULT SongListBox::MeasureItem(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
   HPS hps = WinGetPS(hwnd);

   // Query the font metrics
   FONTMETRICS fmMetrics ;
   GpiQueryFontMetrics(hps,sizeof (FONTMETRICS),&fmMetrics);
   char *S = "9999 M";
   POINTL p[5];
   GpiQueryTextBox(hps,strlen(S),S,5,p);
   LongNum = p[TXTBOX_TOPRIGHT].x + 1;

   WinReleasePS (hps);

   // Width is fixed by the list itself, so return 0 width
   cerr << "Height " << fmMetrics.lMaxBaselineExt << endl;
   return MPFROM2SHORT(max(fmMetrics.lMaxBaselineExt,16),0);
};

/* ########################################################################

   Class - SongListBox (Manages an Owner Draw List box containing a songlist)
   Member - DrawItem (Handles the Owner Draw WM_DRAWITEM message)

   Draws the item.

   ########################################################################
*/
MRESULT SongListBox::DrawItem(MPARAM mp1,MPARAM mp2)
{
   POWNERITEM Item = (POWNERITEM)PVOIDFROMMP(mp2);

   // Get the node pointer
   SongList::iterator Node = (SongList::Node *)Item->hItem;
   if (Node == 0)
      return MPFROMSHORT(TRUE);

   // Check the selection state, and twiddle colours to match
   COLOR clrBackGround;
   COLOR Size;
   COLOR Name;
   COLOR Title;
   if (Item->fsState)
   {
      Size = CLR_WHITE;
      Title = Name = Size;
      clrBackGround = CLR_DARKBLUE;
   }
   else
   {
      Size = CLR_RED;
      Title = CLR_NEUTRAL;

      // Ahh, it's a child of one of our sub nodes..
      if (Node->Parent != Head)
         Name = CLR_PINK;
      else
         Name = CLR_DARKBLUE;
      clrBackGround = CLR_WHITE;
   }

   // Draw background
   WinDrawText (Item->hps,-1,"",&Item->rclItem,CLR_NEUTRAL,clrBackGround,
                DT_LEFT | DT_VCENTER | DT_ERASERECT);



   RECTL DrawRect = Item->rclItem;
   int Span = DrawRect.xRight - DrawRect.xLeft;
   if (NoSize == 0)
   {
      Span -= LongNum;
      DrawRect.xRight = DrawRect.xLeft + Span*0.40;
   }
   else
      DrawRect.xRight = DrawRect.xLeft + Span*0.60;

   DrawRect.xLeft += 2;

   // Draw the name
   char S[300];
   if (Node->TypeId == 0)
      sprintf(S,"%s",Node->Name);
   else
      sprintf(S,"%s%c",Node->Name,App->SngL->Types[Node->TypeId]->Append);

   WinDrawText (Item->hps,
                -1,
                S,
                &DrawRect,
                Name,
                clrBackGround,
                DT_LEFT | DT_VCENTER);

   DrawRect.xRight =  DrawRect.xLeft;
   if (NoSize == 0)
      DrawRect.xLeft = Item->rclItem.xLeft + Span*0.60;
   else
      DrawRect.xLeft = Item->rclItem.xLeft + Span*0.40;

   // Draw the title
   if (Node->Info != 0 && Node->Info->Title != 0)
      strcpy(S,Node->Info->Title);
   else
      S[0] = 0;

   WinDrawText (Item->hps,
                -1,
                S,
                &DrawRect,
                Title,
                clrBackGround,
                DT_LEFT | DT_VCENTER);

   if (NoSize == 0)
   {
      // Draw the size
      DrawRect = Item->rclItem;
      DrawRect.xRight -= 2;
      DrawRect.xLeft = DrawRect.xRight - LongNum;

      // Convert to M,k,b
      if (Node->Size > 1024*1024*10)
         sprintf(S,"%u M",Node->Size/(1024*1024));
      else
         if (Node->Size > 1024*10)
            sprintf(S,"%u k",Node->Size/1024);
         else
            sprintf(S,"%u",Node->Size);

      WinDrawText (Item->hps,
                   -1,
                   S,
                   &DrawRect,
                   Size,
                   clrBackGround,
                   DT_RIGHT | DT_VCENTER);
   }

   Item->fsStateOld = FALSE;    //clear so PM won't hilite
   Item->fsState = FALSE;   //clear so PM won't hilite also
   return MPFROMSHORT(TRUE);
};

/* ########################################################################

   Class - SongListBox (Manages an Owner Draw List box containing a songlist)
   Member - Refresh (Loads the list box with all the items below Head)

   Rewrites the list box with a new head item.

   ########################################################################
*/
void SongListBox::Refresh(iterator Head)
{
   if (Head == 0)
      return;

   // Redraw must be disabled or the list will try to update with 0 item handles
   WinEnableWindowUpdate(hwnd,FALSE);
   WinSendMsg(hwnd,LM_DELETEALL,0,0);
   this->Head = Head;

   // Count em
   int I = 0;
   for (iterator C = Head->Child; C != 0; C++)
      if (C->Directory == 0)
         I++;
      else
      {
         // Add sub dirs with 1 item
         if (C->Refreshed == 1 && C->Child != 0 && C->Child->Next == 0 &&
             C->Child->Directory == 0)
            I++;
      }

   // List is alread empty, so just return
   if (I == 0)
   {
      WinEnableWindowUpdate(hwnd,TRUE);
      return;
   }

   // Get the list to add I blanks
   LBOXINFO InsertInfo;
   memset(&InsertInfo,0,sizeof(InsertInfo));
   InsertInfo.ulItemCount = I;
   InsertInfo.lItemIndex = LIT_END;
   WinSendMsg(hwnd,LM_INSERTMULTITEMS,MPFROMP(&InsertInfo),0);

   // Set the handles for the blanks.
   I = 0;
   for (C = Head->Child; C != 0; C++)
      if (C->Directory == 0)
      {
         WinSendMsg(hwnd,LM_SETITEMHANDLE,MPFROMLONG(I),MPFROMP(C.TrueNode()));
         I++;
      }
      else
      {
         // Add sub dirs with 1 item
         if (C->Refreshed == 1 && C->Child != 0 && C->Child->Next == 0 &&
             C->Child->Directory == 0)
         {
            WinSendMsg(hwnd,LM_SETITEMHANDLE,MPFROMLONG(I),MPFROMP(C->Child));
            I++;
         }
      }

   // Refresh
   WinEnableWindowUpdate(hwnd,TRUE);
};

/* ########################################################################

   Class - DirectoryListBox (Manages an Owner Draw List box containing a songlist)
   Member - Constructor (Simple init)

   Inits the class, finds the listbox HWND

   ########################################################################
*/
DirectoryListBox::DirectoryListBox(HWND Parent,int ID)
{
   hwnd = WinWindowFromID(Parent,ID);
   Depth.construct();
   Depth.reserve(400);

   Folder = WinLoadPointer(HWND_DESKTOP,0L,ICO_FOLDER);
   FolderOpen = WinLoadPointer(HWND_DESKTOP,0L,ICO_FOLDEROPEN);
   Archive = WinLoadPointer(HWND_DESKTOP,0L,ICO_ARCHIVE);
};

DirectoryListBox::~DirectoryListBox()
{
   WinDestroyPointer(Folder);
   WinDestroyPointer(FolderOpen);
   WinDestroyPointer(Archive);
}

/* ########################################################################

   Class - DirectoryListBox (Manages an Owner Draw List box containing a songlist)
   Member - MeasureItem (Returns the item size)

   Sizes the item.

   ########################################################################
*/
MRESULT DirectoryListBox::MeasureItem(HWND hwnd,MPARAM mp1,MPARAM mp2)
{
   HPS hps = WinGetPS(hwnd);

   // Query the font metrics
   FONTMETRICS fmMetrics ;
   GpiQueryFontMetrics(hps,sizeof (FONTMETRICS),&fmMetrics);

   WinReleasePS (hps);

   // Width is fixed by the list itself, so return 0 width
   cerr << "Height " << fmMetrics.lMaxBaselineExt << endl;
   return MPFROM2SHORT(max(fmMetrics.lMaxBaselineExt,16),0);
};

/* ########################################################################

   Class - DirectoryListBox (Manages an Owner Draw List box containing a songlist)
   Member - DrawItem (Handles the Owner Draw WM_DRAWITEM message)

   Draws the item.

   ########################################################################
*/
MRESULT DirectoryListBox::DrawItem(MPARAM mp1,MPARAM mp2)
{
   POWNERITEM Item = (POWNERITEM)PVOIDFROMMP(mp2);

   // Get the node pointer
   SongList::Node *Node = (SongList::Node *)Item->hItem;
   if (Node == 0)
      return MPFROMSHORT(TRUE);

   // Check the selection state, and twiddle colours to match
   COLOR clrForeGround;
   COLOR clrBackGround;
   COLOR Size;
   if (Item->fsState)
   {
      clrForeGround = CLR_WHITE;
      clrBackGround = CLR_DARKBLUE;
      Size = clrForeGround;
   }
   else
   {
      clrForeGround = CLR_BLUE;
      if (Node->Archive == 1)
         clrForeGround = CLR_DARKPINK;          // normal colors
      if (Node->Archived == 1)
         clrForeGround = CLR_DARKGREEN;
      Size = CLR_RED;
      clrBackGround = CLR_WHITE;
      if (Node->Refreshed == 1 && Node->Child != 0 && Node->Child->Next == 0 &&
          Node->Child->Directory == 0)
         clrForeGround = CLR_PINK;
   }

   // Draw background
   WinDrawText (Item->hps,-1,"",&Item->rclItem,clrForeGround,clrBackGround,
                DT_LEFT | DT_VCENTER | DT_ERASERECT);

   char S[300];
   if (Node->TypeId == 0)
      sprintf(S,"%s",Node->Name);
   else
      sprintf(S,"%s%c",Node->Name,App->SngL->Types[Node->TypeId]->Append);

   RECTL DrawRect = Item->rclItem;
   DrawRect.xLeft += 3*Depth[Item->idItem] + 16 + 3;

   if (Node->Archive == 1)
      DrawRect.xRight -= LongNum;

   // Draw it
   WinDrawText (Item->hps,-1,S,&DrawRect,clrForeGround,clrBackGround,
                DT_LEFT | DT_VCENTER);

   HPOINTER Ico = 0;
   if (Depth[Item->idItem] != Depth[Item->idItem + 1])
      Ico = FolderOpen;
   else
      Ico = Folder;

   if (Node->Archive == 1)
      Ico = Archive;

   WinDrawPointer(Item->hps,Item->rclItem.xLeft + 3*Depth[Item->idItem] + 2,Item->rclItem.yBottom + 2,Ico,DP_MINI);

   if (Node->Archive == 1)
   {
      // Draw the size
      DrawRect = Item->rclItem;
      DrawRect.xRight -= 2;
      DrawRect.xLeft = DrawRect.xRight - LongNum;

      // Convert to M,k,b
      if (Node->Size > 1024*1024*10)
         sprintf(S,"%u M",Node->Size/(1024*1024));
      else
         if (Node->Size > 1024*10)
            sprintf(S,"%u k",Node->Size/1024);
         else
            sprintf(S,"%u",Node->Size);

      WinDrawText (Item->hps,
                   -1,
                   S,
                   &DrawRect,
                   Size,
                   clrBackGround,
                   DT_RIGHT | DT_VCENTER);
   }
   Item->fsStateOld = FALSE;    //clear so PM won't hilite
   Item->fsState = FALSE;   //clear so PM won't hilite also
   return MPFROMSHORT(TRUE);
};

/* ########################################################################

   Class - DirectoryListBox (Manages an Owner Draw List box containing a songlist)
   Member - Refresh (Loads the list box with all the items below Head)

   Rewrites the list box with a new head item.

   ########################################################################
*/
int DirectoryListBox::JustParents(iterator Current,int &I)
{
   if (Current == 0)
      return 0;
   int Level = JustParents(Current->Parent,I);
   Depth[I] = Level;
   if (AddNode(Current) != 0)
      I++;
   return Level + 1;
}

void DirectoryListBox::Refresh(iterator Head)
{
   if (Head == 0)
      return;

   // Redraw must be disabled or the list will try to update with 0 item handles
   WinEnableWindowUpdate(hwnd,FALSE);
   WinSendMsg(hwnd,LM_DELETEALL,0,0);
   Depth.erasefrom(Depth.begin());
   this->Head = Head;

   // Top Node..
   int Item = 0;
   int Level = 0;
   if (Head->Parent != 0)
      Level = JustParents(Head.TrueNode(),Item);

   // Set the handles for all Children
   iterator I = Head->Child;
   for (; I != 0; I++)
   {
      Depth[Item] = Level;
      if (AddNode(I) != 0)
         Item++;
   }
   Depth[Item] = Level;
   // Refresh
   WinEnableWindowUpdate(hwnd,TRUE);
};

int DirectoryListBox::AddNode(SongList::Node *N)
{
   if (N->Directory == 0)
      return 0;

   int I = SHORT1FROMMR(WinSendMsg(hwnd,LM_INSERTITEM,MPFROMLONG(LIT_END),MPFROMP("")));
   WinSendMsg(hwnd,LM_SETITEMHANDLE,MPFROMLONG(I),MPFROMP(N));

   return 1;
}
