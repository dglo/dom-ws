/* filldeps.c, cannonicalize a dependency file,
 * fill in all the package dependencies...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* flags
 */
static int prtDangling = 0;
static int prtCircular = 0;

/* string table -- zero terminated... */
static char *strTab;
static int strTabIdx, strTabLen;

static inline const char *idxtostr(int idx) { return strTab + idx; }

typedef struct HashMapStruct {
   int hash; /* full hash value  */
   int idx;  /* string index     */
   int len;  /* length of string */
   struct HashMapStruct *next;
} HashMap;

static HashMap *hashMap;
static int nHashMap;
static unsigned hashMapLen;

static void hashMapClear(HashMap *hm) {
   hm->hash = hm->idx = hm->len = -1; hm->next = NULL;
}

static int hashMapInit(void) {
   int i;
   
   nHashMap = 0;
   hashMapLen = 1024*64;
   hashMap = (HashMap *) calloc(hashMapLen, sizeof(HashMap));
   if (hashMap==NULL) return 1;
   for (i=0; i<hashMapLen; i++) hashMapClear(hashMap + i);
   return 0;
}

/* get hash value for string... */
static int hash(const char *s, int len) {
   int hash = 0;
   for (; len; len--, s++) {
      /* (31 * hash) will probably be optimized to ((hash << 5) - hash). */
      hash = 31 * hash + *s;
   }
   return hash;
}

#if 0
static void hashMapDump(HashMap *hm) {
   HashMap *hma;
   
   printf("hash=%u idx=%d (%s) len=%d next=%p\n",
          hm->hash, hm->idx, idxtostr(hm->idx), hm->len, hm->next);

   for (hma=hm->next; hma!=NULL; hma=hma->next) {
      printf(" : hash=%u idx=%d (%s) len=%d next=%p\n",
             hma->hash, hma->idx, idxtostr(hma->idx), hma->len, hma->next);
   }
}
#endif

/* returns: hash map index or -1 if not found... */
static int lookup(unsigned h, const char *str, int n) {
   unsigned idx = h % hashMapLen;
   HashMap *hm;

   /* is it right? */
   if (hashMap[idx].hash==h && hashMap[idx].len==n && 
       memcmp(idxtostr(hashMap[idx].idx), str, n)==0) 
      return hashMap[idx].idx;
   
   /* check subtrees... */
   for (hm=hashMap[idx].next; hm!=NULL; hm = hm->next) {
      if (hashMap[idx].hash==h && hashMap[idx].len==n && 
          memcmp(idxtostr(hashMap[idx].idx), str, n)==0) 
         return hashMap[idx].idx;
   }

   return -1;
}

/* insert str, h, n into hashmap element
 */
static int hashMapInsert(HashMap *hm, int sidx, int h, int n) {
   if (hm->idx==-1) {
      /* right at the top */
      hm->hash = h;
      hm->idx = sidx;
      hm->len = n;
   }
   else {
      HashMap *hma = (HashMap *) malloc(sizeof(HashMap));
      if (hma==NULL) {
         fprintf(stderr, "filldeps: unable to allocate memory\n");
         return -1;
      }
      hma->hash = h;
      hma->idx = sidx;
      hma->len = n;
      hma->next = hm->next;
      hm->next = hma;
   }

   return 0;
}

/* return index to strtab or -1 on error */
static int strtoidx(const char *str) {
   const int n = strlen(str);
   const int h = hash(str, n);
   const int idx = lookup(h, str, n);

   /* is it there already? */
   if (idx!=-1) return idx;
   
   /* not there, time to resize? */
   if (nHashMap>=hashMapLen) {
      const unsigned nln = hashMapLen*2;
      HashMap *hm = (HashMap *) calloc(nln, sizeof(HashMap));
      int i;
      for (i=0; i<nln; i++) hashMapClear(hm + i);
   
      for (i=0; i<hashMapLen; i++) {
         HashMap *hma;
         
         /* nothing here? */
         if (hashMap[i].idx!=-1) {
            hashMapInsert(hm + (hashMap[i].hash % nln),
                          hashMap[i].idx, hashMap[i].hash, hashMap[i].len);
         }
         for (hma=hashMap[i].next; hma!=NULL; hma=hma->next) {
            if (hma->idx!=-1) {
               hashMapInsert(hm + (hma->hash % nln),
                             hma->idx, hma->hash, hma->len);
            }
            free(hma);
         }
      }

      free(hashMap);
      hashMap = hm;
      hashMapLen = nln;
   }

   /* enough space?
    */
   if ( strTabIdx + n + 1 >= strTabLen ) {
      strTabLen = (strTabLen + 64*1024) + 64*1024;
      if ((strTab = (char *) realloc(strTab, strTabLen))==NULL) {
         fprintf(stderr, "filldeps: can't allocate %d bytes\n", strTabLen);
         return -1;
      }
   }

   /* copy string and return... */
   {  int ret = strTabIdx;
   
      memcpy(strTab + strTabIdx, str, n);
      strTab[strTabIdx + n] = 0;
      strTabIdx += n + 1;

      /* fill in hashmap ... */
      hashMapInsert(hashMap + (h % hashMapLen), ret, h, n);
      nHashMap++;

      return ret;
   }
}

