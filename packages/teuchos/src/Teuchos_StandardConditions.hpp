// @HEADER
// ***********************************************************************
// 
//                    Teuchos: Common Tools Package
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER


#ifndef TEUCHOS_STANDARDCONDITION_HPP_
#define TEUCHOS_STANDARDCONDITION_HPP_

/*! \file Teuchos_StandardConditions.hpp
    \brief Standard Conditions to be used.
*/

#include "Teuchos_Condition.hpp"
#include "Teuchos_InvalidConditionException.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_StandardFunctionObjects.hpp"
#include "Teuchos_DummyObjectGetter.hpp"
#include "Teuchos_ScalarTraits.hpp"


namespace Teuchos{


/**
 * \brief An Abstract Base class for all ParameterConditions.
 *
 * A Parmaeter Condition examines the value of a given
 * parameter and returns a bool based on the condition of
 * that value.
 */
class ParameterCondition : public Condition{

public:

  /** \name Public constants */
  //@{

  /** \brief The default value for the whenParamEqualsValue
   * parameter in the Constructor.
   */
  static const bool& getWhenParamEqualsValueDefault(){
    static const bool WHEN_PARAM_EQUALS_VALUE_DEFAULT = true;
    return WHEN_PARAM_EQUALS_VALUE_DEFAULT;
  }

  //@}

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs a Parameter Condition.
   *
   * @param Parameter The parameter to be evaluated.
   * @param whenParamEqualsValue Indicates whether the condition 
   * should be true when the evaluation
   * results in a true or when the evaluation results in a false. 
   * When set to true, if the parameter
   * evaluates to true then the condition will evaluate to true. If 
   * set to false if the parameter
   * evaluates to false, then the condition will evaluate to true.
   */
  ParameterCondition(
    RCP<ParameterEntry> parameter, 
    bool whenParamEqualsValue=getWhenParamEqualsValueDefault());

  virtual ~ParameterCondition(){}
  
  //@}

  //! @name Attribute/Query Methods 
  //@{

  /**
   * Evaluate the current condition of a parameter and
   * return the result.
   *
   * @param The result of evaluating the current condition
   * of the parameter.
   */
  virtual bool evaluateParameter() const = 0;

  /** \brief Gets a const pointer to the Parameter being
   *  evaluated by this ParameterCondition
   */
  inline RCP<const ParameterEntry> getParameter() const{
    return parameterEntry_.getConst();
  }

  /** \brief Gets the WhenParamEqualsValue */
  inline
  bool getWhenParamEqualsValue() const{
    return whenParamEqualsValue_;
  }
  
  //@}

  /** \name Overridden from Condition */
  //@{

  bool isConditionTrue() const{
    if((whenParamEqualsValue_ && evaluateParameter()) || 
      (!whenParamEqualsValue_ && !evaluateParameter()))
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  bool containsAtLeasteOneParameter() const{
    return true;
  }

  Dependency::ConstParameterEntryList getAllParameters() const;                
  
  //@}

private:

  /** \name Private Members */
  //@{
  
  /**
   * Parameter to be evaluated.
   */
  RCP<ParameterEntry> parameterEntry_;


  /**
   * Whether or not the condition should evaluate to 
   * true if the parameter evaluated to true.
   */
  bool whenParamEqualsValue_;

  //@}

};

/**
 * \brief A String Condition is a Parameter Condition that evaluates
 * whether or not a string parameter has taken on a particular
 * value or set of values.
 */
class StringCondition : public ParameterCondition{

public:

  /** \name Public types */
  //@{

  /**
   * \brief Convience typedef representing an array of strings.
   */
  typedef Array<std::string> ValueList; 
  
  //@}

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs a String Condition.
   *
   * @param parameter The parameter to be evaluated.
   * be evaluated.
   * @param value The value to compare the parameter's value against.
   * @param whenParamEqualsValue Indicates whether the condition 
   * should be true when the evaluation
   * results in a true or when the evaluation results in a false. 
   * When set to true, if the parameter
   * evaluates to true then the condition will evaluate to true.
   * If set to false if the parameter
   * evaluates to false, then the condition will evaluate to true.
   */
  StringCondition(
   RCP<ParameterEntry> parameter,
   std::string value, 
   bool whenParamEqualsValue=true);

