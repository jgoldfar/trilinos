#ifndef MESQUITE_MESH_IMPL_HPP
#define MESQUITE_MESH_IMPL_HPP

#include "MeshInterface.hpp"

namespace Mesquite
{
  class MeshImpl : public Mesquite::Mesh
  {
  public:
//********* Functions that are NOT inherited ************
    MeshImpl();
    ~MeshImpl();
    void read_vtk(const char* file_name, Mesquite::MsqError &err);
    void read_exodus(const char* file_name);
    
//********* Functions that ARE inherited ************
      // Returns whether this mesh lies in a 2D or 3D coordinate system.
    virtual int get_geometric_dimension() const;
    
      // Returns the number of entities of the indicated type.
    virtual size_t get_total_vertex_count() const;
    virtual size_t get_total_element_count() const;
    
      // Fills array with handles to all vertices/elements
      // in the mesh.
    virtual void get_all_vertices(VertexHandle *vert_array,
                                  size_t array_size);
    virtual void get_all_elements(ElementHandle *elem_array,
                                  size_t array_size);
    
      // Returns a pointer to an iterator that iterates over the
      // set of all vertices in this mesh.  The calling code should
      // delete the returned iterator when it is finished with it.
      // If vertices are added or removed from the Mesh after obtaining
      // an iterator, the behavior of that iterator is undefined.
    virtual VertexIterator* vertex_iterator();
    
      // Returns a pointer to an iterator that iterates over the
      // set of all top-level elements in this mesh.  The calling code should
      // delete the returned iterator when it is finished with it.
      // If elements are added or removed from the Mesh after obtaining
      // an iterator, the behavior of that iterator is undefined.
    virtual ElementIterator* element_iterator();

//************ Vertex Properties ********************
      // Returns true or false, indicating whether the vertex
      // is allowed to be repositioned.  True indicates that the vertex
      // is fixed and cannot be moved.  Note that this is a read-only
      // property; this flag can't be modified by users of the
      // Mesquite::Mesh interface.
    virtual bool vertex_is_fixed(VertexHandle vertex);

      // Returns true or false, indicating whether the vertex
      // is on the boundary.  Boundary nodes may be treated as
      // a special case by some algorithms or culling methods.
      // Note that this is a read-only
      // property; this flag can't be modified by users of the
      // Mesquite::Mesh interface.
    virtual bool vertex_is_on_boundary(VertexHandle vertex);
    
      // Get/set location of a vertex
    virtual void vertex_get_coordinates(VertexHandle vertex,
                                        Vector3D &coordinates);
    virtual void vertex_set_coordinates(VertexHandle vertex,
                                        const Vector3D &coordinates);
    
      // Each vertex has a byte-sized flag that can be used to store
      // flags.  This byte's value is neither set nor used by the mesh
      // implementation.  It is intended to be used by Mesquite algorithms.
      // Until a vertex's byte has been explicitly set, its value is 0.
    virtual void vertex_set_byte (VertexHandle vertex,
                                  unsigned char byte);
    virtual void vertices_set_byte (VertexHandle *vert_array,
                                    unsigned char *byte_array,
                                    size_t array_size);
    
      // Retrieve the byte value for the specified vertex or vertices.
      // The byte value is 0 if it has not yet been set via one of the
      // *_set_byte() functions.
    virtual void vertex_get_byte(VertexHandle vertex,
                                 unsigned char *byte);
    virtual void vertices_get_byte(VertexHandle *vertex,
                                   unsigned char *byte_array,
                                   size_t array_size);
    
//**************** Vertex Topology *****************    
      // Gets the number of elements attached to this vertex.
      // Useful to determine how large the "elem_array" parameter
      // of the vertex_get_attached_elements() function must be.
    virtual size_t vertex_get_attached_element_count(VertexHandle vertex) const;
    
      // Gets the elements attached to this vertex.
    virtual void vertex_get_attached_elements(VertexHandle vertex,
                                              ElementHandle* elem_array,
                                              size_t sizeof_elem_array);
    
      // Identifies the elements attached to this vertex by returning
      // each element's global index.  The element's global index indicates
      // where that element can be found in the array returned by
      // Mesh::get_all_elements.
    virtual void vertex_get_attached_element_indices(
      VertexHandle vertex,
      size_t *index_array,
      size_t sizeof_index_array);
    
//*************** Element Topology *************
    
      // Gets the number of vertices in this element.
      // This data can also be found by querying the
      // element's topology and getting the number
      // of vertices per element for that topology type.
    virtual size_t element_get_attached_vertex_count(ElementHandle elem) const;
    
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
    virtual void elements_get_attached_vertices(ElementHandle *elem_handles,
                                                size_t num_elems,
                                                VertexHandle *vert_handles,
                                                size_t &sizeof_vert_handles,
                                                size_t *csr_data,
                                                size_t &sizeof_csr_data,
                                                size_t *csr_offsets);
    
      // Identifies the vertices attached to this element by returning
      // each vertex's global index.  The vertex's global index indicates
      // where that element can be found in the array returned by
      // Mesh::get_all_vertices.
    virtual void element_get_attached_vertex_indices(ElementHandle element,
                                                     size_t *index_array,
                                                     size_t array_size);
    
      // Returns the topology of the given entity.
    virtual EntityTopology element_get_topology(ElementHandle entity_handle);
      // Returns the topologies of the given entities.  The "entity_topologies"
      // array must be at least "num_elements" in size.
    virtual void elements_get_topologies(ElementHandle *element_handle_array,
                                         EntityTopology *element_topologies,
                                         size_t num_elements);
    
//**************** Memory Management ****************
      // Tells the mesh that the client is finished with a given
      // entity handle.  
    virtual void release_entity_handles(EntityHandle *handle_array,
                                        size_t num_handles);
    
      // Instead of deleting a Mesh when you think you are done,
      // call release().  In simple cases, the implementation could
      // just call the destructor.  More sophisticated implementations
      // may want to keep the Mesh object to live longer than Mesquite
      // is using it.
    virtual void release();
  protected:
    class Vertex;
    class Element;
    
    Vertex* vertexArray;
    Element* elementArray;
    size_t vertexCount;
    size_t elementCount;
    
    unsigned char* onBoundaryBits;
    unsigned char* vertexMesquiteByte;
    size_t *newVertIndices;

    size_t *v2eOffset;  // When created, size vertexCount + 1
    size_t totalVertexUses; // Number of vertices in all elements
    size_t *v2E; // When created, size totalVertexUses
    
    class Vertex
    {
    public:
      double coords[3];
    };
    
    class Element
    {
    public:
      Mesquite::EntityTopology mType;
      size_t vertexIndices[8];
    };

    void create_vertex_to_element_data();
  };
}

#endif
