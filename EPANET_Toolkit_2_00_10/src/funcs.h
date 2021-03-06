/*
**************************************************************************

FUNCS.H -- Function Prototypes for EPANET Program

AUTHOR:     M. Lopez-Ibanez (M. L\'opez-Ib\'a\~nez)
            Napier University, Edinburgh, UK.

$LastChangedBy: manu $ $Revision: 77 $
$LastChangedDate: 2006-06-02 17:25:45 +0100 (Fri, 02 Jun 2006) $

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
DATE:       5/8/00
            9/25/00
            10/25/00
            12/29/00
            3/1/01
AUTHOR:     L. Rossman
            US EPA - NRMRL

**************************************************************************
*/

/* ------- EPANET.C --------------------*/
/*
**  NOTE: The exportable functions that can be called
**        via the DLL are prototyped in TOOLKIT.H.
*/
void    initpointers(void);               /* Initializes pointers       */
int     allocdata(void);                  /* Allocates memory           */
void    freeTmplist(STmplist *);          /* Frees items in linked list */
void    freeFloatlist(SFloatlist *);      /* Frees list of floats       */
void    freedata(void);                   /* Frees allocated memory     */
int     openfiles(char *,char *,char *);  /* Opens input & report files */
int     openhydfile(void);                /* Opens hydraulics file      */
int     openoutfile(void);                /* Opens binary output file   */
int     strcomp(char *, char *);          /* Compares two strings       */
float   interp(int, float *,              /* Interpolates a data curve  */
               float *, float);
int     findnode(char *);                 /* Finds node's index from ID */
int     findlink(char *);                 /* Finds link's index from ID */
char*   geterrmsg(int);                   /* Gets text of error message */
void    errmsg(int);                      /* Reports program error      */
void    writecon(char *);                 /* Writes text to console     */
void    writewin(char *);                 /* Passes text to calling app */

/* ------- INPUT1.C --------------------*/
int     getdata(void);                    /* Gets network data          */
void    setdefaults(void);                /* Sets default values        */
void    initreport(void);                 /* Initializes report options */
void    adjustdata(void);                 /* Adjusts input data         */
int     inittanks(void);                  /* Initializes tank levels    */
void    initunits(void);                  /* Determines reporting units */
void    convertunits(void);               /* Converts data to std. units*/

/* -------- INPUT2.C -------------------*/
int     netsize(void);                    /* Determines network size    */
int     readdata(void);                   /* Reads in network data      */
int     newline(int, char *);             /* Processes new line of data */
int     addnodeID(int, char *);           /* Adds node ID to data base  */
int     addlinkID(int, char *);           /* Adds link ID to data base  */
int     addpattern(char *);               /* Adds pattern to data base  */
int     addcurve(char *);                 /* Adds curve to data base    */
STmplist *findID(char *, STmplist *);     /* Locates ID on linked list  */
int     unlinked(void);                   /* Checks for unlinked nodes  */
int     getpumpparams(void);              /* Computes pump curve coeffs.*/
int     getpatterns(void);                /* Gets pattern data from list*/
int     getcurves(void);                  /* Gets curve data from list  */
int     findmatch(char *,char *[]);       /* Finds keyword in line      */
int     match(char *, char *);            /* Checks for word match      */
int     gettokens(char *);                /* Tokenizes input line       */
int     getfloat(char *, float *);        /* Converts string to float   */
float   hour(char *, char *);             /* Converts time to hours     */
int     setreport(char *);                /* Processes reporting command*/
void    inperrmsg(int,int,char *);        /* Input error message        */

/* ---------- INPUT3.C -----------------*/
int     juncdata(void);                   /* Processes junction data    */
int     tankdata(void);                   /* Processes tank data        */
int     pipedata(void);                   /* Processes pipe data        */
int     pumpdata(void);                   /* Processes pump data        */
int     valvedata(void);                  /* Processes valve data       */
int     patterndata(void);                /* Processes pattern data     */
int     curvedata(void);                  /* Processes curve data       */
int     demanddata(void);                 /* Processes demand data      */
int     controldata(void);                /* Processes simple controls  */
int     energydata(void);                 /* Processes energy data      */
int     sourcedata(void);                 /* Processes source data      */
int     emitterdata(void);                /* Processes emitter data     */
int     qualdata(void);                   /* Processes quality data     */
int     reactdata(void);                  /* Processes reaction data    */
int     mixingdata(void);                 /* Processes tank mixing data */
int     statusdata(void);                 /* Processes link status data */
int     reportdata(void);                 /* Processes report options   */
int     timedata(void);                   /* Processes time options     */
int     optiondata(void);                 /* Processes analysis options */
int     optionchoice(int);                /* Processes option choices   */
int     optionvalue(int);                 /* Processes option values    */
int     getpumpcurve(int);                /* Constructs a pump curve    */
int     powercurve(float, float, float,   /* Coeffs. of power pump curve*/
                   float, float, float *,
                   float *, float *);
int     valvecheck(int, int, int);        /* Checks valve placement     */
void    changestatus(int, char, float);   /* Changes status of a link   */
int     coordinates(void);                // Process coordinates of nodes

