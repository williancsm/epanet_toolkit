/*
*********************************************************************

HYDRAUL.C --  Hydraulic Simulator for EPANET Program

AUTHOR:     M. Lopez-Ibanez (M. L\'opez-Ib\'a\~nez)
            Napier University, Edinburgh, UK.

$LastChangedBy: manu $ $Revision: 160 $
$LastChangedDate: 2008-04-02 19:55:41 +0200 (Wed, 02 Apr 2008) $

---------------------------------------------------------------------

 Copyright (c) 2005, 2006 Manuel Lopez-Ibanez
 TeX: \copyright 2005, 2006 Manuel L\'opez-Ib\'a\~nez

 This program is free software (software libre); you can redistribute
 it and/or modify it under the terms of version 2 of the GNU General
 Public License version as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, you can obtain a copy of the GNU
 General Public License at:
                http://www.gnu.org/copyleft/gpl.html
 or by writing to:
          Free Software Foundation, Inc., 59 Temple Place,
                Suite 330, Boston, MA 02111-1307 USA

----------------------------------------------------------------------
----------------------------------------------------------------------

Original public domain sources (no license) from:

VERSION:    2.00
DATE:       6/5/00
            9/7/00
            10/25/00
            12/29/00
            3/1/01
            11/19/01
            6/24/02
AUTHOR:     L. Rossman
            US EPA - NRMRL

  This module contains the network hydraulic simulator.
  It simulates the network's hydraulic behavior over an
  an extended period of time and writes its results to the
  binary file HydFile.

  The entry points for this module are:
     openhyd()    -- called from ENopenH() in EPANET.C
     inithyd()    -- called from ENinitH() in EPANET.C
     runhyd()     -- called from ENrunH() in EPANET.C
     nexthyd()    -- called from ENnextH() in EPANET.C
     closehyd()   -- called from ENcloseH() in EPANET.C
     tankvolume() -- called from ENsetnodevalue() in EPANET.C
     setlinkstatus(),
     setlinksetting(),
     resistance()-- all called from ENsetlinkvalue() in EPANET.C

  External functions called by this module are:
     createsparse() -- see SMATRIX.C
     freesparse()   -- see SMATRIX.C
     linsolve()     -- see SMATRIX.C
     checkrules()   -- see RULES.C
     interp()       -- see EPANET.C
     savehyd()      -- see OUTPUT.C
     savehydstep()  -- see OUTPUT.C
     writehydstat() -- see REPORT.C
     writehyderr()  -- see REPORT.C
     writehydwarn() -- see REPORT.C
*******************************************************************
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "hash.h"
#include "text.h"
#include "types.h"
#include "funcs.h"
#define  EXTERN  extern
#include "vars.h"

#define   QZERO  1.e-6  /* Equivalent to zero flow */
#define   CBIG   1.e8   /* Big coefficient         */
#define   INVCBIG   1.e-8   /* 1.0 / Big coefficient         */
#define   CSMALL 1.e-6  /* Small coefficient       */

/* Constants used for computing Darcy-Weisbach friction factor */
#define A1  0.314159265359e04  /* 1000*PI */
#define A2  0.157079632679e04  /* 500*PI  */
#define A3  0.502654824574e02  /* 16*PI   */
#define A4  6.283185307        /* 2*PI    */
#define A8  4.61841319859      /* 5.74*(PI/4)^.9 */
#define A9  -8.685889638e-01   /* -2/ln(10)      */
#define AA  -1.5634601348      /* -2*.9*2/ln(10) */
#define AB  3.28895476345e-03  /* 5.74/(4000^.9) */
#define AC  -5.14214965799e-03 /* AA*AB */

/*** Updated 3/1/01 ***/
/* Flag used to halt taking further time steps */
int Haltflag;

#if __GNUC__ >= 3
# define inline		inline __attribute__ ((always_inline))
#else
# define inline		/* no inline */
#endif

// Functions defined and called only in this file
static void pumpswitch(int index, char status);
static inline void    pipecoeff(int);    /* Computes pipe coeff.       */


int  openhyd()
/*
 *--------------------------------------------------------------
 *  Input:   none
 *  Output:  returns error code
 *  Purpose: opens hydraulics solver system
 *--------------------------------------------------------------
 */
{
    int  i;
    int  errcode = 0;
    ERRCODE(createsparse());     /* See SMATRIX.C  */
    ERRCODE(allocmatrix());      /* Allocate solution matrices */
    for (i=1; i<=Nlinks; i++)    /* Initialize flows */
        initlinkflow(i,Link[i].Stat,Link[i].Kc);
    return(errcode);
}


/*** Updated 3/1/01 ***/
void inithyd(int initflag)
/*
**--------------------------------------------------------------
**  Input:   initflag > 0 if link flows should be re-initialized
**                    = 0 if not
**  Output:  none
**  Purpose: initializes hydraulics solver system
**--------------------------------------------------------------
*/
{
    int i,j,s;

    /* Initialize tanks */
    for (i=1; i<=Ntanks; i++)
    {
        Tank[i].V = Tank[i].V0;
        H[Tank[i].Node] = Tank[i].H0;

/*** Updated 10/25/00 ***/
        D[Tank[i].Node] = 0.0;

        OldStat[Nlinks+i] = TEMPCLOSED;
    }


    /* Initialize emitter flows */
    memset(E,0,(Nnodes+1)*sizeof(float));
    for (i=1; i<=Njuncs; i++)
        if (Node[i].Ke > 0.0) E[i] = 1.0;

    /* Initialize links */
    for (i=1; i<=Nlinks; i++)
    {
        /* Initialize status and setting */
        S[i] = Link[i].Stat;
        K[i] = Link[i].Kc;

        /* Start active PRVs & PSVs in OPEN position */
        if (
            (Link[i].Type == PRV || Link[i].Type == PSV)
            && (Link[i].Kc != MISSING)
            ) S[i] = OPEN;

/*** Updated 3/1/01 ***/
        /* Initialize flows if necessary */
        if (S[i] <= CLOSED) Q[i] = QZERO;
        else if (ABS(Q[i]) <= QZERO || initflag > 0)
            initlinkflow(i, S[i], K[i]);

        /* Save initial status */
        OldStat[i] = S[i];
    }

    /* Reset pump energy usage
       and pump switches       */
    for (i=1; i<=Npumps; i++)
    {
        for (j=0; j<6; j++) Pump[i].Energy[j] = 0.0;
        Pump[i].Switches = 0;
        Pump[i].Schedule_idx = 0;
        for(s=0; s < 24; s++) Pump[i].Schedule[s] = -1;
    }

    /* Re-position hydraulics file */
    if (Saveflag) fseek(HydFile,HydOffset,SEEK_SET);

/*** Updated 3/1/01 ***/
    /* Initialize current time */
    Haltflag = 0;
    Htime = 0;
    Hydstep = 0;
    Rtime = Rstep;
    checkrules(0);
}


int   runhyd(long *t)
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  t = pointer to current time (in seconds)
**  Returns: error code
**  Purpose: solves network hydraulics in a single time period
**--------------------------------------------------------------
*/
{
    int   iter;                          /* Iteration count   */
    int   errcode;                       /* Error code        */
    float relerr;                        /* Solution accuracy */

    /* Find new demands & control actions */
    *t = Htime;
    demands();
    controls();

    /* Solve network hydraulic equations */
    errcode = netsolve(&iter,&relerr);
    if (!errcode)
    {
        /* Report new status & save results */
        if (Statflag) writehydstat(iter,relerr);

/*** Updated 3/1/01 ***/
        /* If system unbalanced and no extra trials */
        /* allowed, then activate the Haltflag.     */
        if (relerr > Hacc && ExtraIter == -1) Haltflag = 1;

        /* Report any warning conditions */
        if (!errcode) errcode = writehydwarn(iter,relerr);
    }
    return(errcode);
}                               /* end of runhyd */


int  nexthyd(long *tstep)
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  tstep = pointer to time step (in seconds)
**  Returns: error code
**  Purpose: finds length of next time step & updates tank
**           levels and rule-based contol actions
**--------------------------------------------------------------
*/
{
    long  hydstep;         /* Actual time step  */
    int   errcode = 0;     /* Error code        */
    int   i;
    double tmp_float;

/*** Updated 3/1/01 ***/
    /* Save current results to hydraulics file and */
    /* force end of simulation if Haltflag is active */
    if (Saveflag) errcode = savehyd(&Htime);
    if (Haltflag) Htime = Dur;

    /* Compute next time step & update tank levels */
    *tstep = 0;
    hydstep = 0;
    if (Htime < Dur) hydstep = timestep();
    if (Saveflag) errcode = savehydstep(&hydstep);

    /* Compute pumping energy */
    if (Dur == 0) addenergy(0);
    else if (Htime < Dur) addenergy(hydstep);

    if(Htime < Dur) {

        TotalSystemDemand += Dsystem*hydstep;

        for (i=1, tmp_float=0.0; i<=Njuncs; i++) tmp_float += E[i];
        TotalSystemLeakage += tmp_float*hydstep;

        for (i=1, tmp_float=0.0; i<=Ntanks; i++)
            if (Tank[i].A == 0.0)
                tmp_float += D[Tank[i].Node];

        TotalSystemInflow += tmp_float * hydstep;
    }

    /* Update current time. */
    if (Htime < Dur)  /* More time remains */
    {
        Htime += hydstep;
        if (Htime >= Rtime) Rtime += Rstep;
    }
    else
    {
        Htime++;          /* Force completion of analysis */
    }
    *tstep = hydstep;
    return(errcode);
}


void  closehyd()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  returns error code
**  Purpose: closes hydraulics solver system
**--------------------------------------------------------------
*/
{
    freesparse();           /* see SMATRIX.C */
    freematrix();
}


