/* ########################################################################

   SongList - Handles Creating the song list

   ########################################################################
*/

#ifndef SONGLIST_H
#define SONGLIST_H

#include <stdio.h>

class DirItem
{
   public:
   char *Name;
   char *Title;
   unsigned long Flags;
   unsigned long Size;
   void operator =(DirItem const &From)
   {
      Free();
      Flags = From.Flags;
      Size = From.Size;
      if (From.Name != 0)
      {
         Name = new char[strlen(From.Name) + 1];
         strcpy(Name,From.Name);
      }
      if (From.Title != 0)
      {
         Title = (char *)SOMMalloc(strlen(From.Name) + 1);
         strcpy(Title,From.Title);
      }
   }
   void copy(DirItem &From)
   {
      Flags = From.Flags;
      Size = From.Size;
      Name = From.Name;
   }
   void Free()
   {
      delete [] Name;
      Flags = 0;Name = 0;Size = 0;Title = 0;
      SOMFree(Title);
   }
   DirItem() {Flags = 0;Name = 0;Size = 0;Title = 0;};
};

//class museHandlerList;
class SongList
{
   void WildCards(const char *C,IDLSequence<DirItem> &Seq);
   int SpawnZip(const char *Archiver,const char *Archive,const char *Args,const char *Files);

   int ProcessZipInt(const char *Archive, const char *Wild);
   void ProcessZipWithZip(const char *Archive, const char *Wild);
   int ProcessRARInt(const char *Archive, const char *Wild);
   void ProcessRARWithRAR(const char *Archive, const char *Wild);
   int MatchWild(const char *String,const char *Wild);

   unsigned long ZipPid;

   public:
   IDLSequence<DirItem> List;
   museHandlerList *Handlers;

   void ProcessZip(const char *Archive, const char *Wild);
   void ProcessRAR(const char *Archive, const char *Wild);
   void AddItem(DirItem &Item);
   int AddListFile(const char *Path);
   void Add(const char *Path);
   void Dump();

   void Sort();
   void Randomize();

   unsigned char *LoadFile(DirItem &File, char ForceLoad = 0);

   SongList();
   ~SongList();
};

char *stristr(const char *str1, const char *str2);

#endif
