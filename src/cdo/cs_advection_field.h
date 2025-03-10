#ifndef __CS_ADVECTION_FIELD_H__
#define __CS_ADVECTION_FIELD_H__

/*============================================================================
 * Manage the definition/setting of advection fields
 *============================================================================*/

/*
  This file is part of code_saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2023 EDF S.A.

  This program is free software; you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2 of the License, or (at your option) any later
  version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
  details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
  Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_cdo_connect.h"
#include "cs_cdo_local.h"
#include "cs_cdo_quantities.h"
#include "cs_field.h"
#include "cs_mesh_location.h"
#include "cs_param_types.h"
#include "cs_property.h"
#include "cs_xdef.h"
#include "cs_xdef_eval.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

/*!
 * @defgroup cdo_advfield_flags Flags specifying metadata related to an
 *  advection field.
 *
 * @{
 */

/*!  1: Perform the computation and post-processing of the Courant number */

#define CS_ADVECTION_FIELD_POST_COURANT (1 << 0)

/*! @} */

/*============================================================================
 * Type definitions
 *============================================================================*/

typedef cs_flag_t  cs_advection_field_status_t;

/*! \enum cs_advection_field_status_bit_t
 *  \brief Bit values for specifying the definition/behavior of an advection
 *         field
 *
 * @name Category of advection field
 * @{
 *
 * \var CS_ADVECTION_FIELD_NAVSTO
 *      Advection field stemming from the velocity in the (Navier-)Stokes system
 *
 * \var CS_ADVECTION_FIELD_GWF
 *      Advection field stemming from the "GroundWater Flows" module. This is the
 *      Darcean flux.
 *
 * \var CS_ADVECTION_FIELD_USER
 *      User-defined advection field.
 *
 * @}
 * @name Type of definition
 * @{
 *
 * \var CS_ADVECTION_FIELD_TYPE_VELOCITY_VECTOR
 *      The advection field stands for a velocity.
 *      This is described by a vector-valued array or function.
 *
 * \var CS_ADVECTION_FIELD_TYPE_SCALAR_FLUX
 *      The advection field stands for a flux.
 *      This is described by a scalar-valued array or function.
 *
 * @}
 * @name Optional bits
 * @{
 *
 * \var CS_ADVECTION_FIELD_STEADY
 *      Advection field is steady-state
 *
 * \var CS_ADVECTION_FIELD_LEGACY_FV
 *      Advection field shared with the legacy Finite Volume solver
 *
 * \var CS_ADVECTION_FIELD_DEFINE_AT_VERTICES
 *      Define a field structure related to the advection field at vertices
 *      Post-processing and log operations are automatically activated.
 *
 * \var CS_ADVECTION_FIELD_DEFINE_AT_BOUNDARY_FACES
 *      Define a field structure related to the advection field at boundary
 *      faces Post-processing and log operations are automatically activated.
 *
 * @}
 */

typedef enum {

  /* Category of advection field */
  /* --------------------------- */

  CS_ADVECTION_FIELD_NAVSTO                            = 1<<0, /* =   1 */
  CS_ADVECTION_FIELD_GWF                               = 1<<1, /* =   2 */
  CS_ADVECTION_FIELD_USER                              = 1<<2, /* =   4 */

  /* Type of advection field */
  /* ----------------------- */

  CS_ADVECTION_FIELD_TYPE_VELOCITY_VECTOR              = 1<<3, /* =   8 */
  CS_ADVECTION_FIELD_TYPE_SCALAR_FLUX                  = 1<<4, /* =  16 */

  /* Optional */
  /* -------- */

  CS_ADVECTION_FIELD_STEADY                            = 1<<5, /* =  32 */
  CS_ADVECTION_FIELD_LEGACY_FV                         = 1<<6, /* =  64 */
  CS_ADVECTION_FIELD_DEFINE_AT_VERTICES                = 1<<7, /* = 128 */
  CS_ADVECTION_FIELD_DEFINE_AT_BOUNDARY_FACES          = 1<<8  /* = 256 */

} cs_advection_field_status_bit_t;