int  allocmatrix()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  returns error code
**  Purpose: allocates memory used for solution matrix coeffs.
**--------------------------------------------------------------
*/
{
    int errcode = 0;
    Aii = (REAL *) calloc(Nnodes+1,sizeof(REAL));
    Aij = (REAL *) calloc(Ncoeffs+1,sizeof(REAL));
    F   = (REAL *) calloc(Nnodes+1,sizeof(REAL));
    E   = (float *) calloc(Nnodes+1,sizeof(float));
    P   = (float *) calloc(Nlinks+1,sizeof(float));
    Y   = (float *) calloc(Nlinks+1,sizeof(float));
    X   = (float *) calloc(MAX((Nnodes+1),(Nlinks+1)),sizeof(float));
    OldStat = (char *) calloc(Nlinks+Ntanks+1, sizeof(char));
    ERRCODE(MEMCHECK(Aii));
    ERRCODE(MEMCHECK(Aij));
    ERRCODE(MEMCHECK(F));
    ERRCODE(MEMCHECK(E));
    ERRCODE(MEMCHECK(P));
    ERRCODE(MEMCHECK(Y));
    ERRCODE(MEMCHECK(X));
    ERRCODE(MEMCHECK(OldStat));
    return(errcode);
}                               /* end of allocmatrix */


void  freematrix()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: frees memory used for solution matrix coeffs.
**--------------------------------------------------------------
*/
{
    free(Aii);
    free(Aij);
    free(F);
    free(E);
    free(P);
    free(Y);
    free(X);
    free(OldStat);
}                               /* end of freematrix */


void  initlinkflow(int i, char s, float k)
/*
**--------------------------------------------------------------------
**  Input:   i = link index
**           s = link status
**           k = link setting (i.e., pump speed)
**  Output:  none
**  Purpose: sets initial flow in link to QZERO if link is closed,
**           to design flow for a pump, or to flow at velocity of
**           1 fps for other links.
**--------------------------------------------------------------------
*/
{
    if (s == CLOSED) Q[i] = QZERO;
    else if (Link[i].Type == PUMP) Q[i] = k*Pump[PUMPINDEX(i)].Q0;
    else Q[i] = PI*SQR(Link[i].Diam)/4.0;
}


/*** Updated 9/7/00 ***/
void  setlinkflow(int k, float dh)
/*
**--------------------------------------------------------------
**  Input:   k = link index
**           dh = headloss across link
**  Output:  none
**  Purpose: sets flow in link based on current headloss
**--------------------------------------------------------------
*/
{
    int   i,p;
    float h0;
    REAL  x,y;

    switch (Link[k].Type)
    {
    case CV:
    case PIPE:

        /* For Darcy-Weisbach formula: */
        /* use approx. inverse of formula. */
        if (Formflag == DW)
        {
            x = -log(K[k]/3.7/Link[k].Diam);
            y = sqrt(ABS(dh)/Link[k].R/1.32547);
            Q[k] = x*y;
        }

        /* For Hazen-Williams or Manning formulas: */
        /* use inverse of formula. */
        else
        {
            /* Precision warning: Link[k].R is usually very small and
               both dh and Link[k].R are float while x is REAL.
               Therefore, there is some problem with the precision and
               both fabs() and fabsf() break compatibility.
            */
            x = ABS(dh)/Link[k].R;
            y = 1.0/Hexp;
            Q[k] = pow(x,y);
        }

        /* Change sign of flow to match sign of headloss */
        if (dh < 0.0) Q[k] = -Q[k];
        break;

    case PUMP:

        /* Convert headloss to pump head gain */
        dh = -dh;
        p = PUMPINDEX(k);

        /* For custom pump curve, interpolate from curve */
        if (Pump[p].Ptype == CUSTOM)
        {
            dh = -dh*Ucf[HEAD]/SQR(K[k]);
            i = Pump[p].Hcurve;
            Q[k] = interp(Curve[i].Npts,Curve[i].Y,Curve[i].X,
                          dh)*K[k]/Ucf[FLOW];
        }

        /* Otherwise use inverse of power curve */
        else
        {
            h0 = -SQR(K[k])*Pump[p].H0;
            x = pow(K[k],2.0-Pump[p].N);
            x = ABS(h0-dh)/(Pump[p].R*x),
                y = 1.0/Pump[p].N;
            Q[k] = pow(x,y);
        }
        break;
    }
}


void  setlinkstatus(int index, char value, char *s, float *k)
/*----------------------------------------------------------------
**  Input:   index  = link index
**           value  = 0 (CLOSED) or 1 (OPEN)
**           s      = pointer to link status
**           k      = pointer to link setting
**  Output:  none
**  Purpose: sets link status to OPEN or CLOSED
**----------------------------------------------------------------
*/
{
    /* Status set to open */
    if (value == 1)
    {
        /* Adjust link setting for pumps & valves */
        if (Link[index].Type == PUMP) *k = 1.0;

/*** Updated 9/7/00 ***/
        if (Link[index].Type >  PUMP
            &&  Link[index].Type != GPV) *k = MISSING;

        /* Reset link flow if it was originally closed */
        if (*s <= CLOSED) {
            if (Link[index].Type == PUMP) pumpswitch (index, OPEN);
            initlinkflow(index, OPEN, *k);
        }
        *s = OPEN;
    }

    /* Status set to closed */
    else if (value == 0)
    {
        /* Adjust link setting for pumps & valves */
        if (Link[index].Type == PUMP) *k = 0.0;

/*** Updated 9/7/00 ***/
        if (Link[index].Type >  PUMP
            &&  Link[index].Type != GPV) *k = MISSING;

        /* Reset link flow if it was originally open */
        if (*s > CLOSED) {
            if(Link[index].Type == PUMP) pumpswitch(index, CLOSED);
            initlinkflow(index, CLOSED, *k);
        }
        *s = CLOSED;
    }
}


void  setlinksetting(int index, float value, char *status, float *k)
/*----------------------------------------------------------------
**  Input:   index  = link index
**           value  = pump speed or valve setting
**           s      = pointer to link status
**           k      = pointer to link setting
**  Output:  none
**  Purpose: sets pump speed or valve setting, adjusting link
**           status and flow when necessary
**----------------------------------------------------------------
*/
{
    /* For a pump, status is OPEN if speed > 0, CLOSED otherwise */
    if (Link[index].Type == PUMP)
    {
        *k = value;
        if (value > 0 && *status <= CLOSED)
        {
            *status = OPEN;
            pumpswitch (index, OPEN);
            initlinkflow(index, OPEN, value);
        }
        else if (value == 0 && *status > CLOSED)
        {
            *status = CLOSED;
            pumpswitch(index, CLOSED);
            initlinkflow(index, CLOSED, value);
        }
    }

/***  Updated 9/7/00  ***/
    /* For FCV, activate it */
    else if (Link[index].Type == FCV)
    {
        if (*status <= CLOSED) initlinkflow(index, OPEN, value);
        *k = value;
        *status = ACTIVE;
    }

    /* Open closed control valve with fixed status (setting = MISSING) */
    else
    {
        if (*k == MISSING && *status <= CLOSED)
        {
            initlinkflow(index, OPEN, value);
            *status = OPEN;
        }
        *k = value;
    }
}


void  resistance(int k)
/*
**--------------------------------------------------------------------
**  Input:   k = link index
**  Output:  none
**  Purpose: computes link flow resistance
**--------------------------------------------------------------------
*/
{
    float e,d,L;
    Link[k].R = CSMALL;
    if (Link[k].Type == PIPE || Link[k].Type == CV)
        switch (Link[k].Type)
        {

            /* Link is a pipe. Compute resistance based on headloss formula. */
            /* Friction factor for D-W formula gets included during solution */
            /* process in pipecoeff() function.                              */
        case CV:
        case PIPE:
            e = Link[k].Kc;                 /* Roughness coeff. */
            d = Link[k].Diam;               /* Diameter */
            L = Link[k].Len;                /* Length */
            switch(Formflag)
            {
            case HW: Link[k].R = 4.727*L/pow(e,Hexp)/pow(d,4.871);
                break;
            case DW: Link[k].R = L/2.0/32.2/d/SQR(PI*SQR(d)/4.0);
                break;
            case CM: Link[k].R = SQR(4.0*e/(1.49*PI*d*d))*
                         pow((d/4.0),-1.333)*L;
            }
            break;

            /* Link is a pump. Use negligible resistance. */
        case PUMP:
            Link[k].R = CSMALL;
            break;

            /* Link is a valve. Compute resistance for open valve assuming  */
            /* length is 2*diameter and friction factor is 0.02. Use with   */
            /* other formulas as well since resistance should be negligible.*/
        default:
            d = Link[k].Diam;
            L = 2.0*d;
            Link[k].R = 0.02*L/2.0/32.2/d/SQR(PI*SQR(d)/4.0);
            break;
        }
}


