/****************************************************************************
 * Copyright 2006 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#include <limits>
#include <stack>
#include <list>
#include <set>
#include <Moby/AAngle.h>
#include <Moby/Constants.h>
#include <Moby/CompGeom.h>
#include <Moby/CollisionGeometry.h>
#include <Moby/Triangle.h>
#include <Moby/Polyhedron.h>
#include <Moby/RigidBody.h>
#include <Moby/DeformableBody.h>
#include <Moby/ArticulatedBody.h>
#include <Moby/XMLTree.h>
#include <Moby/XMLTree.h>
#include <Moby/EventDrivenSimulator.h>
#include <Moby/CollisionDetection.h>

using namespace Moby;
using boost::shared_ptr;
using boost::dynamic_pointer_cast;
using std::vector;
using std::make_pair;
using std::list;
using std::pair;

CollisionDetection::CollisionDetection()
{
  disable_adjacent_default = true; 
  mode = eFirstContact;
  return_all_contacts = true;
}

/// Sets an object to enabled/disabled in the collision detector
/**
 * If the object is a rigid or deformable body, all of its CollisionGeometry 
 * objects are set to enabled/disabled.  If an object is an articulated body, 
 * all of its RigidBody (and by extension, CollisionGeometry) objects are set 
 * to  enabled/disabled.
 */
void CollisionDetection::set_enabled(BasePtr b, bool enabled)
{
  // try a CollisionGeometry first
  CollisionGeometryPtr cg = dynamic_pointer_cast<CollisionGeometry>(b);
  if (cg)
  {
    if (enabled)
      disabled.erase(cg);
    else
      disabled.insert(cg);
    return;
  }

  // try a deformable body next
  DeformableBodyPtr db = dynamic_pointer_cast<DeformableBody>(b);
  if (db)
  {
    std::list<CollisionGeometryPtr> cgs;
    db->get_all_collision_geometries(std::back_inserter(cgs));
    BOOST_FOREACH(CollisionGeometryPtr cg, cgs)
      set_enabled(cg, enabled);
    return;
  }

  // try a RigidBody next
  RigidBodyPtr rb = dynamic_pointer_cast<RigidBody>(b);
  if (rb)
  {
    std::list<CollisionGeometryPtr> cgs;
    rb->get_all_collision_geometries(std::back_inserter(cgs));
    BOOST_FOREACH(CollisionGeometryPtr cg, cgs)
      set_enabled(cg, enabled);
    return;
  }

  // finally, must be an articulated body
  ArticulatedBodyPtr ab = dynamic_pointer_cast<ArticulatedBody>(b);
  assert(ab);
  const vector<RigidBodyPtr>& links = ab->get_links();
  BOOST_FOREACH(RigidBodyPtr rb, links)
    set_enabled(rb, enabled);
}  

/// Determines whether a pair of geometries is checked for collision detection
bool CollisionDetection::is_checked(CollisionGeometryPtr cg1, CollisionGeometryPtr cg2) const
{
  // if both geometries belong to one rigid body, don't check
  RigidBodyPtr rb1 = dynamic_pointer_cast<RigidBody>(cg1->get_single_body());
  RigidBodyPtr rb2 = dynamic_pointer_cast<RigidBody>(cg2->get_single_body());
  if (rb1 && rb2 && rb1 == rb2)
    return false;

  // check the geometries
  return (is_enabled(cg1) && is_enabled(cg2) && is_enabled(cg1, cg2));
}

/// Sets a pair of objects to enabled/disabled in the collision detector
/**
 * If an object is a rigid or deformable body, all of its CollisionGeometry 
 * objects pairs are set to enabled/disabled.  If an object is an articulated 
 * body, all of its RigidBody (and by extension, CollisionGeometry) object pairs
 * are set to enabled/disabled.
 */
