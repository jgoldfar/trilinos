/*
 * MueLu_SegregationAFilterFactory.hpp
 *
 *  Created on: Oct 25, 2011
 *      Author: wiesner
 */

#ifndef MUELU_SEGREGATIONAFILTERFACTORY_HPP_
#define MUELU_SEGREGATIONAFILTERFACTORY_HPP_

#include <Teuchos_Assert.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_Time.hpp>
#include <Teuchos_TypeNameTraits.hpp>
#include <Teuchos_ScalarTraits.hpp>

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_Exceptions.hpp"
#include "MueLu_SingleLevelFactoryBase.hpp"

#include "Xpetra_MapExtractorFactory.hpp"

#if 0
#include "EpetraExt_RowMatrixOut.h"
#endif

namespace MueLu {

  /*!
    @class SegregationAFilterFactory class.
    @brief Experimental segregation of Dofs in a matrix

  */

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps>
  class SegregationAFilterFactory : public SingleLevelFactoryBase {

    #include "MueLu_UseShortNames.hpp"

    typedef Xpetra::MapExtractor<Scalar, LocalOrdinal, GlobalOrdinal, Node> MapExtractorClass;

  public:
    //! @name Constructors/Destructors.
    //@{

    //! Constructor.
    SegregationAFilterFactory(const std::string& ename, const FactoryBase* fac, Teuchos::RCP<const MapExtractorClass>& rangeMaps)
      : varName_(ename), factory_(fac), mapextractor_(rangeMaps)
    { }

    //! Destructor.
    virtual ~SegregationAFilterFactory() {}
    //@}

    //! Input
    //@{

    void DeclareInput(Level &currentLevel) const {
      currentLevel.DeclareInput(varName_,factory_);
      currentLevel.DeclareInput("SegAMapExtractor", MueLu::NoFactory::get());
    }

    //@}

    //@{
    //! @name Build methods.

    //! Build an object with this factory.
    void Build(Level & currentLevel) const {
      Monitor m(*this, "A filter (segregation)");

      if (currentLevel.IsAvailable("SegAMapExtractor", MueLu::NoFactory::get())==false) {
        GetOStream(Runtime0, 0) << "Use user provided map extractor with " << mapextractor_->NumMaps() << " submaps for segregation filter for " << varName_ << std::endl;
        currentLevel.Set("SegAMapExtractor", mapextractor_, MueLu::NoFactory::get());
      }

      // fetch map extractor from level
      RCP<const MapExtractorClass> mapextractor = currentLevel.Get< RCP<const MapExtractorClass> >("SegAMapExtractor",MueLu::NoFactory::get());

      // fetch matrix/operator from level
      RCP<Operator> Ain = currentLevel.Get< RCP<Operator> >(varName_, factory_);

      // create new empty Operator
      RCP<CrsOperator> Aout = rcp(new CrsOperator(Ain->getRowMap(),Ain->getGlobalMaxNumRowEntries(),Xpetra::StaticProfile));

      GetOStream(Runtime0, 0) << "Segregation filter for " << varName_ << " with " << mapextractor_->NumMaps() << " blocks" << std::endl;

      // loop over local rows
      for(size_t row=0; row<Ain->getNodeNumRows(); row++)
      {
        // get global row id
        GlobalOrdinal grid = Ain->getRowMap()->getGlobalElement(row); // global row id

        // check in which submap of mapextractor grid belongs to
        LocalOrdinal blockId = -Teuchos::ScalarTraits<LocalOrdinal>::one();
        for (size_t bb=0; bb<mapextractor->NumMaps(); bb++) {
          const RCP<const Map> cmap = mapextractor->getMap(bb);
          if (cmap->isNodeGlobalElement(grid)) {
            blockId = bb;
            break;
          }
        }
        TEUCHOS_TEST_FOR_EXCEPTION(blockId == -1, Exceptions::RuntimeError, "MueLu::SegregationAFilterFactory::Build(): grid does not belong to a submap in mapextractor_");

        size_t nnz = Ain->getNumEntriesInLocalRow(row);
        Teuchos::ArrayView<const LocalOrdinal> indices;
        Teuchos::ArrayView<const Scalar> vals;
        Ain->getLocalRowView(row, indices, vals);
        TEUCHOS_TEST_FOR_EXCEPTION(Teuchos::as<size_t>(indices.size()) != nnz, Exceptions::RuntimeError, "MueLu::ThresholdAFilterFactory::Build: number of nonzeros not equal to number of indices? Error.");

        // reserve indices for filtered matrix
        Teuchos::ArrayRCP<LocalOrdinal> indout(indices.size(),Teuchos::ScalarTraits<LocalOrdinal>::zero());
        Teuchos::ArrayRCP<Scalar> valout(indices.size(),Teuchos::ScalarTraits<Scalar>::zero());
        size_t nNonzeros = 0;
        for(size_t i=0; i<(size_t)indices.size(); i++) {

            // get global column id
            GlobalOrdinal gcid = Ain->getColMap()->getGlobalElement(indices[i]); // LID -> GID (column)
            // check in which submap of mapextractor gcid belongs to
            LocalOrdinal blockColId = -Teuchos::ScalarTraits<LocalOrdinal>::one();
            for (size_t bbc=0; bbc<mapextractor->NumMaps(); bbc++) {
              const RCP<const Map> ccmap = mapextractor->getMap(bbc);
              if (ccmap->isNodeGlobalElement(gcid)) {
                blockColId = bbc;
                break;
              }
            }

            if(blockColId == blockId) {
              indout[nNonzeros] = gcid; // LID -> GID (column)
              valout[nNonzeros] = vals[i];
              nNonzeros++;
            }
        }

        indout.resize(nNonzeros);
        valout.resize(nNonzeros);

        TEUCHOS_TEST_FOR_EXCEPTION(nNonzeros == Teuchos::as<size_t>(0), Exceptions::RuntimeError, "MueLu::ThresholdAFilterFactory::Build: Filtered matrix has a zero row. This is really bad.");
        Aout->insertGlobalValues(Ain->getRowMap()->getGlobalElement(row), indout.view(0,indout.size()), valout.view(0,valout.size()));
      }

      Aout->fillComplete(Ain->getDomainMap(), Ain->getRangeMap());

      GetOStream(Statistics0, 0) << "Nonzeros in " << varName_ << "(input): " << Ain->getGlobalNumEntries() << ", Nonzeros after filtering " << varName_ << ": #nonzeros=" << Aout->getGlobalNumEntries() << std::endl;

#if 0
      RCP<CrsMatrix> crsOut = Aout->getCrsMatrix();
      RCP<EpetraCrsMatrix> epcrsOut = Teuchos::rcp_dynamic_cast<EpetraCrsMatrix>(crsOut);
      RCP<const Epetra_CrsMatrix> epOut = epcrsOut->getEpetra_CrsMatrix();
      EpetraExt::RowMatrixToMatrixMarketFile   (   "crashme.mat",
        *epOut,
        "testmat"
        );
      std::cout << " wrote output matrix" << std::endl;
      exit(0);
#endif

      currentLevel.Set(varName_, Teuchos::rcp_dynamic_cast<Operator>(Aout), this);
    }

    //@}

  private:
    std::string         varName_;                 ///< name of input and output variable
    const FactoryBase*  factory_;                 ///< generating factory of input variable
    RCP<const MapExtractorClass> mapextractor_;   ///< user given map extractor (for finest level)


  }; // class ThresholdAFilterFactory

}

#define MUELU_SEGREGATIONAFILTERFACTORY_SHORT

#endif /* MUELU_SEGREGATIONAFILTERFACTORY_HPP_ */
