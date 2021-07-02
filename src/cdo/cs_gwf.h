#ifndef __CS_GWF_H__
#define __CS_GWF_H__

/*============================================================================
 * Set of main functions to handle the groundwater flow module with CDO
 *============================================================================*/

/*
  This file is part of Code_Saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2021 EDF S.A.

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

#include "cs_base.h"
#include "cs_equation.h"
#include "cs_gwf_soil.h"
#include "cs_gwf_tracer.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

#define CS_GWF_ADV_FIELD_NAME   "darcy_velocity"

/*!
 * @name Flags specifying the general behavior of the groundwater flow module
 * @{
 *
 * \enum cs_gwf_model_type_t
 * \brief Type of system of equation(s) to consider for the physical modelling
 *
 * \def CS_GWF_MODEL_SINGLE_PHASE_RICHARDS
 * \brief Single phase (liquid phase) modelling in porous media. This is based
 * on the Richards equation (in case of unsaturated soils)
 *
 * \def CS_GWF_MODEL_TWO_PHASE_RICHARDS
 * \brief Two phase flow modelling (for instance gaz and liquid) in porous
 * media. A Richards-like equation is considered in each phase.
 */

typedef enum {

  CS_GWF_MODEL_SINGLE_PHASE_RICHARDS,
  CS_GWF_MODEL_TWO_PHASE_RICHARDS,
  CS_GWF_N_MODEL_TYPES

} cs_gwf_model_type_t;

typedef cs_flag_t  cs_gwf_option_flag_t;

/*!
 * @name Flags specifying the general behavior of the groundwater flow module
 * @{
 *
 * \enum cs_gwf_model_bit_t
 * \brief elemental modelling choice either from the physical viewpoint or the
 * numerical viewpoint

 * \def CS_GWF_GRAVITATION
 * \brief Gravitation effects are taken into account in the Richards equation
 *
 * \def CS_GWF_RICHARDS_UNSTEADY
 * \brief Richards equation is unsteady (unsatured behavior)
 *
 * \def CS_GWF_SOIL_PROPERTY_UNSTEADY
 * \brief Physical properties related to soil behavior are time-dependent
 *
 * \def CS_GWF_SOIL_ALL_SATURATED
 * \brief Several different hydraulic modeling can be considered. Set a special
 *        flag if all soils are considered as saturated (a simpler treatment
 *        can be performed in this case)
 *
 *
 * \def CS_GWF_FORCE_RICHARDS_ITERATIONS
 * \brief Even if the Richards equation is steady-state, this equation is
 *        solved at each iteration.
 *
 * \def CS_GWF_RESCALE_HEAD_TO_ZERO_MEAN_VALUE
 * \brief Compute the mean-value of the hydraulic head field and subtract this
 *        mean-value to get a field with zero mean-value. It's important to set
 *        this flag if no boundary condition is given.
 *
 * \def CS_GWF_ENFORCE_DIVERGENCE_FREE
 * \brief Activate a treatment to enforce a Darcy flux to be divergence-free
 *
 *
 */

typedef enum {

  /* Main physical modelling */
  /* ----------------------- */

  CS_GWF_GRAVITATION                     = 1<< 0, /* =   1 */
  CS_GWF_RICHARDS_UNSTEADY               = 1<< 1, /* =   2 */
  CS_GWF_SOIL_PROPERTY_UNSTEADY          = 1<< 2, /* =   4 */
  CS_GWF_SOIL_ALL_SATURATED              = 1<< 3, /* =   8 */


  /* Main numerical options */
  /* ---------------------- */

  CS_GWF_FORCE_RICHARDS_ITERATIONS       = 1<< 6, /* =   64 */
  CS_GWF_RESCALE_HEAD_TO_ZERO_MEAN_VALUE = 1<< 7, /* =  128 */
  CS_GWF_ENFORCE_DIVERGENCE_FREE         = 1<< 8  /* =  256 */

} cs_gwf_model_bit_t;

/*! @}
 *!
 * @name Flags specifying the kind of post-processing to perform in
 *       the groundwater flow module
 * @{
 *
 * \def CS_GWF_POST_CAPACITY
 * \brief Activate the post-processing of the capacity (property in front of
 *        the unsteady term in Richards equation)
 *
 * \def CS_GWF_POST_MOISTURE
 * \brief Activate the post-processing of the moisture content
 *
 * \def CS_GWF_POST_PERMEABILITY
 * \brief Activate the post-processing of the permeability field
 *
 * \def CS_GWF_POST_DARCY_FLUX_BALANCE
 * \brief Compute the overall balance at the different boundaries of
 *        the Darcy flux
 *
 * \def CS_GWF_POST_DARCY_FLUX_DIVERGENCE
 * \brief Compute in each control volume (vertices or cells w.r.t the space
 *        scheme) the divergence of the Darcy flux
 *
 * \def CS_GWF_POST_DARCY_FLUX_AT_BOUNDARY
 * \brief Define a field at boundary faces for the Darcy flux and activate the
 *        post-processing
 */

