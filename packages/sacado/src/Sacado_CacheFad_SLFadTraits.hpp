// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef SACADO_CACHEFAD_SLFADTRAITS_HPP
#define SACADO_CACHEFAD_SLFADTRAITS_HPP

#include "Sacado_Traits.hpp"
#include <sstream>

// Forward declarations
namespace Sacado {
  namespace CacheFad {
    template <typename T, int Num> class SLFad;
  }
}

namespace Sacado {

  //! Specialization of %Promote to SLFad types
  template <typename ValueT, int Num>
  struct Promote< CacheFad::SLFad<ValueT,Num>, 
		  CacheFad::SLFad<ValueT,Num> > {
    typedef CacheFad::SLFad<ValueT,Num> type;
  };

  //! Specialization of %Promote to SLFad types
  template <typename ValueT, int Num, typename R>
  struct Promote< CacheFad::SLFad<ValueT,Num>, R > {
    typedef typename ValueType< CacheFad::SLFad<ValueT,Num> >::type value_type_l;
    typedef typename ValueType<R>::type value_type_r;
    typedef typename Promote<value_type_l,value_type_r>::type value_type;

    typedef CacheFad::SLFad<value_type,Num> type;
  };

  //! Specialization of %Promote to SLFad types
  template <typename L, typename ValueT, int Num>
  struct Promote< L, CacheFad::SLFad<ValueT, Num> > {
  public:

    typedef typename ValueType<L>::type value_type_l;
    typedef typename ValueType< CacheFad::SLFad<ValueT,Num> >::type value_type_r;
    typedef typename Promote<value_type_l,value_type_r>::type value_type;

    typedef CacheFad::SLFad<value_type,Num> type;
  };

  //! Specialization of %ScalarType to SLFad types
  template <typename ValueT, int Num>
  struct ScalarType< CacheFad::SLFad<ValueT,Num> > {
    typedef typename CacheFad::SLFad<ValueT,Num>::ScalarT type;
  };

  //! Specialization of %ValueType to SLFad types
  template <typename ValueT, int Num>
  struct ValueType< CacheFad::SLFad<ValueT,Num> > {
    typedef ValueT type;
  };

  //! Specialization of %IsADType to SLFad types
  template <typename ValueT, int Num>
  struct IsADType< CacheFad::SLFad<ValueT,Num> > {
    static const bool value = true;
  };

  //! Specialization of %IsADType to SLFad types
  template <typename ValueT, int Num>
  struct IsScalarType< CacheFad::SLFad<ValueT,Num> > {
    static const bool value = false;
  };

  //! Specialization of %Value to SLFad types
  template <typename ValueT, int Num>
  struct Value< CacheFad::SLFad<ValueT,Num> > {
    typedef typename ValueType< CacheFad::SLFad<ValueT,Num> >::type value_type;
    static const value_type& eval(const CacheFad::SLFad<ValueT,Num>& x) { 
      return x.val(); }
  };

  //! Specialization of %ScalarValue to SLFad types
  template <typename ValueT, int Num>
  struct ScalarValue< CacheFad::SLFad<ValueT,Num> > {
    typedef typename ValueType< CacheFad::SLFad<ValueT,Num> >::type value_type;
    typedef typename ScalarType< CacheFad::SLFad<ValueT,Num> >::type scalar_type;
    static const scalar_type& eval(const CacheFad::SLFad<ValueT,Num>& x) { 
      return ScalarValue<value_type>::eval(x.val()); }
  };

  //! Specialization of %StringName to SLFad types
  template <typename ValueT, int Num>
  struct StringName< CacheFad::SLFad<ValueT,Num> > {
    static std::string eval() { 
      std::stringstream ss;
      ss << "Sacado::CacheFad::SLFad< " 
	 << StringName<ValueT>::eval() << ", " << Num << " >";
      return ss.str(); 
    }
  };

  //! Specialization of %IsEqual to DFad types
  template <typename ValueT, int Num>
  struct IsEqual< CacheFad::SLFad<ValueT,Num> > {
    static bool eval(const CacheFad::SLFad<ValueT,Num>& x, 
		     const CacheFad::SLFad<ValueT,Num>& y) {
      return x.isEqualTo(y);
    }
  };

  //! Specialization of %IsStaticallySized to SLFad types
  template <typename ValueT, int Num>
  struct IsStaticallySized< CacheFad::SLFad<ValueT,Num> > {
    static const bool value = true;
  };

} // namespace Sacado

// Define Teuchos traits classes
#ifdef HAVE_SACADO_TEUCHOS
#include "Teuchos_PromotionTraits.hpp"
#include "Teuchos_ScalarTraits.hpp"
#include "Sacado_Fad_ScalarTraitsImp.hpp"

namespace Teuchos {

  //! Specialization of %Teuchos::PromotionTraits to SLFad types
  template <typename ValueT, int Num>
  struct PromotionTraits< Sacado::CacheFad::SLFad<ValueT,Num>, 
			  Sacado::CacheFad::SLFad<ValueT,Num> > {
    typedef typename Sacado::Promote< Sacado::CacheFad::SLFad<ValueT,Num>,
				      Sacado::CacheFad::SLFad<ValueT,Num> >::type
    promote;
  };

  //! Specialization of %Teuchos::PromotionTraits to SLFad types
  template <typename ValueT, int Num, typename R>
  struct PromotionTraits< Sacado::CacheFad::SLFad<ValueT,Num>, R > {
    typedef typename Sacado::Promote< Sacado::CacheFad::SLFad<ValueT,Num>, R >::type 
    promote;
  };

  //! Specialization of %Teuchos::PromotionTraits to SLFad types
  template <typename L, typename ValueT, int Num>
  struct PromotionTraits< L, Sacado::CacheFad::SLFad<ValueT,Num> > {
  public:
    typedef typename Sacado::Promote< L, Sacado::CacheFad::SLFad<ValueT,Num> >::type 
    promote;
  };

  //! Specializtion of %Teuchos::ScalarTraits
  template <typename ValueT, int Num>
  struct ScalarTraits< Sacado::CacheFad::SLFad<ValueT,Num> > :
    public Sacado::Fad::ScalarTraitsImp< Sacado::CacheFad::SLFad<ValueT,Num> >
  {};

  //! Specialization of %Teuchos::SerializationTraits
  template <typename Ordinal, typename ValueT, int Num>
  struct SerializationTraits<Ordinal, Sacado::CacheFad::SLFad<ValueT,Num> > :
    public Sacado::Fad::SerializationTraitsImp< Ordinal, 
						Sacado::CacheFad::SLFad<ValueT,Num> > 
  {};

  //! Specialization of %Teuchos::ValueTypeSerializer
  template <typename Ordinal, typename ValueT, int Num>
  struct ValueTypeSerializer<Ordinal, Sacado::CacheFad::SLFad<ValueT,Num> > :
    public Sacado::Fad::SerializerImp< Ordinal, 
				       Sacado::CacheFad::SLFad<ValueT,Num>,
				       ValueTypeSerializer<Ordinal,ValueT> > 
  {
    typedef Sacado::CacheFad::SLFad<ValueT,Num> FadType;
    typedef ValueTypeSerializer<Ordinal,ValueT> ValueSerializer;
    typedef Sacado::Fad::SerializerImp< Ordinal,FadType,ValueSerializer> Base;
    ValueTypeSerializer(const Teuchos::RCP<const ValueSerializer>& vs,
			Ordinal sz = 0) :
      Base(vs, sz) {}
  };
}
#endif // HAVE_SACADO_TEUCHOS

#endif // SACADO_CACHEFAD_SLFADTRAITS_HPP
