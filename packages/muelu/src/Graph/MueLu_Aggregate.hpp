#ifndef MUELU_AGGREGATE_HPP
#define MUELU_AGGREGATE_HPP

#include "MueLu_Graph.hpp"

#define MUELOO_AGGR_READY    -11  /* indicates that a node is available to be*/
                                  /* selected as a root node of an aggregate */
#define MUELOO_AGGR_NOTSEL   -12  /* indicates that a node has been rejected */
                                  /* as a root node. This could perhaps be   */
                                  /* because if this node had been selected a*/
                                  /* small aggregate would have resulted.    */
#define MUELOO_AGGR_SELECTED -13  /* indicates that a node has been assigned */
                                  /* to an aggregate.                        */
#define MUELOO_UNAGGREGATED  -1   /* indicates that a node is unassigned to  */
                                  /* any aggregate.                          */
#define MUELOO_UNASSIGNED    -1   /* indicates a vertex is not yet claimed   */
                                  /* by a processor during aggregation.      */
                                  /* Note, it is possible at                 */
                                  /* this stage that some processors may have*/
                                  /* claimed their copy of a vertex for one  */
                                  /* of their aggregates.  However, some     */
                                  /* arbitration still needs to occur.       */
                                  /* The corresponding ProcWinner[]'s remain */
                                  /* as MUELOO_UNASSIGNED until              */
                                  /* MueLu_ArbitrateAndCommunicate() is     */
                                  /* invoked to arbitrate.                   */
#define MUELOO_NOSCORE       -100 /* indicates that a quality score has not  */
                                  /* yet been assigned when determining to   */
                                  /* which existing aggregate a vertex       */
                                  /* should be assigned.                     */
#define MUELOO_DISTONE_VERTEX_WEIGHT 100  /* Weights associated with all     */
                                  /* vertices that have a direct connection  */
                                  /* to the aggregate root.                  */
#define INCR_SCALING 3            /* Determines how much of a penalty should */
                                  /* be deduced from a score during Phase 5  */
                                  /* for each Phase 5 vertex already added   */
                                  /* to this aggregate. Specifically the     */
                                  /* penalty associated with aggregate y is  */
                                  /*   max (INCR_SCALING*NNewVtx,            */
                                  /*        UnpenalizedScore*(1-             */
                                  /*              MUELOO_PENALTYFACTOR))*/
                                  /* where NNewVtx is the number of phase 5  */
                                  /* vertices already assigned to y.         */
#define MUELOO_PENALTYFACTOR .30 /* determines maximum allowable        */
                                  /* percentage of a score that can be       */
                                  /* deducted from this score for having     */
                                  /* already enlargened an aggregate to      */
                                  /* which we are contemplated adding another*/
                                  /* vertex.  Should be between 0 and 1.     */

/***************************************************************************** 
   Structure holding aggregate information. Right now, NAggregates, IsRoot,
   Vertex2AggId, ProcWinner are populated.  This allows us to look at a node
   and determine the aggregate to which it has been assigned and the id of the 
   processor that owns this aggregte.  It is not so easy to determine vertices
   within the kth aggregate or the size of the kth aggregate. Thus, it might be
   useful to have a secondary structure which would be a rectangular CrsGraph 
   where rows (or vertices) correspond to aggregates and colunmns (or edges) 
   correspond to nodes.  While not strictly necessary, it might be convenient.
 *****************************************************************************/
typedef struct MueLu_Aggregate_Struct
{
   char *name;
   int  NAggregates;              /* Number of aggregates on this processor  */

   Epetra_IntVector *Vertex2AggId;/* Vertex2AggId[k] gives a local id        */
                                  /* corresponding to the aggregate to which */
                                  /* local id k has been assigned.  While k  */
   Epetra_IntVector *ProcWinner;  /* is the local id on my processor (MyPID),*/
                                  /* Vertex2AggId[k] is the local id on the  */
                                  /* processor which actually owns the       */
                                  /* aggregate. This owning processor has id */
                                  /* given by ProcWinner[k].                 */

   bool *IsRoot;                  /* IsRoot[i] indicates whether vertex i  */
                                  /* is a root node.                       */

   Epetra_CrsGraph *Agg2Vertex;   /* Currently not used                    */
} MueLu_Aggregate;


MueLu_Aggregate *MueLu_AggregateCreate(MueLu_Graph *Graph, const char *str);
int MueLu_AggregateDestroy(MueLu_Aggregate *agg);

// Constructors to create aggregates. This should really be replaced by an 
// aggregate class.
MueLu_Aggregate *MueLu_AggregateCreate(MueLu_Graph *Graph, const char *str)
{
   MueLu_Aggregate *Aggregates;

   Aggregates = (MueLu_Aggregate *) malloc(sizeof(MueLu_Aggregate));
   Aggregates->name = (char *) malloc(sizeof(char)*80);
   strcpy(Aggregates->name,str);
   Aggregates->NAggregates  = 0;
   Aggregates->Agg2Vertex   = NULL;
   Aggregates->Vertex2AggId = new Epetra_IntVector(Graph->EGraph->ImportMap());
   Aggregates->Vertex2AggId->PutValue(MUELOO_UNAGGREGATED);
   Aggregates->ProcWinner = new Epetra_IntVector(Graph->EGraph->ImportMap());
   Aggregates->ProcWinner->PutValue(MUELOO_UNASSIGNED);
   Aggregates->IsRoot = new bool[Graph->EGraph->ImportMap().NumMyElements()];
   for (int i=0; i < Graph->EGraph->ImportMap().NumMyElements(); i++)
       Aggregates->IsRoot[i] = false;

   return Aggregates;
}
// Destructor for aggregates. This should really be replaced by an 
// aggregate class.
int MueLu_AggregateDestroy(MueLu_Aggregate *agg)
{
   if (agg != NULL) {
      if (agg->IsRoot       != NULL) delete [] (agg->IsRoot);
      if (agg->ProcWinner   != NULL) delete agg->ProcWinner;
      if (agg->Vertex2AggId != NULL) delete agg->Vertex2AggId;
      if (agg->name         != NULL) free(agg->name);
      if (agg->Agg2Vertex   != NULL) delete agg->Agg2Vertex;
      free(agg);
   }
   return 0;
}

#endif
