/* ########################################################################

   ZIP File Node

   ########################################################################
*/
#ifndef ZIP_H
#define ZIP_H

class ZipFileType: public NodeType
{
   public:
   static int ID;

   virtual int Refresh(iterator Head);
   virtual int IsType(const char *Name);
   virtual void ConfigNode(iterator Node);

   void SetTempKill(iterator Head);
   void KillTempKill(iterator Head);

   ZipFileType(SongList *Owner) : NodeType(Owner) {Append = '\\';};
};

class ZipDirType: public NodeType
{
   public:
   static int ID;
   ZipDirType(SongList *Owner) : NodeType(Owner) {Append = '\\';};
};

/*class ZipModuleType: public NodeType
{
   public:
   static int ID;
   ZipModuleType(SongList *Owner) : NodeType(Owner) {};
};*/

#endif
