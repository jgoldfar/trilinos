#include "MeshImpl.hpp"

#include <fstream>
#include <string>

#ifdef MSQ_USING_EXODUS
#include "exodusII.h"
#endif

namespace
{
  template<typename X> class MeshImpl_EntityIterator : public Mesquite::VertexIterator
  {
  public:
    MeshImpl_EntityIterator(X* array,
                            size_t element_count) 
        : mArray(array),
          mCount(element_count),
          mIndex(0)
      {}
    
      // Moves the iterator back to the first
      // entity in the list.
    virtual void restart()
      {
        mIndex = 0;
      }
    
      // *iterator.  Return the handle currently
      // being pointed at by the iterator.
    virtual Mesquite::Mesh::EntityHandle operator*() const
      {
        return is_at_end() ? NULL : reinterpret_cast<void*>(mArray+mIndex);
      }
    
      // ++iterator
    virtual void operator++()
      {
        ++mIndex;
      }
    
      // iterator++
    virtual void operator++(int)
      {
        mIndex++;
      }
    
      // Returns false until the iterator has
      // been advanced PAST the last entity.
      // Once is_at_end() returns true, *iterator
      // returns NULL.
    virtual bool is_at_end() const
      {
        return (mIndex < mCount ? false : true);
      }
    
  private:
    X *mArray;
    size_t mCount;
    size_t mIndex;
  };
}

Mesquite::MeshImpl::MeshImpl() 
    : vertexArray(NULL),
      elementArray (NULL),
      vertexCount(0),
      elementCount(0),
      onBoundaryBits(NULL),
      vertexMesquiteByte(NULL),
      newVertIndices(NULL),
      v2eOffset(NULL),
      totalVertexUses(0),
      v2E(NULL)
{}

Mesquite::MeshImpl::~MeshImpl() 
{
  delete [] vertexArray;
  delete [] elementArray;
  delete [] onBoundaryBits;
  delete [] vertexMesquiteByte;
  delete [] newVertIndices;
  delete [] v2eOffset;
  delete [] v2E;
}

