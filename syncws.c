/* updws.c, update the workspace...
 *
 * we're input a bunch of lines with:
 *  to_directory from_directory files...
 *
 * where (arbitrarily) from_directory is the cvs
 * repository and to_directory is the ws location...
 *
 * to update the repository, always use
 * make update as it syncs the dom-ws ->
 * CVS directories then does a cvs update
 * then syncs the CVS directories to the
 * ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define tmalloc(t) (t *) malloc(sizeof(t)) 
#define tcalloc(n, t) (t *) calloc(n, sizeof(t)) 

#include "hashMap.c"

/* search backwards for ch, return pointer
 * or NULL if ch not found.  don't search
 * farther backwards than st...
 */
static char *strrsrch(const char *st, const char *lk, int ch) {
   while (lk>=st && *lk!=ch) lk--;
   return (lk>=st) ? (char *)lk : NULL;
}

static int baseidx(const char *st) {
   /* first look for basename first... */
   char *et = strrchr(st, '/'), *rt;
   if (et==NULL) return -1;
   if ((rt=strrsrch(st, et-1, '/'))==NULL) return -1;
   {  int len = et - rt - 1, base;
      char *ret = (char *) malloc(len+1);
      memcpy(ret, rt+1, len);
      ret[len] = 0;
      base = strtoidx(ret);
      free(ret);
      return base;
   }
}

typedef struct FileListStruct {
   int fn;   /* from filename... */
   int from; /* from directory string */
   int base; /* from basename string */
   const char *path;
   struct FileListStruct *next;
} FileList;

typedef struct DirListStruct {
   int to;   /* to directory string */
   FileList *files;
   struct DirListStruct *next;
} DirList;

static DirList *dirList;
static int nDirList;

static int dltocmp(const void *p1, const void *p2) {
   const int i1 = dirList[* (const int *) p1].to;
   const int i2 = dirList[* (const int *) p2].to;
   if (i1<i2) return -1;
   if (i1>i2) return 1;
   return 0;
}

static int flfncmp(const void *p1, const void *p2) {
   const int i1 = ((const FileList *) p1)->fn;
   const int i2 = ((const FileList *) p2)->fn;
   if (i1<i2) return -1;
   if (i1>i2) return 1;
   return 0;
}

/* make directories leading up to path */
static int mkPath(const char *path) {
   const int n = strlen(path);
   char *t = (char *) malloc(n+1);
   char *tt = t;
   memcpy(t, path, n); t[n] = 0;
   while (*tt) {
      char *nt;
      struct stat st;
      
      if ((nt=strchr(tt, '/'))==NULL) {
         free(t);
         return 0;
      }
      *nt=0;

      if (stat(t, &st)<0) {
         if (mkdir(t, 0755)<0) {
            perror("syncws: mkdir");
            return 1;
         }
      }
      else if (!S_ISDIR(st.st_mode)) {
         fprintf(stderr, "syncws: '%s' is not a directory!\n", t);
         free(t);
         return 1;
      }
      
      *nt='/';
      tt=nt+1;
   }
   fprintf(stderr, "syncws: '%s' is not a file path!\n", path);
   free(t);
   return 1;
}

/* are f1 and f2 different? */
static int fileDiff(const char *f1, const char *f2) {
   char buf1[1024*4];
   char buf2[1024*4];
   int ret = 0;
   int fd1 = open(f1, O_RDONLY);
   int fd2 = open(f2, O_RDONLY);
   if (fd1<0 || fd2<0) {
      if (fd1>=0) close(fd1);
      if (fd2>=0) close(fd2);
      return 1;
   }
   while (1) {
      const int nr1 = read(fd1, buf1, sizeof(buf1));
      const int nr2 = read(fd2, buf2, sizeof(buf2));
      if (nr1<0 || nr2<0 || nr1!=nr2 ||
          (nr1>0 && memcmp(buf1, buf2, nr1)!=0)) {
         ret = 1;
         break;
      }
      if (nr1==0) break;
   }
   close(fd1);
   close(fd2);
   return ret;
}

