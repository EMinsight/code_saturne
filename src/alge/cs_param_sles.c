/*============================================================================
 * Routines to handle the SLES settings
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

#include "cs_defs.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_PETSC)
#include <petscconf.h> /* Useful to know if HYPRE is accessible through PETSc */
#endif

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include <bft_error.h>
#include <bft_mem.h>
#include <bft_printf.h>

#include "cs_base.h"
#include "cs_log.h"
#include "cs_param_cdo.h"

/*----------------------------------------------------------------------------
 * Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_param_sles.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*! \cond DOXYGEN_SHOULD_SKIP_THIS */

/*============================================================================
 * Type definitions
 *============================================================================*/

/*============================================================================
 * Local private variables
 *============================================================================*/

/*============================================================================
 * Private function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if PETSc or HYPRE is available and return the possible solver
 *        class
 *
 * \param[in] slesp              pointer to a \ref cs_param_sles_t structure
 * \param[in] petsc_mandatory    is PETSc mandatory with this settings
 *
 * \return a solver class
 */
/*----------------------------------------------------------------------------*/

static cs_param_solver_class_t
_get_petsc_or_hypre(const cs_param_sles_t  *slesp,
                    bool                    petsc_mandatory)
{
  assert(slesp != NULL);

  cs_param_solver_class_t  ret_class =
    cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

  if (ret_class != CS_PARAM_SOLVER_CLASS_PETSC && petsc_mandatory)
    bft_error(__FILE__, __LINE__, 0,
              " %s(): Eq. %s Error detected while setting \"CS_EQKEY_PRECOND\""
              " key.\n"
              " PETSc is needed but not available with your installation.\n"
              " Please check your installation settings.\n",
              __func__, slesp->name);

  if (slesp->solver_class == CS_PARAM_SOLVER_CLASS_HYPRE)
    ret_class = cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_HYPRE);

  if (ret_class != CS_PARAM_SOLVER_CLASS_HYPRE &&
      ret_class != CS_PARAM_SOLVER_CLASS_PETSC)
    bft_error(__FILE__, __LINE__, 0,
              " %s(): Eq. %s Error detected while setting \"CS_EQKEY_PRECOND\""
              " key.\n"
              " Neither PETSc nor HYPRE is available with your installation.\n"
              " Please check your installation settings.\n",
              __func__, slesp->name);

  return ret_class;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check if the setting related to the AMG is consistent with the
 *        solver class. If an issue is detected, try to solve it whith the
 *        nearest option.
 *
 * \param[in, out] slesp    pointer to a cs_param_sles_t structure
 */
/*----------------------------------------------------------------------------*/

static void
_check_amg_type(cs_param_sles_t   *slesp)
{
  if (slesp == NULL)
    return;
  if (slesp->precond != CS_PARAM_PRECOND_AMG)
    return;

  switch (slesp->solver_class) {

  case CS_PARAM_SOLVER_CLASS_PETSC:
#if defined(HAVE_PETSC)
    if (slesp->amg_type == CS_PARAM_AMG_INHOUSE_V ||
        slesp->amg_type == CS_PARAM_AMG_INHOUSE_K)
      slesp->amg_type = CS_PARAM_AMG_PETSC_GAMG_V;

    if (!cs_param_sles_hypre_from_petsc()) {
      if (slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_V)
        slesp->amg_type = CS_PARAM_AMG_PETSC_GAMG_V;
      else if (slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_W)
        slesp->amg_type = CS_PARAM_AMG_PETSC_GAMG_W;
    }
#else  /* PETSC is not available */
    bft_error(__FILE__, __LINE__, 0,
              " %s(): System \"%s\" PETSc is not available.\n"
              " Please check your installation settings.\n",
              __func__, slesp->name);
#endif
    break;

  case CS_PARAM_SOLVER_CLASS_HYPRE:
#if defined(HAVE_HYPRE)
    if (slesp->amg_type == CS_PARAM_AMG_INHOUSE_V ||
        slesp->amg_type == CS_PARAM_AMG_INHOUSE_K ||
        slesp->amg_type == CS_PARAM_AMG_PETSC_PCMG ||
        slesp->amg_type == CS_PARAM_AMG_PETSC_GAMG_V)
      slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_V;
    else if (slesp->amg_type == CS_PARAM_AMG_PETSC_GAMG_W)
      slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_W;
#else
#if defined(HAVE_PETSC)
    if (cs_param_sles_hypre_from_petsc()) {

      if (slesp->amg_type == CS_PARAM_AMG_INHOUSE_V ||
          slesp->amg_type == CS_PARAM_AMG_INHOUSE_K ||
          slesp->amg_type == CS_PARAM_AMG_PETSC_PCMG ||
          slesp->amg_type == CS_PARAM_AMG_PETSC_GAMG_V)
        slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_V;
      else if (slesp->amg_type == CS_PARAM_AMG_PETSC_GAMG_W)
        slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_W;

    }
    else
      bft_error(__FILE__, __LINE__, 0,
                " %s(): System \"%s\" HYPRE is not available.\n"
                " Please check your installation settings.\n",
                __func__, slesp->name);

#else  /* Neither HYPRE nor PETSC is available */
    bft_error(__FILE__, __LINE__, 0,
              " %s(): System \"%s\" HYPRE and PETSc are not available.\n"
              " Please check your installation settings.\n",
              __func__, slesp->name);
#endif  /* PETSc */
#endif  /* HYPRE */
    break;

  case CS_PARAM_SOLVER_CLASS_CS:
    if (slesp->amg_type == CS_PARAM_AMG_PETSC_PCMG ||
        slesp->amg_type == CS_PARAM_AMG_PETSC_GAMG_V ||
        slesp->amg_type == CS_PARAM_AMG_PETSC_GAMG_W ||
        slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_V ||
        slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_W)
      slesp->amg_type = CS_PARAM_AMG_INHOUSE_K;
    break;

  default:
    bft_error(__FILE__, __LINE__, 0,
              " %s(): System \"%s\" Incompatible setting detected.\n"
              " Please check your installation settings.\n",
              __func__, slesp->name);
    break; /* Nothing to do */
  }
}