/* -------------- RULES.C --------------*/
void    initrules(void);                  /* Initializes rule base      */
void    addrule(char *);                  /* Adds rule to rule base     */
int     allocrules(void);                 /* Allocates memory for rule  */
int     ruledata(void);                   /* Processes rule input data  */
int     checkrules(long);                 /* Checks all rules           */
void    freerules(void);                  /* Frees rule base memory     */
int     addlevelbasedtrigger(int pump_index, int tank_index,
                             int start_time, int stop_time,
                             float level, int status);
void    clearrules(void);


/* ------------- REPORT.C --------------*/
int     writereport(void);                /* Writes formatted report    */
void    writelogo(void);                  /* Writes program logo        */
void    writesummary(void);               /* Writes network summary     */
void    writehydstat(int,float);          /* Writes hydraulic status    */
void    writeenergy(void);                /* Writes energy usage        */
int     writeresults(void);               /* Writes node/link results   */
void    writeheader(int,int);             /* Writes heading on report   */
void    writeline(char *);                /* Writes line to report file */
void    writerelerr(int, float);          /* Writes convergence error   */
void    writestatchange(int,char,char);   /* Writes link status change  */
void    writecontrolaction(int, int);     /* Writes control action taken*/
void    writeruleaction(int, char *);     /* Writes rule action taken   */
int     writehydwarn(int,float);          /* Writes hydraulic warnings  */
void    writehyderr(int);                 /* Writes hydraulic error msg.*/
int     disconnected(void);               /* Checks for disconnections  */
void    marknodes(int, int *, char *);    /* Identifies connected nodes */
void    getclosedlink(int, char *);       /* Finds a disconnecting link */
void    writelimits(int,int);             /* Writes reporting limits    */
int     checklimits(float *,int,int);     /* Checks variable limits     */
void    writetime(char *);                /* Writes current clock time  */
char    *clocktime(char *, long);         /* Converts time to hrs:min   */
char    *fillstr(char *, char, int);      /* Fills string with character*/
int     getnodetype(int);                 /* Determines node type       */

/* --------- HYDRAUL.C -----------------*/
int     openhyd(void);                    /* Opens hydraulics solver    */

/*** Updated 3/1/01 ***/
void    inithyd(int);                     /* Re-sets initial conditions */

int     runhyd(long *);                   /* Solves 1-period hydraulics */
int     nexthyd(long *);                  /* Moves to next time period  */
void    closehyd(void);                   /* Closes hydraulics solver   */
int     allocmatrix(void);                /* Allocates matrix coeffs.   */
void    freematrix(void);                 /* Frees matrix coeffs.       */
void    initlinkflow(int, char, float);   /* Initializes link flow      */
void    setlinkflow(int, float);          /* Sets link flow via headloss*/
void    setlinkstatus(int, char, char *,  /* Sets link status           */
                      float *);
void    setlinksetting(int, float,        /* Sets pump/valve setting    */
                       char *, float *);
void    resistance(int);                  /* Computes resistance coeff. */
void    demands(void);                    /* Computes current demands   */
int     controls(void);                   /* Controls link settings     */
long    timestep(void);                   /* Computes new time step     */
void    tanktimestep(long *);             /* Time till tanks fill/drain */
void    controltimestep(long *);          /* Time till control action   */
void    ruletimestep(long *);             /* Time till rule action      */
void    addenergy(long);                  /* Accumulates energy usage   */
void    getenergy(int, float *, float *); /* Computes link energy use   */
void    tanklevels(long);                 /* Computes new tank levels   */
float   tankvolume(int,float);            /* Finds tank vol. from grade */
float   tankgrade(int,float);             /* Finds tank grade from vol. */
int     netsolve(int *,float *);          /* Solves network equations   */
int     badvalve(int);                    /* Checks for bad valve       */
int     valvestatus(void);                /* Updates valve status       */
int     linkstatus(void);                 /* Updates link status        */
char    cvstatus(char,float,float);       /* Updates CV status          */
char    pumpstatus(int,float);            /* Updates pump status        */
char    prvstatus(int,char,float,         /* Updates PRV status         */
                  float,float);
char    psvstatus(int,char,float,         /* Updates PSV status         */
                  float,float);
char    fcvstatus(int,char,float,         /* Updates FCV status         */
                  float);
void    tankstatus(int,int,int);          /* Checks if tank full/empty  */
int     pswitch(void);                    /* Pressure switch controls   */
REAL    newflows(void);                   /* Updates link flows         */
void    newcoeffs(void);                  /* Computes matrix coeffs.    */
void    linkcoeffs(void);                 /* Computes link coeffs.      */
void    nodecoeffs(void);                 /* Computes node coeffs.      */
void    valvecoeffs(void);                /* Computes valve coeffs.     */
REAL    DWcoeff(int, REAL *);             /* Computes D-W coeff.        */
void    pumpcoeff(int);                   /* Computes pump coeff.       */

/*** Updated 10/25/00 ***/
/*** Updated 12/29/00 ***/
void    curvecoeff(int,float,float *,     /* Computes curve coeffs.     */
                   float *);