  /**
   * \brief Constructs a String Condition.
   *
   * @param parameter The parameter to be evaluated.
   * @param values The list values to compare the parameter's value against.
   * @param whenParamEqualsValue Indicates 
   * whether the condition should be true when the evaluation
   * results in a true or when the evaluation results in a false.
   * When set to true, if the parameter
   * evaluates to true then the condition will evaluate to true. If set
   * to false if the parameter
   * evaluates to false, then the condition will evaluate to true.
   */
  StringCondition(
    RCP<ParameterEntry> parameter, 
    ValueList values, 
    bool whenParamEqualsValue=true);

  virtual ~StringCondition(){}
  
  //@}

  /** \name Overridden from Condition */
  //@{

  std::string getTypeAttributeValue() const{
    return "StringCondition";
  }
  
  //@}

  /** \name Overridden from ParameterCondition */
  //@{

  bool evaluateParameter() const;
  
  //@}

  /** \name Attribute/Query Functions */
  //@{

    /** \brief Returns the value list being used with this StringCondition. */
    const ValueList& getValueList() const{
      return values_;
    }

  //@}

private:

  /** \name Private Members */
  //@{
  
  /**
   * A list of values against which to evaluate the parameter's value.
   */
  ValueList values_;

  /** \brief Ensures the parameter is the proper type. In this case a string. */
  void checkParameterType();
  
  //@}
  
};


/** \brief Speicialized class for retrieving a dummy object of type
 * StringCondition.
 *
 * \relates StringCondition
 */
template<>
class DummyObjectGetter<StringCondition>{

public:

  /** \name Getter Functions */
  //@{

  /** \brief Retrieves a dummy object of type
  * StringCondition.
  */
  static RCP<StringCondition> getDummyObject();
  
  //@}
  
};


/**
 * \brief A Number Condition is a Parameter Condition that evaluates
 * whether or not a number parameter is greater than 0 (or some other number
 * based on a given function). 
 * If the parameter is  greater than 0 this is interperted as the condition 
 * being "true". Otherwise the oncidiont is interperted as false.
 */
template<class T>
class NumberCondition : public ParameterCondition{

public:

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs a Number Condition.
   *
   * @param parameterName The name of the parameter to be evaluated.
   * @param whenParamEqualsValue Indicates whether the condition should be 
   * true when the evaluation results in a true or when the evaluation results 
   * in a false. When set to true, if the parameter evaluates to true then 
   * the condition will evaluate to true. If seet to false if the parameter
   * evaluates to false, then the condition will evaluate to true.
   */
  NumberCondition(
    RCP<ParameterEntry> parameter,
    bool whenParamEqualsValue=true):
    ParameterCondition(parameter, whenParamEqualsValue), 
    func_(null)
  {}

  /**
   * \brief Constructs a Number Condition.
   *
   * @param parameterName The name of the parameter to be evaluated.
   * @param func A function to run the value of the parameter through. 
   * If the function returns a value
   * greater than 0, this will be interperted as the condition being "true". 
   * If the function returns a value of 0 or less, this will be interperted 
   * as the condition being false.
   */
  NumberCondition(
    RCP<ParameterEntry> parameter,
    RCP<const SimpleFunctionObject<T> > func):
    ParameterCondition(parameter), 
    func_(func)
  {}

  virtual ~NumberCondition(){}
  
  //@}

  /** \name Overridden from Condition */
  //@{

  std::string getTypeAttributeValue() const{
    return "NumberCondition(" + TypeNameTraits<T>::name() + ")";
  }
  
  //@}

  /** \name Overridden from ParameterCondition */
  //@{

  /** \brief. */
  bool evaluateParameter() const{
    T value = getValue<T>(*getParameter());
    if(!func_.is_null()){
      value = func_->runFunction(value);
    }
    return value > 0;
  }
  
  //@}
  
  /** \name Getters/Setters */
  //@{

  /** \brief Gets the funciton this NumberCondition is using.
   * Returns null if the NumberCondition is not using one.
   */
  RCP<const SimpleFunctionObject<T> > getFunctionObject() const{
    return func_.getConst();
  }

  //@}

private:

  /** \name Private Members */
  //@{
  
  /** \brief . */
  RCP<const SimpleFunctionObject<T> > func_;
  
  //@}

};


/** \brief Speicialized class for retrieving a dummy object of type
 * NumberCondition.
 *
 * \relates NumberCondition
 */
template<class T>
class DummyObjectGetter<NumberCondition<T> >{

public:

  /** \name Getter Functions */
  //@{

  /** \brief Retrieves a dummy object of type
  * NumberCondition.
  */
  static RCP<NumberCondition<T> > getDummyObject();
  
