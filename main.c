#include <stdio.h>
#include <stdlib.h>

#include "inobj.h"

int main( int argc, char *argv[] )
{
   fprintf( stdout, "Hello world \n" );

   printf("================= Reading OBJ file =========================== \n" );
   void *p=NULL;
   p = readObjFile( "cube.obj" );
   dumpTecplot( p, "tecplot.dat" );
   clearObj( p );

   return 0;
}