void CollisionDetection::set_enabled(BasePtr b1, BasePtr b2, bool enabled)
{
  // try casting as all object types for simplicity 
  CollisionGeometryPtr cg1 = dynamic_pointer_cast<CollisionGeometry>(b1);
  CollisionGeometryPtr cg2 = dynamic_pointer_cast<CollisionGeometry>(b2);
  DeformableBodyPtr db1 = dynamic_pointer_cast<DeformableBody>(b1); 
  DeformableBodyPtr db2 = dynamic_pointer_cast<DeformableBody>(b2); 
  RigidBodyPtr rb1 = dynamic_pointer_cast<RigidBody>(b1);
  RigidBodyPtr rb2 = dynamic_pointer_cast<RigidBody>(b2);
  ArticulatedBodyPtr ab1 = dynamic_pointer_cast<ArticulatedBody>(b1);
  ArticulatedBodyPtr ab2 = dynamic_pointer_cast<ArticulatedBody>(b2);

  // try CollisionGeometry objects first
  if (cg1 && cg2)
  {
    if (enabled)
      disabled_pairs.erase(make_sorted_pair(cg1, cg2));
    else
      disabled_pairs.insert(make_sorted_pair(cg1, cg2));
      
    return;
  }

  // next case: b1 1 is a CG and b2 is a DB
  if (cg1 && db2)
  {
    std::list<CollisionGeometryPtr> cgs;
    db2->get_all_collision_geometries(std::back_inserter(cgs));
    BOOST_FOREACH(CollisionGeometryPtr cg, cgs)
      set_enabled(cg1, cg, enabled);
    return;
  }

  // next case: b1 is a DB and b2 is a CG
  if (cg2 && db1)
  {
    std::list<CollisionGeometryPtr> cgs;
    db1->get_all_collision_geometries(std::back_inserter(cgs));
    BOOST_FOREACH(CollisionGeometryPtr cg, cgs)
      set_enabled(cg2, cg, enabled);
    return;
  }

  // next case: b1 is a CG and b2 is a RB
  if (cg1 && rb2)
  {
    std::list<CollisionGeometryPtr> cgs;
    rb2->get_all_collision_geometries(std::back_inserter(cgs));
    BOOST_FOREACH(CollisionGeometryPtr cg, cgs)
      set_enabled(cg1, cg, enabled);
    return;
  }

  // next case: b1 is a RB and b2 is a CG
  if (cg2 && rb1)
  {
    std::list<CollisionGeometryPtr> cgs;
    rb1->get_all_collision_geometries(std::back_inserter(cgs));
    BOOST_FOREACH(CollisionGeometryPtr cg, cgs)
      set_enabled(cg2, cg, enabled);
    return;
  }

  // next case: both b1 and b2 are DBs
  if (db1 && db2)
  {
    std::list<CollisionGeometryPtr> cgs1, cgs2;
    db1->get_all_collision_geometries(std::back_inserter(cgs1));
    db2->get_all_collision_geometries(std::back_inserter(cgs2));
    BOOST_FOREACH(CollisionGeometryPtr cg1, cgs1)
      BOOST_FOREACH(CollisionGeometryPtr cg2, cgs2)
        set_enabled(cg1, cg2, enabled);
    
    return;
  }

  // next case: b1 is DB, b2 is RB
  if (db1 && rb2)
  {
    std::list<CollisionGeometryPtr> cgs1, cgs2;
    db1->get_all_collision_geometries(std::back_inserter(cgs1));
    rb2->get_all_collision_geometries(std::back_inserter(cgs2));
    BOOST_FOREACH(CollisionGeometryPtr cg1, cgs1)
      BOOST_FOREACH(CollisionGeometryPtr cg2, cgs2)
        set_enabled(cg1, cg2, enabled);
    
    return;
  }
  
  // next case: b2 is DB, b1 is RB
  if (db2 && rb1)
  {
    std::list<CollisionGeometryPtr> cgs1, cgs2;
    rb1->get_all_collision_geometries(std::back_inserter(cgs1));
    db2->get_all_collision_geometries(std::back_inserter(cgs2));
    BOOST_FOREACH(CollisionGeometryPtr cg1, cgs1)
      BOOST_FOREACH(CollisionGeometryPtr cg2, cgs2)
        set_enabled(cg1, cg2, enabled);
    
    return;
  }

  // next case: both b1 and b2 are RBs
  if (rb1 && rb2)
  {
    std::list<CollisionGeometryPtr> cgs1, cgs2;
    rb1->get_all_collision_geometries(std::back_inserter(cgs1));
    rb2->get_all_collision_geometries(std::back_inserter(cgs2));
    BOOST_FOREACH(CollisionGeometryPtr cg1, cgs1)
      BOOST_FOREACH(CollisionGeometryPtr cg2, cgs2)
        set_enabled(cg1, cg2, enabled);
    
    return;
  }

  // next case: b1 is CG, b2 is AB
  if (cg1 && ab2)
  {
    const vector<RigidBodyPtr>& links = ab2->get_links();
    BOOST_FOREACH(RigidBodyPtr rb, links)
      set_enabled(cg1, rb, enabled);
    return;
  }

  // next case: b2 is CG, b1 is AB
  if (cg2 && ab1)
  {
    const vector<RigidBodyPtr>& links = ab1->get_links();
    BOOST_FOREACH(RigidBodyPtr rb, links)
      set_enabled(cg2, rb, enabled);
    return;
  }

  // next case: b1 is DB, b2 is AB
  if (db1 && ab2)
  {
    const vector<RigidBodyPtr>& links = ab2->get_links();
    BOOST_FOREACH(RigidBodyPtr rb, links)
      set_enabled(db1, rb, enabled);
    return;
  }

  // next case: b1 is AB, b2 is DB
  if (ab1 && db2)
  {
    const vector<RigidBodyPtr>& links = ab1->get_links();
    BOOST_FOREACH(RigidBodyPtr rb, links)
      set_enabled(db2, rb, enabled);
    return;
  }

  // next case: b1 is RB, b2 is AB
  if (rb1 && ab2)
  {
    const vector<RigidBodyPtr>& links = ab2->get_links();
    BOOST_FOREACH(RigidBodyPtr rb, links)
      set_enabled(rb1, rb, enabled);
    return;
  }

  // next case: b2 is RB, b1 is AB
  if (rb2 && ab1)
  {
    const vector<RigidBodyPtr>& links = ab1->get_links();
    BOOST_FOREACH(RigidBodyPtr rb, links)
      set_enabled(rb2, rb, enabled);
    return;
  }

  // final case, must be two articulated bodies
  assert(ab1 && ab2);
  const vector<RigidBodyPtr>& links1 = ab1->get_links();
  const vector<RigidBodyPtr>& links2 = ab2->get_links();
  BOOST_FOREACH(RigidBodyPtr rb1, links1)
    BOOST_FOREACH(RigidBodyPtr rb2, links2)
      set_enabled(rb1, rb2, enabled);
}  