void  demands()
/*
**--------------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: computes demands at nodes during current time period
**--------------------------------------------------------------------
*/
{
    int i,j,n;
    long k,p;
    float djunc, sum;
    Pdemand demand;

    /* Determine total elapsed number of pattern periods */
    p = (Htime+Pstart)/Pstep;

    /* Update demand at each node according to its assigned pattern */
    Dsystem = 0.0;          /* System-wide demand */
    for (i=1; i<=Njuncs; i++)
    {
        sum = 0.0;
        for (demand = Node[i].D; demand != NULL; demand = demand->next)
        {
            /*
              pattern period (k) = (elapsed periods) modulus
              (periods per pattern)
            */
            j = demand->Pat;
            k = p % (long) Pattern[j].Length;
            djunc = (demand->Base)*Pattern[j].F[k]*Dmult;
            if (djunc > 0.0) Dsystem += djunc;
            sum += djunc;
        }
        D[i] = sum;
    }

    /* Update head at fixed grade nodes with time patterns. */
    for (n=1; n<=Ntanks; n++)
    {
        if (Tank[n].A == 0.0)
        {
            j = Tank[n].Pat;
            if (j > 0)
            {
                k = p % (long) Pattern[j].Length;
                i = Tank[n].Node;
                H[i] = Node[i].El*Pattern[j].F[k];
            }
        }
    }

    /* Update status of pumps with utilization patterns */
    for (n=1; n<=Npumps; n++)
    {
        j = Pump[n].Upat;
        if (j > 0)
        {
            i = Pump[n].Link;
            k = p % (long) Pattern[j].Length;
            setlinksetting(i, Pattern[j].F[k], &S[i], &K[i]);
        }
    }
}                        /* End of demands */


int  controls()
/*
**---------------------------------------------------------------------
**  Input:   none
**  Output:  number of links whose setting changes
**  Purpose: implements simple controls based on time or tank levels
**---------------------------------------------------------------------
*/
{
    int   i, k, n, reset, setsum;
    float h, vplus;
    float v1, v2;
    float k1, k2;
    char  s1, s2;

    /* Examine each control statement */
    setsum = 0;
    for (i=1; i<=Ncontrols; i++)
    {
        /* Make sure that link is defined */
        reset = 0;
        if ( (k = Control[i].Link) <= 0) continue;

        /* Link is controlled by tank level */
        if ((n = Control[i].Node) > 0 && n > Njuncs)
        {
            h = H[n];
            vplus = ABS(D[n]);
            v1 = tankvolume(n-Njuncs,h);
            v2 = tankvolume(n-Njuncs,Control[i].Grade);
            if (Control[i].Type == LOWLEVEL && v1 <= v2 + vplus)
                reset = 1;
            if (Control[i].Type == HILEVEL && v1 >= v2 - vplus)
                reset = 1;
        }

        /* Link is time-controlled */
        if (Control[i].Type == TIMER)
        {
            if (Control[i].Time == Htime) reset = 1;
        }

        /* Link is time-of-day controlled */
        if (Control[i].Type == TIMEOFDAY)
        {
            if ((Htime + Tstart) % SECperDAY == Control[i].Time) reset = 1;
        }

        /* Update link status & pump speed or valve setting */
        if (reset == 1)
        {
            if (S[k] <= CLOSED) s1 = CLOSED;
            else                s1 = OPEN;
            s2 = Control[i].Status;
            k1 = K[k];
            k2 = k1;
            if (Link[k].Type > PIPE) k2 = Control[i].Setting;
            if (s1 != s2 || k1 != k2)
            {
                S[k] = s2;
                K[k] = k2;
                if (Statflag) writecontrolaction(k,i);
                if (s1 != s2) {
                    if (Link[k].Type == PUMP) pumpswitch (k, s2);
                    initlinkflow(k, S[k], K[k]);
                }
                setsum++;
            }
        }
    }
    return(setsum);
}                        /* End of controls */


long  timestep(void)
/*
**----------------------------------------------------------------
**  Input:   none
**  Output:  returns time step until next change in hydraulics
**  Purpose: computes time step to advance hydraulic simulation
**----------------------------------------------------------------
*/
{
    long   n,t,tstep;

    /* Normal time step is hydraulic time step */
    tstep = Hstep;

    /* Revise time step based on time until next demand period */
    n = ((Htime+Pstart)/Pstep) + 1;   /* Next pattern period   */
    t = n*Pstep - Htime;              /* Time till next period */
    if (t > 0 && t < tstep) tstep = t;

    /* Revise time step based on time until next reporting period */
    t = Rtime - Htime;
    if (t > 0 && t < tstep) tstep = t;

    /* Revise time step based on smallest time to fill or drain a tank */
    tanktimestep(&tstep);

    /* Revise time step based on smallest time to activate a control */
    controltimestep(&tstep);

    /* Evaluate rule-based controls (which will also update tank levels) */
    if (Nrules > 0) ruletimestep(&tstep);
    else tanklevels(tstep);
    return(tstep);
}


void  tanktimestep(long *tstep)
/*
**-----------------------------------------------------------------
**  Input:   *tstep = current time step
**  Output:  *tstep = modified current time step
**  Purpose: revises time step based on shortest time to fill or
**           drain a tank
**-----------------------------------------------------------------
*/
{
    int    i,n;
    float  h,q,v;
    long   t;

    /* (D[n] is net flow rate into (+) or out of (-) tank at node n) */
    for (i=1; i<=Ntanks; i++)
    {
        if (Tank[i].A == 0.0) continue;           /* Skip reservoirs     */
        n = Tank[i].Node;
        h = H[n];                                 /* Current tank grade  */
        q = D[n];                                 /* Flow into tank      */
        if (ABS(q) <= QZERO) continue;
        if (q > 0.0 && h < Tank[i].Hmax)
        {
            v = Tank[i].Vmax - Tank[i].V;          /* Volume to fill      */
        }
        else if (q < 0.0 && h > Tank[i].Hmin)
        {
            v = Tank[i].Vmin - Tank[i].V;          /* Volume to drain (-) */
        }
        else continue;
        t = (long)ROUND(v/q);                     /* Time to fill/drain  */
        if (t > 0 && t < *tstep) *tstep = t;
    }
}


void  controltimestep(long *tstep)
/*
**------------------------------------------------------------------
**  Input:   *tstep = current time step
**  Output:  *tstep = modified current time step
**  Purpose: revises time step based on shortest time to activate
**           a simple control
**------------------------------------------------------------------
*/
{
    int   i,j,k,n;
    float h,q,v;
    long  t,t1,t2;

    for (i=1; i<=Ncontrols; i++)
    {
        if(Control[i].Link <= 0) continue; // This control is removed
                                           // so I do not look at it
        t = 0;
        if ( (n = Control[i].Node) > 0)           /* Node control:       */
        {
            if ((j = n-Njuncs) <= 0) continue;     /* Node is a tank      */
            h = H[n];                              /* Current tank grade  */
            q = D[n];                              /* Flow into tank      */
            if (ABS(q) <= QZERO) continue;
            if
                ( (h < Control[i].Grade &&
                   Control[i].Type == HILEVEL &&       /* Tank below hi level */
                   q > 0.0)                            /* & is filling        */
                  || (h > Control[i].Grade &&
                      Control[i].Type == LOWLEVEL &&     /* Tank above low level */
                      q < 0.0)                           /* & is emptying        */
                    )
            {                                      /* Time to reach level  */
                v = tankvolume(j,Control[i].Grade)-Tank[j].V;
                t = (long)ROUND(v/q);
            }
        }

        if (Control[i].Type == TIMER)             /* Time control:        */
        {
            if (Control[i].Time > Htime)
                t = Control[i].Time - Htime;
        }

        if (Control[i].Type == TIMEOFDAY)         /* Time-of-day control: */
        {
            t1 = (Htime + Tstart) % SECperDAY;
            t2 = Control[i].Time;
            if (t2 >= t1) t = t2 - t1;
            else t = SECperDAY - t1 + t2;
        }

        if (t > 0 && t < *tstep)               /* Revise time step     */
        {
            /* Check if rule actually changes link status or setting */
            k = Control[i].Link;
            if (
                (Link[k].Type > PIPE && K[k] != Control[i].Setting) ||
                (S[k] != Control[i].Status)
                )
                *tstep = t;
        }
    }
}                        /* End of timestep */


void  ruletimestep(long *tstep)
/*
**--------------------------------------------------------------
**  Input:   *tstep = current time step (sec)
**  Output:  *tstep = modified time step
**  Purpose: updates next time step by checking if any rules
**           will fire before then; also updates tank levels.
**--------------------------------------------------------------
*/
{
    long tnow,      /* Start of time interval for rule evaluation */
        tmax,      /* End of time interval for rule evaluation   */
        dt,        /* Normal time increment for rule evaluation  */
        dt1;       /* Actual time increment for rule evaluation  */

    /* Find interval of time for rule evaluation */
    tnow = Htime;
    tmax = tnow + *tstep;

    /* If no rules, then time increment equals current time step */
    if (Nrules == 0)
    {
        dt = *tstep;
        dt1 = dt;
    }

    /* Otherwise, time increment equals rule evaluation time step and */
    /* first actual increment equals time until next even multiple of */
    /* Rulestep occurs. */
    else
    {
        dt = Rulestep;
        dt1 = Rulestep - (tnow % Rulestep);
    }

    /* Make sure time increment is no larger than current time step */
    dt = MIN(dt, *tstep);
    dt1 = MIN(dt1, *tstep);
    if (dt1 == 0) dt1 = dt;

    /* Step through time, updating tank levels, until either  */
    /* a rule fires or we reach the end of evaluation period. */
    /*
    ** Note: we are updating the global simulation time (Htime)
    **       here because it is used by functions in RULES.C
    **       to evaluate rules when checkrules() is called.
    **       It is restored to its original value after the
    **       rule evaluation process is completed (see below).
    **       Also note that dt1 will equal dt after the first
    **       time increment is taken.
    */
    do
    {
        Htime += dt1;               /* Update simulation clock */
        tanklevels(dt1);            /* Find new tank levels    */
        if (checkrules(dt1)) break; /* Stop if rules fire      */
        dt = MIN(dt, tmax - Htime); /* Update time increment   */
        dt1 = dt;                   /* Update actual increment */
    }  while (dt > 0);             /* Stop if no time left    */

    /* Compute an updated simulation time step (*tstep) */
    /* and return simulation time to its original value */
    *tstep = Htime - tnow;
    Htime = tnow;
}


