/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/* (c) Copyright 1989,1990,1991,1992 by                                      */
/*     Senter for Industriforskning, Oslo, Norway                            */
/*     All rights reserved. See the copyright.h for more details.            */
/*                                                                           */
/*****************************************************************************/

#include "copyright.h"

/*
 *
 * $Id: sh1502.c,v 1.1 1994-04-21 12:10:42 boh Exp $
 *
 */


#define SH1502

#include "sislP.h"

#if defined(SISLNEEDPROTOTYPES)
void 
sh1502(SISLCurve *pc1,double base[],double norm[],double axisA[],double alpha,double ratio,int idim,
	   double aepsco,double aepsge,
	int trackflag,int *jtrack,SISLTrack ***wtrack,
	int *jpt,double **gpar,int **pretop,int *jcrv,SISLIntcurve ***wcurve,int *jstat)
#else
void sh1502(pc1,base,norm,axisA,alpha,ratio,idim,aepsco,aepsge,
	trackflag,jtrack,wtrack,jpt,gpar,pretop,jcrv,wcurve,jstat)
     SISLCurve    *pc1;
     double   base[];
     double   norm[];
     double   axisA[];
     int      idim;
     double   aepsco;
     double   aepsge;
     int       trackflag;
     int       *jtrack;
     SISLTrack ***wtrack;
     int      *jpt;
     double   **gpar;
     int      **pretop;
     int      *jcrv;
     SISLIntcurve ***wcurve;
     int      *jstat;