/* sync up the cvs */
static int syncFile(const char *cvs, const char *ws, const char *mirror) {
   struct stat cvsst, wsst, mirrorst;
   
   /* no cvs ERROR, STOP */
   if (lstat(cvs, &cvsst)<0) {
      char msg[128];
      snprintf(msg, sizeof(msg), strerror(errno));
      fprintf(stderr, "syncws: can't find cvs file '%s': %s\n",
              cvs, msg);
      return 1;
   }

   /* no mirror, link mirror to cvs */
   if (lstat(mirror, &mirrorst)<0) {
      if (mkPath(mirror)) {
         fprintf(stderr, "syncws: can't make cvs mirror path '%s'\n",
                 mirror);
         return 1;
      }
      if (link(cvs, mirror)<0) {
         perror("syncws: link");
         fprintf(stderr, "syncws: can't link cvs '%s' to '%s'\n", cvs, mirror);
         return 1;
      }
      mirrorst = cvsst;
   }

   /* no ws, link to cvs */
   if (lstat(ws, &wsst)<0) {
      if (mkPath(ws)) {
         fprintf(stderr, "syncws: can't make ws path '%s'\n", ws);
         return 1;
      }
      if (link(cvs, ws)<0) {
         perror("syncws: link");
         fprintf(stderr, "syncws: can't link '%s' to '%s'\n", cvs, ws);
         return 1;
      }
      wsst = cvsst;
   }

   /* all files must be regular! */
   if (!S_ISREG(cvsst.st_mode) ||
       !S_ISREG(mirrorst.st_mode) ||
       !S_ISREG(wsst.st_mode)) {
      fprintf(stderr, "syncws: all files must be regular\n");
      return 1;
   }
   
   /* ws==cvs, done */
   if (wsst.st_ino==cvsst.st_ino) return 0;
   
   
   /* cvs==mirror (only edited) */
   if (cvsst.st_ino==mirrorst.st_ino) {
      /* ws not modified more recently?  ERROR, STOP */
      if (wsst.st_mtime < cvsst.st_mtime && fileDiff(ws, cvs)) {
         fprintf(stderr, 
                 "syncws: ws file '%s' is different and older than "
                 "cvs file '%s' you must fix this up by hand...\n", ws, cvs);
         return 1;
      }
      
      /* mirror <- cvs <- ws */
      if (unlink(mirror)<0 || unlink(cvs)<0 || link(ws, cvs)<0 ||
          link(ws, mirror)<0) {
         perror("syncws: link/unlink");
         fprintf(stderr, 
                 "unable to link '%s' and '%s' to '%s'\n",
                 mirror, cvs, ws);
         return 1;
      }

      printf("update cvs with ws file '%s'\n", ws);
   }
   /* ws==mirror (only cvs updated) */
   else if (wsst.st_ino==mirrorst.st_ino) {
      /* ws <- mirror <- cvs */
      if (cvsst.st_mtime < mirrorst.st_mtime && fileDiff(cvs, mirror)) {
         fprintf(stderr, 
                 "synws: cvs file '%s' is different and older "
                 "than mirror file '%s', you must fix this up by hand\n",
                 cvs, mirror);
         return 1;
      }
      
      if (unlink(mirror)<0 || unlink(ws)<0 || link(cvs, ws)<0 ||
          link(cvs, mirror)<0) {
         perror("syncws: link/unlink");
         fprintf(stderr, 
                 "unable to link '%s' and '%s' to '%s'\n",
                 mirror, cvs, ws);
         return 1;
      }

      printf("update ws with new cvs change in '%s'\n", cvs);
   }
   else {
      /* BOTH cvs and ws have changed, ERROR, STOP */
      fprintf(stderr, 
              "syncws: both ws file '%s' and cvs file '%s' are "
              "different than the cvs mirror, you need to fix this by hand\n",
              ws, cvs);
      return 1;
   }

   return 0;
}

static int *platforms;
static int nplatforms;

/* pick correct files from list -- duplicates must be merged:
 *  platform specific files choosen over generic code...
 */