/// Adds the given dynamic body to the collision detector
/**
 * \note if the body is articulated, then adjacent links are not disabled!
 */
void CollisionDetection::add_dynamic_body(DynamicBodyPtr db)
{
  ArticulatedBodyPtr abody = dynamic_pointer_cast<ArticulatedBody>(db);
  if (abody)
    add_articulated_body(abody, false);
  else
    add_rigid_body(dynamic_pointer_cast<RigidBody>(db));
}

/// Removes the given dynamic body from the collision detector
void CollisionDetection::remove_dynamic_body(DynamicBodyPtr db)
{
  ArticulatedBodyPtr abody = dynamic_pointer_cast<ArticulatedBody>(db);
  if (abody)
    remove_articulated_body(abody);
  else
    remove_rigid_body(dynamic_pointer_cast<RigidBody>(db));

}

/// Adds a deformable body to the collision detector
void CollisionDetection::add_deformable_body(DeformableBodyPtr body)
{
  // get all collision geometries for this deformable body
  std::list<CollisionGeometryPtr> geoms;
  body->get_all_collision_geometries(std::back_inserter(geoms));
  
  // process them
  BOOST_FOREACH(CollisionGeometryPtr cg, geoms)
    add_collision_geometry(cg); 
}

/// Removes a deformable body to the collision detector
void CollisionDetection::remove_deformable_body(DeformableBodyPtr body)
{
  // get all collision geometries for this deformable body
  std::list<CollisionGeometryPtr> geoms;
  body->get_all_collision_geometries(std::back_inserter(geoms));
  
  // process them
  BOOST_FOREACH(CollisionGeometryPtr cg, geoms)
    remove_collision_geometry(cg);
}

/// Adds a rigid body to the collision detector
void CollisionDetection::add_rigid_body(RigidBodyPtr body)
{
  // get all collision geometries for this rigid body
  std::list<CollisionGeometryPtr> geoms;
  body->get_all_collision_geometries(std::back_inserter(geoms));
  
  // process them
  BOOST_FOREACH(CollisionGeometryPtr cg, geoms)
    add_collision_geometry(cg); 

  // disable all pairs of geometries for this rigid body
  for (std::list<CollisionGeometryPtr>::const_iterator i = geoms.begin(); i != geoms.end(); i++)
  {
    std::list<CollisionGeometryPtr>::const_iterator j = i;
    j++;
    if (j == geoms.end())
      break;
    for (; j != geoms.end(); j++)
      set_enabled(*i, *j, false);
  }
}