void    gpvcoeff(int);                    /* Computes GPV coeff.        */
void    pbvcoeff(int);                    /* Computes PBV coeff.        */
void    tcvcoeff(int);                    /* Computes TCV coeff.        */
void    prvcoeff(int,int,int);            /* Computes PRV coeff.        */
void    psvcoeff(int,int,int);            /* Computes PSV coeff.        */
void    fcvcoeff(int,int,int);            /* Computes FCV coeff.        */
void    emittercoeffs(void);              /* Computes emitter coeffs.   */
REAL    emitflowchange(int);              /* Computes new emitter flow  */

/* ----------- SMATRIX.C ---------------*/
int     createsparse(void);               /* Creates sparse matrix      */
int     allocsparse(void);                /* Allocates matrix memory    */
void    freesparse(void);                 /* Frees matrix memory        */
int     buildlists(int);                  /* Builds adjacency lists     */
int     paralink(int, int, int);          /* Checks for parallel links  */
void    xparalinks(void);                 /* Removes parallel links     */
void    freelists(void);                  /* Frees adjacency lists      */
void    countdegree(void);                /* Counts links at each node  */
int     reordernodes(void);               /* Finds a node re-ordering   */
int     mindegree(int, int);              /* Finds min. degree node     */
int     growlist(int);                    /* Augments adjacency list    */
int     newlink(Padjlist);                /* Adds fill-ins for a node   */
int     linked(int, int);                 /* Checks if 2 nodes linked   */
int     addlink(int, int, int);           /* Creates new fill-in        */
int     storesparse(int);                 /* Stores sparse matrix       */
int     ordersparse(int);                 /* Orders matrix storage      */
void    transpose(int,int *,int *,        /* Transposes sparse matrix   */
                  int *,int *,int *,int *,int *);
int     linsolve(int, REAL *, REAL *,     /* Solution of linear eqns.   */
                 REAL *);                 /* via Cholesky factorization */

/* ----------- QUALITY.C ---------------*/
int     openqual(void);                   /* Opens WQ solver system     */
void    initqual(void);                   /* Initializes WQ solver      */
int     runqual(long *);                  /* Gets current WQ results    */
int     nextqual(long *);                 /* Updates WQ by hyd.timestep */
int     stepqual(long *);                 /* Updates WQ by WQ time step */
int     closequal(void);                  /* Closes WQ solver system    */
int     gethyd(long *, long *);           /* Gets next hyd. results     */
char    setReactflag(void);               /* Checks for reactive chem.  */
void    transport(long);                  /* Transports mass in network */
void    initsegs(void);                   /* Initializes WQ segments    */
void    reorientsegs(void);               /* Re-orients WQ segments     */
void    updatesegs(long);                 /* Updates quality in segments*/
void    removesegs(int);                  /* Removes a WQ segment       */
void    addseg(int,float,float);          /* Adds a WQ segment to pipe  */
void    accumulate(long);                 /* Sums mass flow into node   */
void    updatenodes(long);                /* Updates WQ at nodes        */
void    sourceinput(long);                /* Computes source inputs     */
void    release(long);                    /* Releases mass from nodes   */
void    updatetanks(long);                /* Updates WQ in tanks        */
void    updatesourcenodes(long);          /* Updates WQ at source nodes */
void    tankmix1(int, long);              /* Complete mix tank model    */
void    tankmix2(int, long);              /* 2-compartment tank model   */
void    tankmix3(int, long);              /* FIFO tank model            */
void    tankmix4(int, long);              /* LIFO tank model            */
float   sourcequal(Psource);              /* Finds WQ input from source */
float   avgqual(int);                     /* Finds avg. quality in pipe */
void    ratecoeffs(void);                 /* Finds wall react. coeffs.  */
float   piperate(int);                    /* Finds wall react. coeff.   */
float   pipereact(int,float,float,long);  /* Reacts water in a pipe     */
float   tankreact(float,float,float,
                  long);                  /* Reacts water in a tank     */
float   bulkrate(float,float,float);      /* Finds bulk reaction rate   */
float   wallrate(float,float,float,float);/* Finds wall reaction rate   */


/* ------------ OUTPUT.C ---------------*/
int     savenetdata(void);                /* Saves basic data to file   */
int     savehyd(long *);                  /* Saves hydraulic solution   */
int     savehydstep(long *);              /* Saves hydraulic timestep   */
int     saveenergy(void);                 /* Saves energy usage         */
int     readhyd(long *);                  /* Reads hydraulics from file */
int     readhydstep(long *);              /* Reads time step from file  */
int     saveoutput(void);                 /* Saves results to file      */
int     nodeoutput(int,float *,float);    /* Saves node results to file */
int     linkoutput(int,float *,float);    /* Saves link results to file */
int     savefinaloutput(void);            /* Finishes saving output     */
int     savetimestat(float *, char);      /* Saves time stats to file   */
int     savenetreacts(REAL, REAL, REAL,
                      REAL);              /* Saves react. rates to file */
int     saveepilog(void);                 /* Saves output file epilog   */


/* ------------ INPFILE.C ---------------*/
int     saveinpfile(char *);             /* Saves network to text file  */
