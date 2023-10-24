/******************************************************************************
 Copyright (c) 2023, Ioannis Nompelis
 All rights reserved.

 Redistribution and use in source and binary forms, with or without any
 modification, are permitted provided that the following conditions are met:
 1. Redistribution of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistribution in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
 3. All advertising materials mentioning features or use of this software
    must display the following acknowledgement:
    "This product includes software developed by Ioannis Nompelis."
 4. Neither the name of Ioannis Nompelis and his partners/affiliates nor the
    names of other contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.
 5. Redistribution or use of source code and binary forms for profit must
    have written permission of the copyright holder.
 
 THIS SOFTWARE IS PROVIDED BY IOANNIS NOMPELIS ''AS IS'' AND ANY
 EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL IOANNIS NOMPELIS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "inobj.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "intiff.h"
#include "injpeg.h"


//
// the OBJ's file object's methods
//

inObj::inObj()
{
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG]  OBJ object instantiated\n" );
#endif
   istate = Unknown;
   num_lines = 0;
}

inObj::~inObj()
{
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG]  Zone object deconstructed\n" );
#endif
   clear();
}


int inObj::read( const char filename_[] )
{
   if( filename_ == NULL ) {
   fprintf( stdout, " [Error]  Filename cannot be null! \n" );
      return -1;
   }
   filename = filename_;

   fp = fopen( filename_, "r" );
   if( fp == NULL ) {
   fprintf( stdout, " [Error]  Could not open file: \"%s\"\n", filename_ );
      filename.clear();
      return -1;
   }
   istate = Open;

   // abstraction to allow for iterative parsing...
   int iret = parse();
   if( iret ) {
      filename.clear();
      iret = 1;
      istate = Unknown;
   } else {
      istate = Ready;
   }

   fclose( fp );

   // real the matllib file
   if( iret == 0 ) {
      iret = parseMtllib();
      if( iret ) {
         iret = 2;
      }
      // drop the buffer (again) if it happened to be used for textures
      if( buf != NULL ) {
         free( buf );
         buf = NULL;
         nbytes=0;
      }
   }

   return iret;
}

int inObj::getState() const
{
   return( istate );
}

void inObj::clear()
{

   filename.clear();
   num_lines=0;
   num_groups=0;
   dgroup = { .fs=0, .fe=0 };
   groups.clear();
   mtllib_name.clear();
   for(int n=0;n<(int)mtls.size();++n) {
      if( mtls[n].img.type == FILEMAGIC_JPEG ) {
         free( mtls[n].img.img_data );
      } else if( mtls[n].img.type == FILEMAGIC_TIFF ) {
         uint32_t* tmp = (uint32_t*) mtls[n].img.img_data;
         _TIFFfree( tmp );
      } else {
         // other formats should not have any data
      }
   }
   mtls.clear();
   vertex.clear();
   normal.clear();
   texel.clear();
   icsr.clear();
   jcsr.clear();

   istate = Unknown;
}

// --------------------- protected/private methods -------------------

int inObj::parse()
{
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG:parse]  Parser of OBJ file starting \n" );
#endif
   int ierr=0;
   pstate = OBJ_OPEN;

   // start the CSR structure for polygons
   icsr.push_back( 0 );

   while( ierr == 0 &&
          pstate != OBJ_ERROR &&
          pstate != OBJ_READY ) {

#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:parse]  Reading line: %d \n", num_lines+1 );
#ifdef _DEBUG_SLOW_
      usleep( 100000 );
#endif
#endif

      int iret = readLine();
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:parse]  Reading line returned: %d \n", iret );
#endif
#ifdef _DEBUG_
      fprintf( stdout, " [DEBUG:parse]  Line %d ==>%s<==\n", num_lines+1, buf );
#endif
      if( iret == -1 && iret == 999 ) {
         ierr = -1;     // the error is internal
      } else if( iret == 1 ) {
#ifdef _DEBUG_
         fprintf( stdout, " [DEBUG:parse]  End-of-file mid-line \n" );
#endif
         ++num_lines;
         pstate = OBJ_READY;
      } else if( iret == 2 ) {
#ifdef _DEBUG_
         fprintf( stdout, " [DEBUG:parse]  Inferred end-of-file \n" );
#endif
         pstate = OBJ_READY;
      } else {
#ifdef _DEBUG2_
         fprintf( stdout, " [DEBUG:parse]  Line read \n" );
#endif
         ++num_lines;
         ierr = handleLine();
      }
   }

#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG:parse]  Read %d lines \n", num_lines );
#endif

   // drop the buffer
   if( buf != NULL ) {
      free( buf );
      buf = NULL;
      nbytes=0;
   }

#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG:parse]  Parser of OBJ file ending \n" );
#endif
#ifdef _DEBUG2_
   fprintf( stdout, " [DEBUG]  Vertices dump (%d) \n", (int) vertex.size() );
   for(int i=0;i<(int) vertex.size();++i) {
      fprintf( stdout, "  %d  v %f %f %f \n", i,
               vertex[i].x, vertex[i].y, vertex[i].z );
   }
   fprintf( stdout, " [DEBUG]  Normals dump (%d) \n", (int) normal.size() );
   for(int i=0;i<(int) normal.size();++i) {
      fprintf( stdout, "  %d  vn %f %f %f \n", i,
               normal[i].x, normal[i].y, normal[i].z );
   }
   fprintf( stdout, " [DEBUG]  Texels dump (%d) \n", (int) texel.size() );
   for(int i=0;i<(int) texel.size();++i) {
      fprintf( stdout, "  %d  vn %f %f \n", i, texel[i].u, texel[i].v );
   }
#endif
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG]  Groups dump (%d) \n", (int) groups.size() );
   if( num_groups )
      for(int i=0;i<(int) groups.size();++i)
         fprintf( stdout, "  %d  [%d:%d] \n", i, groups[i].fs, groups[i].fe );
#endif
   return ierr;
}

//
// function to robustly read a line from the file descriptor
// Returns: 999 when something escapes my logic!
//          -1 on allocation error
//           0 when line ends with a newline
//           1 when line ends with a null character (end of file)
//           2 file terminated
//

int inObj::readLine()
{
   const size_t isize=80;
// const size_t isize=5;   // Used for debugging
   size_t im=0;
   char* bp = buf;

#ifdef _DEBUG2_
   fprintf( stdout, " [DEBUG:readLine]  Buffer has %ld bytes \n", nbytes );
#endif

   while(1) {

      // check for whether there is space in the buffer
      if( im == nbytes ) {
#ifdef _DEBUG_
         fprintf( stdout, " [DEBUG:readLine]  Nead to re-allocate to %ld bytes \n",
                  nbytes + isize+1 );
#ifdef _DEBUG_SLOW_
         usleep( 100000 );
#endif
#endif
         char* p = (char*) realloc( buf, 2*(nbytes + isize+1) );
         if( p == NULL ) {
            fprintf( stdout, " [Error]  Could not re-alloc. %ld byte buffer \n",
                     nbytes + isize+1 );
            // defer to the caller how to deal with the undefined behaviour.
            return -1;
         }
         buf = p;
         nbytes += isize;
         buf2 = &( buf[ nbytes+1 ] );
#ifdef _DEBUG_
         fprintf( stdout, " [DEBUG:readLine]  New buffer %ld bytes \n", nbytes );
#endif
      }

      bp = &( buf[im] );
      memset( bp, '\0', isize+1 );

      fgets( bp, isize+1, fp );      // reads "one less" than requested size...
                                     // ...and we have the last byte as null
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:readLine]  Buffer: --->%s<--- \n", buf );
#endif

      // check what was read in for a line-reading termination condition
      // also turn tabs to spaces
      size_t j=0;
      while( j < isize ) {
         if( bp[j] == '\n' ) {
#ifdef _DEBUG2_
            fprintf( stdout, " [DEBUG:readLine]  Line has newline \n" );
#endif
            bp[j] = '\0';    // remove newline
            return 0;
         } else
         if( bp[j] == '\0' ) {
#ifdef _DEBUG2_
            fprintf( stdout, " [DEBUG:readLine]  Line has nullchar \n" );
#endif
            if( im == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:readLine]  Reached end-of-file \n" );
#endif
               return 2;
            }
            return 1;
         } else
         if( bp[j] == '\t' ) {
#ifdef _DEBUG2_
            fprintf( stdout, " [DEBUG:readLine]  Line has a tab; fixing \n" );
#endif
            bp[j] = ' ';
         }
         ++j;
         ++im;
      }
   }

   return 999;
}


int inObj::handleLine()
{
   int ierr=0, k=0;
   memcpy( buf2, buf, nbytes+1 );
   // clean-up what ChatGPT thought was cool to comment (how nice of her!)
   while( buf2[k] != '\0' ) {
      if( buf2[k] == '#' ) buf2[k] = '\0';
      ++k;
   }

   if( buf[0] == '#' ) {
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:handleLine]  Line is a comment \n" );
#endif
   } else if( buf[0] == '\0' ) {
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:handleLine]  Line is blank; doing nothing \n" );
#endif
   } else {

      std::vector< std::string > strings;
      char *s, *saveptr=NULL;
      int i;
      for( i=0, s=buf2; ; ++i, s=NULL ) {
         char *token = strtok_r( s, " ", &saveptr );
         if( token == NULL ) break;
#ifdef _DEBUG3_
         fprintf( stdout, " [DEBUG:handleLine]  Token: \"%s\"\n", token );
#endif
         strings.push_back( token );
         if( i == 0 ) {
            if( strcmp( token, "v" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"vertex\"\n" );
#endif
               pstate = OBJ_VERTEX;
            } else if( strcmp( token, "vn" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"normal\"\n" );
#endif
               pstate = OBJ_NORMAL;
            } else if( strcmp( token, "vt" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"texel\"\n" );
#endif
               pstate = OBJ_TEXEL;
            } else if( strcmp( token, "f" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"face\"\n" );
#endif
               pstate = OBJ_FACE;
            } else if( strcmp( token, "g" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"group\"\n" );
#endif
               pstate = OBJ_GROUP;
            } else if( strcmp( token, "s" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"smooth\"\n" );
#endif
               pstate = OBJ_SMOOTH;
            } else if( strcmp( token, "o" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"object\"\n" );
#endif
               pstate = OBJ_OBJECT;
            } else if( strcmp( token, "mtllib" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"mtllib\"\n" );
#endif
               pstate = OBJ_MTLLIB;
            } else if( strcmp( token, "usemtl" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleLine]  Detected \"usemtl\"\n" );
#endif
               pstate = OBJ_USEMTL;
            } else {
               pstate = OBJ_ERROR;
            }
         }
      }
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:handleLine]  " );
      for(i=0;i<(int) strings.size();++i) {
         fprintf( stdout, " [%d] \"%s\"", i, strings[i].c_str() );
      }
      fprintf( stdout, "\n" );
#endif

      // check for errors
      if( pstate == OBJ_ERROR ) {
         fprintf( stdout, " [Error]  The OBJ is garbled \n" );
         return 100;
      }

      // processing
      switch( pstate ) {
       case OBJ_VERTEX:
         ierr = handleVertex( strings );
       break;
       case OBJ_NORMAL:
         ierr = handleNormal( strings );
       break;
       case OBJ_TEXEL:
         ierr = handleTexel( strings );
       break;
       case OBJ_FACE:
         ierr = handleFace( strings );
       break;
       case OBJ_GROUP:
         ierr = handleGroup( strings );
       break;
       case OBJ_SMOOTH:
         ierr = handleSmooth( strings );
       break;
       case OBJ_OBJECT:
         ierr = handleObject( strings );
       break;
       case OBJ_MTLLIB:
         ierr = handleMtllib( strings );
       break;
       case OBJ_USEMTL:
         // ...
       break;
       default:
         fprintf( stdout, " [Error]  Parsing state unhandled \n" );
         ierr = 999;
      }
   }

   return ierr;
}


int inObj::handleVertex( std::vector< std::string > & strings )
{
   vec3_s v;
   float r;
   sscanf( strings[1].c_str(), "%f", &r );
   v.x = r;
   sscanf( strings[2].c_str(), "%f", &r );
   v.y = r;
   sscanf( strings[3].c_str(), "%f", &r );
   v.z = r;
   vertex.push_back( v );

   return 0;
}

int inObj::handleNormal( std::vector< std::string > & strings )
{
   vec3_s v;
   float r;
   sscanf( strings[1].c_str(), "%f", &r );
   v.x = r;
   sscanf( strings[2].c_str(), "%f", &r );
   v.y = r;
   sscanf( strings[3].c_str(), "%f", &r );
   v.z = r;
   normal.push_back( v );

   return 0;
}

int inObj::handleTexel( std::vector< std::string > & strings )
{
   vec2_s v;
   float r;
   sscanf( strings[1].c_str(), "%f", &r );
   v.u = r;
   sscanf( strings[2].c_str(), "%f", &r );
   v.v = r;
   texel.push_back( v );

   return 0;
}

int inObj::handleFace( std::vector< std::string > & strings )
{
   int ierr=0,i,itype=0;

   // determine format of polygon's members
   const char* p = strings[1].c_str();
   for( i=1; p[i] != '\0'; ++i ) {
      if( p[i] == '/' ) {
         if( p[i-1] == '/' ) itype += 1;
         ++itype;
      }
   }

   // intelligently increase icsr
   icsr.push_back( icsr[ dgroup.fe ] );

   ( dgroup.fe )++;
   if( num_groups ) {
      ( groups[ num_groups-1 ].fe )++;
   }

   for(i=1;i<(int) strings.size() && ierr==0;++i) {
      unsigned long int ul=0,ul1=0,ul2=0,ul3=0;
      p = strings[i].c_str();
      switch( itype ) {
       case 0:
         ierr = sscanf( p, "%ld", &ul1 );
         if( ierr == 1 ) ierr=0;
       break;
       case 1:
         // this IS a valid format, but we will return an error
         ierr=101;
       break;
       case 2:
         ierr = sscanf( p, "%ld/%ld/%ld", &ul1, &ul2, &ul3 );
         if( ierr == 3 ) ierr=0;
       break;
       case 3:
         ierr = sscanf( p, "%ld//%ld", &ul1, &ul3 );
         if( ierr == 2 ) ierr=0;
       break;
      }

      if( ierr == 0 ) {
         ul = (ul1 << 40) | (ul2 << 20) | ul3;
         // add to jcsr of CSR
         ++( icsr[ dgroup.fe ] );
         jcsr.push_back( ul );
      }
   }
#ifdef _DEBUG2_
   const unsigned long int m3 = 0x0FFFFF;
   const unsigned long int m2 = m3 << 20;
   const unsigned long int m1 = m2 << 20;
// fprintf( stdout, " [DEBUG:handleFace]  Masks: %.16lx %.16lx %.16lx \n",
//          m1,m2,m3);
   fprintf( stdout, " [DEBUG:handleFace]  Polygon %d (%d:%d) \n",
            dgroup.fe-1, icsr[ dgroup.fe-1 ], icsr[ dgroup.fe ] );
   for( i=icsr[ dgroup.fe-1 ]; i<icsr[ dgroup.fe ]; ++i ) {
      const unsigned long int ul = jcsr[i];
      fprintf( stdout, "  [%d]  %ld %ld %ld \n",
               i, (ul & m1) >> 40, (ul & m2) >> 20, (ul & m3) );
   }
#endif

   return ierr;
}

int inObj::handleGroup( std::vector< std::string > & strings )
{
   struct inObjGrp_s ngroup = { .fs = dgroup.fe, .fe = dgroup.fe };
   if( strings.size() > 1 ) { ngroup.name = strings[1].c_str(); }
   dgroup.fs = dgroup.fe;
   groups.push_back( ngroup );
   ++num_groups;
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG:handleGroup]  New group (%d) ", num_groups );
   fprintf( stdout, " [%d:%d] \"%s\"\n", ngroup.fs, ngroup.fe,
            ngroup.name.c_str() );
#endif

   return 0;
}

int inObj::handleSmooth( std::vector< std::string > & strings )
{
   fprintf( stdout, " [Info]  Issues with \"s\" (\"smooth\") directives.\n" );
   fprintf( stdout, "%s\n%s\n%s\n%s\n",
                    "  This parser does _not_ handle hierarchical objects.",
                    "  Objects should comprise of Groups, but not here...",
                    "  Smoothing is associated with objects, and therefore",
                    "  it is neglected here." );
   return 0;
}

int inObj::handleObject( std::vector< std::string > & strings )
{
   fprintf( stdout, " [Info]  Issues with \"o\" (\"object\") directives.\n" );
   fprintf( stdout, "%s\n%s\n%s\n",
                    "  This parser does _not_ handle hierarchical objects.",
                    "  Objects should comprise of Groups, but not here...",
                    "  Groups are fine-grained enough for the time being." );
   return 0;
}

int inObj::handleMtllib( std::vector< std::string > & strings )
{
   if( strings.size() > 1 ) {
      mtllib_name = strings[1].c_str();
#ifdef _DEBUG_
      fprintf( stdout, " [DEBUG:handleMtllib]  Mtllib: \"%s\" \n",
               mtllib_name.c_str() );
#endif
   } else {
      return 1;
   }

   return 0;
}

#define MTLLIB_INIT( mtl ) { mtl = \
                             { .Ka = { -1.0, -1.0, -1.0 },\
                               .Kd = { -1.0, -1.0, -1.0 },\
                               .Ks = { -1.0, -1.0, -1.0 },\
                               .Ns = -9.9,\
                               .Ni = -9.9,\
                               .d = -9.9,\
                               .illum = 9999 };\
                             mtl.name.clear();\
                             mtl.map_Kd.clear();\
                             mtl.img.type = FILEMAGIC_UNKNOWN; }
#ifdef _DEBUG_
#define MTLLIB_VIEW( mtl ) \
      fprintf( stdout, " [DEBUG:mtllib_view]  Mtllib struct contents \n" );\
      fprintf( stdout, " name: \"%s\" \n", mtl.name.c_str() );\
      fprintf( stdout, " Ka[] = %lf %lf %lf \n",mtl.Ka[0],mtl.Ka[1],mtl.Ka[2]);\
      fprintf( stdout, " Kd[] = %lf %lf %lf \n",mtl.Kd[0],mtl.Kd[1],mtl.Kd[2]);\
      fprintf( stdout, " Ks[] = %lf %lf %lf \n",mtl.Ks[0],mtl.Ks[1],mtl.Ks[2]);\
      fprintf( stdout, " Ns[] = %lf, Ni = %lf, d = %lf\n",mtl.Ns,mtl.Ni,mtl.d);\
      fprintf( stdout, " map_Kd: \"%s\" \n", mtl.map_Kd.c_str() );
#endif
int inObj::parseMtllib()
{
  if( mtllib_name.size() == 0 ) return 0;

#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG:parseMtllib]  Starting \n" );
#endif
   struct inObjMtl_s mtl;
   MTLLIB_INIT( mtl )

   fp = fopen( mtllib_name.c_str(), "r" );
   if( fp == NULL ) {
      fprintf( stdout, " [Error]  Could not open \"%s\" for reading. \n",
               mtllib_name.c_str() );
      return 2;
   }
   mstate = MTLLIB_OPEN;

   int ierr=0,nline=0,have_one=0;
   while( ierr == 0 &&
          mstate != MTLLIB_ERROR &&
          mstate != MTLLIB_READY ) {

      int iret = readLine();
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:parseMtllib]  Reading line returned: %d \n", iret );
#endif
#ifdef _DEBUG_
      fprintf( stdout, " [DEBUG:parseMtllib]  Line %d ==>%s<==\n", nline+1, buf );
#endif
      if( iret == -1 && iret == 999 ) {
         ierr = -1;     // the error is internal
      } else if( iret == 1 ) {
#ifdef _DEBUG2_
         fprintf( stdout, " [DEBUG:parseMtllib]  End-of-file mid-line \n" );
#endif
         mstate = MTLLIB_ERROR;
      } else if( iret == 2 ) {
#ifdef _DEBUG2_
         fprintf( stdout, " [DEBUG:parseMtllib]  Inferred end-of-file \n" );
#endif
         // the idea is that if we EOF and we are building one, we must add it
         if( have_one ) {
#ifdef _DEBUG_
            MTLLIB_VIEW( mtl )
#endif
            mtls.push_back( mtl );
         }
         mstate = MTLLIB_READY;
      } else {
         ierr = handleMtlLine( mtl, have_one );
      }

   }

   fclose( fp );

#ifdef _DEBUG_
      fprintf( stdout, " [DEBUG:parseMtllib]  Ending (ierr=%d) \n", ierr );
#endif
   return ierr;
}

int inObj:: handleMtlLine( struct inObjMtl_s & mtl, int & have_one )
{
   int ierr=0, k=0;
   memcpy( buf2, buf, nbytes+1 );
   // clean-up what ChatGPT thought was cool to comment (how nice of her!)
   while( buf2[k] != '\0' ) {
      if( buf2[k] == '#' ) buf2[k] = '\0';
      ++k;
   }

   if( buf[0] == '#' ) {
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:handleMtlLine]  Line is a comment \n" );
#endif
   } else if( buf[0] == '\0' ) {
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:handleMtlLine]  Line is blank; doing nothing \n" );
#endif
   } else {

      std::vector< std::string > strings;
      char *s, *saveptr=NULL;
      int i;
      for( i=0, s=buf2; ; ++i, s=NULL ) {
         char *token = strtok_r( s, " ", &saveptr );
         if( token == NULL ) break;
#ifdef _DEBUG3_
         fprintf( stdout, " [DEBUG:handleMtlLine]  Token: \"%s\"\n", token );
#endif
         strings.push_back( token );
         if( i == 0 ) {
            if( strcmp( token, "newmtl" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"newmtl\"\n" );
#endif
               mstate = MTLLIB_NEWMTL;
            } else
            if( strcmp( token, "Ka" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"Ka\"\n" );
#endif
               mstate = MTLLIB_KA;
            } else
            if( strcmp( token, "Kd" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"\Kd\"\n" );
#endif
               mstate = MTLLIB_KD;
            } else
            if( strcmp( token, "Ks" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"Ks\"\n" );
#endif
               mstate = MTLLIB_KS;
            } else
            if( strcmp( token, "Ns" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"Ns\"\n" );
#endif
               mstate = MTLLIB_NS;
            } else
            if( strcmp( token, "Ni" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"Ni\"\n" );
#endif
               mstate = MTLLIB_NI;
            } else
            if( strcmp( token, "d" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"d\"\n" );
#endif
               mstate = MTLLIB_D;
            } else
            if( strcmp( token, "illum" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"illum\"\n" );
#endif
               mstate = MTLLIB_ILLUM;
            } else
            if( strcmp( token, "map_Kd" ) == 0 ) {
#ifdef _DEBUG2_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Detected \"map_Kd\"\n" );
#endif
               mstate = MTLLIB_MAPKD;
            } else {
               mstate = MTLLIB_ERROR;
            }
         }
      }
#ifdef _DEBUG2_
      fprintf( stdout, " [DEBUG:handleMtlLine]  " );
      for(i=0;i<(int) strings.size();++i) {
         fprintf( stdout, " [%d] \"%s\"", i, strings[i].c_str() );
      }
      fprintf( stdout, "\n" );
#endif

      // check for errors
      if( mstate == MTLLIB_ERROR ) {
         fprintf( stdout, " [Error]  The MTLLIB is garbled \n" );
         return 100;
      }

      // processing
      float r;
      struct inImage_s img = { .type = FILEMAGIC_UNKNOWN };
      switch( mstate ) {
       case MTLLIB_NEWMTL:
         if( have_one == 0 ) {   // the first one we encounter
            // we indicate that we need to add it at the end of parsing
            have_one = 1;
         } else {                // we have one, and we move to a new one
            // we are already working on one and we must add it now
#ifdef _DEBUG_
            MTLLIB_VIEW( mtl )
#endif
            mtls.push_back( mtl );
         }
         // now we re-initialize the one we were using as storage
         MTLLIB_INIT( mtl );
         mtl.name = strings[1].c_str();
       break;
       case MTLLIB_KA:
         sscanf( strings[1].c_str(), "%f", &r );
         mtl.Ka[0] = r;
         sscanf( strings[2].c_str(), "%f", &r );
         mtl.Ka[1] = r;
         sscanf( strings[3].c_str(), "%f", &r );
         mtl.Ka[2] = r;
       break;
       case MTLLIB_KD:
         sscanf( strings[1].c_str(), "%f", &r );
         mtl.Kd[0] = r;
         sscanf( strings[2].c_str(), "%f", &r );
         mtl.Kd[1] = r;
         sscanf( strings[3].c_str(), "%f", &r );
         mtl.Kd[2] = r;
       break;
       case MTLLIB_KS:
         sscanf( strings[1].c_str(), "%f", &r );
         mtl.Ks[0] = r;
         sscanf( strings[2].c_str(), "%f", &r );
         mtl.Ks[1] = r;
         sscanf( strings[3].c_str(), "%f", &r );
         mtl.Ks[2] = r;
       break;
       case MTLLIB_NS:
         sscanf( strings[1].c_str(), "%f", &r );
         mtl.Ns = r;
       break;
       case MTLLIB_NI:
         sscanf( strings[1].c_str(), "%f", &r );
         mtl.Ni = r;
       break;
       case MTLLIB_D:
         sscanf( strings[1].c_str(), "%f", &r );
         mtl.d = r;
       break;
       case MTLLIB_ILLUM:
         sscanf( strings[1].c_str(), "%d", &i );
         mtl.illum = i;
       break;
       case MTLLIB_MAPKD:
         if( strings.size() > 1 ) {
            mtl.map_Kd = strings[1];
            for(i=2;i<(int) strings.size();++i) {
               mtl.map_Kd += " ";
               mtl.map_Kd += strings[i];
            }
            // handle texture reading
            ierr = determineFileType( mtl.map_Kd.c_str() );
#ifdef _DEBUG_
            fprintf( stdout, " [DEBUG:handleMtlLine]  Texture file type: %d \n",
                     ierr );
#endif
            if( ierr == FILEMAGIC_JPEG ) {
               unsigned char *img_data;
               ierr = injpg_ReadImage( mtl.map_Kd.c_str(),
                            &img_data, &img.width, &img.height, &img.irgb );
               img.img_data = (void*) img_data;
               img.type = FILEMAGIC_JPEG;
            } else if( ierr == FILEMAGIC_TIFF ) {
               unsigned int *img_data;
               ierr = intif_ReadImage( mtl.map_Kd.c_str(),
                                       &img.width, &img.height, &img_data );
               img.irgb = 4;    // TIFF reader always returns 4 components...
                                // ...and ChatGPT says alpha will be made 0xFF
               img.img_data = (void*) img_data;
               img.type = FILEMAGIC_TIFF;
            } else {
               ierr = 100;
            }
            if( ierr == 0 ) {
#ifdef _DEBUG_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Image was read \n" );
#endif
               // pass the texture data through the "flattener"...
               ierr = unifyTexture(  &img );
#ifdef _DEBUG_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Bad conversion \n" );
#endif
               if( ierr ) {
                  ierr = 300;
               } else {
                  memcpy( &(mtl.img), &img, sizeof(struct inImage_s) );
               }
            } else {
#ifdef _DEBUG_
               fprintf( stdout, " [DEBUG:handleMtlLine]  Image NOT read \n" );
#endif
               ierr = 200;
            }
         }
       break;
       case MTLLIB_READY:
         // do not let this no-op condition faul into the default
       break;
       default:
         fprintf( stdout, " [Error]  Parsing state unhandled \n" );
         ierr = 999;
      }
   }

   return ierr;
}

// private method to detect (texture image) file type based on magic numbers
int inObj::determineFileType( const char* filepath ) const
{
   if( filepath == NULL ) return -2;

   FILE *fp = fopen( filepath, "rb" );
   if( fp == NULL ) {
      fprintf( stdout, " [Error]  Cannot open file: \"%s\"\n", filepath );
      return -1;
   }

   unsigned char buffer[4];
   fread( buffer, 1, 4, fp );
   fclose( fp );

   // Check for PNG signature
   if( buffer[0] == 0x89 && buffer[1] == 0x50 &&
       buffer[2] == 0x4E && buffer[3] == 0x47 ) { return FILEMAGIC_PNG; }
   // Check for JPEG signature
   else if( buffer[0] == 0xFF && buffer[1] == 0xD8) { return FILEMAGIC_JPEG; }
   // Check for TIFF signatures
   else if( (buffer[0] == 0x49 && buffer[1] == 0x49 &&
             buffer[2] == 0x2A && buffer[3] == 0x00) ||
            (buffer[0] == 0x4D && buffer[1] == 0x4D &&
             buffer[2] == 0x00 && buffer[3] == 0x2A) ) { return FILEMAGIC_TIFF;}
   else {
      return FILEMAGIC_UNKNOWN;
   }
}

// a private method to take a pre-loaded texture struct and returned "unifed"
// RGBA data to its buffer regardless of what file format it came from
int inObj::unifyTexture( struct inImage_s* s )
{
   if( s == NULL ) return 1;

   if( s->type == FILEMAGIC_JPEG ) {
      if( s->irgb == 4 ) {
         return 0;
      } else if( s->irgb == 3 ) {
#ifdef _DEBUG_
         fprintf( stdout, " [DEBUG]  Re-allocating raster \n" );
#endif
         size_t isize = (size_t) (s->width * s->height * 4);
         unsigned char *tmp = (unsigned char*) malloc( isize );
         if( tmp == NULL ) {
            fprintf( stdout, " [Error]  Texture allocation failed \n" );
            return -1;
         }
         unsigned char *tmp0 = (unsigned char*) s->img_data;
         for(size_t i=0;i<isize;++i) {
            tmp[i*4  ] = tmp0[i*3  ];
            tmp[i*4+1] = tmp0[i*3+1];
            tmp[i*4+2] = tmp0[i*3+2];
            tmp[i*4+2] = 0xFF;
         }
         free( tmp0 );
         s->img_data = (void*) tmp;
      } else {
         fprintf( stdout, " [Error]  JPEG file has %d components \n", s->irgb );
         return 101;
      }

   } else if( s->type == FILEMAGIC_TIFF ) {

   } else {
      fprintf( stdout, " [Error]  Unhandled file type: %d \n", s->type );
      return 100;
   }

   return 0;
}

// Method to write a TecPlot file to visualize with ParaView
// (For now all faces need to be triangles.)
// (For now all faces need to have normal vectors, and vertex-normal pairs are
// unique.)

int inObj::dumpTecplot( const char filename[] ) const
{
   if( filename == NULL ) return 1;

   int nvert = (int) vertex.size();
   int nnorm = (int) normal.size();
   int npoly = (int) icsr.size() - 1;
   // switch for only ploting normal vectors if they are pressumed one-to-one
   unsigned char ic=0;
   if( nvert != nnorm ) ic=1;

   // count polygons as collections of triangles
   int ntri=npoly;
   for(int i=0;i<npoly;++i) {
      ntri += icsr[i+1] - icsr[i] - 3;
   }
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG:dumpTecplot]  Poly: %d  Tri: %d \n", npoly, ntri );
#endif

   FILE *fp = fopen( filename, "w" );
   if( fp == NULL ) {
      fprintf( stdout, " [Error]  Could not open \"%s\" for writing. \n",
               filename );
      return -1;
   }

   if( ic ) {
      fprintf( fp, "VARIABLES = x y z \n" );
   } else {
      fprintf( fp, "VARIABLES = x y z u v w \n" );
   }
   fprintf( fp, "ZONE T=\"obj file\" \n" );
   fprintf( fp, "     NODES=%d, ELEMENTS=%d, \n", nvert, ntri );
   fprintf( fp, "     DATAPACKING=POINT, ZONETYPE=FETRIANGLE, \n" );
   if( ic ) {
      fprintf( fp, "     VARLOCATION=([1-3]=NODAL) \n" );
   } else {
      fprintf( fp, "     VARLOCATION=([1-6]=NODAL) \n" );
   }

   for(int n=0;n<nvert;++n) {
      if( ic ) {
         fprintf( fp, " %lf %lf %lf \n",
                  vertex[n].x, vertex[n].y, vertex[n].z );
      } else {
         fprintf( fp, " %lf %lf %lf  %lf %lf %lf \n",
                  vertex[n].x, vertex[n].y, vertex[n].z,
                  normal[n].x, normal[n].y, normal[n].z );
      }
   }

   const unsigned long int m3 = 0x0FFFFF;
   const unsigned long int m2 = m3 << 20;
   const unsigned long int m1 = m2 << 20;
   for(int i=0;i<npoly;++i) {
      unsigned long int uf = jcsr[ icsr[i] ];   // store the first point-index
      unsigned long int up;   // to store the "previous" point-index
      for(int k=icsr[i];k<icsr[i+1];++k) {
         unsigned long int ul = jcsr[k];
         if( k - icsr[i] < 3 ) {
            fprintf( fp, " %d ", (int) ( (ul & m1) >> 40 ) );
         } else {
            if( k - icsr[i] == 3 ) fprintf( fp, "\n" );
            fprintf( fp, " %d ", (int) ( (uf & m1) >> 40 ) );
            fprintf( fp, " %d ", (int) ( (up & m1) >> 40 ) );
            fprintf( fp, " %d ", (int) ( (ul & m1) >> 40 ) );
         }
         up = ul;   // store the previous point-index
      }
      fprintf( fp, "\n" );
   }

   fclose( fp );

   return 0;
}

// --------------------- API methods -------------------

//
// Function of the API to read an Alias Wavefront OBJ file
//

void* readObjFile( const char filename[] )
{
#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG]  C wrapper of OBJ file reader starting \n" );
#endif
   inObj* objp = new inObj();

   int iret = objp->read( filename );
   if( iret ) {
      fprintf( stdout, " [Error]  Could not read OBJ file \"%s\"\n", filename );
      delete objp;
      objp = NULL;
   } else {
#ifdef _DEBUG_
      fprintf( stdout, " [DEBUG]  Read OBJ file \"%s\"\n", filename );
#endif
   }

#ifdef _DEBUG_
   fprintf( stdout, " [DEBUG]  C wrapper of OBJ file reader ending \n" );
#endif
   return (void*) objp;
}


int clearObj( void* p )
{
   if( p == NULL ) return 1;

   inObj* objp = (inObj*) p;

   objp->clear();
   delete objp;

   return 0;
}


int dumpTecplot( void* p, const char filename[] )
{
   if( p == NULL ) return 1;

   inObj* objp = (inObj*) p;

   objp->dumpTecplot( filename );

   return 0;
}

#ifdef __cplusplus
}
#endif

