#ifndef CS_COMBUSTION_EBU_H
#define CS_COMBUSTION_EBU_H

/*============================================================================
 * EBU (Eddy Break-Up) gas combustion model.
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

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Standard library headers
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "base/cs_defs.h"
#include "base/cs_field.h"
#include "pprt/cs_physical_model.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/*============================================================================
 * Global variables
 *============================================================================*/

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief Initialize specific fields for EBU gas combustion model (first step).
 */
/*----------------------------------------------------------------------------*/

void
cs_combustion_ebu_fields_init0(void);

/*----------------------------------------------------------------------------*/
/*!
 * \brief Initialize specific fields for EBU gas combustion model (second step).
 */
/*----------------------------------------------------------------------------*/

void
cs_combustion_ebu_fields_init1(void);

/*----------------------------------------------------------------------------*/
/*
 * \brief Compute physical properties for EBU combustion model.
 *
 * \param[in, out]   mbrom    filling indicator of romb
 */
/*----------------------------------------------------------------------------*/

void
cs_combustion_ebu_physical_prop(int  *mbrom);

/*----------------------------------------------------------------------------*/
/*
 * \brief Compute physical properties for EBU combustion model.
 *
 * \param[in]      f_sc          pointer to scalar field
 * \param[in,out]  smbrs         explicit right hand side
 * \param[in,out]  rovsdt        implicit terms
 */
/*----------------------------------------------------------------------------*/

void
cs_combustion_ebu_source_terms(cs_field_t  *f_sc,
                               cs_real_t    smbrs[],
                               cs_real_t    rovsdt[]);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* CS_COMBUSTION_EBU_H */