/*! (DOXYGEN_SHOULD_SKIP_THIS) \endcond */

/*============================================================================
 * Public function prototypes
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief  Create a \ref cs_param_sles_t structure and assign a default
 *         settings
 *
 * \param[in] field_id      id related to to the variable field or -1
 * \param[in] system_name   name of the system to solve or NULL
 *
 * \return a pointer to a cs_param_sles_t stucture
 */
/*----------------------------------------------------------------------------*/

cs_param_sles_t *
cs_param_sles_create(int          field_id,
                     const char  *system_name)
{
  cs_param_sles_t  *slesp = NULL;

  BFT_MALLOC(slesp, 1, cs_param_sles_t);

  slesp->name = NULL;
  if (system_name != NULL) {
    size_t  len = strlen(system_name);
    BFT_MALLOC(slesp->name, len + 1, char);
    strncpy(slesp->name, system_name, len + 1);
  }

  slesp->field_id = field_id;                   /* associated field id */
  slesp->verbosity = 0;                         /* SLES verbosity */

  slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS; /* solver family */
  slesp->precond = CS_PARAM_PRECOND_DIAG;       /* preconditioner */
  slesp->solver = CS_PARAM_ITSOL_GCR;           /* iterative solver */
  slesp->flexible = false;                      /* not the flexible variant */
  slesp->restart = 15;                          /* restart after ? iterations */
  slesp->amg_type = CS_PARAM_AMG_NONE;          /* no predefined AMG type */

  slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE; /* no block by default */
  slesp->resnorm_type = CS_PARAM_RESNORM_FILTERED_RHS;

  slesp->cvg_param =  (cs_param_convergence_t) {
    .n_max_iter = 10000, /* max. number of iterations */
    .atol = 1e-15,       /* absolute tolerance */
    .rtol = 1e-6,        /* relative tolerance */
    .dtol = 1e3 };       /* divergence tolerance */

  slesp->context_param = NULL;

  return slesp;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Free a \ref cs_param_sles_t structure
 *
 * \param[in, out] slesp    pointer to a \cs_param_sles_t structure to free
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_free(cs_param_sles_t   **p_slesp)
{
  if (p_slesp == NULL)
    return;

  cs_param_sles_t  *slesp = *p_slesp;

  if (slesp == NULL)
    return;

  BFT_FREE(slesp->name);

  /* One asumes that this context has no pointer to free. This is the case up
     to now, since this process is totally managed by the code. */

  BFT_FREE(slesp->context_param);

  BFT_FREE(slesp);
  slesp = NULL;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Log information related to the linear settings stored in the
 *        structure
 *
 * \param[in] slesp    pointer to a \ref cs_param_sles_log
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_log(cs_param_sles_t   *slesp)
{
  if (slesp == NULL)
    return;

  cs_log_printf(CS_LOG_SETUP, "\n### %s | Linear algebra settings\n",
                slesp->name);
  cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Family:", slesp->name);
  if (slesp->solver_class == CS_PARAM_SOLVER_CLASS_CS)
    cs_log_printf(CS_LOG_SETUP, "              code_saturne\n");
  else if (slesp->solver_class == CS_PARAM_SOLVER_CLASS_MUMPS)
    cs_log_printf(CS_LOG_SETUP, "              MUMPS\n");
  else if (slesp->solver_class == CS_PARAM_SOLVER_CLASS_HYPRE)
    cs_log_printf(CS_LOG_SETUP, "              HYPRE\n");
  else if (slesp->solver_class == CS_PARAM_SOLVER_CLASS_PETSC)
    cs_log_printf(CS_LOG_SETUP, "              PETSc\n");

  cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Verbosity:           %d\n",
                slesp->name, slesp->verbosity);
  cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Field id:            %d\n",
                slesp->name, slesp->field_id);

  cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Solver.Name:         %s\n",
                slesp->name, cs_param_get_solver_name(slesp->solver));

  if (slesp->solver == CS_PARAM_ITSOL_MUMPS)
    cs_param_mumps_log(slesp->name, slesp->context_param);

  else { /* Iterative solvers */

    if (slesp->solver == CS_PARAM_ITSOL_AMG) {

      cs_log_printf(CS_LOG_SETUP, "  * %s | SLES AMG.Type:            %s\n",
                    slesp->name, cs_param_amg_get_type_name(slesp->amg_type));

      if (slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_V ||
          slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_W)
        cs_param_amg_boomer_log(slesp->name, slesp->context_param);

    }

    cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Solver.Precond:      %s\n",
                  slesp->name, cs_param_get_precond_name(slesp->precond));

    if (slesp->precond == CS_PARAM_PRECOND_AMG) {

      cs_log_printf(CS_LOG_SETUP, "  * %s | SLES AMG.Type:            %s\n",
                    slesp->name, cs_param_amg_get_type_name(slesp->amg_type));

      if (slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_V ||
          slesp->amg_type == CS_PARAM_AMG_HYPRE_BOOMER_W)
        cs_param_amg_boomer_log(slesp->name, slesp->context_param);

    }
    else if (slesp->precond == CS_PARAM_PRECOND_MUMPS)
      cs_param_mumps_log(slesp->name, slesp->context_param);

    cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Block.Precond:       %s\n",
                  slesp->name,
                  cs_param_get_precond_block_name(slesp->precond_block_type));

    cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Solver.max_iter:     %d\n",
                  slesp->name, slesp->cvg_param.n_max_iter);
    cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Solver.rtol:        % -10.6e\n",
                  slesp->name, slesp->cvg_param.rtol);
    cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Solver.atol:        % -10.6e\n",
                  slesp->name, slesp->cvg_param.atol);

    if (slesp->solver == CS_PARAM_ITSOL_GMRES ||
        slesp->solver == CS_PARAM_ITSOL_FGMRES ||
        slesp->solver == CS_PARAM_ITSOL_GCR)
      cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Solver.Restart:      %d\n",
                    slesp->name, slesp->restart);

    cs_log_printf(CS_LOG_SETUP, "  * %s | SLES Normalization:       ",
                  slesp->name);

    switch (slesp->resnorm_type) {
    case CS_PARAM_RESNORM_NORM2_RHS:
      cs_log_printf(CS_LOG_SETUP, "Euclidean norm of the RHS\n");
      break;
    case CS_PARAM_RESNORM_WEIGHTED_RHS:
      cs_log_printf(CS_LOG_SETUP, "Weighted Euclidean norm of the RHS\n");
      break;
    case CS_PARAM_RESNORM_FILTERED_RHS:
      cs_log_printf(CS_LOG_SETUP, "Filtered Euclidean norm of the RHS\n");
      break;
    case CS_PARAM_RESNORM_NONE:
    default:
      cs_log_printf(CS_LOG_SETUP, "None\n");
      break;
    }

  } /* Iterative solver */

  cs_log_printf(CS_LOG_SETUP, "\n");
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Copy a cs_param_sles_t structure from src to dst
 *
 * \param[in]      src    reference cs_param_sles_t structure to copy
 * \param[in, out] dst    copy of the reference at exit
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_copy_from(const cs_param_sles_t  *src,
                        cs_param_sles_t        *dst)
{
  if (src == NULL || dst == NULL)
    return;

  /* Remark: name is managed at the creation of the structure */

  dst->verbosity = src->verbosity;
  dst->field_id = src->field_id;

  dst->solver_class = src->solver_class;
  dst->precond = src->precond;
  dst->solver = src->solver;
  dst->amg_type = src->amg_type;
  dst->precond_block_type = src->precond_block_type;
  dst->resnorm_type = src->resnorm_type;

  dst->cvg_param.rtol = src->cvg_param.rtol;
  dst->cvg_param.atol = src->cvg_param.atol;
  dst->cvg_param.dtol = src->cvg_param.dtol;
  dst->cvg_param.n_max_iter = src->cvg_param.n_max_iter;

  if (dst->context_param != NULL)
    BFT_FREE(dst->context_param);

  if (dst->precond == CS_PARAM_PRECOND_MUMPS ||
      dst->solver == CS_PARAM_ITSOL_MUMPS)
    dst->context_param = cs_param_mumps_copy(src->context_param);

  else if (cs_param_amg_boomer_is_needed(dst->solver,
                                         dst->precond,
                                         dst->amg_type))
    dst->context_param = cs_param_amg_boomer_copy(src->context_param);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the solver associated to this SLES from its keyval
 *
 * \param[in]      keyval  value of the key
 * \param[in, out] slesp   pointer to a cs_param_sles_t structure
 *
 * \return an error code (> 0) or 0 if there is no error
 */
/*----------------------------------------------------------------------------*/

int
cs_param_sles_set_solver(const char       *keyval,
                         cs_param_sles_t  *slesp)
{
  int  ierr = 0;

  if (slesp == NULL)
    return ierr;

  const char  *sles_name = slesp->name;

  if (strcmp(keyval, "amg") == 0) {

    slesp->solver = CS_PARAM_ITSOL_AMG;
    slesp->amg_type = CS_PARAM_AMG_INHOUSE_K;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->precond = CS_PARAM_PRECOND_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;

  }
  else if (strcmp(keyval, "bicg") == 0) {

    slesp->solver = CS_PARAM_ITSOL_BICG;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "bicgstab2") == 0) {

    slesp->solver = CS_PARAM_ITSOL_BICGSTAB2;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "cg") == 0) {

    slesp->solver = CS_PARAM_ITSOL_CG;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "cr3") == 0) {

    slesp->solver = CS_PARAM_ITSOL_CR3;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "fcg") == 0) {

    slesp->solver = CS_PARAM_ITSOL_FCG;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "gauss_seidel") == 0 ||
           strcmp(keyval, "gs") == 0) {

    slesp->solver = CS_PARAM_ITSOL_GAUSS_SEIDEL;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->precond = CS_PARAM_PRECOND_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;

  }
  else if (strcmp(keyval, "gcr") == 0) {

    slesp->solver = CS_PARAM_ITSOL_GCR;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "gmres") == 0) {

    slesp->solver = CS_PARAM_ITSOL_GMRES;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "fgmres") == 0) {

    slesp->solver = CS_PARAM_ITSOL_FGMRES;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "jacobi") == 0 ||
           strcmp(keyval, "diag") == 0 ||
           strcmp(keyval, "diagonal") == 0) {

    slesp->solver = CS_PARAM_ITSOL_JACOBI;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->precond = CS_PARAM_PRECOND_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "minres") == 0) {

    slesp->solver = CS_PARAM_ITSOL_MINRES;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;
    slesp->flexible = false;

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

    if (ret_class != CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                " %s: SLES \"%s\" Error detected while setting \"%s\" key.\n"
                " PETSc is not available with your installation.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_ITSOL");
    else
      slesp->solver_class = ret_class;

  }
  else if (strcmp(keyval, "mumps") == 0) {

    slesp->solver = CS_PARAM_ITSOL_MUMPS;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_MUMPS;
    slesp->precond = CS_PARAM_PRECOND_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

    /* By default, one considers the stand-alone MUMPS library
     * MUMPS or PETSc are valid choices
     */

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_MUMPS);

    if (ret_class == CS_PARAM_N_SOLVER_CLASSES)
      bft_error(__FILE__, __LINE__, 0,
                " %s: SLES \"%s\" Error detected while setting \"%s\" key.\n"
                " MUMPS is not available with your installation.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_ITSOL");
    else
      slesp->solver_class = ret_class;

    assert(slesp->solver_class != CS_PARAM_SOLVER_CLASS_CS &&
           slesp->solver_class != CS_PARAM_SOLVER_CLASS_HYPRE);

    cs_param_sles_mumps_reset(slesp);

  }
  else if (strcmp(keyval, "sym_gauss_seidel") == 0 ||
           strcmp(keyval, "sgs") == 0) {

    slesp->solver = CS_PARAM_ITSOL_SYM_GAUSS_SEIDEL;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->precond = CS_PARAM_PRECOND_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "user") == 0) {

    slesp->solver = CS_PARAM_ITSOL_USER_DEFINED;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;

  }
  else if (strcmp(keyval, "none") == 0) {

    slesp->solver = CS_PARAM_ITSOL_NONE;
    slesp->precond = CS_PARAM_PRECOND_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;

  }
  else
    ierr = 1;

  return ierr;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the preconditioner associated to this SLES from its keyval
 *
 * \param[in]      keyval  value of the key
 * \param[in, out] slesp   pointer to a cs_param_sles_t structure
 *
 * \return an error code (> 0) or 0 if there is no error
 */