  //@}
  
};

template<class T>
RCP<NumberCondition<T> >
  DummyObjectGetter<NumberCondition<T> >::getDummyObject()
{
  return rcp(new NumberCondition<T>(
    rcp(new ParameterEntry(ScalarTraits<T>::zero()))));
}


/**
 * \brief A Bool Condition is a Parameter Condition that evaluates
 * whether or not a Boolean parameter is ture.
 * */
class BoolCondition : public ParameterCondition{

public:

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs a Bool Condition.
   *
   * @param parameterName The name of the parameter to be evaluated.
   * @param whenParamEqualsValue Indicates whether the condition 
   * should be true when the evaluation
   * results in a true or when the evaluation results in a false. 
   * When set to true, if the parameter
   * evaluates to true then the condition will evaluate to true. 
   * If set to false if the parameter
   * evaluates to false, then the condition will evaluate to true.
   */
  BoolCondition(
    RCP<ParameterEntry>,
    bool whenParamEqualsValue=true);

  virtual ~BoolCondition(){}
  
  //@}

  /** \name Overridden from Condition */
  //@{

  std::string getTypeAttributeValue() const{
    return "BoolCondition";
  }
  
  //@}

  /** \name Overridden from ParameterCondition */
  //@{

  bool evaluateParameter() const;
  
  //@}

};


/** \brief Speicialized class for retrieving a dummy object of type
 * BoolCondition.
 *
 * \relates BoolCondition
 */
template<>
class DummyObjectGetter<BoolCondition>{

public:

  /** \name Getter Functions */
  //@{

  /** \brief Retrieves a dummy object of type
  * BoolCondition.
  */
  static RCP<BoolCondition > getDummyObject();
  
  //@}
  
};


/**
 * \brief An abstract parent class for all Binary Logic Conditions.
 *
 * Binary Logic Conditions return the result of performing some
 * Logical operation on a set of conditions. Note that although the
 * name implies the evaluation of two conditions, Binary Logic Conditions
 * can actually evaluate an arbiturary number of conditions.
 */
class BinaryLogicalCondition : public Condition{

public:

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs a BinaryLogicCondition
   *
   * \param conditions The conditions to be evaluated.
   */
  BinaryLogicalCondition(ConstConditionList& conditions);

  //@}

  /**
   * \brief Deconstructor for a BinaryLogicCondition
   */
  virtual ~BinaryLogicalCondition(){}

  //@}
  
  /** \name Modifier Functions */

  //@{

  /**
   * \brief Adds a Condition to the list of conditions that will
   * be evaluated by this Binary Logical Condition.
   *
   * \param toAdd The condition to be added to the list of
   * conditions this Binary Logic Condition will evaluate.
   */
  void addCondition(RCP<const Condition> toAdd);

  //@}

  //! @name Attribute/Query Methods 
  //@{

  /**
   * \brief Applies a Binary Logic operator to two operands and returns the
   * result.
   *
   * \param op1 The first operand.
   * \param op2 The second operand.
   * \return The result of applying a binary logical operator to
   * the two operands.
   */
  virtual bool applyOperator(bool op1, bool op2) const = 0;

  /**
   * \brief Gets a list of all conditions that are a part of this 
   * BinaryLogicalCondition/
   */
  inline
  const ConstConditionList& getConditions() const{
    return conditions_;
  }

  //@}

  /** \name Overridden from Condition */
  //@{

  /** \brief . */
  virtual bool isConditionTrue() const;

  /** \brief . */
  bool containsAtLeasteOneParameter() const;

  /** \brief . */
  Dependency::ConstParameterEntryList getAllParameters() const;

  //@}

private:

  /** \name Private Members */
  //@{
  
  /*
   * \brief A list of conditions on which to perform some logic operation.
   */
  ConstConditionList conditions_;

  //@}

};

/**
 * \brief A Binary Logic Condition that returns the result
 * or perfroming a logical OR on the conditions.
 */
class OrCondition : public BinaryLogicalCondition{

public:

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs an Or Condition
   *
   * @param conditions The conditions to be evaluated.
   */
  OrCondition(ConstConditionList& conditions);

  /**
   * \brief Deconstructs an Or Condition.
   */
  virtual ~OrCondition(){}

  //@}

  /** \name Overridden from Condition */
  //@{

  std::string getTypeAttributeValue() const{
    return "OrCondition";
  }
  
  //@}

  /** \name Overridden from BinaryLogicalCondition */
  //@{

  /** \brief . */
  bool applyOperator(bool op1, bool op2) const;
  