#define CS_GWF_POST_CAPACITY                   (1 << 0)
#define CS_GWF_POST_MOISTURE                   (1 << 1)
#define CS_GWF_POST_PERMEABILITY               (1 << 2)
#define CS_GWF_POST_DARCY_FLUX_BALANCE         (1 << 3)
#define CS_GWF_POST_DARCY_FLUX_DIVERGENCE      (1 << 4)
#define CS_GWF_POST_DARCY_FLUX_AT_BOUNDARY     (1 << 5)

/*! @} */

/*============================================================================
 * Type definitions
 *============================================================================*/

typedef struct _gwf_t  cs_gwf_t;

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Check if the groundwater flow module has been activated
 *
 * \return true or false
 */
/*----------------------------------------------------------------------------*/

bool
cs_gwf_is_activated(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Initialize the module dedicated to groundwater flows
 *
 * \param[in]   permeability_type     type of permeability (iso, ortho...)
 * \param[in]   model                 type of physical modelling
 * \param[in]   option_flag           optional flag to specify this module
 *
 * \return a pointer to a new allocated groundwater flow structure
 */
/*----------------------------------------------------------------------------*/

cs_gwf_t *
cs_gwf_activate(cs_property_type_t           pty_type,
                cs_gwf_model_type_t          model,
                cs_gwf_option_flag_t         option_flag);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free the main structure related to groundwater flows
 *
 * \return a NULL pointer
 */
/*----------------------------------------------------------------------------*/

cs_gwf_t *
cs_gwf_destroy_all(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Summary of the main cs_gwf_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_log_setup(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Set the flag dedicated to the post-processing of the GWF module
 *
 * \param[in]  post_flag             flag to set
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_set_post_options(cs_flag_t       post_flag);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Add a new equation related to the groundwater flow module
 *         This equation is a particular type of unsteady advection-diffusion
 *         reaction eq.
 *         Tracer is advected thanks to the darcian velocity and
 *         diffusion/reaction parameters result from a physical modelling.
 *         Terms solved in the equation are activated according to the settings.
 *         The advection field corresponds to that of the liquid phase.
 *
 * \param[in]  tr_model   physical modelling to consider (0 = default settings)
 * \param[in]  eq_name    name of the tracer equation
 * \param[in]  var_name   name of the related variable
 */
/*----------------------------------------------------------------------------*/

cs_gwf_tracer_t *
cs_gwf_add_tracer(cs_gwf_tracer_model_t     tr_model,
                  const char               *eq_name,
                  const char               *var_name);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Add a new equation related to the groundwater flow module
 *         This equation is a particular type of unsteady advection-diffusion
 *         reaction eq.
 *         Tracer is advected thanks to the darcian velocity and
 *         diffusion/reaction parameters result from a physical modelling.
 *         Terms are activated according to the settings.
 *         Modelling of the tracer parameters are left to the user
 *
 * \param[in]   eq_name     name of the tracer equation
 * \param[in]   var_name    name of the related variable
 * \param[in]   setup       function pointer (predefined prototype)
 * \param[in]   add_terms   function pointer (predefined prototype)
 */
/*----------------------------------------------------------------------------*/

cs_gwf_tracer_t *
cs_gwf_add_user_tracer(const char                  *eq_name,
                       const char                  *var_name,
                       cs_gwf_tracer_setup_t       *setup,
                       cs_gwf_tracer_add_terms_t   *add_terms);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Retrieve the pointer to the cs_gwf_tracer_t structure associated to
 *         the name given as parameter
 *
 * \param[in]  eq_name    name of the tracer equation
 *
 * \return the pointer to a cs_gwf_tracer_t structure
 */
/*----------------------------------------------------------------------------*/

cs_gwf_tracer_t *
cs_gwf_tracer_by_name(const char   *eq_name);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Predefined settings for the Richards equation and the related
 *         equations defining the groundwater flow module
 *         Create new cs_field_t structures according to the setting
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_init_setup(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Add new terms if needed (such as diffusion or reaction) to tracer
 *         equations according to the settings
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_add_tracer_terms(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Last initialization step of the groundwater flow module
 *
 * \param[in]  connect    pointer to a cs_cdo_connect_t structure
 * \param[in]  quant      pointer to a cs_cdo_quantities_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_finalize_setup(const cs_cdo_connect_t     *connect,
                      const cs_cdo_quantities_t  *quant);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Update the groundwater system (pressure head, head in law, moisture
 *         content, darcian velocity, soil capacity or permeability if needed)
 *
 * \param[in]  mesh       pointer to a cs_mesh_t structure
 * \param[in]  connect    pointer to a cs_cdo_connect_t structure
 * \param[in]  quant      pointer to a cs_cdo_quantities_t structure
 * \param[in]  ts         pointer to a cs_time_step_t structure
 * \param[in]  cur2prev   true or false
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_update(const cs_mesh_t             *mesh,
              const cs_cdo_connect_t      *connect,
              const cs_cdo_quantities_t   *quant,
              const cs_time_step_t        *ts,
              bool                         cur2prev);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the steady-state of the groundwater flows module.
 *         Nothing is done if all equations are unsteady.
 *
 * \param[in]      mesh       pointer to a cs_mesh_t structure
 * \param[in]      time_step  pointer to a cs_time_step_t structure
 * \param[in]      connect    pointer to a cs_cdo_connect_t structure
 * \param[in]      cdoq       pointer to a cs_cdo_quantities_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_compute_steady_state(const cs_mesh_t              *mesh,
                            const cs_time_step_t         *time_step,
                            const cs_cdo_connect_t       *connect,
                            const cs_cdo_quantities_t    *cdoq);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the system related to groundwater flows module
 *
 * \param[in]      mesh       pointer to a cs_mesh_t structure
 * \param[in]      time_step  pointer to a cs_time_step_t structure
 * \param[in]      connect    pointer to a cs_cdo_connect_t structure
 * \param[in]      cdoq       pointer to a cs_cdo_quantities_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_compute(const cs_mesh_t              *mesh,
               const cs_time_step_t         *time_step,
               const cs_cdo_connect_t       *connect,
               const cs_cdo_quantities_t    *cdoq);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Compute the integral over a given set of cells of the field related
 *         to a tracer equation. This integral turns out to be exact for linear
 *         functions.
 *
 * \param[in]    connect   pointer to a \ref cs_cdo_connect_t structure
 * \param[in]    cdoq      pointer to a \ref cs_cdo_quantities_t structure
 * \param[in]    tracer    pointer to a \ref cs_gwf_tracer_t structure
 * \param[in]    z_name    name of the volumic zone where the integral is done
 *                         (if NULL or "" all cells are considered)
 *
 * \return the value of the integral
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_gwf_integrate_tracer(const cs_cdo_connect_t     *connect,
                        const cs_cdo_quantities_t  *cdoq,
                        const cs_gwf_tracer_t      *tracer,
                        const char                 *z_name);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Predefined extra-operations for the groundwater flow module
 *
 * \param[in]  connect   pointer to a cs_cdo_connect_t structure
 * \param[in]  cdoq      pointer to a cs_cdo_quantities_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_extra_op(const cs_cdo_connect_t      *connect,
                const cs_cdo_quantities_t   *cdoq);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Predefined post-processing output for the groundwater flow module
 *         in case of single-phase flows in porous media.
 *         Prototype of this function is given since it is a function pointer
 *         defined in cs_post.h (\ref cs_post_time_mesh_dep_output_t)
 *
 * \param[in, out] input        pointer to a optional structure (here a
 *                              cs_gwf_t structure)
 * \param[in]      mesh_id      id of the output mesh for the current call
 * \param[in]      cat_id       category id of the output mesh for this call
 * \param[in]      ent_flag     indicate global presence of cells (ent_flag[0]),
 *                              interior faces (ent_flag[1]), boundary faces
 *                              (ent_flag[2]), particles (ent_flag[3]) or probes
 *                              (ent_flag[4])
 * \param[in]      n_cells      local number of cells of post_mesh
 * \param[in]      n_i_faces    local number of interior faces of post_mesh
 * \param[in]      n_b_faces    local number of boundary faces of post_mesh
 * \param[in]      cell_ids     list of cells (0 to n-1)
 * \param[in]      i_face_ids   list of interior faces (0 to n-1)
 * \param[in]      b_face_ids   list of boundary faces (0 to n-1)
 * \param[in]      time_step    pointer to a cs_time_step_t struct.
 */
/*----------------------------------------------------------------------------*/

void
cs_gwf_extra_post_single_phase(void                      *input,
                               int                        mesh_id,
                               int                        cat_id,
                               int                        ent_flag[5],
                               cs_lnum_t                  n_cells,
                               cs_lnum_t                  n_i_faces,
                               cs_lnum_t                  n_b_faces,
                               const cs_lnum_t            cell_ids[],
                               const cs_lnum_t            i_face_ids[],
                               const cs_lnum_t            b_face_ids[],
                               const cs_time_step_t      *time_step);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_GWF_H__ */