void Mesquite::MeshImpl::read_vtk(const char* file_name,
                                  Mesquite::MsqError &err)
{
    // Open the file
  std::ifstream ifs(file_name);
  if (!ifs)
  {
    std::string err_msg("file ");
    err_msg += file_name;
    err_msg += " not found";
    err.set_msg(err_msg);
    return;
  }
  
    // Skip the first 4 lines
  char line[256];
  int i;
  for (i = 0; i < 4; ++i)
    ifs.getline(line,256);
  
    // get the number of vertices.
  char word[81];
  ifs >> word;
  if (strcmp(word, "POINTS") != 0)
  {
    std::string err_msg = "MeshImpl::read_vtk: expecting word POINTS, not ";
    err_msg += word;
    err_msg += ".";
    err.set_msg(err_msg);
    return;
  }
  ifs >> vertexCount;
  ifs.getline(line,256);
  
    // Create an array big enough for all the vertices
  vertexArray = new Mesquite::MeshImpl::Vertex[vertexCount];

    // Also create a cache for elements_get_attached_vertices().
  newVertIndices = new size_t[vertexCount];
  memset(newVertIndices, 0, vertexCount*sizeof(size_t));
  
    // Get the coordinates for each vertex
  double* coord_ptr = reinterpret_cast<double*>(vertexArray);
  for(i = 0; i < vertexCount; ++i)
  {
    ifs >> *coord_ptr >> *(coord_ptr + 1) >> *(coord_ptr + 2);
    coord_ptr += 3;
  }
  
    // get the number of regions
  ifs >> word;
  if (strcmp(word, "CELLS") != 0)
  {
    std::string err_msg = "MeshImpl::read_vtk: expecting word CELLS, not ";
    (err_msg += word) += ".";
    err.set_msg(err_msg);
  }
  
  int table_size;
  ifs >> elementCount >> table_size;
  
    // Read the connectivity table
  size_t *connectivity_table = new size_t[table_size];
  size_t *cur_entry = connectivity_table;
  
  for (i = 0; i < table_size; ++i)
  {
    ifs >> *(cur_entry++);
  }
  
    // get the number of CELL_TYPES identifier
  ifs >> word;
  if (strcmp(word, "CELL_TYPES") != 0)
  {
    std::string err_msg = "MeshImpl::read_vtk: expecting word CELL_TYPES, not ";
    (err_msg += word) += ".";
    err.set_msg(err_msg);
    return;
  }
  ifs >> i;
  if (i != elementCount)
  {
    err.set_msg("MeshImpl::read_vtk: nb of CELL_TYPES != nb of CELLS.");
  }
  
    // Store each cell type
  std::vector<size_t> cell_type;
  cell_type.reserve(elementCount);
  for (i=0; i < elementCount; ++i)
    ifs >> cell_type[i];
  
    // Create the array of elements
  elementArray = new MeshImpl::Element[elementCount];
  
    // Create an element for each cell
  totalVertexUses = 0;
  cur_entry = connectivity_table;
  for (i=0; i<elementCount; ++i)
  {
    int j;

    switch (cell_type[i])
    {
      case 5:  // Triangle
        if (*cur_entry++ != 3)
        {
          err.set_msg("MeshImpl::read_vtk: expecting 3 vtx for a triangle.");
          return;
        }
        elementArray[i].mType = Mesquite::TRIANGLE;
        totalVertexUses += 3;
        for (j = 0; j < 3; j++)
        {
          elementArray[i].vertexIndices[j] = *cur_entry++;
        }
        break;
      case 9:  // Quad
        if (*cur_entry++ != 4)
        {
          err.set_msg("MeshImpl::read_vtk: expecting 4 vtx for a Quad.");
          return;
        }
        elementArray[i].mType = Mesquite::QUADRILATERAL;
        totalVertexUses += 4;
        for (j = 0; j < 4; j++)
        {
          elementArray[i].vertexIndices[j] = *cur_entry++;
        }
        break;
      case 10: // Tet
        if (*cur_entry++ != 4)
        {
          err.set_msg("MeshImpl::read_vtk: expecting 4 vtx for a Tet.");
          return;
        }
        elementArray[i].mType = Mesquite::TETRAHEDRON;
        totalVertexUses += 4;
        for (j = 0; j < 4; j++)
        {
          elementArray[i].vertexIndices[j] = *cur_entry++;
        }
        break;
      case 12: // Hex 
        if (*cur_entry++ != 8)
        {
          std::cerr << "MeshImpl::read_vtk: expecting 8 vtx for an Hex.\n";
          return;
        }
        elementArray[i].mType = Mesquite::HEXAHEDRON;
        totalVertexUses += 8;
        for (j = 0; j < 8; j++)
        {
          elementArray[i].vertexIndices[j] = *cur_entry++;
        }
        break;
    }
  }
  
    // Create a set of bits to store whether it's on the boundary or not
  size_t num_bitset_bytes = (vertexCount / 8);
  num_bitset_bytes += (vertexCount % 8) ? 1 : 0;
  onBoundaryBits = new unsigned char[num_bitset_bytes];
  memset(onBoundaryBits, 0, sizeof(unsigned char)*num_bitset_bytes);
  
    // Get the number of BOUNDARY_POINTS,
    // Set them only if they are present.
  ifs >> word;
  if (strcmp(word, "POINT_DATA") == 0)
  {
    int num_boundary_verts;
    ifs >> num_boundary_verts;
    
      // Skip the next 5 words
    for (i = 1; i <= 5; i++)
      ifs >> word;
    
      // See if this vertex is marked as a boundary vertex
    for (int vert_index=0; vert_index < num_boundary_verts; vert_index++)
    {
      int val;
      ifs >> val;
      if (val != 0)
      {
        unsigned char bit_flag = 1 << (vert_index%8);
        onBoundaryBits[vert_index / 8] |= bit_flag;
      }
    }
  }

    // Create a Mesquite byte for each vertex
  vertexMesquiteByte = new unsigned char[vertexCount];
  memset(vertexMesquiteByte, 0, sizeof(unsigned char)*vertexCount);
  
  ifs.close();
}