/*----------------------------------------------------------------------------*/

int
cs_param_sles_set_precond(const char       *keyval,
                          cs_param_sles_t  *slesp)
{
  int  ierr = 0;

  if (slesp == NULL)
    return ierr;

  const char  *sles_name = slesp->name;

  if (strcmp(keyval, "none") == 0) {

    slesp->precond = CS_PARAM_PRECOND_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "jacobi") == 0 || strcmp(keyval, "diag") == 0) {

    slesp->precond = CS_PARAM_PRECOND_DIAG;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "block_jacobi") == 0 ||
           strcmp(keyval, "bjacobi") == 0) {

    /* Either with PETSc or with PETSc/HYPRE using Euclid. In both cases,
       PETSc is mandatory --> second parameter = "true" */

    slesp->solver_class = _get_petsc_or_hypre(slesp, true);

    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_DIAG;
    slesp->precond = CS_PARAM_PRECOND_BJACOB_ILU0;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "bjacobi_sgs") == 0 ||
           strcmp(keyval, "bjacobi_ssor") == 0) {

    /* Only available through the PETSc library */

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

    if (ret_class != CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                " %s(): SLES \"%s\" Error detected while setting \"%s\" key.\n"
                " PETSc is not available with your installation.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_PRECOND");

    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;

    slesp->precond = CS_PARAM_PRECOND_BJACOB_SGS;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_DIAG;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "lu") == 0) {

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

    if (ret_class != CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                " %s(): SLES \"%s\" Error detected while setting \"%s\" key.\n"
                " PETSc is not available with your installation.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_PRECOND");

    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;

    slesp->precond = CS_PARAM_PRECOND_LU;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "ilu0") == 0) {

    /* Either with PETSc or with PETSc/HYPRE or HYPRE using Euclid */

    slesp->solver_class = _get_petsc_or_hypre(slesp, false);

    slesp->precond = CS_PARAM_PRECOND_ILU0;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "icc0") == 0) {

    /* Either with PETSc or with PETSc/HYPRE or HYPRE using Euclid */

    slesp->solver_class = _get_petsc_or_hypre(slesp, false);

    slesp->precond = CS_PARAM_PRECOND_ICC0;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "amg") == 0) {

    slesp->precond = CS_PARAM_PRECOND_AMG;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;

    slesp->flexible = true;

    switch (slesp->solver) {
    case CS_PARAM_ITSOL_CG:
      cs_base_warn(__FILE__, __LINE__);
      cs_log_printf(CS_LOG_WARNINGS,
                    "%s() SLES \"%s\"\n"
                    " >> Switch to a flexible variant for CG.\n",
                    __func__, sles_name);

      slesp->solver = CS_PARAM_ITSOL_FCG;
      break;

    case CS_PARAM_ITSOL_GMRES:
    case CS_PARAM_ITSOL_CR3:
    case CS_PARAM_ITSOL_BICG:
    case CS_PARAM_ITSOL_BICGSTAB2:
      cs_base_warn(__FILE__, __LINE__);
      cs_log_printf(CS_LOG_WARNINGS,
                    "%s() SLES \"%s\"\n"
                    " >> Switch to a flexible variant: GCR solver.\n",
                    __func__, sles_name);

      slesp->solver = CS_PARAM_ITSOL_GCR;
      break;

    default:
      break;  /* Do nothing */

    }

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(slesp->solver_class);

    /* Set the default AMG choice according to the class of solver */

    switch (ret_class) {

    case CS_PARAM_SOLVER_CLASS_CS:
      slesp->amg_type = CS_PARAM_AMG_INHOUSE_K;
      break;
    case CS_PARAM_SOLVER_CLASS_PETSC:
      slesp->amg_type = CS_PARAM_AMG_PETSC_GAMG_V;
      break;
    case CS_PARAM_SOLVER_CLASS_HYPRE:
      slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_V;
      cs_param_sles_boomeramg_reset(slesp);
      break;

    default:
      ierr = 2;
      return ierr;

    } /* End of switch */

  }
  else if (strcmp(keyval, "amg_block") == 0 ||
           strcmp(keyval, "block_amg") == 0) {

    slesp->precond = CS_PARAM_PRECOND_AMG;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_DIAG;
    slesp->flexible = true;

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(slesp->solver_class);

    /* Set the default AMG choice according to the class of solver */

    switch (ret_class) {

    case CS_PARAM_SOLVER_CLASS_CS:
      slesp->amg_type = CS_PARAM_AMG_INHOUSE_K;
      break;

    case CS_PARAM_SOLVER_CLASS_PETSC:
      slesp->amg_type = CS_PARAM_AMG_PETSC_GAMG_V;
      break;

    case CS_PARAM_SOLVER_CLASS_HYPRE:
      slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_V;

      if (cs_param_sles_hypre_from_petsc())
        slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;
      else { /* No block is used in this case */

        slesp->solver_class = CS_PARAM_SOLVER_CLASS_HYPRE;
        slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;

        cs_base_warn(__FILE__, __LINE__);
        cs_log_printf(CS_LOG_WARNINGS,
                      "%s(): SLES \"%s\". Switch to HYPRE.\n"
                      "No block preconditioner will be used.",
                      __func__, sles_name);

      }
      break;

    default:
      ierr = 2;
      return ierr;

    } /* End of switch */

  }
  else if (strcmp(keyval, "mumps") == 0) {

    slesp->flexible = false;
    slesp->precond = CS_PARAM_PRECOND_MUMPS;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;

    /* Only MUMPS is a valid choice in this situation */

    if (cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_MUMPS) !=
        CS_PARAM_SOLVER_CLASS_MUMPS)
      bft_error(__FILE__, __LINE__, 0,
                " %s(): SLES \"%s\" Error detected while setting \"%s\" key.\n"
                " MUMPS is not available with your installation.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_PRECOND");


  }
  else if (strcmp(keyval, "poly1") == 0) {

    slesp->precond = CS_PARAM_PRECOND_POLY1;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "poly2") == 0) {

    slesp->precond = CS_PARAM_PRECOND_POLY2;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = false;

  }
  else if (strcmp(keyval, "ssor") == 0) {

    slesp->precond = CS_PARAM_PRECOND_SSOR;
    slesp->precond_block_type = CS_PARAM_PRECOND_BLOCK_NONE;
    slesp->amg_type = CS_PARAM_AMG_NONE;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;
    slesp->flexible = false;

    if (cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC) !=
        CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                " %s(): SLES \"%s\" Error detected while setting \"%s\" key.\n"
                " PETSc is not available with your installation.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_PRECOND");

  }
  else
    ierr = 1;

  /* Default when using PETSc */

  if (slesp->solver_class == CS_PARAM_SOLVER_CLASS_PETSC)
    slesp->resnorm_type = CS_PARAM_RESNORM_NORM2_RHS;

  return ierr;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the class of solvers associated to this SLES from its keyval
 *        Common choices are "petsc", "hypre" or "mumps"
 *
 * \param[in]      keyval  value of the key
 * \param[in, out] slesp   pointer to a cs_param_sles_t structure
 *
 * \return an error code (> 0) or 0 if there is no error
 */
