#ifndef CTHULHU_EPETRACRSGRAPH_HPP
#define CTHULHU_EPETRACRSGRAPH_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Cthulhu_EpetraConfigDefs.hpp"

#include "Cthulhu_CrsGraph.hpp"

#include "Cthulhu_EpetraMap.hpp"
#include "Cthulhu_EpetraImport.hpp"
#include "Cthulhu_Comm.hpp"

#include <Epetra_CrsGraph.h>

namespace Cthulhu {
 
  // TODO: move that elsewhere
  RCP< const CrsGraph<int, int> > toCthulhu(const Epetra_CrsGraph& graph);
  //

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  // forward declaration
  template <class S, class LO, class GO, class N, class SpMatOps>
  class CrsMatrix;
#endif
  
  class EpetraCrsGraph
    : public CrsGraph<int, int>
  {

    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Constructor with fixed number of indices per row.
    EpetraCrsGraph(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, size_t maxNumEntriesPerRow, ProfileType pftype=DynamicProfile);

    //! Constructor with variable number of indices per row.
    EpetraCrsGraph(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const ArrayRCP< const size_t > &NumEntriesPerRowToAlloc, ProfileType pftype=DynamicProfile);

    //! Constructor with fixed number of indices per row and specified column map.
    EpetraCrsGraph(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &colMap, size_t maxNumEntriesPerRow, ProfileType pftype=DynamicProfile);

    //! Constructor with variable number of indices per row and specified column map.
    EpetraCrsGraph(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rowMap, const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &colMap, const ArrayRCP< const size_t > &NumEntriesPerRowToAlloc, ProfileType pftype=DynamicProfile);

    //! 
    virtual ~EpetraCrsGraph() { }

    //@}

    //! @name Insertion/Removal Methods
    //@{

    //! Insert graph indices, using global IDs.
    void insertGlobalIndices(GlobalOrdinal globalRow, const ArrayView< const GlobalOrdinal > &indices);

    //! Insert graph indices, using local IDs.
    void insertLocalIndices(LocalOrdinal localRow, const ArrayView< const LocalOrdinal > &indices);

    //! Remove all graph indices from the specified local row.
    void removeLocalIndices(LocalOrdinal localRow){ graph_->RemoveMyIndices(localRow); }

    //@}

    //! @name Transformational Methods
    //@{

    //! Signal that data entry is complete, specifying domain and range maps.
    void fillComplete(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &domainMap, const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rangeMap, OptimizeOption os=DoOptimizeStorage);

    //! Signal that data entry is complete.
    void fillComplete(OptimizeOption os=DoOptimizeStorage);

    //@}

    //! @name Methods implementing RowGraph.
    //@{

    //! Returns the communicator.
    const RCP< const Comm< int > >  getComm() const { return toCthulhu(graph_->Comm()); }

    //! Returns the Map that describes the row distribution in this graph.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRowMap() const { return toCthulhu(graph_->RowMap()); }

    //! Returns the Map that describes the column distribution in this graph.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getColMap() const { return toCthulhu(graph_->ColMap()); }

    //! Returns the Map associated with the domain of this graph.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getDomainMap() const { return toCthulhu(graph_->DomainMap()); }

    //! Returns the Map associated with the domain of this graph.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRangeMap() const { return toCthulhu(graph_->RangeMap()); }

    //! Returns the importer associated with this graph.
    RCP< const Import< LocalOrdinal, GlobalOrdinal, Node > > getImporter() const { return toCthulhu(graph_->Importer()); }

    //! Returns the number of global rows in the graph.
    global_size_t getGlobalNumRows() const { return graph_->NumGlobalRows(); }

    //! Returns the number of global columns in the graph.
    global_size_t getGlobalNumCols() const { return graph_->NumGlobalCols(); }

    //! Returns the number of graph rows owned on the calling node.
    size_t getNodeNumRows() const { return graph_->NumMyRows(); }

    //! Returns the number of columns connected to the locally owned rows of this graph.
    size_t getNodeNumCols() const { return graph_->NumMyCols(); }

    //! Returns the index base for global indices for this graph.
    GlobalOrdinal getIndexBase() const { return graph_->IndexBase(); }

    //! Returns the global number of entries in the graph.
    global_size_t getGlobalNumEntries() const { return graph_->NumGlobalEntries(); }

    //! Returns the local number of entries in the graph.
    size_t getNodeNumEntries() const { return graph_->NumMyEntries(); }

    //! Returns the current number of entries on this node in the specified global row.
    size_t getNumEntriesInGlobalRow(GlobalOrdinal globalRow) const { return graph_->NumGlobalIndices(globalRow); }

    //! Returns the current number of entries on this node in the specified local row.
    size_t getNumEntriesInLocalRow(LocalOrdinal localRow) const { return graph_->NumMyIndices(localRow); }

    //! Returns the current number of allocated entries for this node in the specified global row .
    size_t getNumAllocatedEntriesInGlobalRow(GlobalOrdinal globalRow) const { return graph_->NumAllocatedGlobalIndices(globalRow); }

    //! Returns the current number of allocated entries on this node in the specified local row.
    size_t getNumAllocatedEntriesInLocalRow(LocalOrdinal localRow) const { return graph_->NumAllocatedMyIndices(localRow); }

    //! Returns the number of global diagonal entries, based on global row/column index comparisons.
    global_size_t getGlobalNumDiags() const { return graph_->NumGlobalDiagonals(); }

    //! Returns the number of local diagonal entries, based on global row/column index comparisons.
    size_t getNodeNumDiags() const { return graph_->NumMyDiagonals(); }

    //! Returns the maximum number of entries across all rows/columns on all nodes.
    size_t getGlobalMaxNumRowEntries() const { return graph_->GlobalMaxNumIndices(); }

    //! Returns the maximum number of entries across all rows/columns on this node.
    size_t getNodeMaxNumRowEntries() const { return graph_->MaxNumIndices(); }

    //! Indicates whether the graph has a well-defined column map.
    bool hasColMap() const { return graph_->HaveColMap(); }

    //! Indicates whether the graph is lower triangular.
    bool isLowerTriangular() const { return graph_->LowerTriangular(); }

    //! Indicates whether the graph is upper triangular.
    bool isUpperTriangular() const { return graph_->UpperTriangular(); }

    //! If graph indices are in the local range, this function returns true. Otherwise, this function returns false. */.
    bool isLocallyIndexed() const { return graph_->IndicesAreLocal(); }

    //! If graph indices are in the global range, this function returns true. Otherwise, this function returns false. */.
    bool isGloballyIndexed() const { return graph_->IndicesAreGlobal(); }

    //! Returns true if fillComplete() has been called and the graph is in compute mode.
    bool isFillComplete() const { return graph_->Filled(); }

    //! Returns true if storage has been optimized.
    bool isStorageOptimized() const { return graph_->StorageOptimized(); }

    //! Extract a const, non-persisting view of global indices in a specified row of the graph.
    void getGlobalRowView(GlobalOrdinal GlobalRow, ArrayView< const GlobalOrdinal > &Indices) const ;

    //! Extract a const, non-persisting view of local indices in a specified row of the graph.
    void getLocalRowView(LocalOrdinal LocalRow, ArrayView< const LocalOrdinal > &indices) const ;

    //@}

    //! @name Overridden from Teuchos::Describable
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const ;

    //! Print the object with some verbosity level to an FancyOStream object.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const ;

    //@}

    //! @name Cthulhu specific
    //@{

    //! EpetraCrsGraph constructor to wrap a Epetra_CrsGraph object
    EpetraCrsGraph(const Teuchos::RCP<Epetra_CrsGraph> &graph) : graph_(graph) { }

    //! Get the underlying Epetra graph
    RCP< const Epetra_CrsGraph> getEpetra_CrsGraph() const { return graph_; }
    
    //@}
    
  private:
    
    RCP<Epetra_CrsGraph> graph_;

  }; // EpetraCrsGraph class

} // Cthulhu namespace

#endif // CTHULHU_EPETRACRSGRAPH_HPP