/// Removes a rigid body to the collision detector
void CollisionDetection::remove_rigid_body(RigidBodyPtr body)
{
  // get all collision geometries for this rigid body
  std::list<CollisionGeometryPtr> geoms;
  body->get_all_collision_geometries(std::back_inserter(geoms));
  
  // process them
  BOOST_FOREACH(CollisionGeometryPtr cg, geoms)
    remove_collision_geometry(cg); 
}

//// Adds the specified articulated body to the bodies checked for collision
/**
 * \param abody the pointer to the specified body
 * \param disabled_adjacent if set to <b>true</b> collision checking for all
 *        adjacent links will be disabled
 */
void CollisionDetection::add_articulated_body(ArticulatedBodyPtr abody, bool disable_adjacent)
{
  // add each body individually
  const vector<RigidBodyPtr>& links = abody->get_links();
  for (unsigned i=0; i< links.size(); i++) 
    add_rigid_body(links[i]);

  // get the vector of adjacent links, if disable adjacent set to true
  if (disable_adjacent)
  {
    std::list<sorted_pair<RigidBodyPtr> > adjacent;
    abody->get_adjacent_links(adjacent);
    for (std::list<sorted_pair<RigidBodyPtr> >::const_iterator i = adjacent.begin(); i != adjacent.end(); i++)
      set_enabled(i->first, i->second, false);
  }
}

//// Removes the specified articulated body from the bodies checked for collision
/**
 * \param abody the pointer to the specified body
 */
void CollisionDetection::remove_articulated_body(ArticulatedBodyPtr abody)
{
  // remove each body individually 
  const vector<RigidBodyPtr>& links = abody->get_links();
  for (unsigned i=0; i< links.size(); i++)
    remove_rigid_body(links[i]); 
}

/// Removes all geometries from the collision detector
void CollisionDetection::remove_all_collision_geometries()
{
  // remove all geometries
  _geoms.clear();

  // remove all colliding pairs
  colliding_pairs.clear();  

  // remove all distances
  closest_points.clear();

  // clear disabled sets
  disabled.clear();
  disabled_pairs.clear();
}

/// Removes a collision geometry from the collision detector, if present
void CollisionDetection::remove_collision_geometry(CollisionGeometryPtr geom)
{
  // if the geometry is not in the collision detector, quit
  if (_geoms.find(geom) == _geoms.end())
    return;

  /// remove the geometry
  _geoms.erase(geom);

  // remove this geometry from disabled sets
  disabled.erase(geom);
  for (std::set<sorted_pair<CollisionGeometryPtr> >::iterator i = disabled_pairs.begin(); i != disabled_pairs.end(); )
    if (i->first == geom || i->second == geom)
    {
      std::set<sorted_pair<CollisionGeometryPtr> >::iterator next = i;
      next++;
      disabled_pairs.erase(i);
      i = next;
    }
    else
      i++;
}

/// Calculates distances between all pairs of geometries
/**
 * \note does not calculate inter-geometry distances (i.e., in case a geometry
 *       is deformable)
 */  
Real CollisionDetection::calc_distances()
{
  Vector3 cp1, cp2;

  // clear the mapping of distances first
  distances.clear();
  closest_points.clear();

  // determine distance between each pair of bodies
  Real min_dist = std::numeric_limits<Real>::max();
  for (std::set<CollisionGeometryPtr>::const_iterator i = _geoms.begin(); i != _geoms.end(); i++)
  {
    std::set<CollisionGeometryPtr>::const_iterator j = i; 
    for (j++; j != _geoms.end(); j++)
    {
      // see whether the pair is checked
      if (!is_checked(*i, *j))
        continue;

      // get the two geometries
      CollisionGeometryPtr g1 = *i;
      CollisionGeometryPtr g2 = *j;

      // get the inverse transform for g1
      Matrix4 g1Tw = Matrix4::inverse_transform(g1->get_transform());

      // get the transform for g2
      const Matrix4& wTg2 = g2->get_transform(); 

      // otherwise, compute the distance
      Real dist = calc_distance(g1, g2, g1Tw * wTg2, cp1, cp2);
      min_dist = std::min(dist, min_dist);    

      // save the distance
      distances[make_sorted_pair(g1, g2)] = dist;

      // set the closest points
      closest_points[make_pair(g1, g2)] = make_pair(cp1, cp2);
    }
  }

  return min_dist;
}