#ifdef MSQ_USING_EXODUS
void Mesquite::MeshImpl::read_exodus(const char* file_name,
                                     Mesquite::MsqError &err)
{
  int app_float_size = sizeof(double);
  int file_float_size = 0;
  float exo_version = 0;
  int exo_err = 0;
  
    // Open the file
  int file_id = ex_open(file_name, EX_READ, &app_float_size,
                    &file_float_size, &exo_version);

    // Make sure we opened the file correctly
  if (file_id < 0)
  {
    err.set_msg("Unable to open file");
    return;
  }
  
    // make sure the file is saved as doubles
  if (file_float_size != sizeof(double))
  {
    err.set_msg("File saved with float-sized reals.  Can only read files "
                "saved with doubles.");
    return;
  }

  char title[MAX_LINE_LENGTH];
  int dim, vert_count, elem_count, block_count, ns_count, ss_count;
  
    // get info about the file
  exo_err = ex_get_init(file_id, title, &dim, &vert_count,
                        &elem_count, &block_count, &ns_count, &ss_count);

  vertexCount = vert_count;
  elementCount = elem_count;
  
    // get the number of vertices
  
    // get the number of elements by getting the number of
    // blocks, and the number of elements in each block
}
#endif
    
// Returns whether this mesh lies in a 2D or 3D coordinate system.
int Mesquite::MeshImpl::get_geometric_dimension() const
{
  return 3;
}
    
// Returns the number of entities of the indicated type.
size_t Mesquite::MeshImpl::get_total_vertex_count() const
{
  return vertexCount;
}
size_t Mesquite::MeshImpl::get_total_element_count() const
{
  return elementCount;
}
    
// Fills array with handles to all vertices in the mesh.
void Mesquite::MeshImpl::get_all_vertices(
  Mesquite::Mesh::VertexHandle *vert_array,
  size_t array_size)
{
  if (array_size > vertexCount)
    array_size = vertexCount;
  
  for (int i = 0; i < array_size; i++)
    vert_array[i] = vertexArray + i;
}

// Fills array with handles to all elements in the mesh.
void Mesquite::MeshImpl::get_all_elements(
  Mesquite::Mesh::ElementHandle *elem_array,
  size_t array_size)
{
  if (array_size > elementCount)
    array_size = elementCount;
  
  for (int i = 0; i < array_size; i++)
    elem_array[i] = elementArray + i;
}

// Returns a pointer to an iterator that iterates over the
// set of all vertices in this mesh.  The calling code should
// delete the returned iterator when it is finished with it.
// If vertices are added or removed from the Mesh after obtaining
// an iterator, the behavior of that iterator is undefined.
Mesquite::VertexIterator* Mesquite::MeshImpl::vertex_iterator()
{
  return new MeshImpl_EntityIterator<Mesquite::MeshImpl::Vertex>(vertexArray, vertexCount);
}
    
// Returns a pointer to an iterator that iterates over the
// set of all top-level elements in this mesh.  The calling code should
// delete the returned iterator when it is finished with it.
// If elements are added or removed from the Mesh after obtaining
// an iterator, the behavior of that iterator is undefined.
Mesquite::ElementIterator* Mesquite::MeshImpl::element_iterator()
{
  return new MeshImpl_EntityIterator<Mesquite::MeshImpl::Element>(elementArray, elementCount);
}

//************ Vertex Properties ********************
// Returns true or false, indicating whether the vertex
// is allowed to be repositioned.  True indicates that the vertex
// is fixed and cannot be moved.  Note that this is a read-only
// property; this flag can't be modified by users of the
// Mesquite::Mesh interface.
bool Mesquite::MeshImpl::vertex_is_fixed(Mesquite::Mesh::VertexHandle vertex)
{
  return false;
}

// Returns true or false, indicating whether the vertex
// is on the boundary.  Boundary nodes may be treated as
// a special case by some algorithms or culling methods.
// Note that this is a read-only
// property; this flag can't be modified by users of the
// Mesquite::Mesh interface.
bool Mesquite::MeshImpl::vertex_is_on_boundary(
  Mesquite::Mesh::VertexHandle vertex)
{
  size_t index = reinterpret_cast<MeshImpl::Vertex*>(vertex) - vertexArray;
  return (onBoundaryBits[index / 8] & ((unsigned char)(1) << index % 8)) != 0;
}