  //@}

};


/** \brief Speicialized class for retrieving a dummy object of type
 * OrCondition.
 *
 * \relates OrCondition
 */
template<>
class DummyObjectGetter<OrCondition>{

public:

  /** \name Getter Functions */
  //@{

  /** \brief Retrieves a dummy object of type
  * OrCondition.
  */
  static RCP<OrCondition> getDummyObject();
  
  //@}
  
};


/**
 * \brief A Binary Logic Condition that returns the result
 * or perfroming a logical AND on the conditions.
 */
class AndCondition : public BinaryLogicalCondition{

public:

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs an And Condition
   *
   * @param conditions The conditions to be evaluated.
   */
  AndCondition(ConstConditionList& conditions);

  /**
   * \brief Deconstructs an And Condition.
   */
  virtual ~AndCondition(){}
  
  //@}

  /** \name Overridden from Condition */
  //@{

  std::string getTypeAttributeValue() const{
    return "AndCondition";
  }
  
  //@}


  /** \name Overridden from BinaryLogicalCondition */
  //@{

  /** \brief . */
  bool applyOperator(bool op1, bool op2) const;
  
  //@}

};


/** \brief Speicialized class for retrieving a dummy object of type
 * AndCondition.
 *
 * \relates AndCondition
 */
template<>
class DummyObjectGetter<AndCondition>{

public:

  /** \name Getter Functions */
  //@{

  /** \brief Retrieves a dummy object of type
  * AndCondition.
  */
  static RCP<AndCondition > getDummyObject();
  
  //@}
  
};


/**
 * \brief A Binary Logic Condition that returns the result
 * or perfroming a logical EQUALS on the conditions.
 */
class EqualsCondition : public BinaryLogicalCondition{

public:

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs an Equals Condition
   *
   * @param conditions The conditions to be evaluated.
   */
  EqualsCondition(ConstConditionList& conditions);

  /**
   * \brief Deconstructs an Equals Condition.
   */
  virtual ~EqualsCondition(){}
  
  //@}

  /** \name Overridden from Condition */
  //@{

  std::string getTypeAttributeValue() const{
    return "EqualsCondition";
  }
  
  //@}

  /** \name Overridden from BinaryLogicalCondition */
  //@{

  /** \brief . */
  bool applyOperator(bool op1, bool op2) const;
  
  //@}

};


/** \brief Speicialized class for retrieving a dummy object of type
 * EqualsCondition.
 *
 * \relates EqualsCondition
 */
template<>
class DummyObjectGetter<EqualsCondition>{

public:

  /** \name Getter Functions */
  //@{

  /** \brief Retrieves a dummy object of type
  * EqualsCondition.
  */
  static RCP<EqualsCondition > getDummyObject();
  
  //@}
  
};


/**
 * \brief A Not condition returns the result of
 * performing a logical NOT on a given
 * condition.
 */
class NotCondition : public Condition{

public:

  /** \name Constructors/Destructor */
  //@{

  /**
   * \brief Constructs a Not Condition
   *
   * @param condition The condition to be evaluated.
   */
  NotCondition(RCP<const Condition> condition);

  /**
   * \brief Deconstructs a Not Condition.
   */
  virtual ~NotCondition(){}
  
  //@}

  /** \name Attribute/Query Functions */
  //@{

  /** \brief Retrieve the child condition */
  RCP<const Condition> getChildCondition() const{
    return childCondition_;
  }
  
  //@}

  /** \name Overridden from Condition */
  //@{

  /** \brief . */
  bool isConditionTrue() const;

  /** \brief . */
  bool containsAtLeasteOneParameter() const;

  /** \brief . */
  Dependency::ConstParameterEntryList getAllParameters() const;

  std::string getTypeAttributeValue() const{
    return "NotCondition";
  }
  
  //@}

private:

  /** \name Private Members */
  //@{
  
  /**
   * The condition on which to perfrom the logical NOT.
   */
  RCP<const Condition> childCondition_;
  
  //@}

};


/** \brief Speicialized class for retrieving a dummy object of type
 * NotCondition.
 *
 * \relates NotCondition
 */
template<>
class DummyObjectGetter<NotCondition>{

public:

  /** \name Getter Functions */
  //@{

  /** \brief Retrieves a dummy object of type
  * NotCondition.
  */
  static RCP<NotCondition > getDummyObject();
  
  //@}
  
};

} //namespace Teuchos


#endif //TEUCHOS_STANDARDCONDITION_HPP_