/// Calculates the closest points (and squared distance) between geometries a and b
/**
 * \param a the first collision geometry
 * \param b the second collision geometry
 * \param aTb the transform from b's frame to a's frame
 * \param cpa the closest point to b on a (in a's frame)
 * \param cpb the closest point to a on b (in b's frame)
 * \return the squared distance between cpa and cpb
 */
Real CollisionDetection::calc_distance(CollisionGeometryPtr a, CollisionGeometryPtr b, const Matrix4& aTb, Vector3& cpa, Vector3& cpb)
{
  // setup the minimum distance
  Real min_dist = std::numeric_limits<Real>::max();

  // determine the transform from b's frame to a's frame
  Matrix4 bTa = Matrix4::inverse_transform(aTb);

  // get the primitives 
  PrimitivePtr a_primitive = a->get_geometry(); 
  PrimitivePtr b_primitive = b->get_geometry();

  // get the mesh data from the plugins
  const IndexedTriArray& a_mesh = *a_primitive->get_mesh();
  const IndexedTriArray& b_mesh = *b_primitive->get_mesh(); 

  // do pairwise distance checks
  for (unsigned i=0; i< a_mesh.num_tris(); i++)
  {
    // get the triangle
    Triangle ta = a_mesh.get_triangle(i);

    // loop over all triangles in b
    for (unsigned j=0; j< b_mesh.num_tris(); j++)
    {
      // get the untransformed second triangle
      Triangle utb = b_mesh.get_triangle(j);

      // transform the second triangle
      Triangle tb = Triangle::transform(utb, aTb);

      // get distance between the two triangles
      Vector3 cpa_tmp, cpb_tmp;
      Real dist = Triangle::calc_sq_dist(ta, tb, cpa_tmp, cpb_tmp);

      // if it's the minimum distance, save the closest points
      if (dist < min_dist)
      {
        min_dist = dist;
        cpa = cpa_tmp;
        cpb = bTa.mult_point(cpb_tmp);
      }
    }
  }

  return min_dist;
}