// Get/set location of a vertex
void Mesquite::MeshImpl::vertex_get_coordinates(
  Mesquite::Mesh::VertexHandle vertex,
  Vector3D &coordinates)
{
  coordinates.set(reinterpret_cast<double*>(vertex));
}

void Mesquite::MeshImpl::vertex_set_coordinates(
  Mesquite::Mesh::VertexHandle vertex,
  const Vector3D &coordinates)
{
  coordinates.get_coordinates(reinterpret_cast<Vertex*>(vertex)->coords);
}

// Each vertex has a byte-sized flag that can be used to store
// flags.  This byte's value is neither set nor used by the mesh
// implementation.  It is intended to be used by Mesquite algorithms.
// Until a vertex's byte has been explicitly set, its value is 0.
void Mesquite::MeshImpl::vertex_set_byte (
  Mesquite::Mesh::VertexHandle vertex,
  unsigned char byte)
{
  size_t index = reinterpret_cast<Vertex*>(vertex) - vertexArray;
  vertexMesquiteByte[index] = byte;
}

void Mesquite::MeshImpl::vertices_set_byte (
  Mesquite::Mesh::VertexHandle *vert_array,
  unsigned char *byte_array,
  size_t array_size)
{
  for (size_t i = 0; i < array_size; i++)
  {
    size_t index = reinterpret_cast<Vertex*>(vert_array[i]) - vertexArray;
    vertexMesquiteByte[index] = byte_array[i];
  }
}

// Retrieve the byte value for the specified vertex or vertices.
// The byte value is 0 if it has not yet been set via one of the
// *_set_byte() functions.
void Mesquite::MeshImpl::vertex_get_byte(
  Mesquite::Mesh::VertexHandle vertex,
  unsigned char *byte)
{
  size_t index = reinterpret_cast<Vertex*>(vertex) - vertexArray;
  *byte = vertexMesquiteByte[index];
}

void Mesquite::MeshImpl::vertices_get_byte(
  Mesquite::Mesh::VertexHandle *vertex,
  unsigned char *byte_array,
  size_t array_size)
{
  for (size_t i = 0; i < array_size; i++)
  {
    size_t index = reinterpret_cast<Vertex*>(vertex[i]) - vertexArray;
    byte_array[i] = vertexMesquiteByte[index];
  }
}

//**************** Vertex Topology *****************

void Mesquite::MeshImpl::create_vertex_to_element_data()
{
  if (v2E)
    return;
  
  v2eOffset = new size_t[vertexCount + 1];
  v2E = new size_t[totalVertexUses];
  
    // Initialize each use count to zero
  memset(v2eOffset, 0, (vertexCount+1)*sizeof(size_t));
  
  size_t elem_num, vert_num;
  
    // Go through each element, keep track of how many times
    // each vertex is used.
  for (elem_num = elementCount; elem_num--; )
  {
    for (size_t vert_num =
           Mesquite::vertices_in_topology(elementArray[elem_num].mType);
         vert_num--;
         )
    {
      v2eOffset[elementArray[elem_num].vertexIndices[vert_num]]++;
    }
  }
  
    // Convert the uses counts to array offsets.
  elem_num = 0;
  for (vert_num = 0; vert_num < vertexCount; vert_num++)
  {
    size_t temp = v2eOffset[vert_num];
    v2eOffset[vert_num] = elem_num;
    elem_num += temp;
  }
  v2eOffset[vertexCount] = totalVertexUses;

    // Use newVertIndices to store how many elements
    // have already been added to v2E for each vertex.
    // Should already be initialized to zero
  
    // Finally, store the v2E data
  for (elem_num = 0; elem_num < elementCount; elem_num++)
  {
    for (vert_num =
           Mesquite::vertices_in_topology(elementArray[elem_num].mType);
         vert_num--;
         )
    {
      size_t vert_index = elementArray[elem_num].vertexIndices[vert_num];
      v2E[v2eOffset[vert_index] + newVertIndices[vert_index]++] = elem_num;
    }
  }
  
    // Reset newVertIndices to zero
  memset(newVertIndices, 0, vertexCount*sizeof(size_t));
}

