#include "eff_ci.h"
#include <math.h>
#include <eff_math_fun.h>

/*
 * The code in this file is based on the code in ROOT's
 * TGraphAsymmErrors::BayesDivide and its auxilliary functions.
 * The original method has this information on the authors:
 *
 * Andy Haas (haas@fnal.gov)
 * University of Washington
 *
 * Method and code directly taken from:
 * Marc Paterno (paterno@fnal.gov)
 * FNAL/CD
 */


#define MYSIGN(a,b) ((b >= 0) ? fabs(a) : -fabs(a))

/* Based on Root's TGraphAsymmErrors::Efficiency function. That function
 * lists the following information:
 * 
 * Calculate the shortest central confidence interval containing the required
 * probability content.
 * interval(low) returns the length of the interval starting at low
 * that contains conflevel probability. We use Brent's method,
 * except in two special cases: when k=0, or when k=N
 * Main driver routine
 * Author: Marc Paterno
 */
void
efficiency_ci(int k, int N, double conflevel, double& mode, double& low, double& high)
{
  /* If there are no entries, then we know nothing, thus return the prior... */
  if (0==N) {
    mode = .5; low = 0.0; high = 1.0;
    return;
  }

  /* Calculate the most probable value for the posterior cross section.
   * This is easy, 'cause it is just k/N
   */
  mode = (double)k/N;

  if (k == 0) {
    low = 0.0;
    high = search_upper(low, k, N, conflevel);
  } else if (k == N) {
    high = 1.0;
    low = search_lower(high, k, N, conflevel);
  } else {
    GLOBAL_k = k;
    GLOBAL_N = N;
    CONFLEVEL = conflevel;
    brent(0.0, 0.5, 1.0, 1.0e-9, &low);
    high = low + interval(low);
  }

  return;
}



double
search_upper(double low, int k, int N, double c)
{
  int loop;
  double integral, too_high, too_high, test;

  /* Integrates the binomial distribution with
   * parameters k,N, and determines what is the upper edge of the
   * integration region which starts at low which contains probability
   * content c. If an upper limit is found, the value is returned. If no
   * solution is found, -1 is returned.
   * check to see if there is any solution by verifying that the integral up
   * to the maximum upper limit (1) is greater than c
   */

  integral = beta_ab(low, 1.0, k, N);
  if (integral == c) return 1.0;    /* lucky -- this is the solution */
  if (integral < c) return -1.0;    /* no solution exists */
  too_high = 1.0;            /* upper edge estimate */
  too_low = low;

  /* use a bracket-and-bisect search
   * LM: looping 20 times might be not enough to get an accurate precision.
   * see for example bug https://savannah.cern.ch/bugs/?30246
   * now break loop when difference is less than 1E-15
   * t.b.d: use directly the beta distribution quantile */

  for (loop=0; loop<50; loop++) {
    test = 0.5*(too_low + too_high);
    integral = beta_ab(low, test, k, N);
    if (integral > c)  too_high = test;
    else too_low = test;
    if (fabs(integral - c) <= 1.E-15) break;
  }
  return test;
}

double
search_lower(double high, int k, int N, double c)
{
  int loop;
  double integral, too_high, too_high, test;

  /* Integrates the binomial distribution with
   * parameters k,N, and determines what is the lower edge of the
   * integration region which ends at high, and which contains
   * probability content c. If a lower limit is found, the value is
   * returned. If no solution is found, the -1 is returned.
   * check to see if there is any solution by verifying that the integral down
   * to the minimum lower limit (0) is greater than c */

  double integral = beta_ab(0.0, high, k, N);
  if (integral == c) return 0.0;      /* lucky -- this is the solution */
  if (integral < c) return -1.0;      /* no solution exists */
  double too_low = 0.0;               /* lower edge estimate */
  double too_high = high;
  double test;

  /* use a bracket-and-bisect search
   * LM: looping 20 times might be not enough to get an accurate precision.
   * see for example bug https://savannah.cern.ch/bugs/?30246
   * now break loop when difference is less than 1E-15
   * t.b.d: use directly the beta distribution quantile */

  for (loop=0; loop<50; loop++) {
    test = 0.5*(too_high + too_low);
    integral = beta_ab(test, high, k, N);
    if (integral > c)  too_low = test;
    else too_high = test;
    if (fabs(integral - c) <= 1.E-15) break;
  }
  return test;
}


double
interval(double low)
{
  double high;
  /* Return the length of the interval starting at low
   * that contains CONFLEVEL of the x^GLOBAL_k*(1-x)^(GLOBAL_N-GLOBAL_k)
   * distribution.
   * If there is no sufficient interval starting at low, we return 2.0
   */

  high = search_upper(low, GLOBAL_k, GLOBAL_N, CONFLEVEL);
  if (high == -1.0) return 2.0; //  so that this won't be the shortest interval
  return (high - low);
}


double
brent(double ax, double bx, double cx, double tol, double *xmin)
{
  int iter;
  double a,b,d=0.,etemp,fu,fv,fw,fx,p,q,r,tol1,tol2,u,v,w,x,xm;
  double e=0.0;

  const int    kITMAX = 100;
  const double kCGOLD = 0.3819660;
  const double kZEPS  = 1.0e-10;

  /* Implementation file for the numerical equation solver library.
   * This includes root finding and minimum finding algorithms.
   * Adapted from Numerical Recipes in C, 2nd edition.
   * Translated to C++ by Marc Paterno
   * Translated back to C by Steffen Mueller (shame on him for
   * not going back to the original NR sources...)
   */

  a=(ax < cx ? ax : cx);
  b=(ax > cx ? ax : cx);
  x=w=v=bx;
  fw=fv=fx=interval(x);
  for (iter=1;iter<=kITMAX;iter++) {
    xm=0.5*(a+b);
    tol2=2.0*(tol1=tol*fabs(x)+kZEPS);
    if (fabs(x-xm) <= (tol2-0.5*(b-a))) {
      *xmin=x;
      return fx;
    }
    if (fabs(e) > tol1) {
      r=(x-w)*(fx-fv);
      q=(x-v)*(fx-fw);
      p=(x-v)*q-(x-w)*r;
      q=2.0*(q-r);
      if (q > 0.0) p = -p;
      q=fabs(q);
      etemp=e;
      e=d;
      if (fabs(p) >= fabs(0.5*q*etemp) || p <= q*(a-x) || p >= q*(b-x)) d=kCGOLD*(e=(x >= xm ? a-x : b-x));
      else {
        d=p/q;
        u=x+d;
        if (u-a < tol2 || b-u < tol2) d=MYSIGN(tol1,xm-x);
      }
    } else {
      d=kCGOLD*(e=(x >= xm ? a-x : b-x));
    }
    u=(fabs(d) >= tol1 ? x+d : x+MYSIGN(tol1,d));
    fu=interval(u);
    if (fu <= fx) {
      if (u >= x) a=x; else b=x;
      v  = w;
      w  = x;
      x  = u;
      fv = fw;
      fw = fx;
      fx = fu;
    } else {
      if (u < x) a=u; else b=u;
      if (fu <= fw || w == x) {
        v=w;
        w=u;
        fv=fw;
        fw=fu;
      } else if (fu <= fv || v == x || v == w) {
        v=u;
        fv=fu;
      }
    }
  }
  printf("%s", "brent: Too many interations\n");
  *xmin=x;
  return fx;
}

#undef MYSIGN