/*----------------------------------------------------------------------------*/

int
cs_param_sles_set_solver_class(const char       *keyval,
                               cs_param_sles_t  *slesp)
{
  int  ierr = 0;

  if (slesp == NULL)
    return ierr;

  const char  *sles_name = slesp->name;

  if (strcmp(keyval, "cs") == 0 || strcmp(keyval, "saturne") == 0) {

    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;

    if (slesp->precond == CS_PARAM_PRECOND_AMG)
      _check_amg_type(slesp);

  }
  else if (strcmp(keyval, "hypre") == 0) {

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_HYPRE);

    if (ret_class == CS_PARAM_N_SOLVER_CLASSES)
      bft_error(__FILE__, __LINE__, 0,
                " %s: Eq. %s Error detected while setting \"%s\" key.\n"
                " Neither PETSc nor HYPRE is available.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_SOLVER_FAMILY");
    else if (ret_class == CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                " %s: Eq. %s Error detected while setting \"%s\" key.\n"
                " PETSc with HYPRE is not available.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_SOLVER_FAMILY");

    slesp->solver_class = CS_PARAM_SOLVER_CLASS_HYPRE;

    /* Check that the AMG type is correctly set */

    if (slesp->precond == CS_PARAM_PRECOND_AMG) {

      _check_amg_type(slesp);
      cs_param_sles_boomeramg_reset(slesp);

    }

  }
  else if (strcmp(keyval, "mumps") == 0) {

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_MUMPS);

    if (ret_class == CS_PARAM_N_SOLVER_CLASSES)
      bft_error(__FILE__, __LINE__, 0,
                " %s(): Eq. %s Error detected while setting \"%s\" key.\n"
                " MUMPS is not available.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_SOLVER_FAMILY");

    slesp->solver_class = ret_class; /* PETSc or MUMPS */

  }
  else if (strcmp(keyval, "petsc") == 0) {

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

    if (ret_class == CS_PARAM_N_SOLVER_CLASSES)
      bft_error(__FILE__, __LINE__, 0,
                " %s(): Eq. %s Error detected while setting \"%s\" key.\n"
                " PETSc is not available.\n"
                " Please check your installation settings.\n",
                __func__, sles_name, "CS_EQKEY_SOLVER_FAMILY");

    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;

    /* Check that the AMG type is correctly set */

    if (slesp->precond == CS_PARAM_PRECOND_AMG)
      _check_amg_type(slesp);

  }

  return ierr;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the type of algebraic multigrid (AMG) associated to this SLES
 *        from its keyval
 *
 * \param[in]      keyval  value of the key
 * \param[in, out] slesp   pointer to a cs_param_sles_t structure
 *
 * \return an error code (> 0) or 0 if there is no error
 */
