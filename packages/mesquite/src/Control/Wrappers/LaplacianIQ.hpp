// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-
//
//   SUMMARY: 
//     USAGE:
//
//    AUTHOR: Thomas Leurent <tleurent@mcs.anl.gov>
//       ORG: Argonne National Laboratory
//    E-MAIL: tleurent@mcs.anl.gov
//
// ORIG-DATE: 14-Nov-02 at 16:51:36
//  LAST-MOD: 23-Jul-03 at 18:06:13 by Thomas Leurent


/*! \file LaplacianIQ.hpp

This is the second possibility for wrappers. It is based on the InctructionQueue concept. 

 */
// DESCRIP-END.
//


#ifndef LaplacianIQ_hpp
#define LaplacianIQ_hpp

#include "MeanRatioQualityMetric.hpp" 
#include "LaplacianSmoother.hpp"
#include "QualityAssessor.hpp"
#include "InstructionQueue.hpp"
#include "TerminationCriterion.hpp"

namespace Mesquite { 

   class LaplacianIQ : public InstructionQueue {
   private:
      ShapeQualityMetric* meanRatio;
      LaplacianSmoother* lapl1;
      QualityAssessor* mQA;
      TerminationCriterion* mTerm;

   public:
      
      //! Constructor sets the instructions in the queue.  
      LaplacianIQ() {
         MsqError err;
         // creates a mean ratio quality metric ...
         meanRatio = new MeanRatioQualityMetric;
     
         // creates the laplacian smoother  procedures
         lapl1 = new LaplacianSmoother(err);
         mQA = new QualityAssessor(meanRatio,QualityAssessor::MAXIMUM);
     
         //**************Set stopping criterion****************
         mTerm = new TerminationCriterion();
         mTerm->add_criterion_type_with_int(TerminationCriterion::NUMBER_OF_ITERATES,10,err);
 
            lapl1->set_outer_termination_criterion(mTerm);
            // sets a culling method on the first QualityImprover
            lapl1->add_culling_method(PatchData::NO_BOUNDARY_VTX);
      
            // adds 1 pass of pass1 
            this->add_quality_assessor(mQA,err); MSQ_CHKERR(err);
            this->set_master_quality_improver(lapl1, err); MSQ_CHKERR(err);
            this->add_quality_assessor(mQA,err); MSQ_CHKERR(err);
      }

      
      //! Destructor must delete the objects inserted in the queue.
      virtual ~LaplacianIQ()
      {
         delete meanRatio;
         delete lapl1;
         delete mQA;
         delete mTerm;
      }
  
   };


} // namespace

#endif // LaplacianIQ_hpp