void  addenergy(long hstep)
/*
**-------------------------------------------------------------
**  Input:   hstep = time step (sec)
**  Output:  none
**  Purpose: accumulates pump energy usage
**-------------------------------------------------------------
*/
{
    int   i,j,k;
    long  m,n;
    float c0,c,             /* Energy cost (cost/kwh) */
        f0,               /* Energy cost factor */
        dt,               /* Time interval (hr) */
        e,                /* Pump efficiency (fraction) */
        q,                /* Pump flow (cfs) */
        p,                /* Pump energy (kw) */
        psum = 0.0;       /* Total energy (kw) */

    /* Determine current time interval in hours */
    if      (Dur == 0)    dt = 1.0;
    else if (Htime < Dur) dt = (float) hstep / 3600.0;
    else                  dt = 0.0;
    if      (dt == 0.0)   return;
    n = (Htime+Pstart)/Pstep;

    /* Compute default energy cost at current time */
    c0 = Ecost;
    f0 = 1.0;
    if (Epat > 0)
    {
        m = n % (long)Pattern[Epat].Length;
        f0 = Pattern[Epat].F[m];
    }

    /* Examine each pump */
    for (j=1; j<=Npumps; j++)
    {
        /* Skip closed pumps */
        k = Pump[j].Link;
        if (S[k] <= CLOSED) continue;
        q = MAX(QZERO, fabs(Q[k]));

        /* Find pump-specific energy cost */
        if (Pump[j].Ecost > 0.0) c = Pump[j].Ecost;
        else c = c0;

        if ( (i = Pump[j].Epat) > 0)
        {
            m = n % (long)Pattern[i].Length;
            c *= Pattern[i].F[m];
        }
        else c *= f0;

        /* Find pump energy & efficiency */
        getenergy(k,&p,&e);
        psum += p;

        /* Update pump's cumulative statistics */
        Pump[j].Energy[0] += dt;            /* Time on-line */
        Pump[j].Energy[1] += e*dt;          /* Effic.-hrs   */
        Pump[j].Energy[2] += p/q*dt;        /* kw/cfs-hrs   */
        Pump[j].Energy[3] += p*dt;          /* kw-hrs       */
        Pump[j].Energy[4] = MAX(Pump[j].Energy[4],p);
        Pump[j].Energy[5] += c*p*dt;        /* cost-hrs.    */
    }

    /* Update maximum kw value */
    Emax = MAX(Emax,psum);
}                       /* End of pumpenergy */


void  getenergy(int k, float *kw, float *eff)
/*
**----------------------------------------------------------------
**  Input:   k    = link index
**  Output:  *kw  = kwatt energy used
**           *eff = efficiency (pumps only)
**  Purpose: computes flow energy associated with link k
**----------------------------------------------------------------
*/
{
    int   i,j;
    float dh, q, e;

/*** Updated 6/24/02 ***/
    /* No energy if link is closed */
    if (S[k] <= CLOSED)
    {
        *kw = 0.0;
        *eff = 0.0;
        return;
    }
/*** End of update ***/

    /* Determine flow and head difference */
    q = fabs(Q[k]);
    dh = ABS(H[Link[k].N1] - H[Link[k].N2]);

    /* For pumps, find effic. at current flow */
    if (Link[k].Type == PUMP)
    {
        j = PUMPINDEX(k);
        e = Epump;
        if ( (i = Pump[j].Ecurve) > 0)
            e = interp(Curve[i].Npts,Curve[i].X,Curve[i].Y,q*Ucf[FLOW]);
        e = MIN(e, 100.0);
        e = MAX(e, 1.0);
        e /= 100.0;
    }
    else e = 1.0;

    /* Compute energy */
    *kw = dh*q*SpGrav/8.814/e*KWperHP;
    *eff = e;
}


void  tanklevels(long tstep)
/*
**----------------------------------------------------------------
**  Input:   tstep = current time step
**  Output:  none
**  Purpose: computes new water levels in tanks after current
**           time step
**----------------------------------------------------------------
*/
{
    int   i,n;
    float dv;

    for (i=1; i<=Ntanks; i++)
    {

        /* Skip reservoirs */
        if (Tank[i].A == 0.0) continue;

        /* Update the tank's volume & water elevation */
        n = Tank[i].Node;
        dv = D[n]*tstep;
        Tank[i].V += dv;

        /*** Updated 6/24/02 ***/
        /* Check if tank full/empty within next second */
        if (Tank[i].V + D[n] >= Tank[i].Vmax) Tank[i].V = Tank[i].Vmax;
        if (Tank[i].V - D[n] <= Tank[i].Vmin) Tank[i].V = Tank[i].Vmin;

        H[n] = tankgrade(i,Tank[i].V);
    }
}                       /* End of tanklevels */


float  tankvolume(int i, float h)
/*
**--------------------------------------------------------------------
**  Input:   i = tank index
**           h = water elevation in tank
**  Output:  returns water volume in tank
**  Purpose: finds water volume in tank i corresponding to elev. h.
**--------------------------------------------------------------------
*/
{
    int j;

    /* Use level*area if no volume curve */
    j = Tank[i].Vcurve;
    if (j == 0) return(Tank[i].Vmin + (h - Tank[i].Hmin)*Tank[i].A);

    /* If curve exists, interpolate on h to find volume v */
    /* remembering that volume curve is in original units.*/
    else return(interp(Curve[j].Npts, Curve[j].X, Curve[j].Y,
                       (h-Node[Tank[i].Node].El)*Ucf[HEAD])/Ucf[VOLUME]);

}                       /* End of tankvolume */


float  tankgrade(int i, float v)
/*
**-------------------------------------------------------------------
**  Input:   i = tank index
**           v = volume in tank
**  Output:  returns water level in tank
**  Purpose: finds water level in tank i corresponding to volume v.
**-------------------------------------------------------------------
*/
{
    int j;

    /* Use area if no volume curve */
    j = Tank[i].Vcurve;
    if (j == 0) return(Tank[i].Hmin + (v - Tank[i].Vmin)/Tank[i].A);

    /* If curve exists, interpolate on volume (originally the Y-variable */
    /* but used here as the X-variable) to find new level above bottom.  */
    /* Remember that volume curve is stored in original units.           */
    else return(Node[Tank[i].Node].El +
                interp(Curve[j].Npts, Curve[j].Y, Curve[j].X, v*Ucf[VOLUME])/Ucf[HEAD]);

}                        /* End of tankgrade */


int  netsolve(int *iter, float *relerr)
/*
**-------------------------------------------------------------------
**  Input:   none
**  Output:  *iter   = # of iterations to reach solution
**           *relerr = convergence error in solution
**           returns error code
**  Purpose: solves network nodal equations for heads and flows
**           using Todini's Gradient algorithm
**
*** Updated 9/7/00 ***
**  Notes:   Status checks on PRVs and PSVs made every iteration.
**           Checks on other valves, pumps, and pipes to tanks
**           are made every CheckFreq iteration, up until
**           MaxCheck iterations are reached. After this, a
**           status check is made when convergence is achieved.
**           If convergence acheived but status changes, then
**           at least two more iterations are required before
**           solution is reached, even if system converges.
**           If convergence not achieved in MaxIter and ExtraIter
**           > 0 then another ExtraIter trials are made with no
**           status changes made to any links.
**
**   This procedure calls linsolve() which appears in SMATRIX.C.
**-------------------------------------------------------------------
*/
{
    int   i;                     /* Node index */
    int   errcode;               /* Node causing solution error */
    int   nextcheck;             /* Next status check trial */
    int   concheck = 1;          /* Convergence checking flag */
    int   maxtrials;             /* Max. trials for convergence */
    REAL  newerr;                /* New convergence error */

/*** Updated 9/7/00 ***/
    int   valvechange;           /* Valve status change flag */

/*** Updated 9/7/00 ***/
    nextcheck = CheckFreq;

    /* Repeat iterations until convergence or trial limit is exceeded. */
    /* (ExtraIter used to increase trials in case of status cycling.)  */
    if (Statflag == FULL) writerelerr(0,0);
    maxtrials = MaxIter;
    if (ExtraIter > 0) maxtrials += ExtraIter;
    *iter = 1;
    do {
        /*
        ** Compute coefficient matrices A & F and solve A*H = F
        ** where H = heads, A = Jacobian coeffs. derived from
        ** head loss gradients, & F = flow correction terms.
        ** Solution for H is returned in F from call to linsolve().
        */
        newcoeffs();
        errcode = linsolve(Njuncs,Aii,Aij,F);

        /* Take action depending on error code */
        if (errcode < 0) break;    /* Memory allocation problem */
        if (errcode > 0)           /* Ill-conditioning problem */
        {
            /* If control valve causing problem, fix its status & continue, */
            /* otherwise end the iterations with no solution.               */
            if (badvalve(Order[errcode])) continue;
            else break;
        }

        /* Update current solution. */
        /* (Row[i] = row of solution matrix corresponding to node i). */
        for (i=1; i<=Njuncs; i++) H[i] = F[Row[i]];   /* Update heads */
        newerr = newflows();                          /* Update flows */
        *relerr = newerr;

        /* Write convergence error to status report if called for */
        if (Statflag == FULL) writerelerr(*iter,*relerr);

/*** Updated 9/7/00 ***/
        /* Check if a PRV or PSV changes status */
        valvechange = valvestatus();

        /* Check for convergence (concheck equals 0 if previous iteration */
        /* converged but there was a status change somewhere).            */
        if (concheck == 0) concheck = 1;
        else if (*relerr <= Hacc)
        {
            /* We have convergence. Quit if we are into extra iterations. */
            if (*iter > MaxIter) break;

/*** Updated 9/7/00 ***/
            /* Quit if no status changes occur. */
            if (!valvechange
                &&  !linkstatus()
                &&  !pswitch()) break;

            /* We have a status change so force another iteration. */
            concheck = 0;
            nextcheck = *iter + CheckFreq;
        }

        /* See if its time for a periodic status check; update iteration count. */
        if (*iter <= MaxCheck && *iter == nextcheck)
        {

/*** Updated 9/7/00 **/
            /*valvestatus();*/

            linkstatus();
            nextcheck += CheckFreq;
        }
        (*iter)++;
    } while (*iter <= maxtrials);

    /* Iterations ended. Report any errors. */
    if (errcode < 0) errcode = 101;      /* Memory allocation error */
    else if (errcode > 0)
    {
        writehyderr(Order[errcode]);      /* Ill-conditioned eqns. error */
        errcode = 110;
    }

    /* Add any emitter flows to junction demands */
    for (i=1; i<=Njuncs; i++) D[i] += E[i];
    return(errcode);
}                        /* End of netsolve */


