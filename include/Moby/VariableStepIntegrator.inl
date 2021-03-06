/****************************************************************************
 * Copyright 2010 Evan Drumwright
 * This library is distributed under the terms of the GNU Lesser General Public 
 * License (found in COPYING).
 ****************************************************************************/

/// Implements Base::load_from_xml()
template <class T>
void VariableStepIntegrator<T>::load_from_xml(XMLTreeConstPtr node, std::map<std::string, BasePtr>& id_map) 
{ 
  Integrator<T>::load_from_xml(node, id_map); 
 
  // get the absolute error tolerance
  const XMLAttrib* aerr_attrib = node->get_attrib("abs-err-tol");
  if (aerr_attrib)
    aerr_tolerance = aerr_attrib->get_real_value();

  // get the relative error tolerance
  const XMLAttrib* rerr_attrib = node->get_attrib("rel-err-tol");
  if (rerr_attrib)
    rerr_tolerance = rerr_attrib->get_real_value();

  // get the minimum step size
  const XMLAttrib* mss_attrib = node->get_attrib("min-step-size");
  if (mss_attrib)
    min_step_size = mss_attrib->get_real_value();
}

/// Implements Base::save_to_xml()
template <class T>
void VariableStepIntegrator<T>::save_to_xml(XMLTreePtr node, std::list<BaseConstPtr>& shared_objects) const
{ 
  Integrator<T>::save_to_xml(node, shared_objects); 
  node->name = "VariableStepIntegrator";

  // save the method data
  node->attribs.insert(XMLAttrib("rel-err-tol", rerr_tolerance));
  node->attribs.insert(XMLAttrib("abs-err-tol", aerr_tolerance));
  node->attribs.insert(XMLAttrib("min-step-size", min_step_size));
}