/*! \struct cs_advection_field_t
 *  \brief Main structure to handle an advection field
 */

typedef struct {

  /*!
   * \var id
   * identification number
   *
   * \var name
   * name of the advection field
   *
   * \var status
   * category (user, navsto, gwf...) and type (velocity, flux...) of the
   * advection field
   *
   * \var post_flag
   * short descriptor dedicated to postprocessing
   *
   * \var vtx_field_id
   * id to retrieve the related cs_field_t structure (-1 if not used)
   *
   * \var cell_field_id
   * id to retrieve the related cs_field_t structure. It's always
   * defined since it's used during the building of the advection scheme
   *
   * \var bdy_field_id
   * id to retrieve the related cs_field_t structure corresponding to
   * the value of the normal flux across boundary faces. It's always
   * defined since it's used for dealing with the boundary conditions.
   *
   * \var int_field_id
   * id to retrieve the related cs_field_t structure corresponding to
   * the value of the normal flux across interior faces. It's always
   * defined when the advection field arise from the resolution of the
   * Navier-Stokes system with the legacy FV solver.
   *
   * \var definition
   * pointer to the generic structure used to define the advection field. We
   * assume that only one definition is available (i.e. there is a unique
   * zone related to "cells").
   *
   * \var n_bdy_flux_defs
   * Number of definitions related to the normal flux at the boundary
   *
   * \var bdy_flux_defs
   * Array of pointers to the definitions of the normal flux at the boundary
   */

  int                           id;
  char                *restrict name;
  cs_advection_field_status_t   status;
  cs_flag_t                     post_flag;

  int                           vtx_field_id;
  int                           cell_field_id;
  int                           bdy_field_id;
  int                           int_field_id;

  /* We assume that there is only one definition associated to an advection
     field inside the computational domain */

  cs_xdef_t                    *definition;

  /* Optional: Definition(s) for the boundary flux */

  int                           n_bdy_flux_defs;
  cs_xdef_t                   **bdy_flux_defs;
  short int                    *bdy_def_ids;

} cs_adv_field_t;

/*============================================================================
 * Global variables
 *============================================================================*/

/*============================================================================
 * Inline public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Set a new status for the given advection field structure
 *
 * \param[in, out] adv      pointer to an advection field structure
 * \param[in]      status   status flag to add to the current status
 */
/*----------------------------------------------------------------------------*/