/// Implements Base::load_from_xml()
void CollisionDetection::load_from_xml(XMLTreeConstPtr node, std::map<std::string, BasePtr>& id_map)
{
  std::list<XMLTreeConstPtr> child_nodes;
  std::map<std::string, BasePtr>::const_iterator id_iter;

  // ***********************************************************************
  // don't verify that the node name is correct, b/c CollisionDetection can 
  // be subclassed
  // ***********************************************************************

  // call parent load_from_xml() method first
  Base::load_from_xml(node, id_map);

  // get the list of body and geometry child nodes
  std::list<XMLTreeConstPtr> body_children = node->find_child_nodes("Body");
  std::list<XMLTreeConstPtr> geom_children = node->find_child_nodes("CollisionGeometry");

  // if either body or geometry child nodes were specified, we'll remove all
  // geometries currently in the detector 
  if (!body_children.empty() || !geom_children.empty())
  {
    // remove all geometries 
    remove_all_collision_geometries();

    // add any given bodies to the collision detector
    for (std::list<XMLTreeConstPtr>::const_iterator i = body_children.begin(); i != body_children.end(); i++)
    {
      // get the ID attribute
      const XMLAttrib* id_attrib = (*i)->get_attrib("body-id");
      if (!id_attrib)
      {
        std::cerr << "CollisionDetection::load_from_xml() - did not find ";
        std::cerr << "body-id in offending node: " << std::endl << *node; 
        continue;
      }

      // get the ID
      const std::string& ID = id_attrib->get_string_value();

      // find the object
      if ((id_iter = id_map.find(ID)) == id_map.end())
      {
        std::cerr << "CollisionDetection::load_from_xml() - could not find ";
        std::cerr << "object with id" << std::endl;
        std::cerr << "  '" << ID << "' in offending node: " << std::endl << *node;
        continue;
      }
      DynamicBodyPtr db = dynamic_pointer_cast<DynamicBody>(id_iter->second);

      // make sure that the dynamic body was found
      if (!db)
        std::cerr << "CollisionGeometry::load_from_xml()- dynamic body " << ID << " not found" << std::endl;
      else
      {
        // check to see whether adjacent links are disabled
        const XMLAttrib* disable_adj_attrib = (*i)->get_attrib("disable-adjacent-links");
        bool disable_adj = ((disable_adj_attrib && disable_adj_attrib->get_bool_value()) || disable_adjacent_default);

        // add the body to the collision detector
        ArticulatedBodyPtr ab = dynamic_pointer_cast<ArticulatedBody>(db);
        if (ab)
          add_articulated_body(ab, disable_adj);
        else
        {
          RigidBodyPtr rb = dynamic_pointer_cast<RigidBody>(db);
          if (rb)
            add_rigid_body(rb);
          else
            add_deformable_body(dynamic_pointer_cast<DeformableBody>(db));
        }
      }
    }
      
    // add any given geometries to the collision detector
    for (std::list<XMLTreeConstPtr>::const_iterator i = geom_children.begin(); i != geom_children.end(); i++)
    {
      // get the ID attribute
      const XMLAttrib* id_attrib = (*i)->get_attrib("geometry-id");
      if (!id_attrib)
      {
        std::cerr << "CollisionDetection::load_from_xml() - did not find ";
        std::cerr << "geometry-id in offending node: " << std::endl << *node; 
        continue;
      }

      // get the ID
      const std::string& ID = id_attrib->get_string_value();

      // find the object
      if ((id_iter = id_map.find(ID)) == id_map.end())
      {
        std::cerr << "CollisionDetection::load_from_xml() - could not find ";
        std::cerr << "object with id" << std::endl;
        std::cerr << "  '" << ID << "' in offending node: " << std::endl << *node;
        continue;
      }

      // add the geometry to the collision detector
      add_collision_geometry(dynamic_pointer_cast<CollisionGeometry>(id_iter->second));
    }
  }

  // read all disabled pairs
  child_nodes = node->find_child_nodes("DisabledPair");
  for (std::list<XMLTreeConstPtr>::const_iterator i = child_nodes.begin(); i != child_nodes.end(); i++)
  {
    // get the two ID attributes
    const XMLAttrib* id1_attrib = (*i)->get_attrib("object1-id");
    const XMLAttrib* id2_attrib = (*i)->get_attrib("object2-id");

    // make sure that they were read
    if (!id1_attrib || !id2_attrib)
    {
      std::cerr << "CollisionDetection::load_from_xml() - did not find ";
      std::cerr << "obj-first-id and/or obj-second-id" << std::endl;
      std::cerr << "  in offending node: " << std::endl << *node;
      continue;
    }

    // get the two IDs
    const std::string& ID1 = id1_attrib->get_string_value();
    const std::string& ID2 = id2_attrib->get_string_value();

    // find the first object
    if ((id_iter = id_map.find(ID1)) == id_map.end())
    {
      std::cerr << "CollisionDetection::load_from_xml() - could not find ";
      std::cerr << "object with object1-id" << std::endl;
      std::cerr << "  '" << ID1 << "' in offending node: " << std::endl << *node;
      continue;
    }
    BasePtr o1 = id_iter->second;

    // find the second object
    if ((id_iter = id_map.find(ID2)) == id_map.end())
    {
      std::cerr << "CollisionDetection::load_from_xml() - could not find ";
      std::cerr << "object with object2-id" << std::endl;
      std::cerr << "  '" << ID2 << "' in offending node: " << std::endl << *node;
      continue;
    }
    BasePtr o2 = id_iter->second;
    set_enabled(o1, o2, false);
  }

  // read all disabled bodies
  child_nodes = node->find_child_nodes("Disabled");
  for (std::list<XMLTreeConstPtr>::const_iterator i = child_nodes.begin(); i != child_nodes.end(); i++)
  {  
    // get the ID attribute
    const XMLAttrib* id_attrib = (*i)->get_attrib("object-id");
    if (!id_attrib)
    {
      std::cerr << "CollisionDetection::load_from_xml() - did not find ";
      std::cerr << "id in offending node: " << std::endl << *node; 
      continue;
    }

    // get the ID
    const std::string& ID = id_attrib->get_string_value();

    // find the object
    if ((id_iter = id_map.find(ID)) == id_map.end())
    {
      std::cerr << "CollisionDetection::load_from_xml() - could not find ";
      std::cerr << "object with id" << std::endl;
      std::cerr << "  '" << ID << "' in offending node: " << std::endl << *node;
      continue;
    }
    BasePtr o = id_iter->second;

    // add the object to the set of disabled objects
    set_enabled(o, false);
  }

  // read the contact simulator ID, if specified
  const XMLAttrib* sim_attrib = node->get_attrib("simulator-id");
  if (sim_attrib)
  {
    // get the ID
    const std::string& ID = sim_attrib->get_string_value();

    // find the object
    if ((id_iter = id_map.find(ID)) == id_map.end())
    {
      FILE_LOG(LOG_COLDET) << "CollisionDetection::load_from_xml() - could not find ";
      FILE_LOG(LOG_COLDET) << "simulator with id" << std::endl;
      FILE_LOG(LOG_COLDET) << "  '" << ID << "' in offending node: " << std::endl << *node;
      FILE_LOG(LOG_COLDET) << "  *** pointer to the object will likely be set when ContactSimulator is read" << std::endl;
    }
    else
    {
      BasePtr o = id_iter->second;

      // make sure that it casts as an EventDrivenSimulator object
      simulator = dynamic_pointer_cast<EventDrivenSimulator>(o);

      if (simulator.expired())
      {
        std::cerr << "CollisionDetection::load_from_xml() - "; 
        std::cerr << "object with id" << std::endl;
        std::cerr << "  '" << ID << "' does not cast to type EventDrivenSimulator ";
        std::cerr << "in offending node:" << std::endl << *node;
      }
      else
      {
        // add the collision detector to the simulator
        shared_ptr<EventDrivenSimulator> esim(simulator); 
        esim->collision_detectors.push_back(get_this());
      }
    }
  }
}

