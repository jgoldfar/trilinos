/*
 * MueLu_SimpleSmoother_decl.hpp
 *
 *  Created on: 19.03.2013
 *      Author: wiesner
 */

#ifndef MUELU_SIMPLESMOOTHER_DECL_HPP_
#define MUELU_SIMPLESMOOTHER_DECL_HPP_


#include "MueLu_ConfigDefs.hpp"

#include <Teuchos_ParameterList.hpp>

//Xpetra
#include <Xpetra_MapExtractor_fwd.hpp>
#include <Xpetra_MultiVectorFactory_fwd.hpp>
#include <Xpetra_CrsMatrixWrap_fwd.hpp>
#include <Xpetra_Matrix_fwd.hpp>

//MueLu
#include "MueLu_SimpleSmoother_fwd.hpp"
#include "MueLu_SmootherPrototype.hpp"
#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_FactoryManagerBase_fwd.hpp"
#include "MueLu_SmootherBase_fwd.hpp"
#include "MueLu_SubBlockAFactory_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"

#include "MueLu_SchurComplementFactory_fwd.hpp"
#include "MueLu_DirectSolver_fwd.hpp"
#include "MueLu_SmootherFactory_fwd.hpp"
#include "MueLu_FactoryManager_fwd.hpp"

namespace MueLu {

  /*!
    @class SimpleSmoother
    @brief SIMPLE smoother for 2x2 block matrices

  */

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = KokkosClassic::DefaultNode::DefaultNodeType, class LocalMatOps = typename KokkosClassic::DefaultKernels<void,LocalOrdinal,Node>::SparseOps> //TODO: or BlockSparseOp ?
  class SimpleSmoother : public SmootherPrototype<Scalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>
  {
    typedef Xpetra::MapExtractor<Scalar, LocalOrdinal, GlobalOrdinal, Node> MapExtractorClass;

#undef MUELU_SIMPLESMOOTHER_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors / destructors
    //@{

    /*! @brief Constructor
    */
    SimpleSmoother(const LocalOrdinal sweeps = 1, const Scalar omega = 1.0, bool SIMPLEC = false);

    //! Destructor
    virtual ~SimpleSmoother();
    //@}

    //! Input
    //@{

    void DeclareInput(Level &currentLevel) const;

    //! Set factory manager for internal velocity prediction
    void SetVelocityPredictionFactoryManager(RCP<FactoryManager> FactManager);

    //! Set factory manager for internal SchurComplement handling
    void SetSchurCompFactoryManager(RCP<FactoryManager> FactManager);

    //@}

    //! @name Setup and Apply methods.
    //@{

    /*! @brief Setup routine
     */
    void Setup(Level &currentLevel);

    /*! @brief Apply the Braess Sarazin smoother.
    @param X initial guess
    @param B right-hand side
    @param InitialGuessIsZero TODO This option has no effect.
    */
    void Apply(MultiVector &X, MultiVector const &B, bool InitialGuessIsZero = false) const;
    //@}

    RCP<SmootherPrototype> Copy() const;

    //! @name Overridden from Teuchos::Describable
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const;

    //! Print the object with some verbosity level to an FancyOStream object.
    //using MueLu::Describable::describe; // overloading, not hiding
    void print(Teuchos::FancyOStream &out, const VerbLevel verbLevel = Default) const;

    //@}

  private:

    //! smoother type
    std::string type_;

    const bool                            bSIMPLEC_;        //!< use SIMPLEC instead of SIMPLE (i.e. with absolute row sum approximation for A00 instead of plain diagonal)
    const LocalOrdinal                    nSweeps_;         //!< number of Braess Sarazin sweeps
    const Scalar                          omega_;           //!< damping/scaling factor



    RCP<const FactoryBase>                AFact_;           //!< A Factory

    RCP<FactoryManager>                   velpredictFactManager_;  //!< Factory manager for predicting velocity
    RCP<FactoryManager>                   schurFactManager_;       //!< Factory manager for creating the Schur Complement

    //! block operator
    RCP<Matrix>                         A_;               // < ! internal blocked operator "A" generated by AFact_

    RCP<const MapExtractorClass>          rangeMapExtractor_;  //!< range  map extractor (from A_ generated by AFact)
    RCP<const MapExtractorClass>          domainMapExtractor_; //!< domain map extractor (from A_ generated by AFact)

    //! matrices
    Teuchos::RCP<Vector>                  diagFinv_;                      //!< inverse diagonal of fluid operator (vector).
    Teuchos::RCP<Matrix>                F_;                             //!< fluid operator
    Teuchos::RCP<Matrix>                G_;                             //!< pressure gradient operator
    Teuchos::RCP<Matrix>                D_;                             //!< divergence operator
    Teuchos::RCP<Matrix>                Z_;                             //!< pressure stabilization term or null block

    Teuchos::RCP<SmootherBase>            velPredictSmoo_;              //!< smoother for velocity prediction
    Teuchos::RCP<SmootherBase>            schurCompSmoo_;               //!< smoother for SchurComplement equation


  }; // class Amesos2Smoother

} // namespace MueLu

#define MUELU_SIMPLESMOOTHER_SHORT

#endif /* MUELU_SIMPLESMOOTHER_DECL_HPP_ */