static inline void
cs_advection_field_set_status(cs_adv_field_t               *adv,
                              cs_advection_field_status_t   status)
{
  if (adv == NULL)
    return;

  adv->status = status;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  returns true if the advection field is uniform, otherwise false
 *
 * \param[in]    adv    pointer to an advection field to test
 *
 * \return  true or false
 */
/*----------------------------------------------------------------------------*/

static inline bool
cs_advection_field_is_uniform(const cs_adv_field_t   *adv)
{
  if (adv == NULL)
    return false;

  if (adv->definition->state & CS_FLAG_STATE_UNIFORM)
    return true;
  else
    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  returns true if the advection field is uniform in each cell
 *         otherwise false
 *
 * \param[in]    adv    pointer to an advection field to test
 *
 * \return  true or false
 */
/*----------------------------------------------------------------------------*/

static inline bool
cs_advection_field_is_cellwise(const cs_adv_field_t   *adv)
{
  if (adv == NULL)
    return false;

  cs_flag_t  state = adv->definition->state;

  if (state & CS_FLAG_STATE_UNIFORM)
    return true;
  if (state & CS_FLAG_STATE_CELLWISE)
    return true;
  else
    return false;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Retrieve the name of an advection field
 *
 * \param[in]    adv    pointer to an advection field structure
 *
 * \return  the name of the related advection field
 */
/*----------------------------------------------------------------------------*/

static inline const char *
cs_advection_field_get_name(const cs_adv_field_t   *adv)
{
  if (adv == NULL)
    return NULL;

  return adv->name;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Retrieve the type of definition used to set the current advection
 *         field structure
 *
 * \param[in]    adv    pointer to an advection field structure
 *
 * \return  the type of definition
 */
/*----------------------------------------------------------------------------*/

static inline cs_xdef_type_t
cs_advection_field_get_deftype(const cs_adv_field_t   *adv)
{
  if (adv == NULL)
    return CS_N_XDEF_TYPES;

  return cs_xdef_get_type(adv->definition);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get a cs_field_t structure related to an advection field and a mesh
 *         location
 *
 * \param[in]  adv         pointer to a cs_adv_field_t structure
 * \param[in]  ml_type     type of mesh location (cells or vertices)
 *
 * \return a pointer to a cs_field_t structure
 */
/*----------------------------------------------------------------------------*/

static inline cs_field_t *
cs_advection_field_get_field(const cs_adv_field_t       *adv,
                             cs_mesh_location_type_t     ml_type)
{
  cs_field_t  *f = NULL;
  if (adv == NULL)
    return f;

  switch (ml_type) {

  case CS_MESH_LOCATION_CELLS:
    if (adv->cell_field_id > -1)
      f = cs_field_by_id(adv->cell_field_id);
    else
      f = NULL;
    break;

  case CS_MESH_LOCATION_INTERIOR_FACES:
    if (adv->int_field_id > -1)
      f = cs_field_by_id(adv->int_field_id);
    else
      f = NULL;
    break;

  case CS_MESH_LOCATION_BOUNDARY_FACES:
    if (adv->bdy_field_id > -1)
      f = cs_field_by_id(adv->bdy_field_id);
    else
      f = NULL;
    break;

  case CS_MESH_LOCATION_VERTICES:
    if (adv->vtx_field_id > -1)
      f = cs_field_by_id(adv->vtx_field_id);
    else
      f = NULL;
    break;

  default:
    bft_error(__FILE__, __LINE__, 0,
              " %s: Invalid mesh location type %d.\n"
              " Stop retrieving the advection field.\n",
              __func__, ml_type);
  }

  return f;
}

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Set shared pointers to main domain members
 *
 * \param[in]  quant       additional mesh quantities struct.
 * \param[in]  connect     pointer to a cs_cdo_connect_t struct.
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_init_sharing(const cs_cdo_quantities_t  *quant,
                                const cs_cdo_connect_t     *connect);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get the number of allocated cs_adv_field_t structures
 *
 * \return the number of advection fields
 */
/*----------------------------------------------------------------------------*/

int
cs_advection_field_get_n_fields(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Search in the array of advection field structures which one has
 *         the name given in argument
 *
 * \param[in]  name        name of the advection field
 *
 * \return a pointer to a cs_adv_field_t structure or NULL if not found
 */
/*----------------------------------------------------------------------------*/

cs_adv_field_t *
cs_advection_field_by_name(const char   *name);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Search in the array of advection field structures which one has
 *         the id given in argument
 *
 * \param[in]  id        identification number
 *
 * \return a pointer to a cs_adv_field_t structure or NULL if not found
 */
/*----------------------------------------------------------------------------*/

cs_adv_field_t *
cs_advection_field_by_id(int      id);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Add and initialize a new user-defined advection field structure
 *
 * \param[in]  name        name of the advection field
 *
 * \return a pointer to the new allocated cs_adv_field_t structure
 */
/*----------------------------------------------------------------------------*/

cs_adv_field_t *
cs_advection_field_add_user(const char  *name);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Add and initialize a new advection field structure
 *
 * \param[in]  name        name of the advection field
 * \param[in]  status      status of the advection field to create
 *
 * \return a pointer to the new allocated cs_adv_field_t structure
 */
/*----------------------------------------------------------------------------*/

cs_adv_field_t *
cs_advection_field_add(const char                    *name,
                       cs_advection_field_status_t    status);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free all alllocated cs_adv_field_t structures and its related array
 *
 * \return a NULL pointer
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_destroy_all(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Check if the given advection field has the name ref_name
 *
 * \param[in]  adv         pointer to a cs_adv_field_t structure to test
 * \param[in]  ref_name    name of the advection field to find
 *
 * \return true if the name of the advection field is ref_name otherwise false
 */
/*----------------------------------------------------------------------------*/

bool
cs_advection_field_check_name(const cs_adv_field_t   *adv,
                              const char             *ref_name);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Print all setup information related to cs_adv_field_t structures
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_log_setup(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Set optional post-processings
 *
 * \param[in, out]  adv         pointer to a cs_adv_field_t structure
 * \param[in]       post_flag   flag to set
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_set_postprocess(cs_adv_field_t            *adv,
                                   cs_flag_t                  post_flag);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define the value of a cs_adv_field_t structure
 *
 * \param[in, out]  adv       pointer to a cs_adv_field_t structure
 * \param[in]       vector    values to set
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_def_by_value(cs_adv_field_t    *adv,
                                cs_real_t          vector[3]);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define a cs_adv_field_t structure thanks to an analytic function
 *
 * \param[in, out]  adv     pointer to a cs_adv_field_t structure
 * \param[in]       func    pointer to a function
 * \param[in]       input   NULL or pointer to a structure cast on-the-fly
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_def_by_analytic(cs_adv_field_t        *adv,
                                   cs_analytic_func_t    *func,
                                   void                  *input);

/*----------------------------------------------------------------------------*/
/*!
 * \brief Define a cs_adv_field_t structure using a cs_dof_func_t
 *
 * \param[in, out] adv           pointer to a cs_adv_field_t structure
 * \param[in]      dof_location  location where values are computed
 * \param[in]      func          pointer to a cs_dof_func_t function
 * \param[in]      input         NULL or pointer to a structure cast on-the-fly
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_def_by_dof_func(cs_adv_field_t    *adv,
                                   cs_flag_t          dof_location,
                                   cs_dof_func_t     *func,
                                   void              *input);

/*----------------------------------------------------------------------------*/
/*!
 * \brief Define a cs_adv_field_t structure thanks to an array of values. If
 *        an advanced usage of the definition by array is needed, then call
 *        \ref cs_xdef_array_set_sublist or \ref cs_xdef_array_set_adjacency
 *
 * \param[in, out] adv           pointer to a cs_adv_field_t structure
 * \param[in]      val_location  information to know where are located values
 * \param[in]      array         pointer to an array
 * \param[in]      is_owner      transfer the lifecycle to the cs_xdef_t struc.
 *                               (true or false)
 *
 * \return a pointer to the resulting cs_xdef_t structure
 */
/*----------------------------------------------------------------------------*/

cs_xdef_t *
cs_advection_field_def_by_array(cs_adv_field_t    *adv,
                                cs_flag_t          val_location,
                                cs_real_t         *array,
                                bool               is_owner);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define a cs_adv_field_t structure thanks to a field structure
 *
 * \param[in, out]  adv       pointer to a cs_adv_field_t structure
 * \param[in]       field     pointer to a cs_field_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_def_by_field(cs_adv_field_t    *adv,
                                cs_field_t        *field);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define the value of the boundary normal flux for the given
 *         cs_adv_field_t structure
 *
 * \param[in, out]  adv           pointer to a cs_adv_field_t structure
 * \param[in]       zname         name of the boundary zone to consider
 * \param[in]       normal_flux   value to set
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_def_boundary_flux_by_value(cs_adv_field_t    *adv,
                                              const char        *zname,
                                              cs_real_t          normal_flux);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define the value of the boundary normal flux for the given
 *         cs_adv_field_t structure using an analytic function
 *
 * \param[in, out]  adv     pointer to a cs_adv_field_t structure
 * \param[in]       zname   name of the boundary zone to consider
 * \param[in]       func    pointer to a function
 * \param[in]       input   NULL or pointer to a structure cast on-the-fly
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_def_boundary_flux_by_analytic(cs_adv_field_t        *adv,
                                                 const char            *zname,
                                                 cs_analytic_func_t    *func,
                                                 void                  *input);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define the value of the boundary normal flux for the given
 *         cs_adv_field_t structure using an array of values
 *
 * \param[in, out] adv          pointer to a cs_adv_field_t structure
 * \param[in]      zname        name of the boundary zone to consider
 * \param[in]      val_loc      information to know where are located values
 * \param[in]      array        pointer to an array
 * \param[in]      is_owner     transfer the lifecycle to the cs_xdef_t struct.
 *                              (true or false)
 * \param[in]      full_length  if true, the size of "array" should be allocated
 *                              to the total numbers of entities related to the
 *                              given location. If false, a new list is
 *                              allocated and filled with the related subset
 *                              indirection.
 *
 * \return a pointer to the resulting definition
 */
/*----------------------------------------------------------------------------*/

cs_xdef_t *
cs_advection_field_def_boundary_flux_by_array(cs_adv_field_t    *adv,
                                              const char        *zname,
                                              cs_flag_t          val_loc,
                                              cs_real_t         *array,
                                              bool               is_owner,
                                              bool               full_length);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Define the value of the boundary normal flux for the given
 *         cs_adv_field_t structure using a field structure
 *
 * \param[in, out]  adv       pointer to a cs_adv_field_t structure
 * \param[in]       field     pointer to a cs_field_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_def_boundary_flux_by_field(cs_adv_field_t    *adv,
                                              cs_field_t        *field);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Create all needed cs_field_t structures related to an advection
 *         field
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_create_fields(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Last stage of the definition of an advection field based on several
 *         definitions (i.e. definition by subdomains on the boundary)
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_finalize_setup(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value of the advection field at the cell center
 *
 * \param[in]      c_id    id of the current cell
 * \param[in]      adv     pointer to a cs_adv_field_t structure
 * \param[in, out] vect    pointer to a cs_nvec3_t structure (meas + unitv)
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_get_cell_vector(cs_lnum_t               c_id,
                                   const cs_adv_field_t   *adv,
                                   cs_nvec3_t             *vect);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the vector-valued interpolation of the advection field at
 *         a given location inside a cell
 *
 * \param[in]      adv          pointer to a cs_adv_field_t structure
 * \param[in]      cm           pointer to a cs_cell_mesh_t structure
 * \param[in]      xyz          location where to perform the evaluation
 * \param[in]      time_eval    physical time at which one evaluates the term
 * \param[in, out] eval         pointer to a cs_nvec3_t
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_cw_eval_at_xyz(const cs_adv_field_t  *adv,
                                  const cs_cell_mesh_t  *cm,
                                  const cs_real_3_t      xyz,
                                  cs_real_t              time_eval,
                                  cs_nvec3_t            *eval);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the mean-value of the vector-valued field related to the
 *         advection field inside each cell
 *
 * \param[in]      adv           pointer to a cs_adv_field_t structure
 * \param[in]      time_eval     physical time at which one evaluates the term
 * \param[in, out] cell_values   array of values at cell centers
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_in_cells(const cs_adv_field_t  *adv,
                            cs_real_t              time_eval,
                            cs_real_t             *cell_values);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value of the advection field at vertices
 *
 * \param[in]      adv          pointer to a cs_adv_field_t structure
 * \param[in]      time_eval    physical time at which one evaluates the term
 * \param[in, out] vtx_values   array storing the results
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_at_vertices(const cs_adv_field_t  *adv,
                               cs_real_t              time_eval,
                               cs_real_t             *vtx_values);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value of the normal flux of the advection field
 *         across the boundary faces
 *
 * \param[in]      adv          pointer to a cs_adv_field_t structure
 * \param[in]      time_eval    physical time at which one evaluates the term
 * \param[in, out] flx_values   array storing the results
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_across_boundary(const cs_adv_field_t  *adv,
                                   cs_real_t              time_eval,
                                   cs_real_t             *flx_values);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value of the normal flux of the advection field across
 *         the closure of the dual cell related to each vertex belonging to the
 *         boundary face f
 *
 * \param[in]      cm         pointer to a cs_cell_mesh_t structure
 * \param[in]      adv        pointer to a cs_adv_field_t structure
 * \param[in]      f          face id in the cellwise numbering
 * \param[in]      time_eval  physical time at which one evaluates the term
 * \param[in, out] fluxes     normal boundary flux for each vertex of the face
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_cw_boundary_f2v_flux(const cs_cell_mesh_t   *cm,
                                        const cs_adv_field_t   *adv,
                                        short int               f,
                                        cs_real_t               time_eval,
                                        cs_real_t              *fluxes);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value of the normal flux of the advection field across
 *         a boundary face f (cellwise version)
 *
 * \param[in] time_eval   physical time at which one evaluates the term
 * \param[in] f           face id in the cellwise numbering
 * \param[in] cm          pointer to a cs_cell_mesh_t structure
 * \param[in] adv         pointer to a cs_adv_field_t structure
 *
 * \return  the normal boundary flux for the face f
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_advection_field_cw_boundary_face_flux(const cs_real_t          time_eval,
                                         const short int          f,
                                         const cs_cell_mesh_t    *cm,
                                         const cs_adv_field_t    *adv);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value of the flux of the advection field across the
 *         the (primal) faces of a cell
 *
 * \param[in]      cm         pointer to a cs_cell_mesh_t structure
 * \param[in]      adv        pointer to a cs_adv_field_t structure
 * \param[in]      time_eval  physical time at which one evaluates the term
 * \param[in, out] fluxes     array of values attached to primal faces of a cell
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_cw_face_flux(const cs_cell_mesh_t       *cm,
                                const cs_adv_field_t       *adv,
                                cs_real_t                   time_eval,
                                cs_real_t                  *fluxes);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the value of the flux of the advection field across the
 *         the dual faces of a cell
 *
 * \param[in]      cm         pointer to a cs_cell_mesh_t structure
 * \param[in]      adv        pointer to a cs_adv_field_t structure
 * \param[in]      time_eval  physical time at which one evaluates the term
 * \param[in, out] fluxes     array of values attached to dual faces of a cell
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_cw_dface_flux(const cs_cell_mesh_t     *cm,
                                 const cs_adv_field_t     *adv,
                                 cs_real_t                 time_eval,
                                 cs_real_t                *fluxes);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   For each cs_adv_field_t structures, update the values of the
 *          related field(s)
 *
 * \param[in]  t_eval     physical time at which one evaluates the term
 * \param[in]  cur2prev   true or false
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_field_update(cs_real_t    t_eval,
                          bool         cur2prev);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Compute the Peclet number in each cell
 *
 * \param[in]      adv        pointer to the advection field struct.
 * \param[in]      diff       pointer to the diffusion property struct.
 * \param[in]      t_eval     time at which one evaluates the advection field
 * \param[in, out] peclet     pointer to an array storing the Peclet number
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_get_peclet(const cs_adv_field_t     *adv,
                        const cs_property_t      *diff,
                        cs_real_t                 t_eval,
                        cs_real_t                 peclet[]);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Compute the Courant number in each cell
 *
 * \param[in]      adv        pointer to the advection field struct.
 * \param[in]      dt_cur     current time step
 * \param[in, out] courant    pointer to an array storing the Courant number
 */
/*----------------------------------------------------------------------------*/

void
cs_advection_get_courant(const cs_adv_field_t     *adv,
                         cs_real_t                 dt_cur,
                         cs_real_t                 courant[]);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Compute the divergence of the advection field at vertices
 *          Useful for CDO Vertex-based schemes
 *
 * \param[in]      adv         pointer to the advection field struct.
 * \param[in]      t_eval      time at which one evaluates the advection field
 *
 * \return a pointer to an array storing the result
 */
/*----------------------------------------------------------------------------*/

cs_real_t *
cs_advection_field_divergence_at_vertices(const cs_adv_field_t     *adv,
                                          cs_real_t                 t_eval);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_ADVECTION_FIELD_H__ */
