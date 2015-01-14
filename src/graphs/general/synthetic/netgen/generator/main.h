#ifndef NETGEN_MAIN_H
#define NETGEN_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#define PROBLEM_PARMS 13		/* aliases for generation parameters */
#define NODES	    parms[0]		/* number of nodes */
#define SOURCES     parms[1]		/* number of sources (including transshipment) */
#define SINKS	    parms[2]		/* number of sinks (including transshipment) */
#define DENSITY     parms[3]		/* number of (requested) arcs */
#define MINCOST     parms[4]		/* minimum cost of arcs */
#define MAXCOST     parms[5]		/* maximum cost of arcs */
#define SUPPLY	    parms[6]		/* total supply */
#define TSOURCES    parms[7]		/* transshipment sources */
#define TSINKS	    parms[8]		/* transshipment sinks */
#define HICOST	    parms[9]		/* percent of skeleton arcs given maximum cost */
#define CAPACITATED parms[10]		/* percent of arcs to be capacitated */
#define MINCAP	    parms[11]		/* minimum capacity for capacitated arcs */
#define MAXCAP	    parms[12]		/* maximum capacity for capacitated arcs */

#ifdef __cplusplus
}
#endif


#endif