/*----------------------------------------------------------------------------*/

int
cs_param_sles_set_amg_type(const char       *keyval,
                           cs_param_sles_t  *slesp)
{
  int  ierr = 0;

  if (slesp == NULL)
    return ierr;

  const char  *sles_name = slesp->name;

  if (strcmp(keyval, "v_cycle") == 0) {

    slesp->amg_type = CS_PARAM_AMG_INHOUSE_V;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "k_cycle") == 0 || strcmp(keyval, "kamg") == 0) {

    slesp->amg_type = CS_PARAM_AMG_INHOUSE_K;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_CS;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "boomer") == 0 || strcmp(keyval, "bamg") == 0 ||
           strcmp(keyval, "boomer_v") == 0) {

    cs_param_solver_class_t  wanted_class = CS_PARAM_SOLVER_CLASS_HYPRE;
    if (slesp->precond_block_type != CS_PARAM_PRECOND_BLOCK_NONE)
      wanted_class = CS_PARAM_SOLVER_CLASS_PETSC;

    cs_param_solver_class_t ret_class =
      cs_param_sles_check_class(wanted_class);

    slesp->flexible = true;
    slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_V;
    slesp->solver_class = ret_class;

    cs_param_sles_boomeramg_reset(slesp);

  }
  else if (strcmp(keyval, "boomer_w") == 0 || strcmp(keyval, "bamg_w") == 0) {

    cs_param_solver_class_t  wanted_class = CS_PARAM_SOLVER_CLASS_HYPRE;
    if (slesp->precond_block_type != CS_PARAM_PRECOND_BLOCK_NONE)
      wanted_class = CS_PARAM_SOLVER_CLASS_PETSC;

    cs_param_solver_class_t ret_class =
      cs_param_sles_check_class(wanted_class);

    slesp->flexible = true;
    slesp->amg_type = CS_PARAM_AMG_HYPRE_BOOMER_W;
    slesp->solver_class = ret_class;

    cs_param_sles_boomeramg_reset(slesp);

  }
  else if (strcmp(keyval, "gamg") == 0 || strcmp(keyval, "gamg_v") == 0) {

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

    if (ret_class != CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                "%s: Eq. %s\n Invalid choice of AMG type.\n"
                " PETSc is not available."
                " Please check your settings.", __func__, sles_name);

    slesp->amg_type = CS_PARAM_AMG_PETSC_GAMG_V;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "gamg_w") == 0) {

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

    if (ret_class != CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                "%s: Eq. %s\n Invalid choice of AMG type.\n"
                " PETSc is not available."
                " Please check your settings.", __func__, sles_name);

    slesp->amg_type = CS_PARAM_AMG_PETSC_GAMG_W;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;
    slesp->flexible = true;

  }
  else if (strcmp(keyval, "pcmg") == 0) {

    cs_param_solver_class_t  ret_class =
      cs_param_sles_check_class(CS_PARAM_SOLVER_CLASS_PETSC);

    if (ret_class != CS_PARAM_SOLVER_CLASS_PETSC)
      bft_error(__FILE__, __LINE__, 0,
                "%s: Eq. %s\n Invalid choice of AMG type.\n"
                " PETSc is not available."
                " Please check your settings.", __func__, sles_name);

    slesp->amg_type = CS_PARAM_AMG_PETSC_PCMG;
    slesp->solver_class = CS_PARAM_SOLVER_CLASS_PETSC;
    slesp->flexible = true;

  }
  else
    slesp->amg_type = CS_PARAM_AMG_NONE;

  return ierr;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the convergence criteria for the given SLES parameters. One can
 *        use the parameter value CS_CDO_KEEP_DEFAULT to let unchange the
 *        settings.
 *
 * \param[in, out] slesp    pointer to a cs_param_sles_t structure
 * \param[in]      rtol     relative tolerance
 * \param[in]      atol     absolute tolerance
 * \param[in]      dtol     divergence tolerance
 * \param[in]      max_iter max. number of iterations
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_set_cvg_param(cs_param_sles_t  *slesp,
                            double            rtol,
                            double            atol,
                            double            dtol,
                            int               max_iter)
{
  if (slesp == NULL)
    return;

  /* Absolute tolerance */

  if (fabs(atol - CS_CDO_KEEP_DEFAULT) > FLT_MIN)
    slesp->cvg_param.atol = atol;

  /* Relative tolerance */

  if (fabs(rtol - CS_CDO_KEEP_DEFAULT) > FLT_MIN)
    slesp->cvg_param.rtol = rtol;

  /* Divergence tolerance */

  if (fabs(dtol - CS_CDO_KEEP_DEFAULT) > FLT_MIN)
    slesp->cvg_param.dtol = dtol;

  /* Max. number of iterations */

  if (max_iter - CS_CDO_KEEP_DEFAULT != 0)
    slesp->cvg_param.n_max_iter = max_iter;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Allocate and initialize a new context structure for the boomerAMG
 *        settings.
 *
 * \param[in, out] slesp        pointer to a cs_param_sles_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_boomeramg_reset(cs_param_sles_t  *slesp)
{
  if (slesp == NULL)
    return;

  if (slesp->context_param != NULL)
    BFT_FREE(slesp->context_param);

  slesp->context_param = cs_param_amg_boomer_create();
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the main members of a cs_param_amg_boomer_t structure. This
 *        structure is allocated and initialized and then one sets the main
 *        given parameters. Please refer to the HYPRE user guide for more
 *        details about the following options.
 *
 * \param[in, out] slesp           pointer to a cs_param_sles_t structure
 * \param[in]      n_down_iter     number of smoothing steps for the down cycle
 * \param[in]      down_smoother   type of smoother for the down cycle
 * \param[in]      n_up_iter       number of smoothing steps for the up cycle
 * \param[in]      up_smoother     type of smoother for th up cycle
 * \param[in]      coarse_solver   solver at the coarsest level
 * \param[in]      coarsen_algo    type of algoritmh for the coarsening
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_boomeramg(cs_param_sles_t                    *slesp,
                        int                                 n_down_iter,
                        cs_param_amg_boomer_smoother_t      down_smoother,
                        int                                 n_up_iter,
                        cs_param_amg_boomer_smoother_t      up_smoother,
                        cs_param_amg_boomer_smoother_t      coarse_solver,
                        cs_param_amg_boomer_coarsen_algo_t  coarsen_algo)
{
  if (slesp == NULL)
    return;

  cs_param_sles_boomeramg_reset(slesp);

  cs_param_amg_boomer_t  *bamgp = slesp->context_param;

  bamgp->n_down_iter = n_down_iter;
  bamgp->down_smoother = down_smoother;

  bamgp->n_up_iter = n_up_iter;
  bamgp->up_smoother = up_smoother;

  bamgp->coarse_solver = coarse_solver;
  bamgp->coarsen_algo = coarsen_algo;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the members of a cs_param_amg_boomer_t structure used in
 *        advanced settings. This structure is allocated if needed. Other
 *        members are kept to their values. Please refer to the HYPRE user
 *        guide for more details about the following options.
 *
 * \param[in, out] slesp            pointer to a cs_param_sles_t structure
 * \param[in]      strong_thr       value of the strong threshold (coarsening)
 * \param[in]      interp_algo      algorithm used for the interpolation
 * \param[in]      p_max            max number of elements per row (interp)
 * \param[in]      n_agg_lv         aggressive coarsening (number of levels)
 * \param[in]      n_agg_paths      aggressive coarsening (number of paths)
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_boomeramg_advanced(cs_param_sles_t                   *slesp,
                                 double                             strong_thr,
                                 cs_param_amg_boomer_interp_algo_t  interp_algo,
                                 int                                p_max,
                                 int                                n_agg_lv,
                                 int                                n_agg_paths)
{
  if (slesp == NULL)
    return;

  if (slesp->context_param == NULL)
    slesp->context_param = cs_param_amg_boomer_create();

  /* One assumes that the existing context structure is related to boomeramg */

  cs_param_amg_boomer_t  *bamgp = slesp->context_param;

  bamgp->strong_threshold = strong_thr;
  bamgp->interp_algo = interp_algo;
  bamgp->p_max = p_max;
  bamgp->n_agg_levels = n_agg_lv;
  bamgp->n_agg_paths = n_agg_paths;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Allocate and initialize a new context structure for the MUMPS
 *        settings.
 *
 * \param[in, out] slesp         pointer to a cs_param_sles_t structure
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_mumps_reset(cs_param_sles_t  *slesp)
{
  if (slesp == NULL)
    return;

  if (slesp->context_param != NULL)
    BFT_FREE(slesp->context_param);  /* Up to now the context structures have
                                        no allocation inside */

  /* Allocate and initialize a structure to store the MUMPS settings */

  slesp->context_param = cs_param_mumps_create();
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the main members of a cs_param_mumps_t structure. This structure
 *        is allocated and initialized with default settings if needed. If the
 *        structure exists already, then advanced members are kept to their
 *        values.
 *
 * \param[in, out] slesp         pointer to a cs_param_sles_t structure
 * \param[in]      is_single     single-precision or double-precision
 * \param[in]      facto_type    type of factorization to consider
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_mumps(cs_param_sles_t             *slesp,
                    bool                         is_single,
                    cs_param_mumps_facto_type_t  facto_type)
{
  if (slesp == NULL)
    return;

  cs_param_sles_mumps_reset(slesp);

  cs_param_mumps_t  *mumpsp = slesp->context_param;

  mumpsp->is_single = is_single;
  mumpsp->facto_type = facto_type;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Set the members related to an advanced settings of a cs_param_mumps_t
 *        structure. This structure is allocated and initialized if
 *        needed. Please refer to the MUMPS user guide for more details about
 *        the following advanced options.
 *
 * \param[in, out] slesp            pointer to a cs_param_sles_t structure
 * \param[in]      analysis_algo    algorithm used for the analysis step
 * \param[in]      block_analysis   > 1: fixed block size; otherwise do nothing
 * \param[in]      mem_coef         percentage increase in the memory workspace
 * \param[in]      blr_threshold    Accuracy in BLR compression (0: not used)
 * \param[in]      ir_steps         0: No, otherwise the number of iterations
 * \param[in]      mem_usage        strategy to adopt for the memory usage
 * \param[in]      advanced_optim   activate advanced optimization (MPI/openMP)
 */
/*----------------------------------------------------------------------------*/

void
cs_param_sles_mumps_advanced(cs_param_sles_t                *slesp,
                             cs_param_mumps_analysis_algo_t  analysis_algo,
                             int                             block_analysis,
                             double                          mem_coef,
                             double                          blr_threshold,
                             int                             ir_steps,
                             cs_param_mumps_memory_usage_t   mem_usage,
                             bool                            advanced_optim)
{
  if (slesp == NULL)
    return;

  if (slesp->context_param == NULL)
    slesp->context_param = cs_param_mumps_create();

  /* One assumes that the existing context structure is related to MUMPS */

  cs_param_mumps_t  *mumpsp = slesp->context_param;

  mumpsp->analysis_algo = analysis_algo;
  mumpsp->block_analysis = block_analysis;
  mumpsp->mem_coef = mem_coef;
  mumpsp->blr_threshold = blr_threshold;
  mumpsp->ir_steps = CS_MAX(ir_steps, -ir_steps);
  mumpsp->mem_usage = mem_usage;
  mumpsp->advanced_optim = advanced_optim;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check the availability of Hypre solvers from the PETSc library
 *
 * \return return true or false
 */
/*----------------------------------------------------------------------------*/

bool
cs_param_sles_hypre_from_petsc(void)
{
#if defined(HAVE_PETSC)
#if defined(PETSC_HAVE_HYPRE)
  return true;
#else
  return false;
#endif
#else
  return false;
#endif
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Check the availability of a solver library and return the requested
 *        one if this is possible or an alternative or CS_PARAM_N_SOLVER_CLASSES
 *        if no alternative is available.
 *
 * \param[in] wanted_class  requested class of solvers
 *
 * \return the available solver class related to the requested class
 */
/*----------------------------------------------------------------------------*/

cs_param_solver_class_t
cs_param_sles_check_class(cs_param_solver_class_t  wanted_class)
{
  switch (wanted_class) {

  case CS_PARAM_SOLVER_CLASS_CS:  /* No issue */
    return CS_PARAM_SOLVER_CLASS_CS;

  case CS_PARAM_SOLVER_CLASS_HYPRE:
    /* ------------------------- */
#if defined(HAVE_HYPRE)
    return CS_PARAM_SOLVER_CLASS_HYPRE;
#else
#if defined(HAVE_PETSC)
    if (cs_param_sles_hypre_from_petsc())
      return CS_PARAM_SOLVER_CLASS_HYPRE;
    else {
      cs_base_warn(__FILE__, __LINE__);
      cs_log_printf(CS_LOG_WARNINGS,
                    "%s: Switch to PETSc library since Hypre is not available",
                    __func__);
      return CS_PARAM_SOLVER_CLASS_PETSC; /* Switch to PETSc */
    }
#else
    return CS_PARAM_N_SOLVER_CLASSES;     /* Neither HYPRE nor PETSc */
#endif  /* PETSc */
#endif  /* HYPRE */

  case CS_PARAM_SOLVER_CLASS_PETSC:
    /* ------------------------- */
#if defined(HAVE_PETSC)
    return CS_PARAM_SOLVER_CLASS_PETSC;
#else
    return CS_PARAM_N_SOLVER_CLASSES;
#endif

  case CS_PARAM_SOLVER_CLASS_MUMPS:
    /* ------------------------- */
#if defined(HAVE_MUMPS)
    return CS_PARAM_SOLVER_CLASS_MUMPS;
#else
#if defined(HAVE_PETSC)
#if defined(PETSC_HAVE_MUMPS)
    cs_base_warn(__FILE__, __LINE__);
    cs_log_printf(CS_LOG_WARNINGS,
                  "%s: Switch to PETSc library since MUMPS is not available as"
                  " a stand-alone library\n", __func__);
    return CS_PARAM_SOLVER_CLASS_PETSC;
#else
    return CS_PARAM_N_SOLVER_CLASSES;
#endif  /* PETSC_HAVE_MUMPS */
#else
    return CS_PARAM_N_SOLVER_CLASSES; /* PETSc without MUMPS  */
#endif  /* HAVE_PETSC */
    return CS_PARAM_N_SOLVER_CLASSES; /* Neither MUMPS nor PETSc */
#endif

  default:
    return CS_PARAM_N_SOLVER_CLASSES;
  }
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