#endif
/*
*********************************************************************
*
*********************************************************************
*                                                                   
* PURPOSE    : Find all intersections between a curve and an elliptic cone.
*
*
*
* INPUT      : pc1    - Pointer to the curve.
*              base   - Base point of cone
*              norm   - Direction of cone axis
*              axisA  - One of the two ellipse axis vectors
*              alpha  - The opening angle of the cone at axisA
*              ratio  - The ratio of axisA to axisB
*              idim   - Dimension of the space in which the plane/line
*                       lies. idim should be equal to two or three.
*              aepsco - Computational resolution.
*              aepsge - Geometry resolution.
*              trackflag - If true, create tracks.
*
*
*
* OUTPUT     : jtrack - Number of tracks created
*              wtrack - Array of pointers to tracks
*              jpt    - Number of single intersection points.
*              gpar   - Array containing the parameter values of the
*                       single intersection points in the parameter
*                       interval of the curve. The points lie continuous. 
*                       Intersection curves are stored in wcurve.
*              pretop - Topology info. for single intersection points.
*              *jcrv  - Number of intersection curves.
*              wcurve  - Array containing descriptions of the intersection
*                       curves. The curves are only described by points
*                       in the parameter interval. The curve-pointers points
*                       to nothing. (See description of Intcurve
*                       in intcurve.dcl).
*              jstat  - status messages  
*                                         > 0      : warning
*                                         = 0      : ok
*                                         < 0      : error
*
*
* METHOD     : The vertices of the curve is put into the equation of the
*              cone achieving a curve in the one-dimentional space.
*              The zeroes of this curve is found.
*
*
* REFERENCES :
*
*- 
* CALLS      : sh1761 - Find point/curve intersections.
*              hp_s1880 - Put intersections into output format.
*              s1500 - Make description of cone as matrix
*              s1370 - Put curve into implicit equation
*              make_cv_kreg - Ensure k-regularity of input curve.
*              newCurve   - Create new curve.
*              newPoint   - Create new point.                        
*              newObject  - Create new object.
*              freeObject - Free the space occupied by a given object.
*              freeIntdat - Free the space occupied by intersection result.
*
* WRITTEN BY : Mike Floater, SI, 18/10/90.
*
*********************************************************************
*/                                                               
{                                                                     
  double *nullp = NULL;
  int kstat = 0;             /* Local status variable.                       */
  int kpos = 0;              /* Position of error.                           */
  int kdim=1;                /* Dimension of curve in curve/point intersection.*/
  double sarray[16];         /* Array for represetnation of implicit surface */
  double spoint[1];          /* SISLPoint in curve/point intersection.           */
  double *spar = NULL;       /* Values of intersections in the parameter 
				area of the second object. Empty in this case. */
  SISLCurve *qc = NULL;          /* Pointer to curve in 
				curve/point intersection.  */
  SISLPoint *qp = NULL;          /* Pointer to point in 
				curve/point intersection.  */ 
  SISLObject *qo1 = NULL;        /* Pointer to object in 
				object/point intersection.*/
  SISLObject *qo2 = NULL;        /* Pointer to object in 
				object/point intersection.*/
  SISLIntdat *qintdat = NULL;   /* Pointer to intersection data */
  int      ksurf=0;         /* Dummy number of Intsurfs. */
  SISLIntsurf **wsurf=NULL;    /* Dummy array of Intsurfs. */
  int      kdeg=2000;       /* input to int_join_per. */
  SISLObject *track_obj=NULL;
  SISLCurve *qkreg=NULL; /* Input surface ensured k-regularity. */

  /* -------------------------------------------------------- */  

  if (pc1->cuopen == SISL_CRV_PERIODIC)
  {
     /* Cyclic curve. */

     make_cv_kreg(pc1,&qkreg,&kstat);
     if (kstat < 0) goto error;
   }
  else
    qkreg = pc1;

  /*
  * Create new object and connect curve to object.
  * ------------------------------------------------
  */
  
  if (!(track_obj = newObject (SISLCURVE)))
    goto err101;
  track_obj->c1 = pc1;

  /* 
   * Check dimension.  
   * ----------------
   */

  *jpt  = 0;
  *jcrv = 0;
  *jtrack = 0;

  if ( idim != 3) goto err104;
  if (qkreg -> idim != idim) goto err103;

  /* 
   * Make a matrix of dimension (idim+1)(idim+1) to describe the cone
   * -------------------------------------------------------------------------
   */

  s1500(base,norm,axisA,alpha,ratio,idim,kdim,sarray,&kstat);
  if (kstat<0) goto error;

  /* 
   * Put curve into cone equation 
   * -------------------------------------------
   */ 

  s1370(qkreg,sarray,idim,kdim,0,&qc,&kstat);
  if (kstat<0) goto error;

  /* 
   * Create new object and connect curve to object.  
   * ----------------------------------------------
   */

  if (!(qo1 = newObject(SISLCURVE))) goto err101;
  qo1 -> c1 = qc;
  qo1 -> o1 = qo1;

  /*
   * Create new object and connect point to object.
   * ----------------------------------------------
   */

  if (!(qo2 = newObject(SISLPOINT))) goto err101;
  spoint[0] = DNULL;
  if (!(qp = newPoint(spoint,kdim,1))) goto err101;
  qo2 -> p1 = qp;

  /* 
   * Find intersections.  
   * -------------------
   */

  sh1761(qo1,qo2,aepsge,&qintdat,&kstat);
  if (kstat < 0) goto error;

  /* Join periodic curves */
  int_join_per( &qintdat,track_obj,track_obj,nullp,kdeg,aepsge,&kstat);
  if (kstat < 0)
    goto error;

  /* Create tracks */
  if (trackflag && qintdat)
    {
      make_tracks (qo1, qo2, 0, nullp,
		   qintdat->ilist, qintdat->vlist, 
		   jtrack, wtrack, aepsge, &kstat);
      if (kstat < 0)
	goto error;
    }

  /* 
   * Express intersections on output format.  
   * ---------------------------------------
   */

  if (qintdat)/* Only if there were intersections found */
    {
      hp_s1880(track_obj, track_obj, kdeg,
	       1,0,qintdat,jpt,gpar,&spar,pretop,jcrv,wcurve,&ksurf,&wsurf,&kstat);
      if (kstat < 0) goto error;
    }
  
  /* 
   * Intersections found.  
   * --------------------
   */

  *jstat = 0;
  goto out;

  /* Error in space allocation.  */

 err101: *jstat = -101;                                                   
        s6err("sh1502",*jstat,kpos);
        goto out;

  /* Dimensions conflicting.  */

 err103: *jstat = -103;
        s6err("sh1502",*jstat,kpos);
        goto out;

  /* Dimension not equal to two or three.  */

 err104: *jstat = -104;                          
        s6err("sh1502",*jstat,kpos);
        goto out;

  /* Error in lower level routine.  */

  error : *jstat = kstat;
        s6err("sh1502",*jstat,kpos);
        goto out;

 out:

  /* Free allocated space.  */

  if (spar)    freearray(spar);
  if (qo1)     freeObject(qo1);
  if (qo2)     freeObject(qo2);
  if (qintdat) freeIntdat(qintdat);
  if (track_obj)
    {
       track_obj->c1 = NULL;
       freeObject(track_obj);
    }

  /* Free local curve.  */

  if (qkreg != NULL && qkreg != pc1) freeCurve(qkreg);
return;
}                                               
                                           
                       