int  badvalve(int n)
/*
**-----------------------------------------------------------------
**  Input:   n = node index
**  Output:  returns 1 if node n belongs to an active control valve,
**           0 otherwise
**  Purpose: determines if a node belongs to an active control valve
**           whose setting causes an inconsistent set of eqns. If so,
**           the valve status is fixed open and a warning condition
**           is generated.
**-----------------------------------------------------------------
*/
{
    int i,k,n1,n2;
    for (i=1; i<=Nvalves; i++)
    {
        k = Valve[i].Link;
        n1 = Link[k].N1;
        n2 = Link[k].N2;
        if (n == n1 || n == n2)
        {
            if (Link[k].Type == PRV ||
                Link[k].Type == PSV ||
                Link[k].Type == FCV)
            {
                if (S[k] == ACTIVE)
                {
                    if (Statflag == FULL)
                    {
                        sprintf(Msg,FMT61,clocktime(Atime,Htime),Link[k].ID);
                        writeline(Msg);
                    }
                    if (Link[k].Type == FCV) S[k] = XFCV;
                    else                     S[k] = XPRESSURE;
                    return(1);
                }
            }
            return(0);
        }
    }
    return(0);
}


int  valvestatus()
/*
**-----------------------------------------------------------------
**  Input:   none
**  Output:  returns 1 if any pressure control valve changes status,
**           0 otherwise
**  Purpose: updates status for PRVs & PSVs whose status
**           is not fixed to OPEN/CLOSED
**-----------------------------------------------------------------
*/
{
    int   change = FALSE,            /* Status change flag      */
        i,k,                       /* Valve & link indexes    */
        n1,n2;                     /* Start & end nodes       */
    char  s;                         /* Valve status settings   */
    float hset;                      /* Valve head setting      */

    for (i=1; i<=Nvalves; i++)                   /* Examine each valve   */
    {
        k = Valve[i].Link;                        /* Link index of valve  */
        if (K[k] == MISSING) continue;            /* Valve status fixed   */
        n1 = Link[k].N1;                          /* Start & end nodes    */
        n2 = Link[k].N2;
        s  = S[k];                                /* Save current status  */
        if (s != CLOSED                           /* No change if flow is */
            && ABS(Q[k]) < Qtol) continue;            /* negligible.          */
        switch (Link[k].Type)                     /* Evaluate new status: */
        {
        case PRV:  hset = Node[n2].El + K[k];
            S[k] = prvstatus(k,s,hset,H[n1],H[n2]);
            break;
        case PSV:  hset = Node[n1].El + K[k];
            S[k] = psvstatus(k,s,hset,H[n1],H[n2]);
            break;
        default:   continue;
        }

/*** Updated 9/7/00 ***/
        /* Do not reset flow in valve if its status changes. */
        /* This strategy improves convergence. */

        /* Check for status change */
        if (s != S[k])
        {
            if (Statflag == FULL) writestatchange(k,s,S[k]);
            change = TRUE;
        }
    }
    return(change);
}                       /* End of valvestatus() */


/*** Updated 9/7/00 ***/
int  linkstatus()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  returns 1 if any link changes status, 0 otherwise
**  Purpose: determines new status for pumps, CVs, FCVs & pipes
**           to tanks.
**--------------------------------------------------------------
*/
{
    int   change = FALSE,             /* Status change flag      */
        k,                          /* Link index              */
        n1,                         /* Start node index        */
        n2;                         /* End node index          */
    float dh;                         /* Head difference         */
    char  status;                     /* Current status          */

    /* Examine each link */
    for (k=1; k<=Nlinks; k++)
    {
        n1 = Link[k].N1;
        n2 = Link[k].N2;
        dh = H[n1] - H[n2];

        /* Re-open temporarily closed links (status = XHEAD or TEMPCLOSED) */
        status = S[k];
        if (status == XHEAD || status == TEMPCLOSED) S[k] = OPEN;

        /* Check for status changes in CVs and pumps */
        if (Link[k].Type == CV) S[k] = cvstatus(S[k],dh,Q[k]);
        if (Link[k].Type == PUMP && S[k] >= OPEN)
            S[k] = pumpstatus(k,-dh);

        /* Check for status changes in non-fixed FCVs */
        if (Link[k].Type == FCV && K[k] != MISSING)
            S[k] = fcvstatus(k,status,H[n1],H[n2]);

        /* Check for flow into (out of) full (empty) tanks */
        if (n1 > Njuncs || n2 > Njuncs) tankstatus(k,n1,n2);

        /* Note change in link status and revise link flow */
        if (status != S[k])
        {
            change = TRUE;
            if (Statflag == FULL) writestatchange(k,status,S[k]);
            if (S[k] <= CLOSED) Q[k] = QZERO;
            else setlinkflow(k, dh);
        }
    }
    return(change);
}                        /* End of linkstatus */


char  cvstatus(char s, float dh, float q)
/*
**--------------------------------------------------
**  Input:   s  = current status
**           dh = headloss
**           q  = flow
**  Output:  returns new link status
**  Purpose: updates status of a check valve.
**--------------------------------------------------
*/
{
    /* Prevent reverse flow through CVs */
    if (ABS(dh) > Htol)
    {
        if (dh < -Htol)     return(CLOSED);
        else if (q < -Qtol) return(CLOSED);
        else                return(OPEN);
    }
    else
    {
        if (q < -Qtol) return(CLOSED);
        else           return(s);
    }
}


char  pumpstatus(int k, float dh)
/*
**--------------------------------------------------
**  Input:   k  = link index
**           dh = head gain
**  Output:  returns new pump status
**  Purpose: updates status of a pump.
**--------------------------------------------------
*/
{
    int   p;
    float hmax;

    /* Prevent reverse flow through pump */
    p = PUMPINDEX(k);
    if (Pump[p].Ptype == CONST_HP) hmax = BIG;
    else hmax = SQR(K[k])*Pump[p].Hmax;
    if (dh > hmax + Htol) return(XHEAD);

    /* Check if pump cannot deliver flow */
    if (Q[k] > K[k]*Pump[p].Qmax + Qtol) return(XFLOW);
    return(OPEN);
}


char  prvstatus(int k, char s, float hset, float h1, float h2)
/*
**-----------------------------------------------------------
**  Input:   k    = link index
**           s    = current status
**           hset = valve head setting
**           h1   = head at upstream node
**           h2   = head at downstream node
**  Output:  returns new valve status
**  Purpose: updates status of a pressure reducing valve.
**-----------------------------------------------------------
*/
{
    char  status;     /* New valve status */
    float hml;        /* Minor headloss   */

    status = s;
    if (K[k] == MISSING) return(status);       /* Status fixed by user */
    hml = Link[k].Km*SQR(Q[k]);                /* Head loss when open  */
    switch (s)
    {
    case ACTIVE:
        if (Q[k] < -Qtol)            status = CLOSED;
        else if (h1 < hset+hml-Htol) status = OPEN;
        else                         status = ACTIVE;
        break;
    case OPEN:
        if (Q[k] < -Qtol)            status = CLOSED;
        else if (h1 > hset+hml+Htol) status = ACTIVE;
        else                         status = OPEN;
        break;
    case CLOSED:
        if (h1 > h2+Htol
            &&  h1 < hset-Htol)          status = OPEN;
        else if (h1 > h2+Htol
                 &&  h2 < hset-Htol)     status = ACTIVE;
        else                         status = CLOSED;
        break;
    case XPRESSURE:
        if (Q[k] < -Qtol)            status = CLOSED;
        break;
    }
    return(status);
}


char  psvstatus(int k, char s, float hset, float h1, float h2)
/*
**-----------------------------------------------------------
**  Input:   k    = link index
**           s    = current status
**           hset = valve head setting
**           h1   = head at upstream node
**           h2   = head at downstream node
**  Output:  returns new valve status
**  Purpose: updates status of a pressure sustaining valve.
**-----------------------------------------------------------
*/
{
    char  status;       /* New valve status */
    float hml;          /* Minor headloss   */

    status = s;
    if (K[k] == MISSING) return(status);       /* Status fixed by user */
    hml = Link[k].Km*SQR(Q[k]);                /* Head loss when open  */
    switch (s)
    {
    case ACTIVE:
        if (Q[k] < -Qtol)            status = CLOSED;
        else if (h2+hml > hset+Htol) status = OPEN;
        else                         status = ACTIVE;
        break;
    case OPEN:
        if (Q[k] < -Qtol)            status = CLOSED;
        else if (h2+hml < hset-Htol) status = ACTIVE;
        else                         status = OPEN;
        break;
    case CLOSED:
        if (h1 > h2+Htol
            &&  h2 > hset+Htol)          status = OPEN;
        else if (h1 > h2+Htol
                 &&  h1 > hset+Htol)          status = ACTIVE;
        else                         status = CLOSED;
        break;
    case XPRESSURE:
        if (Q[k] < -Qtol)            status = CLOSED;
        break;
    }
    return(status);
}


