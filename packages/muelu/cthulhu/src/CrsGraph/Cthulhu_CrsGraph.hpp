#ifndef CTHULHU_CRSGRAPH_HPP
#define CTHULHU_CRSGRAPH_HPP

#include <Teuchos_Describable.hpp>
#include <Teuchos_SerialDenseMatrix.hpp>
#include <Teuchos_CompileTimeAssert.hpp>
#include <Kokkos_DefaultNode.hpp>
#include <Kokkos_DefaultKernels.hpp>
#include <Kokkos_CrsGraph.hpp>
#include <Kokkos_NodeHelpers.hpp>
#include "Cthulhu_ConfigDefs.hpp"

namespace Cthulhu {

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  // forward declaration
  template <class S, class LO, class GO, class N, class SpMatOps>
  class CrsMatrix;
#endif

  struct RowInfo {
    size_t localRow;
    size_t allocSize;
    size_t numEntries;
    size_t offset1D;
  };

  enum ELocalGlobal {
    LocalIndices,
    GlobalIndices
  };

  template <class LocalOrdinal, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps>
  class CrsGraph
    // : public RowGraph<LocalOrdinal,GlobalOrdinal,Node>,
    //   public DistObject<GlobalOrdinal,LocalOrdinal,GlobalOrdinal,Node>
  {

  public:

    //! @name Constructor/Destructor Methods
    //@{ 

    // !Destructor.
    virtual ~CrsGraph() { }

    //@}

    //! @name Insertion/Removal Methods
    //@{

    //! Insert graph indices, using global IDs.
    virtual void insertGlobalIndices(GlobalOrdinal globalRow, const ArrayView< const GlobalOrdinal > &indices)= 0;

    //! Insert graph indices, using local IDs.
    virtual void insertLocalIndices(LocalOrdinal localRow, const ArrayView< const LocalOrdinal > &indices)= 0;

    //! Remove all graph indices from the specified local row.
    virtual void removeLocalIndices(LocalOrdinal localRow)= 0;

    //@}

    //! @name Transformational Methods
    //@{

    //! Signal that data entry is complete, specifying domain and range maps.
    virtual void fillComplete(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &domainMap, const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rangeMap, OptimizeOption os=DoOptimizeStorage)= 0;

    //! Signal that data entry is complete.
    virtual void fillComplete(OptimizeOption os=DoOptimizeStorage)= 0;

    //@}

    //! @name Methods implementing RowGraph.
    //@{

    //! Returns the communicator.
    virtual const RCP< const Comm< int > >  getComm() const = 0;

    //! Returns the Map that describes the row distribution in this graph.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRowMap() const = 0;

    //! Returns the Map that describes the column distribution in this graph.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getColMap() const = 0;

    //! Returns the Map associated with the domain of this graph.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getDomainMap() const = 0;

    //! Returns the Map associated with the domain of this graph.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRangeMap() const = 0;

    //! Returns the importer associated with this graph.
    virtual RCP< const Import< LocalOrdinal, GlobalOrdinal, Node > > getImporter() const = 0;

    //! Returns the number of global rows in the graph.
    virtual global_size_t getGlobalNumRows() const = 0;

    //! Returns the number of global columns in the graph.
    virtual global_size_t getGlobalNumCols() const = 0;

    //! Returns the number of graph rows owned on the calling node.
    virtual size_t getNodeNumRows() const = 0;

    //! Returns the number of columns connected to the locally owned rows of this graph.
    virtual size_t getNodeNumCols() const = 0;

    //! Returns the index base for global indices for this graph.
    virtual GlobalOrdinal getIndexBase() const = 0;

    //! Returns the global number of entries in the graph.
    virtual global_size_t getGlobalNumEntries() const = 0;

    //! Returns the local number of entries in the graph.
    virtual size_t getNodeNumEntries() const = 0;

    //! Returns the current number of entries on this node in the specified global row.
    virtual size_t getNumEntriesInGlobalRow(GlobalOrdinal globalRow) const = 0;

    //! Returns the current number of entries on this node in the specified local row.
    virtual size_t getNumEntriesInLocalRow(LocalOrdinal localRow) const = 0;

    //! Returns the current number of allocated entries for this node in the specified global row .
    virtual size_t getNumAllocatedEntriesInGlobalRow(GlobalOrdinal globalRow) const = 0;

    //! Returns the current number of allocated entries on this node in the specified local row.
    virtual size_t getNumAllocatedEntriesInLocalRow(LocalOrdinal localRow) const = 0;

    //! Returns the number of global diagonal entries, based on global row/column index comparisons.
    virtual global_size_t getGlobalNumDiags() const = 0;

    //! Returns the number of local diagonal entries, based on global row/column index comparisons.
    virtual size_t getNodeNumDiags() const = 0;

    //! Returns the maximum number of entries across all rows/columns on all nodes.
    virtual size_t getGlobalMaxNumRowEntries() const = 0;

    //! Returns the maximum number of entries across all rows/columns on this node.
    virtual size_t getNodeMaxNumRowEntries() const = 0;

    //! Indicates whether the graph has a well-defined column map.
    virtual bool hasColMap() const = 0;

    //! Indicates whether the graph is lower triangular.
    virtual bool isLowerTriangular() const = 0;

    //! Indicates whether the graph is upper triangular.
    virtual bool isUpperTriangular() const = 0;

    //! If graph indices are in the local range, this function returns true. Otherwise, this function returns false. */.
    virtual bool isLocallyIndexed() const = 0;

    //! If graph indices are in the global range, this function returns true. Otherwise, this function returns false. */.
    virtual bool isGloballyIndexed() const = 0;

    //! Returns true if fillComplete() has been called and the graph is in compute mode.
    virtual bool isFillComplete() const = 0;

    //! Returns true if storage has been optimized.
    virtual bool isStorageOptimized() const = 0;

    //! Extract a const, non-persisting view of global indices in a specified row of the graph.
    virtual void getGlobalRowView(GlobalOrdinal GlobalRow, ArrayView< const GlobalOrdinal > &Indices) const = 0;

    //! Extract a const, non-persisting view of local indices in a specified row of the graph.
    virtual void getLocalRowView(LocalOrdinal LocalRow, ArrayView< const LocalOrdinal > &indices) const = 0;

    //@}

    //! @name Overridden from Teuchos::Describable
    //@{

    //! Return a simple one-line description of this object.
    virtual std::string description() const = 0;

    //! Print the object with some verbosity level to an FancyOStream object.
    virtual void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const = 0;

    //@}

  }; // CrsGraph class

} // Cthulhu namespace

#define CTHULHU_CRSGRAPH_SHORT
#endif // CTHULHU_CRSGRAPH_HPP
