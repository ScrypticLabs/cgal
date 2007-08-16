// Copyright (c) 2006  Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s)     : Baruch Zukerman <baruchzu@post.tau.ac.il>
//                 Ron Wein <wein@post.tau.ac.il>
//                 Efi Fogel <efif@post.tau.ac.il>

#ifndef CGAL_ARR_SPHERICAL_CONSTRUCTION_HELPER_H
#define CGAL_ARR_SPHERICAL_CONSTRUCTION_HELPER_H

/*! \file
 * Definition of the Arr_spherical_construction_helper class-template.
 */

#include <CGAL/Sweep_line_empty_visitor.h>
#include <CGAL/Unique_hash_map.h>

#include <CGAL/Arr_spherical_arc_traits_2.h>

CGAL_BEGIN_NAMESPACE

/*! \class Arr_spherical_construction_helper
 * A helper class for the construction sweep-line visitor, suitable
 * for an Arrangement_on_surface_2 instantiated with a topology-traits class
 * for bounded curves in the plane.
 */
template <class Traits_, class Arrangement_, class Event_, class Subcurve_> 
class Arr_spherical_construction_helper
{
public:
  typedef Traits_                                         Traits_2;
  typedef Arrangement_                                    Arrangement_2;
  typedef Event_                                          Event;
  typedef Subcurve_                                       Subcurve;

  typedef typename Traits_2::X_monotone_curve_2           X_monotone_curve_2;
  typedef typename Traits_2::Point_2                      Point_2;

  typedef Sweep_line_empty_visitor<Traits_2, Subcurve, Event>
                                                          Base_visitor;

  typedef typename Arrangement_2::Vertex_handle           Vertex_handle;
  typedef typename Arrangement_2::Halfedge_handle         Halfedge_handle;
  typedef typename Arrangement_2::Face_handle             Face_handle;
  
  typedef typename Subcurve::Halfedge_indices_list        Indices_list;
  typedef Unique_hash_map<Halfedge_handle, Indices_list>  Halfedge_indices_map;

protected:
  typedef typename Arrangement_2::Topology_traits         Topology_traits;

  typedef typename Topology_traits::Vertex                DVertex;
  typedef typename Topology_traits::Halfedge              DHalfedge;

  // Data members:

  //! The topology-traits class
  Topology_traits        * m_top_traits;
  Arr_accessor<Arrangement_2>
                           m_arr_access;  // An arrangement accessor.

  //! The unbounded arrangement face
  Face_handle m_spherical_face;

  //! Indices of the curves that "see" the north face
  Indices_list m_subcurves_at_nf;

  //! A pointer to a map of halfedges to indices lists
  // (stored in the visitor class)
  Halfedge_indices_map * m_he_ind_map_p;

public:
  /*! Constructor. */
  Arr_spherical_construction_helper(Arrangement_2 * arr) :
    m_top_traits(arr->topology_traits()),
    m_arr_access(*arr),
    m_he_ind_map_p(NULL)
  {}

  /*! Destructor. */
  virtual ~Arr_spherical_construction_helper() {}

  /// \name Notification functions.
  //@{

  /* A notification issued before the sweep process starts. */
  virtual void before_sweep()
  {
    // Get the unbounded face.
    m_spherical_face = Face_handle(m_top_traits->spherical_face());
  }

