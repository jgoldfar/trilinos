// Some Macro Magic to ensure that if CUDA and KokkosCompat are enabled
// only the .cu version of this file is actually compiled
#include <Tpetra_config.h>
#ifdef HAVE_TPETRA_KOKKOSCOMPAT
#include <KokkosCore_config.h>
#ifdef KOKKOS_HAVE_CUDA
#define KOKKOS_USE_CUDA_BUILD
#include "Map/Map_Bug6051.cpp"
#undef KOKKOS_USE_CUDA_BUILD
#endif
#endif