char  fcvstatus(int k, char s, float h1, float h2)
/*
**-----------------------------------------------------------
**  Input:   k    = link index
**           s    = current status
**           h1   = head at upstream node
**           h2   = head at downstream node
**  Output:  returns new valve status
**  Purpose: updates status of a flow control valve.
**
**    Valve status changes to XFCV if flow reversal.
**    If current status is XFCV and current flow is
**    above setting, then valve becomes active.
**    If current status is XFCV, and current flow
**    positive but still below valve setting, then
**    status remains same.
**-----------------------------------------------------------
*/
{
    char  status;        /* New valve status */
    status = s;
    if (h1 - h2 < -Htol)                status = XFCV;
    else if (s == XFCV && Q[k] >= K[k]) status = ACTIVE;
    return(status);
}


/*** Updated 9/7/00 ***/
/*** Updated 11/19/01 ***/
void  tankstatus(int k, int n1, int n2)
/*
**----------------------------------------------------------------
**  Input:   k  = link index
**           n1 = start node of link
**           n2 = end node of link
**  Output:  none
**  Purpose: closes link flowing into full or out of empty tank
**----------------------------------------------------------------
*/
{
    int   i,n;
    float h,q;

    /* Make node n1 be the tank */
    q = Q[k];
    i = n1 - Njuncs;
    if (i <= 0)
    {
        i = n2 - Njuncs;
        if (i <= 0) return;
        n = n1;
        n1 = n2;
        n2 = n;
        q = -q;
    }
    h = H[n1] - H[n2];

    /* Skip reservoirs & closed links */
    if (Tank[i].A == 0.0 || S[k] <= CLOSED) return;

    /* If tank full, then prevent flow into it */
    if (H[n1] >= Tank[i].Hmax - Htol)
    {

        /* Case 1: Link is a pump discharging into tank */
        if ( Link[k].Type == PUMP )
        {
            if (Link[k].N2 == n1) S[k] = TEMPCLOSED;
        }

        /* Case 2: Downstream head > tank head */
        /* (i.e., an open outflow check valve would close) */
        else if (cvstatus(OPEN, h, q) == CLOSED) S[k] = TEMPCLOSED;
    }

    /* If tank empty, then prevent flow out of it */
    if (H[n1] <= Tank[i].Hmin + Htol)
    {

        /* Case 1: Link is a pump discharging from tank */
        if ( Link[k].Type == PUMP)
        {
            if (Link[k].N1 == n1) S[k] = TEMPCLOSED;
        }

        /* Case 2: Tank head > downstream head */
        /* (i.e., a closed outflow check valve would open) */
        else if (cvstatus(CLOSED, h, q) == OPEN) S[k] = TEMPCLOSED;
    }
}                        /* End of tankstatus */


int  pswitch()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  returns 1 if status of any link changes, 0 if not
**  Purpose: adjusts settings of links controlled by junction
**           pressures after a hydraulic solution is found
**--------------------------------------------------------------
*/
{
    int   i,                 /* Control statement index */
        k,                 /* Link being controlled */
        n,                 /* Node controlling link */
        reset,             /* Flag on control conditions */
        change,            /* Flag for status or setting change */
        anychange = 0;     /* Flag for 1 or more changes */
    char  s;                 /* Current link status */

    /* Check each control statement */
    for (i=1; i<=Ncontrols; i++)
    {
        reset = 0;
        if ( (k = Control[i].Link) <= 0) continue;

        /* Determine if control based on a junction, not a tank */
        if ( (n = Control[i].Node) > 0 && n <= Njuncs)
        {
            /* Determine if control conditions are satisfied */
            if (Control[i].Type == LOWLEVEL
                && H[n] <= Control[i].Grade + Htol )
                reset = 1;
            if (Control[i].Type == HILEVEL
                && H[n] >= Control[i].Grade - Htol )
                reset = 1;
        }

        /* Determine if control forces a status or setting change */
        if (reset == 1)
        {
            change = 0;
            s = S[k];
            if (Link[k].Type == PIPE)
            {
                if (s != Control[i].Status) change = 1;
            }
            if (Link[k].Type == PUMP)
            {
                if (K[k] != Control[i].Setting) change = 1;
            }
            if (Link[k].Type >= PRV)
            {
                if (K[k] != Control[i].Setting) change = 1;
                else if (K[k] == MISSING &&
                         s != Control[i].Status) change = 1;
            }

            /* If a change occurs, update status & setting */
            if (change)
            {
                S[k] = Control[i].Status;
                if (Link[k].Type > PIPE) K[k] = Control[i].Setting;
                if (Statflag == FULL) writestatchange(k,s,S[k]);

                /* Re-set flow if status has changed */
                if (S[k] != s) {
                    if (Link[k].Type == PUMP) pumpswitch(k, S[k]);
                    initlinkflow(k, S[k], K[k]);
                }
                anychange = 1;
            }
        }
    }
    return(anychange);
}                        /* End of pswitch */


REAL newflows()
/*
**----------------------------------------------------------------
**  Input:   none
**  Output:  returns solution convergence error
**  Purpose: updates link flows after new nodal heads computed
**----------------------------------------------------------------
*/
{
    REAL  dh,                    /* Link head loss       */
        dq;                    /* Link flow change     */
    REAL  dqsum,                 /* Network flow change  */
        qsum;                  /* Network total flow   */
    int   k, n, n1, n2;

    /* Initialize net inflows (i.e., demands) at tanks */
    for (n=Njuncs+1; n <= Nnodes; n++) D[n] = 0.0;

    /* Initialize sum of flows & corrections */
    qsum  = 0.0;
    dqsum = 0.0;

    /* Update flows in all links */
    for (k=1; k<=Nlinks; k++)
    {

        /*
        ** Apply flow update formula:
        **   dq = Y - P*(new head loss)
        **    P = 1/(dh/dq)
        **    Y = P*(head loss based on current flow)
        ** where P & Y were computed in newcoeffs().
        */

        n1 = Link[k].N1;
        n2 = Link[k].N2;
        dh = H[n1] - H[n2];
        dq = Y[k] - P[k]*dh;

        /* Prevent flow in constant HP pumps from going negative */
        if (Link[k].Type == PUMP)
        {
            n = PUMPINDEX(k);
            if (Pump[n].Ptype == CONST_HP && dq > Q[k]) dq = Q[k]/2.0;
        }
        Q[k] -= dq;

        /* Update sum of absolute flows & flow corrections */
        qsum += ABS(Q[k]);
        dqsum += ABS(dq);

        /* Update net flows to tanks */
        if (n1 > Njuncs) D[n1] -= Q[k];
        if (n2 > Njuncs) D[n2] += Q[k];

    }

    /* Update emitter flows */
    for (k=1; k<=Njuncs; k++)
    {
        if (Node[k].Ke == 0.0) continue;
        dq = emitflowchange(k);
        E[k] -= dq;
        qsum += ABS(E[k]);
        dqsum += ABS(dq);
    }

    /* Return ratio of total flow corrections to total flow */
    if (qsum > Hacc) return(dqsum/qsum);
    else return(dqsum);

}                        /* End of newflows */


void   newcoeffs()
/*
**--------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: computes coefficients of linearized network eqns.
**--------------------------------------------------------------
*/
{
    memset(Aii,0,(Nnodes+1)*sizeof(REAL));   /* Reset coeffs. to 0 */
    memset(Aij,0,(Ncoeffs+1)*sizeof(REAL));
    memset(F,0,(Nnodes+1)*sizeof(REAL));
    memset(X,0,(Nnodes+1)*sizeof(float));
    memset(P,0,(Nlinks+1)*sizeof(float));
    memset(Y,0,(Nlinks+1)*sizeof(float));
    linkcoeffs();                            /* Compute link coeffs.  */
    emittercoeffs();                         /* Compute emitter coeffs.*/
    nodecoeffs();                            /* Compute node coeffs.  */
    valvecoeffs();                           /* Compute valve coeffs. */
}                        /* End of newcoeffs */


void  linkcoeffs()
/*
**--------------------------------------------------------------
**   Input:   none
**   Output:  none
**   Purpose: computes solution matrix coefficients for links
**--------------------------------------------------------------
*/
{
    int   k,n1,n2;

    /* Examine each link of network */
    for (k=1; k<=Nlinks; k++)
    {
        n1 = Link[k].N1;           /* Start node of link */
        n2 = Link[k].N2;           /* End node of link   */

        /* Compute P[k] = 1 / (dh/dQ) and Y[k] = h * P[k]   */
        /* for each link k (where h = link head loss).      */
        /* FCVs, PRVs, and PSVs with non-fixed status       */
        /* are analyzed later.                              */

        switch (Link[k].Type)
        {
        case CV:
        case PIPE:  pipecoeff(k); break;
        case PUMP:  pumpcoeff(k); break;
        case PBV:   pbvcoeff(k);  break;
        case TCV:   tcvcoeff(k);  break;
        case GPV:   gpvcoeff(k);  break;
        case FCV:
        case PRV:
        case PSV:   /* If valve status fixed then treat as pipe */
            /* otherwise ignore the valve for now. */
            if (K[k] == MISSING) pipecoeff(k);
            else continue;
            break;
        default:    continue;
        }

        /* Update net nodal inflows (X), solution matrix (A) and RHS array (F) */
        /* (Use covention that flow out of node is (-), flow into node is (+)) */
        X[n1] -= Q[k];
        X[n2] += Q[k];
        Aij[Ndx[k]] -= P[k];              /* Off-diagonal coeff. */
        if (n1 <= Njuncs)                 /* Node n1 is junction */
        {
            Aii[Row[n1]] += P[k];          /* Diagonal coeff. */
            F[Row[n1]] += Y[k];            /* RHS coeff.      */
        }
        else F[Row[n2]] += (P[k]*H[n1]);  /* Node n1 is a tank   */
        if (n2 <= Njuncs)                 /* Node n2 is junction */
        {
            Aii[Row[n2]] += P[k];          /* Diagonal coeff. */
            F[Row[n2]] -= Y[k];            /* RHS coeff.      */
        }
        else  F[Row[n1]] += (P[k]*H[n2]); /* Node n2 is a tank   */
    }
}                        /* End of linkcoeffs */


