#ifndef MUELU_HIERARCHY_HELPERS_HPP
#define MUELU_HIERARCHY_HELPERS_HPP

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_Level.hpp"
#include "MueLu_FactoryManagerBase.hpp"

namespace MueLu {

  //! An exception safe way to call the method 'Level::SetFactoryManager()'
  class SetFactoryManager {

  public:

    //@{

    //!
    SetFactoryManager(Level & level, const RCP<const FactoryManagerBase> & factoryManager)
      : level_(level)
    {
      level.SetFactoryManager(factoryManager);
    }
    
    //! Destructor.
    virtual ~SetFactoryManager() { 
      level_.SetFactoryManager(Teuchos::null);
    }

    //@}

  private:
    Level & level_;
    //TODO: const RCP<const FactoryManagerBase> prevFactoryManager_;, save & restore previous factoryManager instead of reset to Teuchos::null.
  };

  // On the first level, 'A' is not generated by a factory (but is user-defined)
  // On other levels, we can use either NoFactory or the real 'A" factory.
  class InternalFactoryManager : public FactoryManagerBase {

  public:

    //!
    InternalFactoryManager(RCP<const FactoryManagerBase> & parentFactoryManager)
      : factoryManager_(parentFactoryManager), noFact_(NoFactory::getRCP())
    { }
    
    //! Destructor.
    virtual ~InternalFactoryManager() { }

    //! GetFactory
    const RCP<const FactoryBase> & GetFactory(const std::string & varName) const {
      if (varName == "A") return noFact_;
      // if (varName == "P") return noFact_;
      // if (varName == "R") return noFact_;
      // if (varName == "PreSmoother")  return noFact_;
      // if (varName == "PostSmoother") return noFact_;
      
      return factoryManager_->GetFactory(varName);
    }
    
    //! Clean
    void Clean() const { factoryManager_->Clean(); }

  private:
    RCP<const FactoryManagerBase> factoryManager_;
    RCP<const FactoryBase> noFact_; //TODO: remove
  };


  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps>
  class TopRAPFactory : public TwoLevelFactoryBase {
#include "MueLu_UseShortNames.hpp"

  public:

    TopRAPFactory(RCP<const FactoryManagerBase> parentFactoryManager)
      : factoryManager_(rcp( new InternalFactoryManager(parentFactoryManager))), PFact_(parentFactoryManager->GetFactory("P")), RFact_(parentFactoryManager->GetFactory("R")), AcFact_(parentFactoryManager->GetFactory("A"))
    { }
    
    virtual ~TopRAPFactory() { }

    void DeclareInput(Level & fineLevel, Level & coarseLevel) const {
      SetFactoryManager SFM2(fineLevel,   factoryManager_);
      SetFactoryManager SFM1(coarseLevel, factoryManager_);
      
      if (PFact_  != Teuchos::null) coarseLevel.DeclareInput("P", PFact_.get());
      if (RFact_  != Teuchos::null) coarseLevel.DeclareInput("R", RFact_.get());
      if (AcFact_ != Teuchos::null) coarseLevel.DeclareInput("A", AcFact_.get());    
    }
    
    void Build(Level & fineLevel, Level & coarseLevel) const {
      SetFactoryManager SFM1(fineLevel,   factoryManager_);
      SetFactoryManager SFM2(coarseLevel, factoryManager_);
      
      if (PFact_ != Teuchos::null) {
        RCP<Operator> P = coarseLevel.Get<RCP<Operator> >("P", PFact_.get());
        coarseLevel.Set("P", P);
      }
      
      if (RFact_ != Teuchos::null) {
        RCP<Operator> R = coarseLevel.Get<RCP<Operator> >("R", RFact_.get());
        coarseLevel.Set("R", R);
      }
      
      if (AcFact_ != Teuchos::null) {
        RCP<Operator> Ac = coarseLevel.Get<RCP<Operator> >("A", AcFact_.get());
        coarseLevel.Set("A", Ac);
      }
    }
    
  private:
    RCP<const FactoryManagerBase> factoryManager_;
    RCP<const FactoryBase> PFact_;
    RCP<const FactoryBase> RFact_;
    RCP<const FactoryBase> AcFact_;
  };

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps>
  class TopSmootherFactory : public SingleLevelFactoryBase { //TODO: inherit from SmootherFactoryBase ?
#include "MueLu_UseShortNames.hpp"

  public:

    TopSmootherFactory(RCP<const FactoryManagerBase> parentFactoryManager, const std::string & varName)
      : factoryManager_(rcp( new InternalFactoryManager(parentFactoryManager))), smootherFact_(parentFactoryManager->GetFactory(varName))
    { }

    virtual ~TopSmootherFactory() { }

    void DeclareInput(Level & level) const {
      SetFactoryManager SFM(level, factoryManager_);
           
      if (smootherFact_ != Teuchos::null) {
        level.DeclareInput("PreSmoother",  smootherFact_.get());
        level.DeclareInput("PostSmoother", smootherFact_.get());
      }
    }

    void Build(Level & level) const {
      SetFactoryManager SFM(level, factoryManager_);

      if (smootherFact_ != Teuchos::null) {
        smootherFact_->NewBuild(level);
          
        if (level.IsAvailable("PreSmoother", smootherFact_.get())) {
          RCP<SmootherBase> Pre  = level.Get<RCP<SmootherBase> >("PreSmoother", smootherFact_.get());
          level.Set("PreSmoother", Pre);
        }
          
        if (level.IsAvailable("PostSmoother", smootherFact_.get())) {
          RCP<SmootherBase> Post = level.Get<RCP<SmootherBase> >("PostSmoother", smootherFact_.get());
          level.Set("PostSmoother", Post);
        }
      }
    }

  private:
    RCP<const FactoryManagerBase> factoryManager_;
    RCP<const FactoryBase> smootherFact_;
  };

} // namespace MueLu
  
#define MUELU_HIERARCHY_HELPERS_SHORT
#endif //ifndef MUELU_HIERARCHY_HELPERS_HPP

// TODO: remove 'RCP' for TopRAPFactory::factoryManager_ and TopSmootherFactory::factoryManager_?
