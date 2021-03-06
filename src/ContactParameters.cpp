/****************************************************************************
 * Copyright 2011 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

#include <string.h>
#include <iostream>
#include <Moby/XMLTree.h>
#include <Moby/ContactParameters.h>

using namespace Moby;

/// Constructs a ContactParameters object with no object pointers
/**
 * All gains and coefficients are set to zero.
 */
ContactParameters::ContactParameters()
{
  epsilon = 0.0;
  mu_coulomb = mu_viscous = 0.0;
  NK = 2;
}

/// Constructs a ContactParameters object with the given object IDs
/**
 * All gains and coefficients are set to zero.
 */
ContactParameters::ContactParameters(BasePtr o1, BasePtr o2) 
{ 
  objects = make_sorted_pair(o1, o2);
  epsilon = 0.0;
  mu_coulomb = mu_viscous = 0.0;
  NK = 2;
}

/// Implements Base::load_from_xml()
/**
 * This method does not read the Base information (i.e., name()), because
 * a name for this object is unnecessary.
 */
void ContactParameters::load_from_xml(XMLTreeConstPtr node, std::map<std::string, BasePtr>& id_map)
{
  std::map<std::string, BasePtr>::const_iterator id_iter;

  // verify that the node name is correct
  assert(strcasecmp(node->name.c_str(), "ContactParameters") == 0);

  // verify that there are object IDs
  const XMLAttrib* o1_attr = node->get_attrib("object1-id");
  const XMLAttrib* o2_attr = node->get_attrib("object2-id");
  if (!o1_attr || !o2_attr)
  {
    std::cerr << "ContactParameters::load_from_xml() - no object1-id and/or ";
    std::cerr << "object2-id attributes!" << std::endl;
    std::cerr << "  in offending node: " << std::endl << *node;
    return;
  }

  // get the ids
  const std::string& ID1 = o1_attr->get_string_value();
  const std::string& ID2 = o2_attr->get_string_value();

  // verify that the object corresponding to the first ID is found
  if ((id_iter = id_map.find(ID1)) == id_map.end())
  {
    std::cerr << "ContactParameters::load_from_xml() - unable to find object w/ID '";
    std::cerr << ID1 << "'" << std::endl << "  in offending node: ";
    std::cerr << std::endl << *node;
    return;
  }

  // get the object
  BasePtr o1 = id_iter->second;

  // verify that the object corresponding to the second ID is found
  if ((id_iter = id_map.find(ID2)) == id_map.end())
  {
    std::cerr << "ContactParameters::load_from_xml() - unable to find object w/ID '";
    std::cerr << ID2 << "'" << std::endl << "  in offending node: ";
    std::cerr << std::endl << *node;
    return;
  }

  // get the object
  BasePtr o2 = id_iter->second;

  // form the sorted pair
  objects = make_sorted_pair(o1, o2);

  // get the value for epsilon, if specified
  const XMLAttrib* rest_attr = node->get_attrib("epsilon");
  if (rest_attr)
    epsilon = rest_attr->get_real_value();

  // get the coefficient of Coulombic friction
  const XMLAttrib* fc_attr = node->get_attrib("mu-coulomb");
  if (fc_attr)
    mu_coulomb = fc_attr->get_real_value();

  // get the coefficient of viscous friction
  const XMLAttrib* fv_attr = node->get_attrib("mu-viscous");
  if (fv_attr)
    mu_viscous = fv_attr->get_real_value();

  // get the number of friction directions, if specified
  const XMLAttrib* nk_attr = node->get_attrib("friction-cone-edges");
  if (nk_attr)
    NK = nk_attr->get_unsigned_value();
}

/**
 * This method does not write the Base information, because neither a name
 * nor a unique ID are useful.
 */
void ContactParameters::save_to_xml(XMLTreePtr node, std::list<BaseConstPtr>& shared_objects) const
{
  // set the node name
  node->name = "ContactParameters";

  // if the object IDs are blank, skip this node..
  if (!objects.first || !objects.second)
    return;

  // write the two object IDs
  node->attribs.insert(XMLAttrib("object1-id", objects.first->id));
  node->attribs.insert(XMLAttrib("object2-id", objects.second->id));

  // write the coefficient of epsilon 
  node->attribs.insert(XMLAttrib("epsilon", epsilon));

  // write the coefficient of friction for Coulombic friction
  node->attribs.insert(XMLAttrib("mu-coulomb", mu_coulomb));

  // write the coefficient of friction for Viscous friction
  node->attribs.insert(XMLAttrib("mu-viscous", mu_viscous));

  // write the number of friction cone edges
  node->attribs.insert(XMLAttrib("friction-cone-edges", NK));
}

