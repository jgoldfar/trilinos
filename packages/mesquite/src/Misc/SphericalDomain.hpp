#ifndef MSQ_SPHERICAL_DOMAIN_HPP
#define MSQ_SPHERICAL_DOMAIN_HPP

#include "MeshInterface.hpp"

namespace Mesquite
{
  class Mesh;
  
  class SphericalDomain : public Mesquite::MeshDomain
  {
  public:
    SphericalDomain(const Vector3D& center,
                    double radius,
                    Mesquite::Mesh* mesh)
        : mCenter(center),
          mRadius(radius),
          mMesh(mesh)
      {}
    
    void set_sphere(const Vector3D& center, double radius)
      {
        mCenter = center;
        mRadius = radius;
      }

    void set_mesh(Mesquite::Mesh* mesh)
      {
        mMesh = mesh;
      }
    
    virtual void snap_to(Mesh::EntityHandle entity_handle,
                         Vector3D &coordinate);
    
    virtual void normal_at(Mesh::EntityHandle entity_handle,
                           Vector3D &coordinate);

  private:
    Vector3D mCenter;
    double mRadius;
    Mesquite::Mesh* mMesh;
  };
}

#endif