void  nodecoeffs()
/*
**----------------------------------------------------------------
**  Input:   none
**  Output:  none
**  Purpose: completes calculation of nodal flow imbalance (X)
**           & flow correction (F) arrays
**----------------------------------------------------------------
*/
{
    int   i;

    /* For junction nodes, subtract demand flow from net */
    /* flow imbalance & add imbalance to RHS array F.    */
    for (i=1; i<=Njuncs; i++)
    {
        X[i] -= D[i];
        F[Row[i]] += X[i];
    }
}                        /* End of nodecoeffs */


void  valvecoeffs()
/*
**--------------------------------------------------------------
**   Input:   none
**   Output:  none
**   Purpose: computes matrix coeffs. for PRVs, PSVs & FCVs
**            whose status is not fixed to OPEN/CLOSED
**--------------------------------------------------------------
*/
{
    int i,k,n1,n2;

    for (i=1; i<=Nvalves; i++)                   /* Examine each valve   */
    {
        k = Valve[i].Link;                        /* Link index of valve  */
        if (K[k] == MISSING) continue;            /* Valve status fixed   */
        n1 = Link[k].N1;                          /* Start & end nodes    */
        n2 = Link[k].N2;
        switch (Link[k].Type)                     /* Call valve-specific  */
        {                                         /*   function           */
        case PRV:  prvcoeff(k,n1,n2); break;
        case PSV:  psvcoeff(k,n1,n2); break;
        case FCV:  fcvcoeff(k,n1,n2); break;
        default:   continue;
        }
    }
}                        /* End of valvecoeffs */


void  emittercoeffs()
/*
**--------------------------------------------------------------
**   Input:   none
**   Output:  none
**   Purpose: computes matrix coeffs. for emitters
**
**   Note: Emitters consist of a fictitious pipe connected to
**         a fictitious reservoir whose elevation equals that
**         of the junction. The headloss through this pipe is
**         Ke*(Flow)^Qexp, where Ke = emitter headloss coeff.
**--------------------------------------------------------------
*/
{
    int   i;
    REAL  ke;
    REAL  p;
    REAL  q;
    REAL  y;
    REAL  z;
    for (i=1; i<=Njuncs; i++)
    {
        if (Node[i].Ke == 0.0) continue;
        ke = MAX(CSMALL, Node[i].Ke);
        q = E[i];
        z = ke*pow(ABS(q),Qexp);
        p = Qexp*z/ABS(q);
        if (p < RQtol) p = 1.0/RQtol;
        else p = 1.0/p;
        y = SGN(q)*z*p;
        Aii[Row[i]] += p;
        F[Row[i]] += y + p*Node[i].El;
        X[i] -= q;
    }
}


REAL  emitflowchange(int i)
/*
**--------------------------------------------------------------
**   Input:   i = node index
**   Output:  returns change in flow at an emitter node
**   Purpose: computes flow change at an emitter node
**--------------------------------------------------------------
*/
{
    REAL ke, p;
    ke = MAX(CSMALL, Node[i].Ke);
    p = Qexp*ke*pow(ABS(E[i]),(Qexp-1.0));
    if (p < RQtol)
        p = 1/RQtol;
    else
        p = 1.0/p;
    return(E[i]/Qexp - p*(H[i] - Node[i].El));
}


