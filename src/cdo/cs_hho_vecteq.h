#ifndef __CS_HHO_VECTEQ_H__
#define __CS_HHO_VECTEQ_H__

/*============================================================================
 * Build an algebraic system for vector conv./diff. eq. with Hybrid High Order
 * space discretization
 *============================================================================*/

/*
  This file is part of code_saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2025 EDF S.A.

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

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "base/cs_base.h"
#include "cdo/cs_cdo_connect.h"
#include "cdo/cs_cdo_local.h"
#include "cdo/cs_cdo_quantities.h"
#include "cdo/cs_equation_builder.h"
#include "cdo/cs_equation_param.h"
#include "base/cs_field.h"
#include "cdo/cs_hho_builder.h"
#include "alge/cs_matrix.h"
#include "alge/cs_matrix_assembler.h"
#include "mesh/cs_mesh.h"
#include "base/cs_restart.h"
#include "cdo/cs_source_term.h"
#include "base/cs_time_step.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/* Algebraic system for HHO discretization */
typedef struct _cs_hho_vecteq_t cs_hho_vecteq_t;

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Allocate work buffer and general structures related to HHO schemes
 *         Set shared pointers
 *
 * \param[in] scheme_flag  flag to identify which kind of numerical scheme is
 *                         requested to solve the computational domain
 * \param[in] quant        pointer to a \ref cs_cdo_quantities_t struct.
 * \param[in] connect      pointer to a \ref cs_cdo_connect_t struct.
 * \param[in] time_step    pointer to a \ref cs_time_step_t struct.
