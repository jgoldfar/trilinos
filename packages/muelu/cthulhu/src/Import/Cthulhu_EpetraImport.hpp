#ifndef CTHULHU_EPETRAIMPORT_HPP
#define CTHULHU_EPETRAIMPORT_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Cthulhu_ConfigDefs.hpp"

#include "Cthulhu_Import.hpp"

#include "Cthulhu_EpetraMap.hpp"//TMP

#include "Epetra_Import.h"

namespace Cthulhu {

  // TODO: move that elsewhere
  //   const Epetra_Import & toEpetra(const Import<int, int> &import);

  RCP< const Import<int, int > > toCthulhu(const Epetra_Import *import);
  //

  class EpetraImport
    : public Import<int, int>
  {

    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Constructs a Import object from the source and target Maps.
    EpetraImport(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &source, const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &target);

    //! copy constructor.
    EpetraImport(const Import< LocalOrdinal, GlobalOrdinal, Node > &import);

    //! destructor.
    ~EpetraImport() { }

    //@}

    //! @name Export Attribute Methods
    //@{

    //! Returns the number of entries that are identical between the source and target maps, up to the first different ID.
    size_t getNumSameIDs() const { return import_->NumSameIDs(); }

    //! Returns the number of entries that are local to the calling image, but not part of the first getNumSameIDs() entries.
    size_t getNumPermuteIDs() const { return import_->NumPermuteIDs(); }

    //! Returns the Source Map used to construct this importer.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getSourceMap() const { return toCthulhu(import_->SourceMap()); }

    //! Returns the Target Map used to construct this importer.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getTargetMap() const { return toCthulhu(import_->TargetMap()); }

    //@}

    //! @name Cthulhu specific
    //@{

    //! EpetraImport constructor to wrap a Epetra_Import object
    EpetraImport(const RCP<const Epetra_Import> &import) : import_(import) { }

    //! Get the underlying Epetra import
    RCP< const Epetra_Import> getEpetra_Import() const { return import_; }

    //@}
    
  private:
    
    RCP<const Epetra_Import> import_;

  }; // EpetraImport class

} // Cthulhu namespace

#endif // CTHULHU_EPETRAIMPORT_HPP
