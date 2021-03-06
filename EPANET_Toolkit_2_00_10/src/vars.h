/*
************************************************************************
            Global Variables for EPANET Program

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
            6/24/02
AUTHOR:     L. Rossman
            US EPA - NRMRL

************************************************************************
*/
EXTERN FILE     *InFile,               /* Input file pointer           */
                *OutFile,              /* Output file pointer          */
                *RptFile,              /* Report file pointer          */
                *HydFile,              /* Hydraulics file pointer      */
                *TmpOutFile;           /* Temporary file handle        */
EXTERN long     HydOffset,             /* Hydraulics file byte offset  */
                OutOffset1,            /* 1st output file byte offset  */
                OutOffset2;            /* 2nd output file byte offset  */
EXTERN char     Msg[MAXMSG+1],         /* Text of output message       */
                InpFname[MAXFNAME+1],  /* Input file name              */
                Rpt1Fname[MAXFNAME+1], /* Primary report file name     */
                Rpt2Fname[MAXFNAME+1], /* Secondary report file name   */
                HydFname[MAXFNAME+1],  /* Hydraulics file name         */
                OutFname[MAXFNAME+1],  /* Binary output file name      */
                MapFname[MAXFNAME+1],  /* Map file name                */
                Title[MAXTITLE][MAXMSG+1], /* Problem title            */
                ChemName[MAXID+1],     /* Name of chemical             */
                ChemUnits[MAXID+1],    /* Units of chemical            */
                DefPatID[MAXID+1],     /* Default demand pattern ID    */

/*** Updated 6/24/02 ***/
                Atime[13],             /* Clock time (hrs:min:sec)     */

                Hydflag,               /* Hydraulics flag              */
                Qualflag,              /* Water quality flag           */
                Unitsflag,             /* Unit system flag             */
                Flowflag,              /* Flow units flag              */
                Pressflag,             /* Pressure units flag          */
                Formflag,              /* Hydraulic formula flag       */
                Rptflag,               /* Report flag                  */
                Summaryflag,           /* Report summary flag          */
                Messageflag,           /* Error/warning message flag   */
                Statflag,              /* Status report flag           */
                Energyflag,            /* Energy report flag           */
                Nodeflag,              /* Node report flag             */
                Linkflag,              /* Link report flag             */
                Tstatflag,             /* Time statistics flag         */
                Warnflag,              /* Warning flag                 */
                Openflag,              /* Input processed flag         */
                OpenHflag,             /* Hydraul. system opened flag  */
                SaveHflag,             /* Hydraul. results saved flag  */
                OpenQflag,             /* Quality system opened flag   */
                SaveQflag,             /* Quality results saved flag   */
                Saveflag;              /* General purpose save flag    */
EXTERN int      MaxNodes,              /* Node count from input file   */
                MaxLinks,              /* Link count from input file   */
                MaxJuncs,              /* Junction count               */
                MaxPipes,              /* Pipe count                   */
                MaxTanks,              /* Tank count                   */
                MaxPumps,              /* Pump count                   */
                MaxValves,             /* Valve count                  */
                MaxControls,           /* Control count                */
                MaxRules,              /* Rule count                   */
                MaxPats,               /* Pattern count                */
                MaxCurves,             /* Curve count                  */
                Nnodes,                /* Number of network nodes      */
                Ntanks,                // Number of tanks and reservoirs
                Nreservs,              // Number of reservoirs
                Njuncs,                /* Number of junction nodes     */
                Nlinks,                /* Number of network links      */
                Npipes,                /* Number of pipes              */
                Npumps,                /* Number of pumps              */
                Nvalves,               /* Number of valves             */
                Ncontrols,             /* Number of simple controls    */
                Nrules,                /* Number of control rules      */
                Npats,                 /* Number of time patterns      */
                Ncurves,               /* Number of data curves        */
                Nperiods,              /* Number of reporting periods  */
                Ncoeffs,               /* Number of non-0 matrix coeffs*/
                DefPat,                /* Default demand pattern       */
                Epat,                  /* Energy cost time pattern     */
                MaxIter,               /* Max. hydraulic trials        */
                ExtraIter,             /* Extra hydraulic trials       */
                TraceNode,             /* Source node for flow tracing */
                PageSize,              /* Lines/page in output report  */
                CheckFreq,             /* Hydraulics solver parameter  */
                MaxCheck,              /* Hydraulics solver parameter  */
                Nwarnings;             // Number of warnings caused by
                                       // negative pressures