/// Implements Base::save_to_xml()
/**
 * \note neither the contact cache nor the pairs currently in collision are 
 *       saved
 */
void CollisionDetection::save_to_xml(XMLTreePtr node, std::list<BaseConstPtr>& shared_objects) const
{
  // call parent save_to_xml() method first
  Base::save_to_xml(node, shared_objects);

  // (re)set the name, though this node will be renamed later (b/c this class
  // is abstract)
  node->name = "CollisionDetection";

  // save all disabled pairs
  for (std::set<sorted_pair<CollisionGeometryPtr> >::const_iterator i = disabled_pairs.begin(); i != disabled_pairs.end(); i++)
  {
    XMLTreePtr child_node(new XMLTree("DisabledPair"));
    child_node->attribs.insert(XMLAttrib("object1-id", i->first->id));
    child_node->attribs.insert(XMLAttrib("object2-id", i->second->id));
    node->add_child(child_node);
  }

  // save all disabled individual bodies  
  for (std::set<CollisionGeometryPtr>::const_iterator i = disabled.begin(); i != disabled.end(); i++)
  {
    XMLTreePtr child_node(new XMLTree("Disabled"));
    child_node->attribs.insert(XMLAttrib("object-id", (*i)->id));
    node->add_child(child_node);
  }

  // save all geometry ids
  for (std::set<CollisionGeometryPtr>::const_iterator i = _geoms.begin(); i != _geoms.end(); i++)
  {
    XMLTreePtr child_node(new XMLTree("CollisionGeometry"));
    child_node->attribs.insert(XMLAttrib("geometry-id", (*i)->id));
    node->add_child(child_node);
  }

  // add the simulator ID
  shared_ptr<EventDrivenSimulator> sim(simulator);
  node->attribs.insert(XMLAttrib("simulator-id", sim->id));
}

/// Outputs this class data to the stream
/**
 * This method outputs all of the low-level details to the stream; if
 * serialization is desired, use save_to_xml() instead.
 * \sa save_to_xml()
 */
void CollisionDetection::output_object_state(std::ostream& out) const
{
  // indicate the object type
  out << "CollisionDetection object" << std::endl; 

  // indicate disable by default flag 
  out << "  adjacent articulated body links disabled by default? ";
  out << disable_adjacent_default << std::endl;

  // output disabled object pointers
  out << "  disabled objects: " << std::endl;
  for (std::set<CollisionGeometryPtr>::const_iterator i = disabled.begin(); i != disabled.end(); i++)
    out << "    object: " << *i << std::endl;

  // output disabled object pair pointers
  out << "  disabled pairs: " << std::endl;
  for (std::set<sorted_pair<CollisionGeometryPtr> >::const_iterator i = disabled_pairs.begin(); i != disabled_pairs.end(); i++)
    out << "    object1: " << i->first << "  object2: " << i->second << std::endl;
}

