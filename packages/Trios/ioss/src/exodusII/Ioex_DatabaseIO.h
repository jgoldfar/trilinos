 /*--------------------------------------------------------------------*/
/*    Copyright 2000 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/

// -*- Mode: c++ -*-
#ifndef SIERRA_Ioex_DatabaseIO_h
#define SIERRA_Ioex_DatabaseIO_h

#include <Ioss_DatabaseIO.h>
#include <Ioss_Field.h>
#include <Ioss_DBUsage.h>
#include <Ioss_Map.h>
#include <Ioss_Utils.h>

#include <exodusII.h>

#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <time.h>

namespace Ioss {
   class GroupingEntity;
   class Region;
   class EntityBlock;
   class NodeBlock;
   class FaceBlock;
   class ElementBlock;
   class NodeSet;
   class EdgeSet;
   class FaceSet;
   class CommSet;
   class ElementTopology;
}

namespace Ioex {
  struct CommunicationMetaData;

  // Used for variable name index mapping
  typedef std::map<std::string, int, std::less<std::string> > VariableNameMap;
  typedef VariableNameMap::value_type VNMValuePair;

  // Used to store reduction variables
  typedef std::vector<double> ValueContainer;

  // Used for persistent entity IDs
  // The set contains a pair of <int, int>. The first int denotes the entity type:
  // EX_ELEM_BLOCK element block, EX_NODE_SET nodeset, EX_SIDE_SET sideset, 0 commset, EX_NODAL nodeblock.
  // The 'int' is the entity id.
  // The set is used for output databases to ensure that there are no id collisions.
  typedef std::set<std::pair<int, int> > EntityIdSet;

  class DatabaseIO : public Ioss::DatabaseIO
    {
    public:
      DatabaseIO(Ioss::Region *region, const std::string& filename,
		 Ioss::DatabaseUsage db_usage, MPI_Comm communicator);
      ~DatabaseIO();

      // Check to see if database state is ok...
      bool ok(bool write_message = false) const;

      // Check capabilities of input/output database...
      bool supports_nodal_fields()    const {return true;}
      bool supports_edge_fields()     const {return true;}
      bool supports_face_fields()     const {return true;}
      bool supports_element_fields()  const {return true;}
      bool supports_nodelist_fields() const {return true;}

      int    node_local_to_global(int local)  const;

      /*!
       * Determine the local position of the node with the global id
       * 'global'.  If 'must_exist' is false, then the global id possibly
       * does not exist in the map; otherwise, it must exist and will
       * throw an exception if not found.
       */
      int    node_global_to_local(int global, bool must_exist) const;
      int element_local_to_global(int local)  const;
      int element_global_to_local(int global) const;

      bool begin(Ioss::State state);
      bool   end(Ioss::State state);

      bool begin_state(Ioss::Region *region, int state, double time);
      bool   end_state(Ioss::Region *region, int state, double time);
      void get_step_times();

      std::string title()               const     {return databaseTitle;}
      int    spatial_dimension()   const     {return spatialDimension;}
      int    node_count()          const     {return nodeCount;}
      int    edge_count()          const     {return 0;}
      int    face_count()          const     {return 0;}
      int    element_count()       const     {return elementCount;}
      int    node_block_count()    const     {return nodeBlockCount;}
      int    element_block_count() const     {return elementBlockCount;}
      int    faceset_count()       const     {return sidesetCount;}
      int    edgeset_count()       const     {return 0;}
      int    nodeset_count()       const     {return nodesetCount;}
      int    maximum_symbol_length() const {return 32;}

      void get_block_adjacencies(const Ioss::ElementBlock *eb,
				 std::vector<std::string> &block_adjacency) const;

      void compute_block_membership(int id, std::vector<std::string> &block_membership) const;
      void compute_block_membership(Ioss::EntityBlock *efblock,
				    std::vector<std::string> &block_membership) const;

    private:
      int get_field_internal(const Ioss::Region* reg, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int get_field_internal(const Ioss::ElementBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::FaceBlock* fb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::EdgeBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::NodeBlock* nb, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int get_field_internal(const Ioss::NodeSet* ns, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::EdgeSet* es, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::FaceSet* fs, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int get_field_internal(const Ioss::CommSet* cs, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int put_field_internal(const Ioss::Region* reg, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int put_field_internal(const Ioss::ElementBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::FaceBlock* fb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::EdgeBlock* eb, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::NodeBlock* nb, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      int put_field_internal(const Ioss::NodeSet* ns, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::EdgeSet* es, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::FaceSet* fs, const Ioss::Field& field,
			     void *data, size_t data_size) const;
      int put_field_internal(const Ioss::CommSet* cs, const Ioss::Field& field,
			     void *data, size_t data_size) const;

      // Private member functions
      DatabaseIO(const DatabaseIO& from); // do not implement
      DatabaseIO& operator=(const DatabaseIO& from); // do not implement

      virtual void openDatabase() const {
	get_file_pointer();
      }

      virtual void closeDatabase() const {
	free_file_pointer();
      }

      int get_file_pointer() const; // Open file and set exodusFilePtr.
      int free_file_pointer() const; // Close file and set exodusFilePtr.

      void put_qa();
      void put_info();
      int read_nodal_coordinates();
      void read_elements(const Ioss::ElementBlock& block);

      void compute_block_adjacencies() const;

      // Metadata-related functions.
      void read_meta_data();
      void read_communication_metadata();

      int read_transient_field(ex_entity_type type, const Ioss::Field& field,
			       const Ioss::GroupingEntity *ge,
			       void *variables) const;

      // Handles subsetting of edge/face blocks.
      int read_ss_transient_field(const Ioss::Field& field,
				  int id, void *variables,
				  std::vector<int> &is_valid_face) const;

      // Should be made more generic again so can rejoin with write_element_transient field
      void write_nodal_transient_field(ex_entity_type type, const Ioss::Field& field,
				       const Ioss::NodeBlock *ge,
				       int count, void *variables) const;
      // Should be made more generic again so can rejoin with write_nodal_transient field
      void write_entity_transient_field(ex_entity_type type, const Ioss::Field& field,
					const Ioss::GroupingEntity *ge,
					int count, void *variables) const;
      void write_meta_data();
      void write_attribute_names();
      void gather_communication_metadata(Ioex::CommunicationMetaData *meta);
      void write_results_metadata();

      void generate_truth_table(ex_entity_type type);
      void generate_element_truth_table();
      void generate_nodeset_truth_table();
      void generate_sideset_truth_table();

      void output_results_names(ex_entity_type type, VariableNameMap &variables) const;
      int  gather_names(ex_entity_type type, const Ioss::GroupingEntity *ge,
			int index, bool reduction);

      // Read related metadata and store it in the region...
      void read_region();
      void get_nodeblocks();
      void get_elemblocks();
      void get_facesets();
      void get_edgesets();
      void get_edge_face_sets(int topology_dimension);
      void get_nodesets();
      void get_commsets();


      // ID Mapping functions.
      const Ioss::MapContainer& get_node_map()            const;
      const Ioss::MapContainer& get_element_map()         const;

      // Internal data handling
      void build_element_reorder_map(int start, int count);
      void build_node_reorder_map(int *new_ids, int count);

      int handle_node_ids(int* ids, size_t num_to_get);
      int handle_element_ids(const Ioss::ElementBlock *eb, int* ids, size_t num_to_get);

      int add_results_fields(ex_entity_type type, Ioss::GroupingEntity *entity,
			     int count, int position=0);
      int get_face_connectivity(const Ioss::EntityBlock* fb, int id, int face_count,
				int *fconnect, size_t data_size) const;
      int get_face_distributions(const Ioss::EntityBlock* fb, int id,
				 int face_count, double *dist_fact, size_t data_size) const;

      void add_region_fields();
      void store_reduction_field(ex_entity_type type,
				 const Ioss::Field& field,
				 const Ioss::GroupingEntity *ge,
				 void *variables) const;

      void get_reduction_field(ex_entity_type type,
			       const Ioss::Field& field,
			       const Ioss::GroupingEntity *ge,
			       void *variables) const;
      void write_reduction_fields() const;
      void read_reduction_fields() const;

      int get_face_edge_field(const Ioss::EntityBlock* ef_blk,
			      const Ioss::Field& field,
			      void *data, size_t data_size) const;
      int put_face_edge_field(const Ioss::EntityBlock* fb,
			      const Ioss::Field& field,
			      void *data, size_t data_size) const;

      // Handle special output time requests -- primarily restart (cycle, keep, overwrite)
      // Given the global region step, return the step on the database...
      int get_database_step(int global_step) const;

      void finalize_write(double sim_time);

      void add_attribute_fields(Ioss::ElementBlock *block,
				int attribute_count,
				const std::string& type);

      // Private member data...
      mutable int exodusFilePtr;
      mutable EntityIdSet ids_;

      std::string databaseTitle;
      int exodusMode;

      int spatialDimension;
      int nodeCount;
      int elementCount;

      int nodeBlockCount;
      int elementBlockCount;
      int nodesetCount;
      int sidesetCount;

      // Communication Set Data
      int *nodeCmapIds;
      int *nodeCmapNodeCnts;
      int *elemCmapIds;
      int *elemCmapElemCnts;
      int commsetNodeCount;
      int commsetElemCount;

      int *elementTruthTable;
      int *nodesetTruthTable;
      int *sidesetTruthTable;

      // Bulk Data

      // MAPS -- Used to convert from local exodusII ids/names to Sierra
      // database global ids/names

      //---Node Map -- Maps internal (1..NUMNP) ids to global ids used on the
      //               sierra side.   global = nodeMap[local]
      // nodeMap[0] contains: -1 if sequential, 0 if ordering unknown, 1
      // if nonsequential
      mutable Ioss::MapContainer        nodeMap;
      mutable Ioss::MapContainer        reorderNodeMap;
      mutable Ioss::ReverseMapContainer reverseNodeMap;
      // (local==global)

      //---Element Map -- Maps internal (1..NUMEL) ids to global ids used on the
      //               sierra side.   global = elementMap[local]
      // elementMap[0] contains: -1 if sequential, 0 if ordering unknown,
      // 1 if nonsequential
      mutable Ioss::MapContainer        elementMap;
      mutable Ioss::MapContainer        reorderElementMap;
      mutable Ioss::ReverseMapContainer reverseElementMap;

      // --- Nodal/Element/Attribute Variable Names -- Maps from sierra
      // field names to index of nodal/element/attribute variable in
      // exodusII. Note that the component suffix of the field is added on
      // prior to searching the map for the index.  For example, given the
      // Sierra field 'displ' which is a VECTOR_3D, the names stored in
      // 'elementMap' would be 'displ_x', 'displ_y' and 'displ_z'.  All
      // names are converted to lowercase.

      VariableNameMap nodalVariables;
      VariableNameMap elementVariables;
      VariableNameMap elementAttributes;
      VariableNameMap nodesetVariables;
      VariableNameMap sidesetVariables;
      VariableNameMap globalVariables;

      // Similar for entity attributes (currently only element blocks); however, the
      // names are not mapped to the individual components.
      // The other difference is that that name stored in the map is a
      // combination of the entity-block name and the attribute name
      // so we can use a single VariableNameMap for the region instead
      // of one-per entity block.
      VariableNameMap attributeNames;

      mutable ValueContainer  globalValues;

      mutable std::vector<std::vector<bool> > blockAdjacency;

      time_t timeLastFlush;

      mutable bool sequentialNG2L; // true if reverse node map is sequential
      mutable bool sequentialEG2L; // true if reverse element map is
				   // sequential (local==global)
      mutable bool fileExists; // False if file has never been opened/created
      mutable bool minimizeOpenFiles;

      mutable bool blockAdjacenciesCalculated; // True if the lazy creation of
                                              // block adjacencies has been calculated.
    };

  // ------------------------------------------------------------------------
  // Node and Element mapping functions.  The ExodusII database
  // stores ids in a local-id system (1..NUMNP), (1..NUMEL) but
  // Sierra wants entities in a global system. These routines
  // take care of the mapping from local <-> global

  typedef std::vector<Ioss::IdPair>::const_iterator RMapI;
  inline int DatabaseIO::node_global_to_local(int global, bool must_exist) const
    {
      if (nodeMap.empty()) {
	get_node_map();
      }
      int local = global;
      if (!sequentialNG2L) {
	std::pair<RMapI, RMapI> iter = std::equal_range(reverseNodeMap.begin(),
							reverseNodeMap.end(),
							global,
							Ioss::IdPairCompare());
	if (iter.first != iter.second)
	  local = (iter.first)->second;
	else
	  local = 0;
	if (must_exist && iter.first == iter.second) {
	  std::ostringstream errmsg;
	  errmsg << "Node with global id equal to " << global
		 << " does not exist in this mesh on this processor\n";
	  IOSS_ERROR(errmsg);
	}
      } else if (!must_exist && global > nodeCount) {
	local = 0;
      }
      if (local > nodeCount || (local <= 0 && must_exist)) {
	std::ostringstream errmsg;
	errmsg << "Node with global id equal to " << global
	       << " returns a local id of " << local
	       << " which is invalid. This should not happen, please report.\n";
	IOSS_ERROR(errmsg);
      }
      return local;
    }

  inline int DatabaseIO::element_global_to_local(int global) const
    {
      if (elementMap.empty()) {
	get_element_map();
      }
      int local = global;
      if (!sequentialEG2L) {
	std::pair<RMapI, RMapI> iter = std::equal_range(reverseElementMap.begin(),
							reverseElementMap.end(),
							global,
							Ioss::IdPairCompare());
	if (iter.first == iter.second) {
	  std::ostringstream errmsg;
	  errmsg << "Element with global id equal to " << global
		 << " does not exist in this mesh on this processor\n";
	  IOSS_ERROR(errmsg);
	}
	local = (iter.first)->second;
      }
      if (local > elementCount || local <= 0) {
	std::ostringstream errmsg;
	errmsg << "Element with global id equal to " << global
	       << " returns a local id of " << local
	       << " which is invalid. This should not happen, please report.\n";
	IOSS_ERROR(errmsg);
      }
      return local;
    }
}
#endif