typedef struct JarEntStruct {
   int jarIdx; /* jar string index */
   int pkgIdx; /* package string index */
   int *deps;  /* dependency entries */
   int ndeps;  /* number of dependency entries */
   int deplen; /* length of dependency entries */
   int flags;
} JarEnt;

static int addDep(JarEnt *e, int idx) {
   /* enough space? */
   if (e->deplen - e->ndeps == 0) {
      e->deplen = e->deplen * 2 + 16;
      if ((e->deps=(int *) realloc(e->deps, e->deplen*sizeof(int)))==NULL) {
         return 1;
      }
   }
   
   e->deps[e->ndeps] = idx;
   e->ndeps = e->ndeps+1;
   
   return 0;
}

static int addDepStr(JarEnt *e, const char *str) {
   if (strchr(str, '\n')!=NULL) abort();
   
   /* empty string? */
   if (*str==0) return 0;

   return addDep(e, strtoidx(str));
}

static int lookupPackage(JarEnt *ents, int nents, int idx) {
   int i;

   for (i=0; i<nents; i++) {
      if (idx==230) {
         printf("lookupPackage: %s [%s] (%d:%d)\n", idxtostr(idx),
                idxtostr(ents[i].pkgIdx), idx, ents[i].pkgIdx);
      }
      
      if (ents[i].pkgIdx == idx) return i;
   }
   
   return -1;
}

static void fixupEnt(JarEnt *ents, int nents, int i, int frm) {
   int j;

#if 0
   printf("fixupEnt: '%s' <- '%s'\n",
          idxtostr(ents[i].pkgIdx), idxtostr(ents[frm].pkgIdx));
#endif    
 
   /* this entry is already fixed up? */
   if (ents[i].flags==2) return;

   /* check for circular reference */
   if (ents[i].flags==1) {
      if (i!=frm && prtCircular) {
         printf("filldep: circular reference to %s from %s\n", 
                idxtostr(ents[i].pkgIdx), idxtostr(ents[frm].pkgIdx));
      }
      return;
   }

   ents[i].flags = 1; /* mark that we're working on it... */
   
   /* entries are always appended so nents will change... */
   for (j=0; j<ents[i].ndeps; j++) {
      const int didx = ents[i].deps[j];
      const int idx = lookupPackage(ents, nents, didx);
      int k, m;
      
      if (idx==-1) {
         if (prtDangling) {
            printf("filldep: dangling reference to %s from %s\n",
                   idxtostr(didx), idxtostr(ents[i].pkgIdx));
         }
         continue;
      }
      
      /* fixup this dependency... */
      fixupEnt(ents, nents, idx, i);

      /* ok, we're both fixed up, lets merge the dependencies... */
      for (k=0; k<ents[idx].ndeps; k++) {
         int found = 0;
         
         for (m=0; m<ents[i].ndeps && !found; m++) {
            if (ents[i].deps[m]==ents[idx].deps[k]) found=1;
         }
         
         if (!found) {
            /* we need to add it...
             */
            addDep(ents + i, ents[idx].deps[k]);
         }
      }
   }

   /* we made it through ndeps, we're done...
    */
   ents[i].flags = 2;
}