EXTERN float    Ucf[MAXVAR],           /* Unit conversion factors      */
                Ctol,                  /* Water quality tolerance      */
                Htol,                  /* Hydraulic head tolerance     */
                Qtol,                  /* Flow rate tolerance          */
                RQtol,                 /* Flow resistance tolerance    */
                Hexp,                  /* Exponent in headloss formula */
                Qexp,                  /* Exponent in orifice formula  */
                Dmult,                 /* Demand multiplier            */
                Hacc,                  /* Hydraulics solution accuracy */
                BulkOrder,             /* Bulk flow reaction order     */
                WallOrder,             /* Pipe wall reaction order     */
                TankOrder,             /* Tank reaction order          */
                Kbulk,                 /* Global bulk reaction coeff.  */
                Kwall,                 /* Global wall reaction coeff.  */
                Climit,                /* Limiting potential quality   */
                Rfactor,               /* Roughness-reaction factor    */
                Diffus,                /* Diffusivity (sq ft/sec)      */
                Viscos,                /* Kin. viscosity (sq ft/sec)   */
                SpGrav,                /* Specific gravity             */
                Ecost,                 /* Base energy cost per kwh     */
                Dcost,                 /* Energy demand charge/kw/day  */
                Epump,                 /* Global pump efficiency       */
                Emax,                  /* Peak energy usage            */
                Dsystem,               /* Total system demand          */
                TotalSystemDemand,     // Simulation total system demand
                TotalSystemInflow,     // Simulation total system inflow
                TotalSystemLeakage,     // Simulation total system leakage
                Wbulk,                 /* Avg. bulk reaction rate      */
                Wwall,                 /* Avg. wall reaction rate      */
                Wtank,                 /* Avg. tank reaction rate      */
                Wsource;               /* Avg. mass inflow             */
EXTERN long     Tstart,                /* Starting time of day (sec)   */
                Hstep,                 /* Nominal hyd. time step (sec) */
                Qstep,                 /* Quality time step (sec)      */
                Pstep,                 /* Time pattern time step (sec) */
                Pstart,                /* Starting pattern time (sec)  */
                Rstep,                 /* Reporting time step (sec)    */
                Rstart,                /* Time when reporting starts   */
                Rtime,                 /* Next reporting time          */
                Htime,                 /* Current hyd. time (sec)      */
                Qtime,                 /* Current quality time (sec)   */
                Hydstep,               /* Actual hydraulic time step   */
                Rulestep,              /* Rule evaluation time step    */
                Dur;                   /* Duration of simulation (sec) */
EXTERN SField   Field[MAXVAR];         /* Output reporting fields      */

/* Array pointers not allocated and freed in same routine */
EXTERN char     *S,                    /* Link status                  */
                *OldStat;              /* Previous link/tank status    */
EXTERN float    *D,                    /* Node actual demand           */
                *C,                    /* Node actual quality          */
                *E,                    /* Emitter flows                */
                *K,                    /* Link settings                */
                *Q,                    /* Link flows                   */
                *R,                    /* Pipe reaction rate           */
                *X;                    /* General purpose array        */
EXTERN REAL     *H;                    /* Node heads                   */
EXTERN STmplist *Patlist;              /* Temporary time pattern list  */
EXTERN STmplist *Curvelist;            /* Temporary list of curves     */
EXTERN Spattern *Pattern;              /* Time patterns                */
EXTERN Scurve   *Curve;                /* Curve data                   */
EXTERN Snode    *Node;                 /* Node data                    */
EXTERN Slink    *Link;                 /* Link data                    */
EXTERN Stank    *Tank;                 /* Tank data                    */
EXTERN Spump    *Pump;                 /* Pump data                    */
EXTERN Svalve   *Valve;                /* Valve data                   */
EXTERN Scontrol *Control;              /* Control data                 */
EXTERN HTtable  *Nht, *Lht;            /* Hash tables for ID labels    */
EXTERN Padjlist *Adjlist;              /* Node adjacency lists         */

/*
** NOTE: Hydraulic analysis of the pipe network at a given point in time
**       is done by repeatedly solving a linearized version of the
**       equations for conservation of flow & energy:
**
**           A*H = F
**
**       where H = vector of heads (unknowns) at each node,
**             F = vector of right-hand side coeffs.
**             A = square matrix of coeffs.
**       and both A and F are updated at each iteration until there is
**       negligible change in pipe flows.
**
**       Each row (or column) of A corresponds to a junction in the pipe
**       network. Each link (pipe, pump or valve) in the network has a
**       non-zero entry in the row-column of A that corresponds to its
**       end points. This results in A being symmetric and very sparse.
**       The following arrays are used to efficiently manage this sparsity:
*/

EXTERN REAL     *Aii,        /* Diagonal coeffs. of A               */
                *Aij,        /* Non-zero, off-diagonal coeffs. of A */
                *F;          /* Right hand side coeffs.             */
EXTERN float    *P,          /* Inverse headloss derivatives        */
                *Y;          /* Flow correction factors             */
EXTERN int      *Order,      /* Node-to-row of A                    */
                *Row,        /* Row-to-node of A                    */
                *Ndx;        /* Index of link's coeff. in Aij       */
