// -*- mode: cpp; mode: fold -*-

#include <stdio.h>
#include <llist.h>
#include <string>

struct Test
{
   int I;
};

template class LList<int>;
template class LList<Test>;

int main(char argc,char *argv[])
{
   LList<Test> List;

   Test T;
   for (T.I = 0; T.I != 100; T.I++)
      List.push_front(T); 
   
   LList<Test>::iterator C;
   for (C = List.begin(); C != List.end(); C++)
      printf("%i\n",C->I);
   
   printf("%u\n",sizeof(string));
};

// LineFile's tester							/*{{{*/
#ifdef linefile
#include <linefile.h>
#include <stdio.h>

int main(char argc,char *argv[])
{
   FILE *Test = fopen(argv[1],"r");
   char S[400];
   while (fgets(S,sizeof(S),Test) != 0);
//      printf("%s\n",S);

/*   try
   {
      LineFile F(argv[1]);
   
      char *C;
      while (F.GetLine(C) == true);
//	 printf("%s\n",C);

   }
   catch(char *C)
   {
      printf("%s\n",C);
   }*/

   return 0;
}
#endif
   									/*}}}*/