static inline
void  pipecoeff(int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  none
**  Purpose:  computes P & Y coefficients for pipe k
**
**    P = inverse head loss gradient = 1/(dh/dQ)
**    Y = flow correction term = h*P
**--------------------------------------------------------------
*/
{
    REAL  hpipe,     /* Normal head loss          */
        hml,       /* Minor head loss           */
        ml,        /* Minor loss coeff.         */
        p,         /* q*(dh/dq)                 */
        q,         /* Abs. value of flow        */
        r,         /* Resistance coeff.         */
        r1,        /* Total resistance factor   */
        f,         /* D-W friction factor       */
        dfdq;      /* Derivative of fric. fact. */

    /* For closed pipe use headloss formula: h = CBIG*q */
    if (S[k] <= CLOSED)
    {
        P[k] = INVCBIG;
        Y[k] = Q[k];
        return;
    }

    /* Evaluate headloss coefficients */
    q = fabs(Q[k]);                         /* Absolute flow       */
    ml = Link[k].Km;                       /* Minor loss coeff.   */
    r = Link[k].R;                         /* Resistance coeff.   */
    f = 1.0;                               /* D-W friction factor */
    if (Formflag == DW) f = DWcoeff(k,&dfdq);
    r1 = f*r+ml;

    /* Use large P coefficient for small flow resistance product */
    if (r1*q < RQtol)
    {
        P[k] = 1.0/RQtol;
        Y[k] = Q[k]/Hexp;
        return;
    }

    /* Compute P and Y coefficients */
    if (Formflag == DW)                  /* D-W eqn. */
    {
        hpipe = r1*SQR(q);                /* Total head loss */
        p = 2.0*r1*q;                     /* |dh/dQ| */
        /* + dfdq*r*q*q;*/                 /* Ignore df/dQ term */
        p = 1.0/p;
        P[k] = p;
        Y[k] = SGN(Q[k])*hpipe*p;
    }
    else                                 /* H-W or C-M eqn.   */
    {
        hpipe = r*pow(q,Hexp);            /* Friction head loss  */
        p = Hexp*hpipe;                   /* Q*dh(friction)/dQ   */
        if (ml > 0.0)
        {
            hml = ml*q*q;                  /* Minor head loss   */
            p += 2.0*hml;                  /* Q*dh(Total)/dQ    */
        }
        else  hml = 0.0;
        p = Q[k]/p;                       /* 1 / (dh/dQ) */
        P[k] = fabs(p);
        Y[k] = p*(hpipe + hml);
    }
}                        /* End of pipecoeff */


REAL DWcoeff(int k, REAL *dfdq)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  returns Darcy-Weisbach friction factor
**   Purpose: computes Darcy-Weisbach friction factor
**
**    Uses interpolating polynomials developed by
**    E. Dunlop for transition flow from 2000 < Re < 4000.
**
**   df/dq term is ignored as it slows convergence rate.
**--------------------------------------------------------------
*/
{
    REAL q,             /* Abs. value of flow */
        f;             /* Friction factor    */
    REAL x1,x2,x3,x4,
        y1,y2,y3,
        fa,fb,r;
    REAL s,w;

    *dfdq = 0.0;
    if (Link[k].Type > PIPE) return(1.0); /* Only apply to pipes */
    q = ABS(Q[k]);
    s = Viscos*Link[k].Diam;
    w = q/s;                       /* w = Re(Pi/4) */
    if (w >= A1)                   /* Re >= 4000; Colebrook Formula */
    {
        y1 = A8/pow(w,0.9);
        y2 = Link[k].Kc/(3.7*Link[k].Diam) + y1;
        y3 = A9*log(y2);
        f = 1.0/SQR(y3);
        /*  *dfdq = (2.0+AA*y1/(y2*y3))*f; */   /* df/dq */
    }
    else if (w > A2)              /* Re > 2000; Interpolation formula */
    {
        y2 = Link[k].Kc/(3.7*Link[k].Diam) + AB;
        y3 = A9*log(y2);
        fa = 1.0/SQR(y3);
        fb = (2.0+AC/(y2*y3))*fa;
        r = w/A2;
        x1 = 7.0*fa - fb;
        x2 = 0.128 - 17.0*fa + 2.5*fb;
        x3 = -0.128 + 13.0*fa - (fb+fb);
        x4 = r*(0.032 - 3.0*fa + 0.5*fb);
        f = x1 + r*(x2 + r*(x3+x4));
        /*  *dfdq = (x1 + x1 + r*(3.0*x2 + r*(4.0*x3 + 5.0*x4)));  */
    }
    else if (w > A4)              /* Laminar flow: Hagen-Poiseuille Formula */
    {
        f = A3*s/q;
        /*  *dfdq = A3*s; */
    }
    else
    {
        f = 8.0;
        *dfdq = 0.0;
    }
    return(f);
}                        /* End of DWcoeff */


/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void  pumpcoeff(int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  none
**   Purpose: computes P & Y coeffs. for pump in link k
**--------------------------------------------------------------
*/
{
    int   p;         /* Pump index             */
    float h0,        /* Shutoff head           */
        q,         /* Abs. value of flow     */
        r,         /* Flow resistance coeff. */
        n;         /* Flow exponent coeff.   */

    /* Use high resistance pipe if pump closed or cannot deliver head */
    if (S[k] <= CLOSED || K[k] == 0.0)
    {
        pipecoeff(k);
        return;
    }

    q = fabs(Q[k]);
    q = MAX(q,TINY);
    p = PUMPINDEX(k);

    /* Get pump curve coefficients for custom pump curve. */
    if (Pump[p].Ptype == CUSTOM)
    {
        /* Find intercept (h0) & slope (r) of pump curve    */
        /* line segment which contains speed-adjusted flow. */
        curvecoeff(Pump[p].Hcurve,q/K[k],&h0,&r);

        /* Determine head loss coefficients. */
        Pump[p].H0 = -h0;
        Pump[p].R  = -r;
        Pump[p].N  = 1.0;
    }

    /* Adjust head loss coefficients for pump speed. */
    h0 = SQR(K[k])*Pump[p].H0;
    n  = Pump[p].N;
    r  = Pump[p].R*pow(K[k],2.0-n);
    if (n != 1.0) r = n*r*pow(q,n-1.0);

    /* Compute inverse headloss gradient (P) and flow correction factor (Y) */
    P[k] = 1.0/MAX(r,RQtol);
    Y[k] = Q[k]/n + P[k]*h0;
}                        /* End of pumpcoeff */


/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void  curvecoeff(int i, float q, float *h0, float *r)
/*
**-------------------------------------------------------------------
**   Input:   i   = curve index
**            q   = flow rate
**   Output:  *h0 = head at zero flow (y-intercept)
**            *r  = dHead/dFlow (slope)
**   Purpose: computes intercept and slope of head v. flow curve
**            at current flow.
**-------------------------------------------------------------------
*/
{
    int   k1, k2, npts;
    float *x, *y;

    /* Remember that curve is stored in untransformed units */
    q *= Ucf[FLOW];
    x = Curve[i].X;           /* x = flow */
    y = Curve[i].Y;           /* y = head */
    npts = Curve[i].Npts;

    /* Find linear segment of curve that brackets flow q */
    k2 = 0;
    while (k2 < npts && x[k2] < q) k2++;
    if      (k2 == 0)    k2++;
    else if (k2 == npts) k2--;
    k1  = k2 - 1;

    /* Compute slope and intercept of this segment */
    *r  = (y[k2]-y[k1])/(x[k2]-x[k1]);
    *h0 = y[k1] - (*r)*x[k1];

    /* Convert units */
    *h0 = (*h0)/Ucf[HEAD];
    *r  = (*r)*Ucf[FLOW]/Ucf[HEAD];
}                       /* End of curvecoeff */


void  gpvcoeff(int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  none
**   Purpose: computes P & Y coeffs. for general purpose valve
**--------------------------------------------------------------
*/
{
    float h0,        /* Headloss curve intercept */
        q,         /* Abs. value of flow       */
        r;         /* Flow resistance coeff.   */

/*** Updated 9/7/00 ***/
    /* Treat as a pipe if valve closed */
    if (S[k] == CLOSED) pipecoeff(k);

    /* Otherwise utilize headloss curve   */
    /* whose index is stored in K */
    else
    {
        /* Find slope & intercept of headloss curve. */
        q = ABS(Q[k]);
        q = MAX(q,TINY);

/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
        curvecoeff((int)ROUND(K[k]),q,&h0,&r);

        /* Compute inverse headloss gradient (P) and flow  */
        /* correction factor (Y) using formulas for pumps. */
        P[k] = 1.0 / MAX(r,RQtol);
        Y[k] = Q[k] + P[k]*h0;
    }
}


void  pbvcoeff(int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  none
**   Purpose: computes P & Y coeffs. for pressure breaker valve
**--------------------------------------------------------------
*/
{
    /* If valve fixed OPEN or CLOSED then treat as a pipe */
    if (K[k] == MISSING || K[k] == 0.0) pipecoeff(k);

    /* If valve is active */
    else
    {
        /* Treat as a pipe if minor loss > valve setting */
        if (Link[k].Km*SQR(Q[k]) > K[k]) pipecoeff(k);

        /* Otherwise force headloss across valve to be equal to setting */
        else
        {
            P[k] = CBIG;
            Y[k] = K[k]*CBIG;
        }
    }
}                        /* End of pbvcoeff */


void  tcvcoeff(int k)
/*
**--------------------------------------------------------------
**   Input:   k = link index
**   Output:  none
**   Purpose: computes P & Y coeffs. for throttle control valve
**--------------------------------------------------------------
*/
{
    float km;

    /* Save original loss coeff. for open valve */
    km = Link[k].Km;

    /* If valve not fixed OPEN or CLOSED, compute its loss coeff. */
    if (K[k] != MISSING)
        Link[k].Km = 0.02517*K[k]/(SQR(Link[k].Diam)*SQR(Link[k].Diam));

    /* Then apply usual pipe formulas */
    pipecoeff(k);

    /* Restore original loss coeff. */
    Link[k].Km = km;
}                        /* End of tcvcoeff */


void  prvcoeff(int k, int n1, int n2)
/*
**--------------------------------------------------------------
**   Input:   k    = link index
**            n1   = upstream node of valve
**            n2   = downstream node of valve
**   Output:  none
**   Purpose: computes solution matrix coeffs. for pressure
**            reducing valves
**--------------------------------------------------------------
*/
{
    int   i,j;                       /* Rows of solution matrix */
    float hset;                      /* Valve head setting      */
    i  = Row[n1];                    /* Matrix rows of nodes    */
    j  = Row[n2];
    hset   = Node[n2].El + K[k];     /* Valve setting           */

    if (S[k] == ACTIVE)
    {
        /*
          Set coeffs. to force head at downstream
          node equal to valve setting & force flow (when updated in
          newflows()) equal to flow imbalance at downstream node.
        */
        P[k] = 0.0;
        Y[k] = Q[k] + X[n2];       /* Force flow balance   */
        F[j] += (hset*CBIG);       /* Force head = hset    */
        Aii[j] += CBIG;            /*   at downstream node */
        if (X[n2] < 0.0) F[i] += X[n2];
        return;
    }
    /*
      For OPEN, CLOSED, or XPRESSURE valve
      compute matrix coeffs. using the pipecoeff() function.
    */
    pipecoeff(k);
    Aij[Ndx[k]] -= P[k];
    Aii[i] += P[k];
    Aii[j] += P[k];
    F[i] += (Y[k]-Q[k]);
    F[j] -= (Y[k]-Q[k]);
}                        /* End of prvcoeff */


void  psvcoeff(int k, int n1, int n2)
/*
**--------------------------------------------------------------
**   Input:   k    = link index
**            n1   = upstream node of valve
**            n2   = downstream node of valve
**   Output:  none
**   Purpose: computes solution matrix coeffs. for pressure
**            sustaining valve
**--------------------------------------------------------------
*/
{
    int   i,j;                       /* Rows of solution matrix */
    float hset;                      /* Valve head setting      */
    i  = Row[n1];                    /* Matrix rows of nodes    */
    j  = Row[n2];
    hset   = Node[n1].El + K[k];     /* Valve setting           */

    if (S[k] == ACTIVE)
    {
        /*
          Set coeffs. to force head at upstream
          node equal to valve setting & force flow (when updated in
          newflows()) equal to flow imbalance at upstream node.
        */
        P[k] = 0.0;
        Y[k] = Q[k] - X[n1];              /* Force flow balance   */
        F[i] += (hset*CBIG);              /* Force head = hset    */
        Aii[i] += CBIG;                   /*   at upstream node   */
        if (X[n1] > 0.0) F[j] += X[n1];
        return;
    }

    /*
      For OPEN, CLOSED, or XPRESSURE valve
      compute matrix coeffs. using the pipecoeff() function.
    */
    pipecoeff(k);
    Aij[Ndx[k]] -= P[k];
    Aii[i] += P[k];
    Aii[j] += P[k];
    F[i] += (Y[k]-Q[k]);
    F[j] -= (Y[k]-Q[k]);
}                        /* End of psvcoeff */


void  fcvcoeff(int k, int n1, int n2)
/*
**--------------------------------------------------------------
**   Input:   k    = link index
**            n1   = upstream node of valve
**            n2   = downstream node of valve
**   Output:  none
**   Purpose: computes solution matrix coeffs. for flow control
**            valve
**--------------------------------------------------------------
*/
{
    int   i,j;                   /* Rows in solution matrix */
    float q;                     /* Valve flow setting      */
    q = K[k];
    i = Row[n1];
    j = Row[n2];

    /*
      If valve active, break network at valve and treat
      flow setting as external demand at upstream node
      and external supply at downstream node.
    */
    if (S[k] == ACTIVE)
    {
        X[n1] -= q;
        F[i] -= q;
        X[n2] += q;
        F[j] += q;
        P[k] = 0.0;
        Y[k] = Q[k] - q;
    }
    /*
      Otherwise treat valve as an open pipe
    */
    else
    {
        pipecoeff(k);
        Aij[Ndx[k]] -= P[k];
        Aii[i] += P[k];
        Aii[j] += P[k];
        F[i] += (Y[k]-Q[k]);
        F[j] -= (Y[k]-Q[k]);
    }
}                        /* End of fcvcoeff */

/* 
   This function updates the number of switches and the schedule
   of a pump. Invoke this function whenever a pump changes status.
   
   index : link index of the pump.
   status : new status of the pump.
*/
static void 
pumpswitch (int index, char status)
{
    int pump_idx;

    pump_idx = PUMPINDEX(index);

    /* Htime == 0 is ignored because if the final status is the same
       as the initial status, there is no pump switch.  */
    if (Htime > 0 && status != Link[index].Stat) { 
        Pump[pump_idx].Switches++; 
    }
    
    if (Htime < 24 * 3600 && Pump[pump_idx].Schedule_idx < 24) {
        Pump[pump_idx].Schedule[Pump[pump_idx].Schedule_idx] = Htime;
        Pump[pump_idx].Schedule_idx++;
    }
}

/****************  END OF HYDRAUL.C  ***************/
