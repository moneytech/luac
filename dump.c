/*
** $Id: dump.c,v 1.13 1999/03/11 17:09:10 lhf Exp lhf $
** save bytecodes to file
** See Copyright Notice in lua.h
*/

#include <stdio.h>
#include <stdlib.h>
#include "luac.h"

#define DumpBlock(b,size,D)	fwrite(b,size,1,D)
#define	DumpNative(t,D)		DumpBlock(&t,sizeof(t),D)
#define	DumpInt			DumpLong

static void DumpWord(int i, FILE* D)
{
 int hi= 0x0000FF & (i>>8);
 int lo= 0x0000FF &  i;
 fputc(hi,D);
 fputc(lo,D);
}

static void DumpLong(long i, FILE* D)
{
 int hi= 0x00FFFF & (i>>16);
 int lo= 0x00FFFF & i;
 DumpWord(hi,D);
 DumpWord(lo,D);
}

#if ID_NUMBER==ID_REAL4			/* LUA_NUMBER */
/* assumes sizeof(long)==4 and sizeof(float)==4 (IEEE) */
static void DumpFloat(float f, FILE* D)
{
 long l=*(long*)&f;
 DumpLong(l,D);
}
#endif

#if ID_NUMBER==ID_REAL8			/* LUA_NUMBER */
/* assumes sizeof(long)==4 and sizeof(double)==8 (IEEE) */
static void DumpDouble(double f, FILE* D)
{
 long* l=(long*)&f;
 int x=1;
 if (*(char*)&x==1)			/* little-endian */
 {
  DumpLong(l[1],D);
  DumpLong(l[0],D);
 }
 else					/* big-endian */
 {
  DumpLong(l[0],D);
  DumpLong(l[1],D);
 }
}
#endif

static void DumpCode(TProtoFunc* tf, FILE* D)
{
 int size=luaU_codesize(tf);
 DumpLong(size,D);
 DumpBlock(tf->code,size,D);
}

static void DumpString(char* s, int size, FILE* D)
{
 if (s==NULL)
  DumpLong(0,D);
 else
 {
  DumpLong(size,D);
  DumpBlock(s,size,D);
 }
}

static void DumpTString(TaggedString* s, FILE* D)
{
 if (s==NULL)
  DumpString(NULL,0,D);
 else
  DumpString(s->str,s->u.s.len+1,D);
}

static void DumpLocals(TProtoFunc* tf, FILE* D)
{
 int n;
 LocVar* lv;
 for (n=0,lv=tf->locvars; lv && lv->line>=0; lv++) ++n;
 DumpInt(n,D);
 for (lv=tf->locvars; lv && lv->line>=0; lv++)
 {
  DumpInt(lv->line,D);
  DumpTString(lv->varname,D);
 }
}

static void DumpFunction(TProtoFunc* tf, FILE* D);

static void DumpConstants(TProtoFunc* tf, FILE* D)
{
 int i,n=tf->nconsts;
 DumpInt(n,D);
 for (i=0; i<n; i++)
 {
  TObject* o=tf->consts+i;
  fputc(-ttype(o),D);			/* ttype(o) is negative - ORDER LUA_T */
  switch (ttype(o))
  {
   case LUA_T_NUMBER:
	DumpNumber(nvalue(o),D);
	break;
   case LUA_T_STRING:
	DumpTString(tsvalue(o),D);
	break;
   case LUA_T_PROTO:
	DumpFunction(tfvalue(o),D);
	break;
   case LUA_T_NIL:
	break;
   default:				/* cannot happen */
	luaL_verror("cannot dump constant #%d: type=%d [%s]"
		" in %p (\"%s\":%d)",
		i,ttype(o),luaO_typename(o),tf,tf->source->str,tf->lineDefined);
	break;
  }
 }
}

static void DumpFunction(TProtoFunc* tf, FILE* D)
{
 DumpInt(tf->lineDefined,D);
 DumpTString(tf->source,D);
 DumpCode(tf,D);
 DumpLocals(tf,D);
 DumpConstants(tf,D);
}

static void DumpHeader(TProtoFunc* Main, FILE* D)
{
 real t=TEST_NUMBER;
 fputc(ID_CHUNK,D);
 fputs(SIGNATURE,D);
 fputc(VERSION,D);
 fputc(ID_NUMBER,D);
 fputc(sizeof(t),D);
 DumpNumber(t,D);
}

void luaU_dumpchunk(TProtoFunc* Main, FILE* D)
{
 DumpHeader(Main,D);
 DumpFunction(Main,D);
}