*/
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_init_sharing(cs_flag_t                      scheme_flag,
                           const cs_cdo_quantities_t     *quant,
                           const cs_cdo_connect_t        *connect,
                           const cs_time_step_t          *time_step);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Retrieve work buffers used for building a CDO system cellwise
 *
 * \param[out]  csys    pointer to a pointer on a cs_cell_sys_t structure
 * \param[out]  cb      pointer to a pointer on a cs_cell_builder_t structure
 * \param[out]  hhob    pointer to a pointer on a cs_hho_builder_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_get(cs_cell_sys_t       **csys,
                  cs_cell_builder_t   **cb,
                  cs_hho_builder_t    **hhob);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Free buffers and generic structures related to HHO schemes
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_finalize_sharing(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief Initialize a cs_hho_vecteq_t structure storing data useful for
 *        managing such a scheme
 *
 * \param[in, out] eqp       set of parameters related an equation
 * \param[in]      var_id    id of the variable field
 * \param[in]      bflux_id  id of the boundary flux field
 * \param[in, out] eqb       pointer to a \ref cs_equation_builder_t structure
 *
 * \return a pointer to a new allocated cs_hho_vecteq_t structure
 */
/*----------------------------------------------------------------------------*/

void  *
cs_hho_vecteq_init_context(cs_equation_param_t    *eqp,
                           int                     var_id,
                           int                     bflux_id,
                           cs_equation_builder_t  *eqb);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Destroy a cs_hho_vecteq_t structure
 *
 * \param[in, out]  data    pointer to a cs_hho_vecteq_t structure
 *
 * \return a null pointer
 */
/*----------------------------------------------------------------------------*/

void *
cs_hho_vecteq_free_context(void   *data);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Set the initial values of the variable field taking into account
 *         the boundary conditions.
 *         Case of vector-valued HHO schemes.
 *
 * \param[in]      t_eval     time at which one evaluates BCs
 * \param[in]      field_id   id related to the variable field of this equation
 * \param[in]      mesh       pointer to a cs_mesh_t structure
 * \param[in]      eqp        pointer to a cs_equation_param_t structure
 * \param[in, out] eqb        pointer to a cs_equation_builder_t structure
 * \param[in, out] context    pointer to the scheme context (cast on-the-fly)
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_init_values(cs_real_t                     t_eval,
                          const int                     field_id,
                          const cs_mesh_t              *mesh,
                          const cs_equation_param_t    *eqp,
                          cs_equation_builder_t        *eqb,
                          void                         *context);

/*----------------------------------------------------------------------------*/
/*!
 * \brief   Compute the contributions of source terms (store inside builder)
 *
 * \param[in]      eqp      pointer to a cs_equation_param_t structure
 * \param[in, out] eqb      pointer to a cs_equation_builder_t structure
 * \param[in, out] data     pointer to a cs_hho_vecteq_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_compute_source(const cs_equation_param_t  *eqp,
                             cs_equation_builder_t      *eqb,
                             void                       *data);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Build the linear system arising from a scalar convection/diffusion
 *         equation with a HHO scheme.
 *         One works cellwise and then process to the assembly
 *
 * \param[in]      mesh       pointer to a cs_mesh_t structure
 * \param[in]      field_val  pointer to the current value of the field
 * \param[in]      eqp        pointer to a cs_equation_param_t structure
 * \param[in, out] eqb        pointer to a cs_equation_builder_t structure
 * \param[in, out] context    pointer to cs_hho_vecteq_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_build_system(const cs_mesh_t            *mesh,
                           const cs_real_t            *field_val,
                           const cs_equation_param_t  *eqp,
                           cs_equation_builder_t      *eqb,
                           void                       *context);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Store solution(s) of the linear system into a field structure
 *         Update extra-field values required for hybrid discretization
 *
 * \param[in]      solu       solution array
 * \param[in]      rhs        rhs associated to this solution array
 * \param[in]      eqp        pointer to a cs_equation_param_t structure
 * \param[in, out] eqb        pointer to a cs_equation_builder_t structure
 * \param[in, out] data       pointer to cs_hho_vecteq_t structure
 * \param[in, out] field_val  pointer to the current value of the field
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_update_field(const cs_real_t            *solu,
                           const cs_real_t            *rhs,
                           const cs_equation_param_t  *eqp,
                           cs_equation_builder_t      *eqb,
                           void                       *data,
                           cs_real_t                  *field_val);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get the computed values at faces (DoF used in the linear system are
 *         located at primal faces)
 *
 * \param[in, out]  data      pointer to a data structure cast-on-fly
 * \param[in]       previous  retrieve the previous state (true/false)
 *
 * \return  a pointer to an array of cs_real_t
 */
/*----------------------------------------------------------------------------*/

cs_real_t *
cs_hho_vecteq_get_face_values(void          *data,
                              bool           previous);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Get the computed values at cells (DoF used in the linear system are
 *         located at primal faces)
 *
 * \param[in, out]  data      pointer to a data structure cast-on-fly
 * \param[in]       previous  retrieve the previous state (true/false)
 *
 * \return  a pointer to an array of cs_real_t
 */
/*----------------------------------------------------------------------------*/

cs_real_t *
cs_hho_vecteq_get_cell_values(void          *data,
                              bool           previous);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Read additional arrays (not defined as fields) but useful for the
 *         checkpoint/restart process
 *
 * \param[in, out]  restart         pointer to \ref cs_restart_t structure
 * \param[in]       eqname          name of the related equation
 * \param[in]       scheme_context  pointer to a data structure cast on-the-fly
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_read_restart(cs_restart_t    *restart,
                           const char      *eqname,
                           void            *scheme_context);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Write additional arrays (not defined as fields) but useful for the
 *         checkpoint/restart process
 *
 * \param[in, out]  restart         pointer to \ref cs_restart_t structure
 * \param[in]       eqname          name of the related equation
 * \param[in]       scheme_context  pointer to a data structure cast on-the-fly
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_write_restart(cs_restart_t    *restart,
                            const char      *eqname,
                            void            *scheme_context);

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Predefined extra-operations related to this equation
 *
 * \param[in]       eqp        pointer to a cs_equation_param_t structure
 * \param[in, out]  eqb        pointer to a cs_equation_builder_t structure
 * \param[in, out]  context    pointer to cs_hho_vecteq_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_hho_vecteq_extra_post(const cs_equation_param_t  *eqp,
                         cs_equation_builder_t      *eqb,
                         void                       *context);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_HHO_VECTEQ_H__ */