// Gets the number of elements attached to this vertex.
// Useful to determine how large the "elem_array" parameter
// of the vertex_get_attached_elements() function must be.
size_t Mesquite::MeshImpl::vertex_get_attached_element_count(
  Mesquite::Mesh::VertexHandle vertex) const
{
  const_cast<Mesquite::MeshImpl*>(this)->create_vertex_to_element_data();
  
  size_t i = reinterpret_cast<MeshImpl::Vertex*>(vertex) - vertexArray;
  return v2eOffset[i+1] - v2eOffset[i];
}

// Gets the elements attached to this vertex.
void Mesquite::MeshImpl::vertex_get_attached_elements(
  Mesquite::Mesh::VertexHandle vertex,
  Mesquite::Mesh::ElementHandle* elem_array,
  size_t sizeof_elem_array)
{
  create_vertex_to_element_data();
  
  size_t index = reinterpret_cast<MeshImpl::Vertex*>(vertex) - vertexArray;
  
  if (sizeof_elem_array > v2eOffset[index+1] - v2eOffset[index])
    sizeof_elem_array = v2eOffset[index+1] - v2eOffset[index];
  
  for ( ; sizeof_elem_array--; )
  {
    elem_array[sizeof_elem_array] =
      elementArray + v2E[v2eOffset[index] + sizeof_elem_array];
  }
}

// Identifies the elements attached to this vertex by returning
// each element's global index.  The element's global index indicates
// where that element can be found in the array returned by
// Mesh::get_all_elements.
void Mesquite::MeshImpl::vertex_get_attached_element_indices(
  Mesquite::Mesh::VertexHandle vertex,
  size_t *index_array,
  size_t sizeof_index_array)
{
  create_vertex_to_element_data();
  
  size_t index = reinterpret_cast<MeshImpl::Vertex*>(vertex) - vertexArray;
  if (sizeof_index_array > v2eOffset[index+1] - v2eOffset[index])
    sizeof_index_array = v2eOffset[index+1] - v2eOffset[index];
  
  for ( ; sizeof_index_array--; )
  {
    index_array[sizeof_index_array] =
      v2E[v2eOffset[index] + sizeof_index_array];
  }
}

// Gets the number of vertices in this element.
// This data can also be found by querying the
// element's topology and getting the number
// of vertices per element for that topology type.
size_t Mesquite::MeshImpl::element_get_attached_vertex_count(
  Mesquite::Mesh::ElementHandle elem) const
{
  return Mesquite::vertices_in_topology(reinterpret_cast<MeshImpl::Element*>(elem)->mType);
}