  /*! A notification invoked before the sweep-line starts handling the given
   * event.
   */
  virtual void before_handle_event(Event * event)
  {
    // Act according to the boundary type:
    const Boundary_type bound_x = event->boundary_in_x();
    const Boundary_type bound_y = event->boundary_in_y();

    if (bound_y == AFTER_SINGULARITY)
    {
      // The event has only one (right or left) curve.
      CGAL_assertion(((event->number_of_left_curves() == 0) &&
                      (event->number_of_right_curves() == 1)) ||
                     ((event->number_of_left_curves() == 1) &&
                      (event->number_of_right_curves() == 0)));
      Curve_end ind = (event->number_of_left_curves() == 0 &&
                       event->number_of_right_curves() == 1) ?
        MIN_END : MAX_END;
      const X_monotone_curve_2 & xc = (ind == MIN_END) ?
        (*(event->right_curves_begin()))->last_curve() :
        (*(event->left_curves_begin()))->last_curve();

      // Check whether we have a vertex that corresponds to the south pole.
      // If not, we create one.
      if (m_top_traits->south_pole() == NULL)
      {
        Vertex_handle v = m_arr_access.create_vertex(event->point(),
                                                     bound_x, bound_y);
        m_top_traits->notify_on_boundary_vertex_creation(&(*v), xc, ind,
                                                         bound_x, bound_y);
        event->set_vertex_handle(v);
      }
      else
      {
        event->set_vertex_handle(Vertex_handle (m_top_traits->south_pole()));
      }
      return;
    }

    if (bound_y == BEFORE_SINGULARITY)
    {
      // The event has only one (right or left) curve.
      CGAL_assertion(((event->number_of_left_curves() == 0) &&
                      (event->number_of_right_curves() == 1)) ||
                     ((event->number_of_left_curves() == 1) &&
                      (event->number_of_right_curves() == 0)));
      Curve_end ind = (event->number_of_left_curves() == 0 &&
                       event->number_of_right_curves() == 1) ?
        MIN_END : MAX_END;

      const X_monotone_curve_2 & xc = (ind == MIN_END) ?
        (*(event->right_curves_begin()))->last_curve() :
        (*(event->left_curves_begin()))->last_curve();

      // Check whether we have a vertex that corresponds to the north pole.
      // If not, we create one.
      if (m_top_traits->north_pole() == NULL)
      {
        Vertex_handle v = m_arr_access.create_vertex(event->point(),
                                                     bound_x, bound_y);
        m_top_traits->notify_on_boundary_vertex_creation(&(*v), xc, ind,
                                                         bound_x, bound_y);
        event->set_vertex_handle(v);

        // Since this is the first event corresponding to the north pole,
        // the list m_subcurves_at_nf contains all subcurves whole left
        // endpoint lies between the curve of discontinuity and the current
        // curve incident to the north pole. In case these subcurves represent
        // holes, these holes should stay in the "north face" that contains the
        // line of discontinuity, and we should not keep track of them in order
        // to later move them to another face.
        m_subcurves_at_nf.clear();
      }
      else
      {
        event->set_vertex_handle(Vertex_handle (m_top_traits->north_pole()));

        DHalfedge * dprev =
          m_top_traits->locate_around_boundary_vertex(m_top_traits->
                                                      north_pole(), xc, ind,
                                                      bound_x, bound_y);

        if (dprev != NULL)
        {
          Halfedge_handle prev = Halfedge_handle(dprev);
          event->set_halfedge_handle(prev);
          
          // Associate all curve indices of subcurves that "see" the top face
          // from below with the left portion of the twin of the predecessor.
          if (m_he_ind_map_p != NULL) {
            Indices_list & list_ref = (*m_he_ind_map_p)[prev->twin()];
            list_ref.splice(list_ref.end(), m_subcurves_at_nf);
          }
          else
          {
            m_subcurves_at_nf.clear();
          }
          CGAL_assertion(m_subcurves_at_nf.empty());
        }
        return;
      }

      return;
    }

    if (bound_x == AFTER_DISCONTINUITY)
    {
      // The event has only right curves.
      CGAL_assertion(event->number_of_left_curves() == 0 &&
                     event->number_of_right_curves() == 1);
      const X_monotone_curve_2 & xc =
        (*(event->right_curves_begin()))->last_curve();
      DVertex * v = m_top_traits->discontinuity_vertex(xc, MIN_END);

      // Check whether a corresponding vertex already exists on the line
      // of discontinuity. If not, create one now.
      if (v == NULL)
      {
        Vertex_handle vh =  m_arr_access.create_vertex(event->point(),
                                                       bound_x, bound_y);
        m_top_traits->notify_on_boundary_vertex_creation(&(*vh), xc, MIN_END,
                                                         bound_x, bound_y);
        event->set_vertex_handle(vh);
      }
      else
      {
        event->set_vertex_handle(Vertex_handle(v));
      }
      return;
    }

    if (bound_x == BEFORE_DISCONTINUITY)
    {
       // The event has only left curves.
      CGAL_assertion(event->number_of_left_curves() == 1 &&
                     event->number_of_right_curves() == 0);
      const X_monotone_curve_2 & xc =
        (*(event->left_curves_begin()))->last_curve();
      DVertex * v = m_top_traits->discontinuity_vertex(xc, MAX_END);

      // Check whether a corresponding vertex already exists on the line
      // of discontinuity. If not, create one now.
      if (v == NULL)
      {
        Vertex_handle vh = m_arr_access.create_vertex(event->point(),
                                                      bound_x, bound_y);
        m_top_traits->notify_on_boundary_vertex_creation(&(*vh), xc, MAX_END,
                                                         bound_x, bound_y);
        event->set_vertex_handle(vh);
      }
      else
      {
        event->set_vertex_handle(Vertex_handle(v));
      }
      return;
    }
  }

  /*! A notification invoked when a new subcurve is created. */
  virtual void add_subcurve(Halfedge_handle he, Subcurve * sc) { return; }

  /*! Collect a subcurve index that does not see any status-line from below.
   */
  void add_subcurve_in_top_face(unsigned int index)
  {
    m_subcurves_at_nf.push_back(index);
    return;
  }

  /*! A notification invoked before the given event it deallocated. */
  void before_deallocate_event(Event * event) { return; }
  //@} 
  
  /*! Set the map that maps each halfedge to the list of subcurve indices
   * that "see" the halfedge from below.
   */
  void set_halfedge_indices_map(Halfedge_indices_map & table)
  {
    m_he_ind_map_p = &table;
    return;
  }

  /*! Determine if we should swap the order of predecessor halfedges when
   * calling insert_at_vertices_ex() .
   */
  bool swap_predecessors (Event * event) const
  {
    // If we insert an edge whose right end lies on the north pole, we have
    // to flip the order of predecessor halfegdes.
    return (event->boundary_in_x() == NO_BOUNDARY &&
            event->boundary_in_y() == BEFORE_SINGULARITY);
  }

  /*! Get the current top face. */
  Face_handle top_face() const { return m_spherical_face; }
};

CGAL_END_NAMESPACE

#endif