static FileList *resolveFiles(const FileList *nfl, int n, int *nnflp) {
   int nnfl = 0;
   int j;
   FileList *fl = tcalloc(n, FileList);
   
   /* compact files... */
   j=0;
   while (j<n) {
      const int fnidx = nfl[j].fn;
      int kj = j+1;

      while (kj<n && nfl[kj].fn==fnidx) kj++;
      
      {  const int ndups = kj - (j+1);

         if (ndups==0) {
            fl[nnfl] = nfl[j];
            nnfl++;
         }
         else {
            int k, ndropped = 0;
            
            for (k=-1; k<ndups; k++) {
               int m;
               int found = 0;

#if 0               
               {  const FileList *f = nfl + j + k + 1;
                  printf(" %s/%s [%s]", idxtostr(f->from), idxtostr(f->fn),
                         idxtostr(f->base));
               }
#endif
               
               /* drop fns without the platform specific name... */
               for (m=0; !found && m<nplatforms; m++) {
#if 0
                  printf(" cmp: %s <-> %s [%d <-> %d]\n",
                         idxtostr(nfl[j+1+k].base),
                         idxtostr(platforms[m]),
                         nfl[j+1+k].base, platforms[m]);
#endif
                  
                  if (nfl[j+1+k].base==platforms[m])
                     found = 1;
               }
               
               if (found) { fl[nnfl] = nfl[j+1+k]; nnfl++; }
               else ndropped++;
            }
            
            if (ndups!=ndropped) {
               fprintf(stderr,
                       "syncws: duplicate file names %s\n", idxtostr(fnidx));
               return NULL;
            }
         }
      }
      
      /* increment j */
      j = kj;
   }

   /* link up array... */
   for (j=0; j<nnfl-1; j++) fl[j].next = fl+j+1;
   fl[nnfl-1].next = NULL;

   *nnflp = nnfl;
   return fl;
}

   /* sort dirList... */
