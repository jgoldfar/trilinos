/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#include <stk_util/unit_test_support/stk_utest_macros.hpp>
#include <Shards_BasicTopologies.hpp>

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/Entity.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/Selector.hpp>
#include <stk_mesh/base/GetBuckets.hpp>

#include <stk_mesh/fem/BoundaryAnalysis.hpp>
#include <stk_mesh/fem/SkinMesh.hpp>
#include <stk_mesh/fem/EntityRanks.hpp>

#include <unit_tests/UnitTestGridMeshWithShellFixture.hpp>

#include <iomanip>
#include <algorithm>

class UnitTestStkMeshSkinning {
public:
  UnitTestStkMeshSkinning(stk::ParallelMachine pm) : m_comm(pm),  m_num_procs(0), m_rank(0)
  {
    m_num_procs = stk::parallel_machine_size( m_comm );
    m_rank = stk::parallel_machine_rank( m_comm );
  }

  void test_skinning();

  stk::ParallelMachine m_comm;
  int m_num_procs;
  int m_rank;
};

namespace {

STKUNIT_UNIT_TEST( UnitTestStkMeshSkinning , testUnit )
{
  UnitTestStkMeshSkinning unit(MPI_COMM_WORLD);
  unit.test_skinning();
}

} //end namespace

void UnitTestStkMeshSkinning::test_skinning()
{
  // This test will only work for np=1
  if (m_num_procs > 1) {
    return;
  }

  GridMeshWithShellFixture grid_mesh(MPI_COMM_WORLD);

  stk::mesh::BulkData& bulk_data = grid_mesh.bulk_data();

  // Create a part for the skin
  stk::mesh::Part & skin_part =
    grid_mesh.meta_data().declare_part("skin_part");
  grid_mesh.meta_data().commit();

  grid_mesh.bulk_data().modification_begin();
  grid_mesh.generate_grid();
  grid_mesh.bulk_data().modification_end();

  // skin the boundary
  stk::mesh::skin_mesh(bulk_data, stk::mesh::Face, &skin_part);

  // Grab the skin entities
  stk::mesh::Selector skin_selector(skin_part);
  const std::vector<stk::mesh::Bucket*>& edge_buckets =
    bulk_data.buckets(stk::mesh::Edge);
  std::vector<stk::mesh::Entity*> skin_entities;
  stk::mesh::get_selected_entities(skin_selector, edge_buckets, skin_entities);

  unsigned num_expected_skin_entites = 16;
  STKUNIT_EXPECT_EQUAL(num_expected_skin_entites, skin_entities.size());

  std::vector<std::pair<unsigned, unsigned> > results;
  std::vector<std::pair<unsigned, unsigned> > expected_results;

  // first item is the edge id of the skin entity, second is the id of the face
  // it's related to.
  expected_results.push_back(std::pair<unsigned, unsigned>(1, 1));
  expected_results.push_back(std::pair<unsigned, unsigned>(2, 1));
  expected_results.push_back(std::pair<unsigned, unsigned>(3, 2));
  expected_results.push_back(std::pair<unsigned, unsigned>(4, 3));
  expected_results.push_back(std::pair<unsigned, unsigned>(5, 4));
  expected_results.push_back(std::pair<unsigned, unsigned>(6, 5));
  expected_results.push_back(std::pair<unsigned, unsigned>(7, 9));
  expected_results.push_back(std::pair<unsigned, unsigned>(8, 13));
  expected_results.push_back(std::pair<unsigned, unsigned>(9, 13));
  expected_results.push_back(std::pair<unsigned, unsigned>(10, 14));
  expected_results.push_back(std::pair<unsigned, unsigned>(11, 15));
  expected_results.push_back(std::pair<unsigned, unsigned>(12, 16));
  expected_results.push_back(std::pair<unsigned, unsigned>(13, 42));
  expected_results.push_back(std::pair<unsigned, unsigned>(14, 43));
  expected_results.push_back(std::pair<unsigned, unsigned>(15, 44));
  expected_results.push_back(std::pair<unsigned, unsigned>(15, 46));
  expected_results.push_back(std::pair<unsigned, unsigned>(16, 45));
  expected_results.push_back(std::pair<unsigned, unsigned>(16, 47));

  for (std::vector<stk::mesh::Entity*>::const_iterator
       itr = skin_entities.begin(); itr != skin_entities.end(); ++itr) {
    stk::mesh::PairIterRelation upward_relation_itr =
      (*itr)->relations(stk::mesh::Face);
    for ( ; !upward_relation_itr.empty() ; ++upward_relation_itr ) {
      results.push_back(std::pair<unsigned, unsigned>
                        ((*itr)->identifier(),
                         upward_relation_itr->entity()->identifier()));
    }
  }

  STKUNIT_EXPECT_TRUE(results == expected_results);
}