int main(int argc, char *argv[]) {
   char line[4096];
   JarEnt *ents = NULL;
   int nents = 0, entln = 0;
   int lineno=0;
   int i;
   int ai = 1;
   int prtDeps = 0;
   int prtJarDeps = 0;

   for (ai=1; ai<argc; ai++) {
      if (argv[ai][0]!='-') break;

      if (strcmp(argv[ai], "-deps")==0) {
         prtDeps = 1;
      }
      else if (strcmp(argv[ai], "-dangling")==0) {
         prtDangling = 1;
      }
      else if (strcmp(argv[ai], "-circular")==0) {
         prtCircular = 1;
      }
      else if (strcmp(argv[ai], "-jardep")==0) {
         prtJarDeps = 1;
      }
      else {
         printf("filldeps: [-deps -dangling -circular -jardep]\n");
         return 0;
      }
   }

   /* if no options set, print deps... */
   if (prtDeps==0 && prtDangling==0 && prtCircular==0 && prtJarDeps==0) 
      prtDeps = 1;

   /* initialize hashMap... */
   if (hashMapInit()) {
      fprintf(stderr, "filldeps: unable to init hash map\n");
      return 1;
   }
   
   while (fgets(line, sizeof(line), stdin)!=NULL) {
      char *t = line, *t2;

      lineno++;
      
      /* skip empty lines */
      if (strcmp(line, "")==0) continue;

      /* first get the jar file name... */
      if ((t2 = strchr(t, ' '))==NULL) {
         fprintf(stderr, "filldeps: invalid dependency, line %d\n", lineno);
         return 1;
      }
      *t2 = 0;
      
      if (entln-nents==0) {
         /* resize ents... */
         entln = entln*2 + 4096;
         if ((ents = (JarEnt *) realloc(ents, entln*sizeof(JarEnt)))==NULL) {
            fprintf(stderr, "filldeps: unable to realloc: %d\n", entln);
            return 1;
         }
      }
      
      ents[nents].jarIdx = strtoidx(t);
      t = t2 + 1;

      /* now get package name... */
      if ((t2 = strchr(t, ' '))!=NULL) {
         *t2 = 0;
      }
      else {
         t2 = NULL;
         if (strlen(t)==0) {
            fprintf(stderr, "filldeps: empty package on line %d\n", lineno);
            return 1;
         }
      }
      
      ents[nents].pkgIdx = strtoidx(t);
      ents[nents].deps = NULL;
      ents[nents].ndeps = 0;
      ents[nents].deplen = 0;
      ents[nents].flags = 0;
      
      if (t2!=NULL) {
         t = t2+1;
         while ((t2=strchr(t, ' '))!=NULL) {
            *t2 = 0;
            if (addDepStr(ents + nents, t)) {
               fprintf(stderr, "filldeps: can't add dep: line: %d\n", lineno);
               return 1;
            }
            t = t2 + 1;
         }

         /* clean up end of line marker if it's there... */
         if ((t2=strchr(t, '\n'))!=NULL) *t2 = 0;
         
         if (addDepStr(ents + nents, t)) {
            fprintf(stderr, "filldeps: can't add dep: line: %d\n", lineno);
            return 1;
         }
      }
      
      nents++;
   }

   /* canonicalize:
    *
    * for each package:
    *   for each dep 
    */
   for (i=0; i<nents; i++) fixupEnt(ents, nents, i, i);

#if 0
   printf("found: %d deps\n", nents);
   
   {  int maxdeps = 0;
      for (i=0; i<nents; i++) if (ents[i].ndeps>maxdeps) maxdeps=ents[i].ndeps;
      printf("maxdeps: %d\n", maxdeps);
   }
#endif

   if (prtDeps) {
      for (i=0; i<nents; i++) {
         int j;
         
         printf("%s %s", idxtostr(ents[i].jarIdx), idxtostr(ents[i].pkgIdx));
         for (j=0; j<ents[i].ndeps; j++) {
            printf(" %s", idxtostr(ents[i].deps[j]));
         }
         printf("\n");
      }
   }
   
   if (prtJarDeps) {
      int jdeplen=0;
      int *jdeps=NULL;
      
      for (i=0; i<nents; i++) {
         int j;
         int njdeps = 0;
         for (j=0; j<ents[i].ndeps; j++) {
            const int idx = lookupPackage(ents, nents, ents[i].deps[j]);
            if (idx>=0) {
               int k;
               int found = 0;

               /* check for this entry already! */
               for (k=0; k<njdeps && !found; k++) {
                  if (jdeps[k]==ents[idx].jarIdx) found = 1;
               }

               if (found) continue;

               /* make sure there is space... */
               if (njdeps<=jdeplen) {
                  jdeplen = jdeplen + 1024;
                  if ((jdeps = (int *) realloc(jdeps, jdeplen*sizeof(int)))
                      ==NULL) {
                     fprintf(stderr, "filldeps: can't reallocate\n");
                     return 1;
                  }
               }

               jdeps[njdeps] = ents[idx].jarIdx;
               njdeps++;
            }
         }

         if (njdeps>0) {
            printf("%s", idxtostr(ents[i].pkgIdx));
            for (j=0; j<njdeps; j++) {
               printf(" %s", idxtostr(jdeps[j]));
            }
            printf("\n");
         }
      }
   }

   return 0;
}
