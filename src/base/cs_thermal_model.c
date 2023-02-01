/*============================================================================
 * Base thermal model data.
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

#include "cs_defs.h"

/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*----------------------------------------------------------------------------
 * Local headers
 *----------------------------------------------------------------------------*/

#include "bft_mem.h"
#include "bft_error.h"
#include "bft_printf.h"

#include "cs_field.h"
#include "cs_field_pointer.h"
#include "cs_log.h"
#include "cs_map.h"
#include "cs_parall.h"
#include "cs_mesh_location.h"
#include "cs_physical_constants.h"
#include "cs_air_props.h"
#include "cs_mesh_location.h"

#include "cs_field_operator.h"
#include "cs_field_pointer.h"
#include "cs_field_default.h"
#include "cs_xdef.h"
#include "cs_cf_model.h"

/*----------------------------------------------------------------------------
 * Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_thermal_model.h"

/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Additional doxygen documentation
 *============================================================================*/

/*!
  \file cs_thermal_model.c
        base thermal model data.

  \struct cs_thermal_model_t

  \brief Thermal model descriptor.

  Members of this thermal model are publicly accessible, to allow for concise
  syntax, as it is expected to be used in many places.

  \var  cs_thermal_model_t::thermal_variable
        Thermal variable solved for this physical model.
           - 0: no thermal model
           - 1: temperature
           - 2: enthalpy
           - 3: total energy (only for compressible module)
           - 4: internal energy
  \var  cs_thermal_model_t::itherm
        \deprecated alias/old name for thermal_variable

  \var  cs_thermal_model_t::temperature_scale
        Temperature scale
        The specification of the temperature scale in a consistent
        manner with the values used (initial and boundary conditions)
        is especially important in case of radiation modelling.
        - 0: none
        - 1: Kelvin
        - 2: Celsius
  \var  cs_thermal_model_t::itpscl
        \deprecated alias/old name for temperature_scale
  \var  cs_thermal_model_t::has_kinetic_st
        Take kinetic source terme in energy equation into account
        (see Amino, Flageul, Carissimo, Tiselj, Benhamadouche, Ferrand 2022)
        - 0 (default)
        - 1
  \var  cs_thermal_model_t::cflt
        Take kinetic source terme in energy equation into account
        (see Amino, Flageul, Carissimo, Tiselj, Benhamadouche, Ferrand 2022)
        - false (default)
        - true
  \var  cs_thermal_model_t::cflp
        Take kinetic source terme in energy equation into account
        (see Amino, Flageul, Carissimo, Tiselj, Benhamadouche, Ferrand 2022)
        - false (default)
        - true
  \var  cs_thermal_model_t::has_pdivu
        Add to the right hand side the term equal to -p div(u)
  \var  cs_thermal_model_t::has_dissipation
        Add to the right hand side the thermal dissipation term
  \var  cs_thermal_model_t::unstd_multiplicator
        Multiply the unsteady thermal equation term by :
        - 0 : multiplication by 1
        - 1 : multiplication by cp
        - 2 : multiplication by cv

*/

/*! \cond DOXYGEN_SHOULD_SKIP_THIS */

/*=============================================================================
 * Macro definitions
 *============================================================================*/

/*============================================================================
 * Type definitions
 *============================================================================*/

/*============================================================================
 * Static global variables
 *============================================================================*/

/* main thermal model structure and associated pointer */

static cs_thermal_model_t  _thermal_model = {
  .thermal_variable = -999,
  .temperature_scale = 1,
  .has_kinetic_st = 0,
  .cflt = false,
  .cflp = false,
  .has_pdivu = 0,
  .has_dissipation = 0,
  .unstd_multiplicator = -1,
  };

const cs_thermal_model_t  *cs_glob_thermal_model = &_thermal_model;

/*============================================================================
 * Prototypes for functions intended for use only by Fortran wrappers.
 * (descriptions follow, with function bodies).
 *============================================================================*/

void
cs_f_thermal_model_get_pointers(int     **itherm,
                                int     **itpscl,
                                int     **unstd_multiplicator);

/*! (DOXYGEN_SHOULD_SKIP_THIS) \endcond */

/*============================================================================
 * Private function definitions
 *============================================================================*/

/*============================================================================
 * Fortran wrapper function definitions
 *============================================================================*/

/*! \cond DOXYGEN_SHOULD_SKIP_THIS */

/*----------------------------------------------------------------------------
 * Get pointers to members of the global thermal model structure.
 *
 * This function is intended for use by Fortran wrappers, and
 * enables mapping to Fortran global pointers.
 *
 * parameters:
 *   itherm              --> pointer to cs_glob_thermal_model->thermal_variable
 *   itpscl              --> pointer to cs_glob_thermal_model->temperature_scale
 *   unstd_multiplicator --> pointer
 *                           to cs_glob_thermal_model->unstd_multiplicator
 *----------------------------------------------------------------------------*/

void
cs_f_thermal_model_get_pointers(int     **itherm,
                                int     **itpscl,
                                int     **unstd_multiplicator)
{
  *itherm = &(_thermal_model.thermal_variable);
  *itpscl = &(_thermal_model.temperature_scale);
  *unstd_multiplicator = &(_thermal_model.unstd_multiplicator);
}

/*! (DOXYGEN_SHOULD_SKIP_THIS) \endcond */

/*=============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------
 *!
 * \brief Return thermal field (temperature, enthalpy, total energy according to
 *        thermal model).
 *
 * \return   pointer to thermal field
 *----------------------------------------------------------------------------*/

cs_field_t *
cs_thermal_model_field(void)
{
  cs_field_t *th_f;
  switch (_thermal_model.itherm) {
  case CS_THERMAL_MODEL_TEMPERATURE:
    th_f = CS_F_(t);
    break;
  case CS_THERMAL_MODEL_ENTHALPY:
    th_f = CS_F_(h);
    break;
  case CS_THERMAL_MODEL_TOTAL_ENERGY:
    th_f = CS_F_(e_tot);
    break;
  default:
    th_f = NULL;
  }

  return th_f;
}

/*----------------------------------------------------------------------------
 *!
 * \brief Provide access to cs_glob_thermal_model
 *
 * needed to initialize structure with GUI
 *----------------------------------------------------------------------------*/

cs_thermal_model_t *
cs_get_glob_thermal_model(void)
{
  return &_thermal_model;
}

/*----------------------------------------------------------------------------
 *!
 * \brief Print the thermal model structure to setup.log.
 *
 *----------------------------------------------------------------------------*/