int main(int argc, char *argv[]) {
   char line[1024*8];
   int zidx;
   int lineno = 1;
   int pubidx, prividx;
   int i;

   if (argc<2) {
      fprintf(stderr, "usage: updws platform...\n");
      return 1;
   }

   /* initialize hashMap... */
   if (hashMapInit()) {
      fprintf(stderr, "updws: unable to initialize hash map\n");
      return 1;
   }

   /* initialize platform indecies... */
   nplatforms = argc-1;
   platforms = (int *) malloc(nplatforms*sizeof(int));
   for (i=0; i<nplatforms; i++) platforms[i] = strtoidx(argv[i+1]);

   /* allocate a few more string indecies... */
   pubidx = strtoidx("public");
   prividx = strtoidx("private");
   zidx = strtoidx("");

   /* read loop... */
   while (fgets(line, sizeof(line), stdin)!=NULL) {
      char *t = line, *st;
      DirList *dl = tmalloc(DirList);
      int base, from;

      /* get to directory... */
      st = t;
      while (*t!=' ' && *t!='\n' && *t) t++;
      *t = 0;
      if ((dl->to = strtoidx(st))==zidx) {
         fprintf(stderr, "updws: line %d: no from directory!\n", lineno);
         return 1;
      }
      
      /* get from directory... */
      t++;
      st = t;
      while (*t!=' ' && *t!='\n' && *t) t++;
      *t = 0;
      
      if ((base=baseidx(st))<0 || base==zidx) {
         fprintf(stderr, "updws: line %d: invalid base directory\n",
                 lineno);
         return 1;
      }
      
      from = strtoidx(st);

      /* get files... */
      t++;
      st = t;
      dl->files = NULL;
      while (1) {
         char *nt;
         
         /* get the next string... */
         while (*t!=' ' && *t!='\n' && *t) t++;
         if (*t!=0) {
            *t = 0;
            nt = t+1;
         }
         else nt = t;
         
         {  const int fidx = strtoidx(st);
            FileList *fl;
            if (fidx==zidx) break;
            fl = tmalloc(FileList);
            fl->fn = fidx;
            fl->base = base;
            fl->from = from;
            fl->path = NULL;
            fl->next = dl->files;
            dl->files = fl;
         }

         t = st = nt;
      }

      /* if there are files, link the new entry... */
      if (dl->files!=NULL) {
         dl->next = dirList;
         dirList = dl;
      }
      else free(dl);
      
      /* next line... */
      lineno++;
   }

   /* turn dirList into an array... */
   {  DirList *d = (DirList *) calloc(lineno, sizeof(DirList));
      int i=0;
      while (i<lineno && dirList) {
         DirList *dlf = dirList;
         d[i] = *dirList;
         dirList = dirList->next;
         free(dlf);
         i++;
      }
      dirList = d;
      nDirList = i;
   }

   /* compact the dirlist */
   {  int *keys = (int *) calloc(nDirList, sizeof(int));
      DirList *ndl = tcalloc(nDirList, DirList);
      int nndl = 0;

      for (i=0; i<nDirList; i++) keys[i] = i;

      /* now sort indecies on to... */
      qsort(keys, nDirList, sizeof(int), dltocmp);

      i=0;
      while (i<nDirList) {
         /* coalesce to entries... */
         DirList *dl = dirList + keys[i];
         const int toidx = dl->to;
         int ki = i+1;
         
         ndl[nndl] = *dl; nndl++;

          while (ki<nDirList && dirList[keys[ki]].to==toidx) ki++;
         
         {  const int ndups = ki - (i + 1);
            int j;

            for (j=0; j<ndups; j++) {
               DirList *ddl = dirList + keys[i+j+1];
               
               /* merge files in... */
               while (ddl->files!=NULL) {
                  FileList *tfl = ddl->files;
                  ddl->files = ddl->files->next;
                  tfl->next = ndl[nndl-1].files;
                  ndl[nndl-1].files = tfl;
               }
            }
         }

         /* increment i */
         i=ki;
      }

      /* new variables... */
      free(dirList);
      free(keys);
      dirList = ndl;
      nDirList = nndl;
   }

   /* ok, dirList is now compacted, we need to pick the
    * proper files in each list...
    *
    * convert files list to arrays and sort them... 
    */
   for (i=0; i<nDirList; i++) {
      FileList *fl, *nfl;
      int n, j;

      /* turn linked list into array... */
      for (n=0, fl=dirList[i].files; fl!=NULL; fl=fl->next, n++) ;
      
      nfl = tcalloc(n, FileList);
      fl=dirList[i].files;
      for (j=0; j<n; j++) { 
         FileList *tl = fl;
         nfl[j] = *fl;
         fl=fl->next; 
         free(tl);
      }
      dirList[i].files = nfl;

      /* now sort the list by filename... */
      qsort(nfl, n, sizeof(FileList), flfncmp);

      /* compact files... */
      {  int nn;
         FileList *fl = resolveFiles(nfl, n, &nn);
         if (fl==NULL) {
            fprintf(stderr, 
                    "syncws: can not resolve files in %s\n",
                    idxtostr(dirList[i].to));
            return 1;
         }
         free(nfl);
         dirList[i].files = fl;
      }
   }

   /* the lists should be clean now, let's go ahead and sync...
    */
   {  int i;
      for (i=0; i<nDirList; i++) {
         FileList *f;
         for (f=dirList[i].files; f!=NULL; f=f->next) {
            char cvs[512], ws[512], mirror[512];
            
            snprintf(cvs, sizeof(cvs),
                     "%s/%s", idxtostr(f->from), idxtostr(f->fn));
            snprintf(ws, sizeof(ws),
                     "%s/%s", idxtostr(dirList[i].to), idxtostr(f->fn));
            snprintf(mirror, sizeof(mirror),
                     "../.cvs-mirror/tmp/%s/%s", 
                     idxtostr(f->from), idxtostr(f->fn));

            if (syncFile(cvs, ws, mirror)) {
               fprintf(stderr, "syncws: can't sync '%s' and '%s'\n",
                       cvs, ws);
               return 1;
            }
         }
      }
   }
   
#if 0
   /* dump the database... */
   {  DirList *d;
      int i;
   
      for (i=0; i<nDirList; i++) {
         FileList *f;
         printf("%s", idxtostr(dirList[i].to));
         for (f=dirList[i].files; f!=NULL; f=f->next) {
            printf(" %s/%s [%s]", idxtostr(f->from), idxtostr(f->fn),
                   idxtostr(f->base));
         }
         printf("\n");
      }
   }
#endif

   return 0;
}








