#include <math.h>

#define TESTING
#undef TESTING

#ifndef CORDICBITS
#define CORDICBITS 12
#endif

/* cordic phase calculator...
 *
 * output is in radians * pi * (2^bits)
 */
short getPhase(int I, int Q) {
   static int isInit;
   int i;
   signed short accPhase = 0;
   const int bits = CORDICBITS;
   static signed short table[CORDICBITS];
   const int loopCount = (bits - 1);
   const signed short maxVal = (1<<bits) - 1;
   const double pi = acos(-1);

   if (!isInit) {
      for (i=0; i<loopCount; i++) {
         int v = (maxVal / pi) * atan( (double) 1.0 / (1<<i));
         if (v<-maxVal) v = -maxVal;
         if (v>maxVal) v = maxVal;
         table[i] = (signed short) v;
      }
      isInit = 1;
   }

   /* start condition -- do we need to rotate by +-(pi/2)? 
    */
   if (I<0) {
      const signed short pid2 = (signed short)((maxVal/pi)*acos(0));
      const int II = I;
      if (Q > 0) {
         I = Q;
         Q = -II;
         accPhase = -pid2;
      } 
      else {
         I = -Q;
         Q = II;
         accPhase = pid2;
      }
   } 
   else accPhase = 0;

   /* for each bit... */
   for (i=0; i<loopCount; i++) {
      const signed short phase = table[i];
      const int IShift = I>>i; /* signed shift */
      const int QShift = Q>>i; /* signed shift */
      
      if (Q>=0) {
         I += QShift;
         Q -= IShift;
         accPhase -= phase;
      }
      else {
         I -= QShift;
         Q += IShift;
         accPhase += phase;
      }
   }

   return -accPhase;
}

#if defined(TESTING)

#include <stdio.h>
#include <stdlib.h>

int main(void) {
   int i;
   const int n = (int) 1e6;
   const double pi = acos(-1);
   const int maxVal = (1<<CORDICBITS)-1;
   double maxErr = 0;

   for (i=0; i<n; i++) {
      /* lv a little room at the top */
      const int I = (random() - RAND_MAX/2)>>1;
      const int Q = (random() - RAND_MAX/2)>>1;
      const double phase = pi*getPhase(I, Q)/maxVal;
      const double angle = atan2(Q, I);
      const double err = fabs(angle-phase);
      
      if (err>maxErr) {
         maxErr = err;
#if 0
      printf("err = %f [%f should be %f] %d\n", fabs(angle - phase),
             phase, angle, (int)(angle*maxVal/pi));
#endif
      }
      
   }

   printf("maxErr = %f radians (1 part in %d)\n",
          maxErr, (int) ((2*pi)/maxErr));

   return 0;
}

#endif