void
cs_thermal_model_log_setup(void)
{
  int itherm = cs_glob_thermal_model->itherm;
  int itpscl = cs_glob_thermal_model->itpscl;

  cs_log_printf(CS_LOG_SETUP,
                ("\n"
                 "Thermal model options\n"
                 "---------------------\n\n"
                 "  Continuous phase:\n\n"));

  const char *itherm_value_str[]
    = {N_("no thermal model"),
       N_("temperature)"),
       N_("enthalpy"),
       N_("total energy")};

  const char *itpscl_value_str[]
    = {N_("none"),
       N_("temperature in Kelvin"),
       N_("temperature in Celsius")};

  cs_log_printf(CS_LOG_SETUP,
                ("    Thermal model\n"));
  cs_log_printf(CS_LOG_SETUP,
                _("    itherm:    %d (%s)\n"),
                itherm, _(itherm_value_str[itherm]));

  cs_log_printf(CS_LOG_SETUP,
                ("    Temperature scale\n"));
  cs_log_printf(CS_LOG_SETUP,
                _("    itpscl:    %d (%s)\n"),
                itpscl, _(itpscl_value_str[itpscl]));

  cs_field_t *tf = cs_thermal_model_field();
  if (tf != NULL)
    cs_log_printf
      (CS_LOG_SETUP,
       _("    Thermal variable solved: %s (field id %d)\n"),
       tf->name, tf->id);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Initialize thermal variables if needed
 */
/*----------------------------------------------------------------------------*/

void
cs_thermal_model_init(void)
{
  cs_real_t *xcvv = cs_field_by_name("isobaric_heat_capacity")->val;
  cs_thermal_model_cv(xcvv);
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the inverse of the square of sound velocity multiplied
 *        by gamma.
 *
 * \param[in]      cp      array of isobaric specific heat values for dry air
 * \param[in]      cpv     array of isobaric specific heat values for moist air
 * \param[in]      l00     latent heat
 * \param[in]      temp    array of temperature values
 * \param[in]      pres    array of pressure values
 * \param[in,out]  fracv   array of volume fraction values
 * \param[in,out]  fracm   array of mass fraction values
 * \param[in,out]  frace   array of energy fraction values
 * \param[out]     dc2      array of the values of the square of sound velocity
 * \param[in]      l_size  l_size of the array
 */
/*----------------------------------------------------------------------------*/

void
cs_thermal_model_c_square(cs_real_t  *cp,
                          cs_real_t   cpv,
                          cs_real_t   cpl,
                          cs_real_t   l00,
                          cs_real_t  *temp,
                          cs_real_t  *pres,
                          cs_real_t  *fracv,
                          cs_real_t  *fracm,
                          cs_real_t  *frace,
                          cs_real_t  *dc2,
                          cs_lnum_t   l_size)
{
  /*  Local variables */
  int ieos = cs_glob_cf_model->ieos;
  cs_real_t rair = cs_glob_fluid_properties->r_pg_cnst;
  const cs_fluid_properties_t *phys_pro = cs_get_glob_fluid_properties();
  cs_real_t rvsra = phys_pro->rvsra;
  /* no specific eos : the pressure equation is a Poisson equation */
  cs_field_t *fhyd = cs_field_by_name_try("H2");

  /* Ideal gas */
  if (ieos == CS_EOS_GAS_MIX && fhyd != NULL) {
    /* WIP : only available in this function for hydrogen and air */
    cs_real_t rh = 4157.; /* R/MH2 */
    cs_real_t *yhyd = cs_field_by_name("H2")->val;
    for (cs_lnum_t ii = 0; ii < l_size; ii++)
      dc2[ii] = 1. / (temp[ii]*((1. - yhyd[ii])*rair) + yhyd[ii]*rh);
  }
  else if (ieos == CS_EOS_IDEAL_GAS) {
     for (cs_lnum_t ii = 0; ii < l_size; ii++)
      dc2[ii] = 1. / (rair * temp[ii]);
  }

  /* Ideal gas mixture (only water accounted for). TODO : other gases */
  else if (ieos == CS_EOS_MOIST_AIR) {
    cs_real_t ps, drhodt, dedp, drhodp,dedt, prest;
    /* B, C are the Antoine's law constants */
    cs_real_t B = 17.438;
    cs_real_t C = 239.78;
    cs_real_t cvv = cpv - 461.914;
    for (cs_lnum_t ii = 0; ii < l_size; ii++) {
      if (fracv[ii] < frace[ii]) {
        prest = pres[ii] + phys_pro ->p0;
        // Saturation pressure
        ps  = cs_air_pwv_sat(temp[ii]
            - cs_physical_constants_celsius_to_kelvin);
        // partial rho / partial p
        drhodp = -prest / (rair * pow(temp[ii], 2)
            * (1. - frace[ii] + fracv[ii] * rvsra))
          + (1. /ps)*(prest)*B*C
          /(rair*temp[ii]*pow(prest*(1. /ps)
                - (1. - 1. /rvsra), 2)
              *pow(1. - frace[ii] + fracv[ii]*rvsra, 2)
              *pow(C + temp[ii]
                - cs_physical_constants_celsius_to_kelvin, 2));
        // partial e / partial p
        dedp = -(1. /ps)*(1. /rvsra)*(l00+temp[ii]*(cvv - cpl))
          /pow(prest*(1. /ps) - (1. - 1. /rvsra), 2);
        // partial rho / partial T
        drhodt = rair*((1. - frace[ii] + fracv[ii] *rvsra)
            + temp[ii] *B *C *prest *(1./ps)
              /pow(prest*(1. /ps) - (1. - 1. /rvsra), 2)
            *pow(C + temp[ii]
              - cs_physical_constants_celsius_to_kelvin, 2));
        // partial e/ partial T
        dedt = cs_thermal_model_demdt(prest,
                                      temp[ii],
                                      frace[ii]);
        // compute drho/dp (at constant internal energy)
        dc2[ii] = -drhodt * dedp /dedt + drhodp;
      }
      else {
        dc2[ii] = 1. / (rair * temp[ii] * (1. - frace[ii] + fracv[ii] * rvsra));
      }
    }
  }
  else {
    for (cs_lnum_t ii = 0; ii < l_size; ii++)
      dc2[ii] = 0.;
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the derivative of the internal energy related to the
 *        temperature at constant pressure.
 *
 * \param[in]  pres  array of pressure values
 * \param[in]  temp  array of temperature values (in Kelvin)
 * \param[in]  yw    array of the total water mass fraction
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_demdt(cs_real_t  pres,
                       cs_real_t  temp,
                       cs_real_t  yw)
{
  /*  Local variables */
  /* sat = A + B*t / (C + t) */
  cs_real_t sat = 6.4147
                + 17.438 * (temp - cs_physical_constants_celsius_to_kelvin)
                / (239.78 + temp - cs_physical_constants_celsius_to_kelvin);
  cs_real_t rvsra = cs_glob_fluid_properties->rvsra;
  cs_real_t cva =   cs_glob_fluid_properties->cp0
                  - cs_glob_fluid_properties->r_pg_cnst;
  cs_real_t cvv =   cs_glob_fluid_properties->cpv0
                  - cs_glob_fluid_properties->r_v_cnst;
  cs_real_t cpl = cs_glob_fluid_properties->cvl;
  cs_real_t l00 = cs_glob_fluid_properties->l00;
  cs_real_t f = l00 - cpl*temp;

  cs_real_t d = cva*(1.0 - yw) + cpl * yw;
  //cs_real_t dem1 = exp(-sat);
  //0.622 * (cvv - cpl)/ (pres *  exp(-sat) - 0.378);
  cs_real_t demdt = d + (1. /rvsra) * (cvv - cpl)
    / (pres *  exp(-sat) - (1. - 1. /rvsra))
    + (1. /rvsra) * 17.438 * 239.78 * pres * (f + cvv * temp)
    * exp(-sat)
    / (pow(239.78 + temp - cs_physical_constants_celsius_to_kelvin, 2)
        * pow(pres * exp(-sat) - (1. - 1. /rvsra), 2));

  return demdt;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the derivative of the internal energy related to the
 *        temperature at constant internal energy.
 *
 * \param[in]     pres    array of pressure values
 * \param[in]     temp    array of temperature values (in Kelvin)
 * \param[in]     yw      array of the total water mass fraction
 * \param[in]     cpa     heat capacity of the dry air
 * \param[in]     cpv     heat capacity of the water in its gaseous phase
 * \param[in]     cpl     heat capacity of the water in its liquid phase
 * \param[in]     l00     water latent heat
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_demdt_ecsnt(cs_real_t  pres,
                             cs_real_t  temp,
                             cs_real_t  yw,
                             cs_real_t  cpa,
                             cs_real_t  cpv,
                             cs_real_t  cpl,
                             cs_real_t  l00)
{
  // dedt at constant pressure:
  cs_real_t dedt = cs_thermal_model_demdt(pres, temp, yw);
  cs_real_t sat =   6.4147 + 17.438 * (temp
                  - cs_physical_constants_celsius_to_kelvin)
              / (239.78 + temp - cs_physical_constants_celsius_to_kelvin);
  cs_real_t cvv = cpv - 461.914;
  cs_real_t F = l00 - cpl *temp;
  cs_real_t rair = cs_glob_fluid_properties->r_pg_cnst;
  cs_real_t rvsra = cs_glob_fluid_properties->rvsra;
  cs_real_t D = (cpa - rair) *(1.0 - yw) + cpl * yw;
  // cs_real_t dem1 = exp(-sat);
  // 0.622 * (cvv - cpl)/ (pres *  exp(-sat) - 0.378);
  // dedp at constant temperature
  cs_real_t dedp =   -exp(-sat) *(1. /rvsra) *(l00 + temp  *(cvv - cpl))
                   / pow(pres *exp(-sat) - (1. - 1. /rvsra), 2);
  cs_real_t dpdt = dedt /dedp;
  cs_real_t demdt = D + (1. /rvsra) * (cvv - cpl)
    / (pres *  exp(-sat) - (1. - 1. /rvsra))
    - (1. / rvsra) * (F + cvv * temp) * exp(-sat)
    / pow(pres * exp(-sat) - (1. - 1. /rvsra), 2) *
    (dpdt - 17.438 * 239.78 * exp(-sat)
     /pow(239.78 + temp - cs_physical_constants_celsius_to_kelvin, 2));

  return demdt;
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the kinetic energy based source term
 *
 * \param[in]     croma     array of density values at the last time iteration
 * \param[in]     cromaa    array of density values at the n-2 time iteration
 * \param[in]     crom_eos  density value
 * \param[in]     vel       array of velocity
 * \param[in]     vela      array of ancient velocity
 * \param[in]     sk        kinetic source term
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_compute_kinetic_st(cs_real_t  *croma,
                                    cs_real_t  *cromaa,
                                    cs_real_t  *crom_eos,
                                    cs_real_t  *vel[3],
                                    cs_real_t  *vela[3],
                                    cs_real_t  *sk)
{
  cs_real_t *restrict dt = CS_F_(dt)->val;
  const cs_mesh_t *m = cs_glob_mesh;
  cs_mesh_quantities_t *fvq = cs_glob_mesh_quantities;
  const cs_lnum_t n_cells_ext = m->n_cells_with_ghosts;
  const cs_lnum_t n_i_faces = m->n_i_faces;
  const cs_lnum_t n_b_faces = m->n_b_faces;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;

  const cs_real_t *restrict cell_f_vol = fvq->cell_f_vol;
  const cs_field_t *f_vel = CS_F_(vel);

  const cs_equation_param_t *eqp_u
    = cs_field_get_equation_param_const(f_vel);
  cs_real_t *rho_k_prev;
  rho_k_prev = cs_field_by_name("rho_k_prev")->val;
  cs_real_t thetv = eqp_u -> thetav;

  /* Get useful arrays */
  cs_real_t *imasfl_prev = cs_field_by_name("imasfl")->val_pre;
  cs_real_t *bmasfl_prev = cs_field_by_name("bmasfl")->val_pre;
  cs_real_3_t *utildeif
    = (cs_real_3_t *)cs_field_by_name("inner_face_velocity")->val;
  cs_real_3_t *utildebf
    = (cs_real_3_t *)cs_field_by_name("boundary_face_velocity")->val;
  cs_real_3_t *utildeifa
    = (cs_real_3_t *)cs_field_by_name("inner_face_velocity")->val_pre;
  cs_real_3_t *utildebfa
    = (cs_real_3_t *)cs_field_by_name("boundary_face_velocity")->val_pre;

  /* Loop over the interior faces */
  for (cs_lnum_t f_id = 0; f_id < n_i_faces; f_id++) {
    cs_lnum_t ii = i_face_cells[f_id][0];
    cs_lnum_t jj = i_face_cells[f_id][1];
    /* right cell */
    cs_real_t rhok_theta = thetv *crom_eos[ii] + (1 - thetv) *croma[ii];
    cs_real_t rhoka_theta = thetv *rho_k_prev[ii] + (1 - thetv) *croma[ii];

    cs_real_t norm_dv = pow(thetv *utildeif[f_id][0]
        + (1-thetv) *utildeifa[f_id][0], 2)
        + pow(thetv *utildeif[f_id][1] + (1-thetv) *utildeifa[f_id][1], 2)
        + pow(thetv *utildeif[f_id][2] + (1-thetv) *utildeifa[f_id][2], 2);

    sk[ii] -= 0.5 * imasfl_prev[f_id] * norm_dv * (1 - rhoka_theta
        /rhok_theta);

    norm_dv =   pow(thetv *utildeif[f_id][0] + (1-thetv) *utildeifa[f_id][0]
                    - vel[ii][0], 2)
              + pow(thetv *utildeif[f_id][1] + (1-thetv)*utildeifa[f_id][1]
                    - vel[ii][1], 2)
              + pow(thetv *utildeif[f_id][2] + (1-thetv)*utildeifa[f_id][2]
                    - vel[ii][2], 2);

    sk[ii] -= 0.5 * imasfl_prev[f_id] * norm_dv * rhoka_theta /rhok_theta;

    /* left cell */
    rhok_theta = thetv *crom_eos[jj] + (1 - thetv) *croma[jj];
    rhoka_theta = thetv *rho_k_prev[jj] + (1 - thetv) *croma[jj];

    norm_dv = pow(thetv *utildeif[f_id][0] + (1-thetv)
                  * utildeifa[f_id][0], 2)
      + pow(thetv*utildeif[f_id][1] + (1-thetv)*utildeifa[f_id][1], 2)
      + pow(thetv*utildeif[f_id][2] + (1-thetv)*utildeifa[f_id][2], 2);

    sk[jj] += 0.5 * imasfl_prev[f_id] * norm_dv
      * (1 - rhoka_theta/rhok_theta);

    norm_dv = pow(thetv*utildeif[f_id][0] + (1-thetv)*utildeifa[f_id][0]
                  - vel[jj][0], 2)
      + pow(  thetv *utildeif[f_id][1] + (1-thetv)*utildeifa[f_id][1]
            - vel[jj][1], 2)
      + pow(  thetv *utildeif[f_id][2] + (1-thetv)*utildeifa[f_id][2]
            - vel[jj][2], 2);

    sk[jj] += 0.5 * imasfl_prev[f_id] * norm_dv * rhoka_theta /rhok_theta;

  }

  /* Boundary faces */
  for (cs_lnum_t f_id = 0; f_id < n_b_faces; f_id++) {
    cs_lnum_t ii = b_face_cells[f_id];
    cs_real_t rhok_theta = thetv *crom_eos[ii] + (1 - thetv) *croma[ii];
    cs_real_t rhoka_theta = thetv *rho_k_prev[ii] + (1 - thetv) *croma[ii];
    cs_real_t norm_dv = pow(thetv *utildebf[f_id][0]
                            + (1-thetv) *utildebfa[f_id][0], 2)
      + pow(thetv *utildebf[f_id][1] + (1-thetv) *utildebfa[f_id][1], 2)
      + pow(thetv *utildebf[f_id][2] + (1-thetv) *utildebfa[f_id][2], 2);

    sk[ii] -= 0.5 * bmasfl_prev[f_id] * norm_dv
      * (1 - rhoka_theta /rhok_theta);

    norm_dv = pow(thetv *utildebf[f_id][0] + (1-thetv) *utildebfa[f_id][0]
        - vel[ii][0], 2)
      + pow(thetv *utildebf[f_id][1] + (1-thetv) *utildebfa[f_id][1]
          - vel[ii][1], 2)
      + pow(thetv *utildebf[f_id][2] + (1-thetv) *utildebfa[f_id][2]
          - vel[ii][2], 2);

    sk[ii] -= 0.5 * bmasfl_prev[f_id] * norm_dv * rhoka_theta /rhok_theta;
  }

  /* Unsteady part */
  for (cs_lnum_t c_id = 0; c_id < n_cells_ext; c_id++) {
    cs_real_t rhoa_theta = thetv *croma[c_id] + (1 - thetv) *cromaa[c_id];
    cs_real_t rhok_theta = thetv *crom_eos[c_id] + (1 - thetv) *croma[c_id];
    cs_real_t rhoka_theta = thetv *rho_k_prev[c_id] + (1 - thetv) *croma[c_id];
    cs_real_t norm_dv = pow(vel[c_id][0] - vela[c_id][0], 2)
      + pow(vel[c_id][1] - vela[c_id][1], 2)
      + pow(vel[c_id][2] - vela[c_id][2], 2);
    sk[c_id] += 0.5 * cell_f_vol[c_id] * rhoa_theta
      * (pow(vela[c_id][0], 2) + pow(vela[c_id][1], 2)
          * + pow(vela[c_id][2], 2))
      * (1 - rhoka_theta/rhok_theta) / dt[c_id]
      + 0.5 * cell_f_vol[c_id] * (rhoa_theta * rhoka_theta/rhok_theta)
      * norm_dv / dt[c_id];
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Add the kinetic source term if needed
 *
 * \param[in]     smbrs     RHS of the thermal equation
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_add_kst(cs_real_t      *smbrs)
{
  if (cs_glob_thermal_model->has_kinetic_st == 1) {

    const cs_mesh_t *m = cs_glob_mesh;
    const cs_lnum_t n_cells = m->n_cells;
    const cs_real_t *kst = cs_field_by_name("kinetic_energy_thermal_st")->val;

    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
      smbrs[c_id] += kst[c_id];
    }
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the CFL number related to the pressure equation.
 *
 * \param[in]  croma     array of density values at the last time iteration
 * \param[in]  trav2     array of the predicted velocity
 * \param[in]  cvara_pr  array of pressure values at the last time iteration
 * \param[in]  imasfl    array of the faces mass fluxes
 * \param[in]  cflp      CFL condition related to the pressure equation
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_cflp(cs_real_t  *croma,
                      cs_real_t  *trav2[3],
                      cs_real_t  *cvara_pr,
                      cs_real_t  *imasfl,
                      cs_real_t  *cflp)
{
  /* Get global data */
  const cs_mesh_t *m = cs_glob_mesh;
  const cs_mesh_quantities_t *fvq = cs_glob_mesh_quantities;
  const cs_lnum_t n_i_faces = m->n_i_faces;
  const cs_lnum_t n_b_faces = m->n_b_faces;
  const cs_real_t *restrict dt = CS_F_(dt)->val;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;
  const cs_real_t *alphafij = fvq->weight;
  const cs_real_3_t *restrict cell_cen
    = (const cs_real_3_t *restrict)fvq->cell_cen;

  const cs_real_t *restrict cell_f_vol = fvq->cell_f_vol;
  const cs_field_t *f_vel = CS_F_(vel);

  const cs_equation_param_t *eqp_u
    = cs_field_get_equation_param_const(f_vel);
  const cs_real_3_t *surfac = (const cs_real_3_t *) fvq->i_face_normal;
  const cs_real_3_t *surfbo = (const cs_real_3_t *) fvq->b_face_normal;

  if (eqp_u -> blencv > 0 && eqp_u -> ischcv == 1) {
    for (cs_lnum_t f_id = 0; f_id < n_i_faces; f_id++) {
      cs_lnum_t ii = i_face_cells[f_id][0];
      cs_lnum_t jj = i_face_cells[f_id][1];
      cflp[ii] += dt[ii] /(croma[ii] * cell_f_vol[ii])
        * (alphafij[f_id] * (trav2[ii][0] * surfac[f_id][0]
              + trav2[ii][1] * surfac[f_id][1]
              + trav2[ii][2] * surfac[f_id][2])
            + (1 - alphafij[f_id]) * (trav2[jj][0] * surfac[f_id][0]
              + trav2[jj][1] * surfac[f_id][1]
              + trav2[jj][2] * surfac[f_id][2]));
      cflp[ii] += dt[ii] /(croma[ii] * cell_f_vol[ii])
        * (1 - eqp_u-> thetav) * dt[ii]
        * pow( pow(surfac[f_id][0], 2)
            + pow(surfac[f_id][1], 2)
            + pow(surfac[f_id][2], 2), 0.5)
        * (cvara_pr[ii] - cvara_pr[jj])
        /pow(pow(cell_cen[jj][0] - cell_cen[ii][0], 2)
            + pow(cell_cen[jj][1] - cell_cen[ii][1], 2)
            + pow(cell_cen[jj][2] - cell_cen[ii][2], 2), 0.5);
      cflp[jj] -= dt[jj] /(croma[jj] * cell_f_vol[jj])
        * (alphafij[f_id] * (trav2[ii][0] * surfac[f_id][0]
              + trav2[ii][1] * surfac[f_id][1]
              + trav2[ii][2] * surfac[f_id][2])
            + (1 - alphafij[f_id]) * (trav2[jj][0] * surfac[f_id][0]
              + trav2[jj][1] * surfac[f_id][1]
              + trav2[jj][2] * surfac[f_id][2]));
      cflp[jj] -= dt[jj] /(croma[jj]*cell_f_vol[jj])
        * (1 - eqp_u-> thetav) * dt[jj]
        * pow( pow(surfac[f_id][0], 2)
            + pow(surfac[f_id][1], 2)
            + pow(surfac[f_id][2], 2), 0.5)
        * (cvara_pr[jj] - cvara_pr[ii])
        /pow(pow(cell_cen[jj][0] - cell_cen[ii][0], 2)
            + pow(cell_cen[jj][1] - cell_cen[ii][1], 2)
            + pow(cell_cen[jj][2] - cell_cen[ii][2], 2), 0.5);
    }
    for (cs_lnum_t f_id = 0; f_id < n_b_faces; f_id++) {
      cs_lnum_t ii = b_face_cells[f_id];
      cflp[ii] += dt[ii] / (croma[ii] * cell_f_vol[ii])
        * (trav2[ii][0]*surfbo[f_id][0]
            + trav2[ii][1]*surfbo[f_id][1]+trav2[ii][2]*surfbo[f_id][2]);
    }
  }
  else if (eqp_u->blencv <= 0 && eqp_u->ischcv == 1) {
    for (cs_lnum_t f_id = 0; f_id < n_i_faces; f_id++) {
      cs_lnum_t ii = i_face_cells[f_id][0];
      cs_lnum_t jj = i_face_cells[f_id][1];
      if (imasfl[f_id] > 0) {
        cflp[ii] += dt[ii] /(croma[ii] * cell_f_vol[ii])
          * (trav2[ii][0] * surfac[f_id][0]
              + trav2[ii][1] * surfac[f_id][1]
              + trav2[ii][2] * surfac[f_id][2]);
        cflp[jj] -= dt[ii] /(croma[ii] * cell_f_vol[ii])
          * (trav2[ii][0] * surfac[f_id][0]
              + trav2[ii][1] * surfac[f_id][1]
              + trav2[ii][2] * surfac[f_id][2]);
      }
      else {
        cflp[ii] += dt[ii] /(croma[ii] * cell_f_vol[ii])
          * (trav2[jj][0] * surfac[f_id][0]
              + trav2[jj][1] * surfac[f_id][1]
              + trav2[jj][2] * surfac[f_id][2]);
        cflp[jj] -= dt[ii] /(croma[ii] * cell_f_vol[ii])
          * (trav2[jj][0] * surfac[f_id][0]
              + trav2[jj][1] * surfac[f_id][1]
              + trav2[jj][2] * surfac[f_id][2]);
      }
      cflp[ii] += dt[ii] /(croma[ii] * cell_f_vol[ii])
        * (1 - eqp_u-> thetav) * dt[ii]
        * pow( pow(surfac[f_id][0], 2)
            + pow(surfac[f_id][1], 2)
            + pow(surfac[f_id][2], 2), 0.5)
        * (cvara_pr[ii] - cvara_pr[jj])
        /pow(pow(cell_cen[jj][0] - cell_cen[ii][0], 2)
            + pow(cell_cen[jj][1] - cell_cen[ii][1], 2)
            + pow(cell_cen[jj][2] - cell_cen[ii][2], 2), 0.5);
      cflp[jj] -= dt[jj] /(croma[jj]*cell_f_vol[jj])
        * (1 - eqp_u-> thetav) * dt[jj]
        * pow( pow(surfac[f_id][0], 2)
            + pow(surfac[f_id][1], 2)
            + pow(surfac[f_id][2], 2), 0.5)
        * (cvara_pr[jj] - cvara_pr[ii])
        /pow(pow(cell_cen[jj][0] - cell_cen[ii][0], 2)
            + pow(cell_cen[jj][1] - cell_cen[ii][1], 2)
            + pow(cell_cen[jj][2] - cell_cen[ii][2], 2), 0.5);

    }
    for (cs_lnum_t f_id = 0; f_id < n_b_faces; f_id++) {
      cs_lnum_t ii = b_face_cells[f_id];
      cflp[ii] +=    dt[ii] / (croma[ii] * cell_f_vol[ii])
                   * ( trav2[ii][0]*surfbo[f_id][0]
                      + trav2[ii][1]*surfbo[f_id][1]
                      + trav2[ii][2]*surfbo[f_id][2]);
    }
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Perform the Newton method to compute the temperature from the
 * internal energy
 *
 * \param[in]  yw        array of total water mass fraction
 * \param[in]  yv        array of vapor of water mass fraction
 * \param[in]  temp      array of temperature values
 * \param[in]  th_scal   array of internal energy values
 * \param[in]  pk1       array of pressure values at the last
 *                       inner iteration
 * \param[in]  cvar_pr   array of pressure values
 * \param[in]  cvara_pr  array of pressure values at the last time iteration
 * \param[in]  method    method used to compute the temperature
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_newton_t(cs_real_t  *yw,
                          cs_real_t  *yv,
                          cs_real_t  *temp,
                          cs_real_t  *th_scal,
                          cs_real_t  *pk1,
                          cs_real_t  *cvar_pr,
                          cs_real_t  *cvara_pr,
                          int         method)
{
  /* Get global data */
  const cs_mesh_t *m = cs_glob_mesh;
  cs_mesh_quantities_t *fvq = cs_glob_mesh_quantities;
  const cs_lnum_t n_cells = m->n_cells;
  const cs_field_t *f_vel = CS_F_(vel);

  const cs_equation_param_t *eqp_u
    = cs_field_get_equation_param_const(f_vel);

  const cs_real_3_t *restrict cell_cen
    = (const cs_real_3_t *restrict)fvq->cell_cen;

  const cs_fluid_properties_t *phys_pro = cs_glob_fluid_properties;
  cs_physical_constants_t *pc = cs_get_glob_physical_constants();

  /* Newton method error threshold */
  cs_real_t epsy = 1e-7;

  /* Thermodynamic constants */
  cs_real_t cva = phys_pro->cp0 - phys_pro->r_pg_cnst;
  cs_real_t cvv = phys_pro->cpv0 - phys_pro->r_v_cnst;
  cs_real_t cpv = phys_pro->cpv0;
  cs_real_t cvl = phys_pro->cvl;
  cs_real_t l00 = phys_pro->l00;

  /* Declaration of local variables */
  cs_real_t pres , ysat, em_, errort, yv_, demdt;

  /* Two methods can be used to correct yv
   * The first, yv_cor = 1, performs the newton method
   * using both temperature and pressure at n+1,k.
   * The second, yv_cor = 2, performs a increment on
   * the vapour mass fraction following dp */

  const cs_real_t *xyzp0 = phys_pro->xyzp0;
  const cs_real_t *gravity = pc->gravity;

  if (method == 1) {

    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {

      cs_real_t xcvv =   cva * (1 - yw[c_id]) + cvv *yv[c_id]
                       + cvl *(yw[c_id] - yv[c_id]);
      temp[c_id] = th_scal[c_id] /xcvv - l00 *yv[c_id] /xcvv;
      // yvsat
      pres =   cvar_pr[c_id] + phys_pro->p0
             + phys_pro->ro0 * cs_math_3_distance_dot_product(xyzp0,
                                                              cell_cen[c_id],
                                                              gravity);
      ysat = cs_air_yw_sat(temp[c_id]
                           - cs_physical_constants_celsius_to_kelvin, pres);

      if (yv[c_id] < yw[c_id]) {
        yv_ = ysat;
        xcvv = cva * (1 - yw[c_id]) + cvv *yv_ + cvl *(yw[c_id] - yv_);
        // Estimate new temperature
        em_ = temp[c_id] *xcvv + l00 *yv_;
        errort = fabs(th_scal[c_id] - em_);

        while (errort > epsy) {
          demdt = cs_thermal_model_demdt_ecsnt(pres,
                                               temp[c_id],
                                               yw[c_id],
                                               phys_pro->cp0,
                                               cpv,
                                               cvl,
                                               l00);
          temp[c_id] = (th_scal[c_id] - em_) /demdt + temp[c_id];
          yv_ = cs_air_yw_sat(temp[c_id]
                              - cs_physical_constants_celsius_to_kelvin, pres);
          xcvv = cva * (1 - yw[c_id]) + cvv *yv_ + cvl *(yw[c_id] - yv_);
          em_ = temp[c_id] *xcvv + l00 *yv_;
          errort = fabs(th_scal[c_id] - em_);
        }

        if (yv_ > yw[c_id]) {
          yv[c_id] = yw[c_id];
          xcvv = cva * (1 - yw[c_id]) + cvv *yv[c_id];
          temp[c_id] = th_scal[c_id] /xcvv - yv[c_id] *l00 /xcvv;
        }
        else {
          yv[c_id] = yv_;
        }
      }

      else { //previous iteration not saturation
        // verifiy if saturation is reached:
        if (yw[c_id] > ysat) {
          yv_ = ysat;
          xcvv = cva * (1 - yw[c_id]) + cvv *yv_ + cvl *(yw[c_id] - yv_);
          // Estimate new temperature
          em_ = temp[c_id] *xcvv + l00 *yv_;
          errort = fabs(th_scal[c_id] - em_);
          while (errort > epsy) {
            demdt = cs_thermal_model_demdt_ecsnt(pres,
                                                 temp[c_id],
                                                 yw[c_id],
                                                 phys_pro->cp0,
                                                 cpv,
                                                 cvl,
                                                 l00);
            temp[c_id] = (th_scal[c_id] - em_) /demdt + temp[c_id];
            yv_ = cs_air_yw_sat(temp[c_id]
                                - cs_physical_constants_celsius_to_kelvin, pres);
            xcvv = cva * (1 - yw[c_id]) + cvv *yv_ + cvl *(yw[c_id] - yv_);
            em_ = temp[c_id] *xcvv + l00 *yv_;
            errort = fabs(th_scal[c_id] - em_);
          }
          if (yv_ > yw[c_id]) {
            yv[c_id] = yw[c_id];
            xcvv = cva * (1 - yw[c_id]) + cvv *yv[c_id];
            temp[c_id] = th_scal[c_id] /xcvv - yv[c_id] *l00 /xcvv;
          }
          else {
            yv[c_id] = yv_;
          }
        }
        else {
          yv_ = yw[c_id];
        }
      }

      if (yv_ > yw[c_id]) {
        yv[c_id] = yw[c_id];
        xcvv = cva * (1 - yw[c_id]) + cvv *yv[c_id];
        temp[c_id] = th_scal[c_id] /xcvv - yv[c_id] *l00 /xcvv;
      }
      else {
        yv[c_id] = yv_;
      }
      //tempk[c_id] = temp[c_id];

    } /* End of loop on cells */

  }
  else {  /* if (method != 1) */

    cs_real_t dyvdp;
    cs_real_t drop;
    const cs_real_t rvsra = phys_pro->rvsra;

    cs_real_t _coef = (eqp_u->thetav >= 1) ?  1. :  2.;

    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {

      if (yv[c_id] < yw[c_id]) {
        cs_real_t xcvv =   cva * (1 - yw[c_id])
                         + cvv * yv[c_id]
                         + cvl * (yw[c_id] - yv[c_id]);
        cs_real_t ps = cs_air_pwv_sat(temp[c_id]
                                      - cs_physical_constants_celsius_to_kelvin);
        // dyv/dp;

        pres =   cvar_pr[c_id] + phys_pro->p0
               + phys_pro->ro0 * cs_math_3_distance_dot_product(xyzp0,
                                                                cell_cen[c_id],
                                                                gravity);
        dyvdp = - (1 /rvsra) *ps /pow(pres - (1 - 1/rvsra) *ps, 2);
        // correction of yv
        drop = (  _coef * cvar_pr[c_id] - (_coef - 1)
                *  cvara_pr[c_id] - pk1[c_id]) *dyvdp;
        yv[c_id] = yv[c_id] + drop;
        temp[c_id] = th_scal[c_id] /xcvv - l00 *yv[c_id] /xcvv;
        if (yv[c_id] > yw[c_id]) {
          yv[c_id] = yw[c_id];
          xcvv = cva * (1 - yw[c_id]) + cvv *yv[c_id];
        }
        else {
          xcvv =   cva * (1 - yw[c_id])
                 + cvv *yv[c_id] + cvl*(yw[c_id] - yv[c_id]);
        }
        temp[c_id] = th_scal[c_id] / xcvv - yv[c_id] * l00 / xcvv;
        //tempk[c_id] = temp[c_id];
      }

    } /* End of loop on cells */

  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Add the term pdivu to the thermal equation rhs
 *
 * \param[in]       temp_     array of temperature
 * \param[in]       tempa_    array of temperature at the previous time step
 * \param[in]       cvar_var  array of the internal energy
 * \param[in]       cvara_var array of the internal energy at the previous
 *                            time step
 * \param[in]       tempa_    array of temperature at the previous time step
 * \param[in]       thetv     theta parameter
 * \param[in]       vel       array of the velocity
 * \param[in]       xcvv      array of the isobaric heat capacity
 * \param[in]       cpro_yw   array of the total water mass fraction
 * \param[in]       cpro_ywa  array of the total water mass fraction at the
 *                            previous time step
 * \param[in]       cpro_yv   array of the vapor of water mass fraction
 * \param[in]       cpro_yva  array of the vapor of water mass fraction at the
 *                            previous time step
 * \param[in]       gradp     array of the pressure gradient
 * \param[in]       gradphi   array of the pressure increment gradient
 * \param[in, out]  smbrs     array of the right hand side
 */
/*----------------------------------------------------------------------------*/

void
cs_thermal_model_pdivu(cs_real_t  *temp_,
                       cs_real_t  *tempa_,
                       cs_real_t  *cvar_var,
                       cs_real_t  *cvara_var,
                       cs_real_t   thetv,
                       cs_real_t  *vel[3],
                       cs_real_t  *xcvv,
                       cs_real_t  *cpro_yw,
                       cs_real_t  *cpro_ywa,
                       cs_real_t  *cpro_yv,
                       cs_real_t  *cpro_yva,
                       cs_real_t  *gradp[3],
                       cs_real_t  *gradphi[3],
                       cs_real_t  *smbrs)
{
  const cs_mesh_t *m = cs_glob_mesh;
  const cs_mesh_quantities_t *fvq = cs_glob_mesh_quantities;
  const cs_lnum_t n_cells = m->n_cells;
  const cs_lnum_t n_i_faces = m->n_i_faces;
  const cs_lnum_t n_b_faces = m->n_b_faces;
  const cs_real_t *restrict cell_f_vol = fvq->cell_f_vol;

  /*  Local variables */
  int itherm = cs_glob_thermal_model->thermal_variable;
  int has_pdivu = cs_glob_thermal_model->has_pdivu;
  cs_real_t rair = cs_glob_fluid_properties->r_pg_cnst;
  cs_real_t rvapor = cs_glob_fluid_properties->r_v_cnst;
  cs_real_t l00 = cs_glob_fluid_properties->l00;
  cs_real_t cpv = cs_glob_fluid_properties->cv0;
  cs_real_t cvl = cs_glob_fluid_properties->cvl;
  cs_real_t cp0 = cs_glob_fluid_properties->cp0;

  const cs_fluid_properties_t *phys_pro = cs_glob_fluid_properties;
  cs_real_t rvsra = phys_pro->rvsra;
  cs_field_t *fimasfl = cs_field_by_name_try("imasfl");
  cs_field_t *fbmasfl = cs_field_by_name_try("bmasfl");

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;

  if (has_pdivu == 1 && fimasfl != NULL && fbmasfl != NULL) {
    cs_real_t *pdivu;
    cs_real_t *imasfl = cs_field_by_name("imasfl")->val;
    cs_real_t *bmasfl = cs_field_by_name("bmasfl")->val;
    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
      pdivu[c_id] = 0.;
    }
    if (itherm == CS_THERMAL_MODEL_TEMPERATURE) {
      // Interior faces contribution
      for (cs_lnum_t f_id = 0; f_id < n_i_faces; f_id++) {
        cs_lnum_t ii = i_face_cells[f_id][0];
        cs_lnum_t jj = i_face_cells[f_id][1];
        if (imasfl>=0) {
          // Left cell, imasfl +
          pdivu[ii] = pdivu[ii] + thetv *rair *imasfl[f_id] *temp_[ii]
            + (1-thetv) *imasfl[f_id] *rair *tempa_[ii];
          // Right cell, imasfl -
          pdivu[jj] = pdivu[jj] - thetv *imasfl[f_id] *rair *temp_[ii]
            - (1-thetv) *imasfl[f_id] *rair *tempa_[ii];

        }
        else {
          // Right cell, imasfl +
          pdivu[jj] = pdivu[jj] - imasfl[f_id] *rair
            *(thetv *temp_[jj] + (1-thetv) *tempa_[jj]);
          // Left cell, imasfl -
          pdivu[ii] = pdivu[ii] + imasfl[f_id] *rair
            *(thetv *temp_[jj] + (1-thetv) *tempa_[jj]);
        }
      }
      // Boundary faces contribution
      for (cs_lnum_t f_id = 0; f_id < n_b_faces; f_id++) {
        cs_lnum_t ii = b_face_cells[f_id];
        pdivu[ii] = pdivu[ii] + bmasfl[f_id] *rair
          *(thetv *temp_[ii] + (1-thetv) *tempa_[ii]);
      }
    }
    else if (itherm == CS_THERMAL_MODEL_INTERNAL_ENERGY) {
      cs_real_t pdrho, pdrhoa, cvma;
      // Interior faces contribution
      for (cs_lnum_t f_id = 0; f_id < n_i_faces; f_id++) {
        cs_lnum_t ii = i_face_cells[f_id][0];
        cs_lnum_t jj = i_face_cells[f_id][1];
        if (imasfl >= 0) {
          pdrho = (cvar_var[ii] - cpro_yv[ii] *l00) *rair
            *(1 - cpro_yw[ii] + cpro_yv[ii] *rvsra) /(xcvv[ii]);
          cvma = (cp0 - rair) * (1 - cpro_ywa[ii]) + (cpv - rvapor)
            * cpro_yva[ii] + (cpro_ywa[ii] - cpro_yva[ii]) * cvl;
          pdrhoa = (cvara_var[ii] - cpro_yva[ii] *l00) *rair
            *(1 - cpro_ywa[ii] + cpro_yva[ii] *rvsra) / cvma;
          // Left cell, imasfl +
          pdivu[ii] = pdivu[ii] + thetv *pdrho *imasfl[f_id]
            + (1-thetv) *imasfl[f_id] *pdrhoa;
          // Right cell, imasfl -
          pdivu[jj] = pdivu[jj] - thetv *imasfl[f_id] *pdrho
            - (1-thetv) *imasfl[f_id] *pdrhoa;
        }
        else {
          pdrho = (cvar_var[jj] - cpro_yv[jj] *l00) *rair
            *(1 - cpro_yw[jj] + cpro_yv[jj] *rvsra) /(xcvv[jj]);
          cvma = (cp0 - rair) * (1 - cpro_ywa[jj]) + (cpv - rvapor)
            * cpro_yva[jj] + (cpro_ywa[jj] - cpro_yva[jj]) * cvl;
          pdrhoa = (cvara_var[jj] - cpro_yva[jj] *l00) *rair
            *(1 - cpro_ywa[jj] + cpro_yva[jj] *rvsra) / cvma;
          // Right cell, imasfl +
          pdivu[jj] = pdivu[jj] - imasfl[f_id]
            *(thetv *pdrho + (1-thetv) *pdrhoa);
          // Left cell, imasfl -
          pdivu[ii] = pdivu[ii] + imasfl[f_id]
            *(thetv *pdrhoa + (1-thetv) *pdrhoa);
        }
      }
      // Boundary faces contribution
      for (cs_lnum_t f_id = 0; f_id < n_b_faces; f_id++) {
        cs_lnum_t ii = b_face_cells[f_id];
        pdrho = (cvar_var[ii] - cpro_yv[ii] *l00) *rair
          *(1 - cpro_yw[ii] + cpro_yv[ii] *rvsra) /(xcvv[ii]);
        cvma = (cp0 - rair) * (1 - cpro_ywa[ii]) + (cpv - rvapor)
          * cpro_yva[ii] + (cpro_ywa[ii] - cpro_yva[ii]) * cvl;
        pdrhoa = (cvara_var[ii] - cpro_yva[ii] *l00) *rair
          *(1 - cpro_ywa[ii] + cpro_yva[ii] *rvsra) / cvma;
        pdivu[ii] = pdivu[ii] + bmasfl[f_id] *
          (thetv *pdrho + (1-thetv) *pdrhoa);
      }
    }
    // pdiv(u) = div(pu) - u.grad p
    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
      pdivu[c_id] = pdivu[c_id] - cell_f_vol[c_id]
        *(vel[c_id][0] *(gradp[c_id][0] + gradphi[c_id][0])
        + vel[c_id][1] *(gradp[c_id][1] + gradphi[c_id][1])
        + vel[c_id][2] *(gradp[c_id][2] + gradphi[c_id][2]));
      smbrs[c_id] -= pdivu[c_id];
    }
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute and add the dissipation term of the thermal equation to
 *        its right hand side.
 *
 * \param[in]      vistot  array for the total viscosity
 * \param[in]      gradv   tensor for the velocity gradient
 * \param[in,out]  smbrs   array of equation right hand side
 */
/*----------------------------------------------------------------------------*/

void
cs_thermal_model_dissipation(cs_real_t  *vistot,
                             cs_real_t  *gradv[3][3],
                             cs_real_t  *smbrs)
{
  const cs_mesh_t *m = cs_glob_mesh;
  cs_mesh_quantities_t *fvq = cs_glob_mesh_quantities;
  const cs_lnum_t n_cells = m->n_cells;
  const cs_real_t *restrict cell_f_vol = fvq->cell_f_vol;

  /*  Local variables */
  int has_dissipation = cs_glob_thermal_model->has_dissipation;
  if (has_dissipation == 1) {
    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
      smbrs[c_id] +=  2. * cell_f_vol[c_id] * vistot[c_id]
                  * (  pow(gradv[c_id][0][0], 2)
                     + pow(gradv[c_id][1][1], 2)
                     + pow(gradv[c_id][2][2], 2)
                  + 0.5 * (  pow(gradv[c_id][1][0] + gradv[c_id][0][1], 2)
                           + pow(gradv[c_id][2][0] + gradv[c_id][0][2], 2)
                           + pow(gradv[c_id][2][1] + gradv[c_id][1][2], 2))
                  - 1./3. * pow(  gradv[c_id][0][0] + gradv[c_id][1][1]
                                + gradv[c_id][2][2], 2));
    }
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the CFL number related to the thermal equation
 *
 * \param[in]     croma     array of density values at the last time iteration
 * \param[in]     tempk     array of the temperature
 * \param[in]     tempka    array of the temperature at the previous time step
 * \param[in]     xcvv      array of the isochoric heat capacity
 * \param[in]     vel       array of the velocity
 * \param[in]     imasfl    array of the faces mass fluxes
 * \param[in]     cflt      CFL condition related to thermal equation
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_cflt (cs_real_t  *croma,
                       cs_real_t  *tempk,
                       cs_real_t  *tempka,
                       cs_real_t  *xcvv,
                       cs_real_t  *vel[3],
                       cs_real_t  *imasfl,
                       cs_real_t  *cflt)
{
  // TODO: make it compatible for others EOS
  /* Get global data */
  const cs_mesh_t *m = cs_glob_mesh;
  const cs_mesh_quantities_t *fvq = cs_glob_mesh_quantities;
  const cs_lnum_t n_cells = m->n_cells;
  const cs_lnum_t n_i_faces = m->n_i_faces;
  const cs_lnum_t n_b_faces = m->n_b_faces;
  cs_real_t *restrict dt = CS_F_(dt)->val;

  const cs_lnum_2_t *restrict i_face_cells
    = (const cs_lnum_2_t *restrict)m->i_face_cells;
  const cs_lnum_t *restrict b_face_cells
    = (const cs_lnum_t *restrict)m->b_face_cells;

  const cs_real_t *restrict cell_f_vol = fvq->cell_f_vol;
  const cs_field_t *f_vel = CS_F_(vel);

  const cs_equation_param_t *eqp_u
    = cs_field_get_equation_param_const(f_vel);
  cs_fluid_properties_t *phys_pro = cs_get_glob_fluid_properties();

  cs_real_t thetv = eqp_u -> thetav;

  if (cs_glob_thermal_model->itherm == CS_THERMAL_MODEL_TEMPERATURE) {
    cs_real_3_t *gradp
      = (cs_real_3_t *)cs_field_by_name("pressure_gradient")-> val;
    cs_real_3_t *gradphi
      = (cs_real_3_t *)cs_field_by_name("pressure_increment_gradient")-> val;
    cs_real_t gammagp = phys_pro->cp0/(phys_pro->cp0  - phys_pro->r_pg_cnst);

    for (cs_lnum_t f_id = 0; f_id < n_i_faces; f_id++) {
      cs_lnum_t ii = i_face_cells[f_id][0];
      cs_lnum_t jj = i_face_cells[f_id][1];
      if (imasfl[f_id] > 0) {
        cflt[ii] +=  (dt[ii] /(croma[ii] *cell_f_vol[ii]))
          * (imasfl[f_id])* (thetv *(gammagp - 1.) *tempk[ii] /tempka[ii]
                             + (1. - thetv) *(2. - gammagp));
      }
      else {
        cflt[jj] -=  (dt[jj] /(croma[jj] *cell_f_vol[jj]))
          * (imasfl[f_id])* (thetv *(gammagp - 1.) *tempk[jj] /tempka[jj]
                             + (1. - thetv) *(2. - gammagp));

      }
    }
    for (cs_lnum_t f_id = 0; f_id < n_b_faces; f_id++) {
      cs_lnum_t ii = b_face_cells[f_id];
      cflt[ii] +=  (dt[ii] /(croma[ii] *cell_f_vol[ii]))
        * (imasfl[f_id])* (thetv *(gammagp - 1.) *tempk[ii] /tempka[ii]
                           + (1. - thetv) *(2. - gammagp));
    }
    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
      cflt[c_id] += dt[c_id] *(gammagp-1.0)
        *(  vel[c_id][0] *(gradp[c_id][0] + gradphi[c_id][0])
          + vel[c_id][1] *(gradp[c_id][1] + gradphi[c_id][1])
          + vel[c_id][2] *(gradp[c_id][2] + gradphi[c_id][2]))
        /(croma[c_id]*tempka[c_id]*xcvv[c_id]);
    }
  }
}

/*----------------------------------------------------------------------------*/
/*!
 * \brief Compute the isobaric heat capacity
 *
 * \param[in]     xcvv      isobaric heat capacity
 */
/*----------------------------------------------------------------------------*/

cs_real_t
cs_thermal_model_cv(cs_real_t  *xcvv)
{
  /* Get global data */
  const cs_mesh_t *m = cs_glob_mesh;
  const cs_lnum_t n_cells = m->n_cells;

  const cs_fluid_properties_t *phys_pro = cs_glob_fluid_properties;

  if (cs_glob_cf_model->ieos == CS_EOS_MOIST_AIR) {
    /* get useful arrays and constants */
    cs_real_t *yw = cs_field_by_name("yw")->val;
    cs_real_t *yv = cs_field_by_name("yv")->val;
    cs_real_t cva = phys_pro->cp0 - phys_pro->r_pg_cnst;
    cs_real_t cvv = phys_pro->cpv0 - phys_pro->r_v_cnst;
    cs_real_t cvl = phys_pro->cvl;

    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
      xcvv[c_id] = cva * (1. - yw[c_id]) + cvv * yv[c_id]
        + (yw[c_id] - yv[c_id]) * cvl;
    }
  }
  else if (cs_glob_cf_model->ieos == CS_EOS_IDEAL_GAS) {
    if (phys_pro->icp > 0) {
      cs_real_t *cp = CS_F_(cp)->val;
      for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
        xcvv[c_id] = cp[c_id] - phys_pro->r_pg_cnst;
      }
    }
    else {
      for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
        xcvv[c_id] = phys_pro->cp0 - phys_pro->r_pg_cnst;
      }
    }
  }
  else { /* quid when ieos = -1 */
    for (cs_lnum_t c_id = 0; c_id < n_cells; c_id++) {
      xcvv[c_id] = 1.;
    }
  }
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
