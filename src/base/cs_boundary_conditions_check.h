#ifndef __CS_BOUNDARY_CONDITIONS_CHECK_H__
#define __CS_BOUNDARY_CONDITIONS_CHECK_H__

/*============================================================================
 * Handle boundary condition type codes.
 *============================================================================*/

/*
  This file is part of code_saturne, a general-purpose CFD tool.

  Copyright (C) 1998-2024 EDF S.A.

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
 * Standard C library headers
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "base/cs_defs.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*! \file cs_boundary_conditions_check.c
 *
 * \brief Check boundary condition codes.
 */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*
 * \param[in]  bc_type        type per boundary face
 * \param[int] ale_bc_type    ALE type per boundary face
 */
/*----------------------------------------------------------------------------*/

void
cs_boundary_conditions_check(int   bc_type[],
                             int   ale_bc_type[]);

/*----------------------------------------------------------------------------*/

END_C_DECLS

#endif /* __CS_BOUNDARY_CONDITIONS_CHECK_H__ */