// Returns the vertices that are part of the topological definition of each
// element in the "elem_handles" array.  When this function is called, the
// following must be true:
//   a) "elem_handles" points at an array of "num_elems" element handles.
//   b) "vert_handles" points at an array of size "sizeof_vert_handles"
//   c) "csr_data" points at an array of size "sizeof_csr_data"
//   d) "csr_offsets" points at an array of size "num_elems+1"
//      
// When this function returns, adjacency information will be stored
// in csr format:
//    a) "vert_handles" stores handles to all vertices found in one
//       or more of the elements.  Each vertex appears only
//       once in "vert_handles", even if it is in multiple elements.
//    b) "sizeof_vert_handles" is set to the number of vertex
//       handles placed into "vert_handles".
//    c) "sizeof_csr_data" is set to the total number of vertex uses (for
//       example, sizeof_csr_data = 6 in the case of 2 TRIANGLES, even if
//       the two triangles share some vertices).
//    c) "csr_offsets" is filled such that csr_offset[i] indicates the location
//       of entity i's first adjacency in "csr_data".  The number of vertices
//       in element i is equal to csr_offsets[i+1] - csr_offsets[i].  For this
//       reason, csr_offsets[num_elems] is set to the new value of
//       "sizeof_csr_data".
//    d) "csr_data" stores integer offsets which give the location of
//       each adjacency in the "vert_handles" array.
//
// As an example of how to use this data, you can get the handle of the first
// vertex in element #3 like this:
//   VertexHandle vh = vert_handles[ csr_data[ csr_offsets[3] ] ]
//
// and the second vertex of element #3 like this:
//   VertexHandle vh = vert_handles[ csr_data[ csr_offsets[3]+1 ] ]
// 
void Mesquite::MeshImpl::elements_get_attached_vertices(
  Mesquite::Mesh::ElementHandle *elem_handles,
  size_t num_elems,
  Mesquite::Mesh::VertexHandle *vert_handles,
  size_t &sizeof_vert_handles,
  size_t *csr_data,
  size_t &sizeof_csr_data,
  size_t *csr_offsets)
{
  if (num_elems == 0)
    return;
  
  size_t total_verts = 0;
  csr_offsets[0] = 0;
  
    // for each element
  for (size_t i = 0; i < num_elems; i++)
  {
    MeshImpl::Element* elem =
      reinterpret_cast<MeshImpl::Element*>(elem_handles[i]);
    size_t verts_in_elem = Mesquite::vertices_in_topology(elem->mType);
    csr_offsets[i+1] = csr_offsets[i] + verts_in_elem;

      // Make sure we've got enough room in csr_data
    if (sizeof_csr_data < csr_offsets[i+1])
        // Error!!!
      return;
    
      // for each vertex in this element
    for (size_t j = 0; j < verts_in_elem; j++)
    {
        // Get the index for this vertex
      size_t vert_index = elem->vertexIndices[j];
      size_t new_vert_index = newVertIndices[vert_index];
      if (new_vert_index == 0)
      {
        new_vert_index = newVertIndices[vert_index] = ++total_verts;
        if (total_verts > sizeof_vert_handles) // ERROR!!!
          return;
        vert_handles[total_verts-1] =
          reinterpret_cast<Mesquite::Mesh::VertexHandle>(vertexArray+vert_index);
      }
      
        // Place that vertex into the csr_data array
      csr_data[csr_offsets[i] + j] = new_vert_index-1;
    }
  }
  
    // Set the amount of data we are returning
  sizeof_csr_data = csr_offsets[num_elems];
  sizeof_vert_handles = total_verts;
  
    // Set newVertIndices back to 0
  for ( ; total_verts--; )
  {
    newVertIndices[reinterpret_cast<Mesquite::MeshImpl::Vertex*>(vert_handles[total_verts]) - vertexArray] = 0;
  }
}

// Identifies the vertices attached to this element by returning
// each vertex's global index.  The vertex's global index indicates
// where that element can be found in the array returned by
// Mesh::get_all_vertices.
void Mesquite::MeshImpl::element_get_attached_vertex_indices(
  Mesquite::Mesh::ElementHandle element,
  size_t *index_array,
  size_t array_size)
{
  Mesquite::MeshImpl::Element* elem =
    reinterpret_cast<Mesquite::MeshImpl::Element*>(element);

  size_t num_verts = Mesquite::vertices_in_topology(elem->mType);
  if (array_size > num_verts)
    array_size = num_verts;

  for ( ; array_size--; )
  {
    index_array[array_size] = elem->vertexIndices[array_size];
  }
}

// Returns the topology of the given entity.
Mesquite::EntityTopology Mesquite::MeshImpl::element_get_topology(
  Mesquite::Mesh::ElementHandle entity_handle)
{
  return reinterpret_cast<MeshImpl::Element*>(entity_handle)->mType;
}

// Returns the topologies of the given entities.  The "entity_topologies"
// array must be at least "num_elements" in size.
void Mesquite::MeshImpl::elements_get_topologies(
  Mesquite::Mesh::ElementHandle *element_handle_array,
  Mesquite::EntityTopology *element_topologies,
  size_t num_elements)
{
  for (size_t i = 0; i < num_elements; i++)
  {
    element_topologies[i] =
      reinterpret_cast<MeshImpl::Element*>(element_handle_array[i])->mType;
  }
}

//**************** Memory Management ****************
// Tells the mesh that the client is finished with a given
// entity handle.  
void Mesquite::MeshImpl::release_entity_handles(
  Mesquite::Mesh::EntityHandle *handle_array,
  size_t num_handles)
{
    // Do nothing
}

// Instead of deleting a Mesh when you think you are done,
// call release().  In simple cases, the implementation could
// just call the destructor.  More sophisticated implementations
// may want to keep the Mesh object to live longer than Mesquite
// is using it.
void Mesquite::MeshImpl::release()
{
  delete this;
}
