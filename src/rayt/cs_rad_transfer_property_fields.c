/*============================================================================
 * Radiation solver operations.
 *============================================================================*/

/* This file is part of code_saturne, a general-purpose CFD tool.

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
  Street, Fifth Floor, Boston, MA 02110-1301, USA. */

/*----------------------------------------------------------------------------*/

#include "cs_defs.h"
#include "cs_math.h"

/*----------------------------------------------------------------------------
 * Standard C library headers
 *----------------------------------------------------------------------------*/

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <float.h>

#if defined(HAVE_MPI)
#include <mpi.h>
#endif

/*----------------------------------------------------------------------------
 *  Local headers
 *----------------------------------------------------------------------------*/

#include "cs_log.h"
#include "cs_mesh.h"
#include "cs_mesh_location.h"

#include "cs_field.h"
#include "cs_field_pointer.h"

#include "cs_parameters.h"
#include "cs_post.h"

#include "cs_rad_transfer.h"

/*----------------------------------------------------------------------------
 *  Header for the current file
 *----------------------------------------------------------------------------*/

#include "cs_rad_transfer_property_fields.h"
#include "cs_physical_model.h"
/*----------------------------------------------------------------------------*/

BEGIN_C_DECLS

/*=============================================================================
 * Additional Doxygen documentation
 *============================================================================*/

/*! \cond DOXYGEN_SHOULD_SKIP_THIS */

/*  \file cs_rad_transfer_property_field.c    */
/*  \brief Properties definitions for radiative model.     */


/*! (DOXYGEN_SHOULD_SKIP_THIS) \endcond */

/*=============================================================================
 * Public function definitions
 *============================================================================*/

/*----------------------------------------------------------------------------*/
/*!
 * \brief Create property fields for radiative solver
 */
/*----------------------------------------------------------------------------*/

void
cs_rad_transfer_prp(void)
{
  if (cs_glob_rad_transfer_params->type <= CS_RAD_TRANSFER_NONE)
    return;

  const int keylbl = cs_field_key_id("label");
  const int keyvis = cs_field_key_id("post_vis");
  const int keylog = cs_field_key_id("log");

  cs_rad_transfer_params_t *rt_params = cs_glob_rad_transfer_params;
  cs_field_t *f = NULL;

  int field_type = CS_FIELD_INTENSIVE | CS_FIELD_PROPERTY;
  int location_id = CS_MESH_LOCATION_CELLS;

  {
    f = cs_field_create("rad_energy",
                        field_type,
                        location_id,
                        1,
                        false);

    /* hide property */
    cs_field_set_key_int(f, keyvis, 0);
    cs_field_set_key_int(f, keylog, 0);

    /* set label */
    cs_field_set_key_str(f, keylbl, "Rad energy");

    cs_field_pointer_map(CS_ENUMF_(rad_energy), f);
  }

  {
    f = cs_field_create("radiative_flux",
                        field_type,
                        location_id,
                        3,
                        false);

    /* hide property */
    cs_field_set_key_int(f, keyvis, 0);
    cs_field_set_key_int(f, keylog, 0);

    /* set label */
    cs_field_set_key_str(f, keylbl, "Qrad");

    cs_field_pointer_map(CS_ENUMF_(rad_q), f);
  }

  for (int irphas = 0; irphas < rt_params->nrphas; irphas++) {

    char suffix[16];

    if (irphas > 0)
      snprintf(suffix,  15, "_%02d", irphas+1);
    else
      suffix[0]  = '\0';

    suffix[15] = '\0';

    char f_name[64], f_label[64];
    {
      snprintf(f_name, 63, "rad_st%s", suffix); f_name[63] = '\0';
      snprintf(f_label, 63, "Srad%s", suffix); f_label[63]  = '\0';
      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);

      /* hide property */
      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 0);

      /* set label */
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_pointer_map_indexed(CS_ENUMF_(rad_est), irphas, f);
    }

    {
      snprintf(f_name, 63, "rad_st_implicit%s", suffix); f_name[63] = '\0';
      snprintf(f_label, 63, "ITSRI%s", suffix); f_label[63]  = '\0';
      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);

      /* hide property */
      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 0);

      /* set label */
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_pointer_map_indexed(CS_ENUMF_(rad_ist), irphas, f);
    }

    {
      snprintf(f_name, 63, "rad_absorption%s", suffix); f_name[63] = '\0';
      snprintf(f_label, 63, "Absorp%s", suffix); f_label[63]  = '\0';
      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);

      /* hide property */
      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 0);

      /* set label */
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_pointer_map_indexed(CS_ENUMF_(rad_abs), irphas, f);
    }

    {
      snprintf(f_name, 63, "rad_emission%s", suffix); f_name[63] = '\0';
      snprintf(f_label, 63, "Emiss%s", suffix); f_label[63]  = '\0';
      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);

      /* hide property */
      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 0);

      /* set label */
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_pointer_map_indexed(CS_ENUMF_(rad_emi), irphas, f);
    }

    {
      snprintf(f_name, 63, "rad_absorption_coeff%s", suffix); f_name[63] = '\0';
      snprintf(f_label, 63, "CoefAb%s", suffix); f_label[63]  = '\0';
      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);
      /* hide property */
      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 0);

      /* set label */
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_pointer_map_indexed(CS_ENUMF_(rad_cak), irphas, f);
    }
  }

  /* Add bands for Direct Solar, diFUse solar and InfraRed
   * and for solar: make the distinction between UV-visible (absorbed by O3)
   * and Solar IR (SIR) absobed by H2O
   * if activated */
  {
    if (rt_params->atmo_model
        != CS_RAD_ATMO_3D_NONE)
      rt_params->nwsgg = 0;

    /* Fields for atmospheric Direct Solar (DR) model
     * (SIR only if SUV is not activated) */
    if (rt_params->atmo_model
        & CS_RAD_ATMO_3D_DIRECT_SOLAR) {
      rt_params->atmo_dr_id = rt_params->nwsgg;
      rt_params->nwsgg++;
    }

    /* Fields for atmospheric Direct Solar (DR) model
     * (SUV band) */
    if (rt_params->atmo_model
        & CS_RAD_ATMO_3D_DIRECT_SOLAR_O3BAND) {
      rt_params->atmo_dr_o3_id = rt_params->nwsgg;
      rt_params->nwsgg++;
    }

    /* Fields for atmospheric diFfuse Solar (DF) model
     * (SIR only if SUV is activated) */
    if (rt_params->atmo_model & CS_RAD_ATMO_3D_DIFFUSE_SOLAR) {
      rt_params->atmo_df_id = rt_params->nwsgg;
      rt_params->nwsgg++;
    }

    /* Fields for atmospheric diFfuse Solar (DF) model
     * (SUV band) */
    if (rt_params->atmo_model & CS_RAD_ATMO_3D_DIFFUSE_SOLAR_O3BAND) {
      rt_params->atmo_df_o3_id = rt_params->nwsgg;
      rt_params->nwsgg++;
    }

    /* Fields for atmospheric infrared absorption model */
    if (rt_params->atmo_model & CS_RAD_ATMO_3D_INFRARED) {
      rt_params->atmo_ir_id = rt_params->nwsgg;
      rt_params->nwsgg++;
    }
  }

  /* SLFM gas combustion radiation or atmospheric:
   * add cell fields per band */
  if (   cs_glob_physical_model_flag[CS_COMBUSTION_SLFM] == 1
      || cs_glob_physical_model_flag[CS_COMBUSTION_SLFM] == 3
      || rt_params->atmo_model != CS_RAD_ATMO_3D_NONE) {

    for (int gg_id = 0; gg_id < rt_params->nwsgg; gg_id ++) {

      char suffix[16];

      snprintf(suffix,  15, "_%02d", gg_id + 1);

      suffix[15] = '\0';

      char f_name[64], f_label[64];

      snprintf(f_name, 63, "spectral_absorption%s", suffix); f_name[63] ='\0';
      snprintf(f_label, 63, "Spectral Absorption%s", suffix); f_label[63] ='\0';

      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 1);

      //TODO map it as
      // cs_field_pointer_map_indexed(CS_ENUMF_(rad_abs), gg_id, f);
      // Note: would be in conflict with rad_abs

      snprintf(f_name, 63, "spectral_absorption_coeff%s", suffix);
      f_name[63] ='\0';
      snprintf(f_label, 63, "Spectral Abs coef%s", suffix); f_label[63] ='\0';

      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 1);


      snprintf(f_name, 63, "spectral_emission%s", suffix); f_name[63] ='\0';
      snprintf(f_label, 63, "Spectral Emission%s", suffix); f_label[63] ='\0';

      f = cs_field_create(f_name,
                          field_type,
                          location_id,
                          1,
                          false);
      cs_field_set_key_str(f, keylbl, f_label);

      cs_field_set_key_int(f, keyvis, 0);
      cs_field_set_key_int(f, keylog, 1);
      //TODO map it as
      //cs_field_pointer_map_indexed(CS_ENUMF_(rad_emi), gg_id, f);
      // Note: would be in conflict with rad_abs
    }
  }

  int vis_gg = (rt_params->nwsgg == 1) ? 1 : 0;

  /* Atmospheric radiation: add cells field by band */
  if (rt_params->atmo_model != CS_RAD_ATMO_3D_NONE) {

    /* Upward rad flux by band */
    f = cs_field_create("rad_flux_up",
                        field_type,
                        location_id,
                        rt_params->nwsgg,
                        false);
    cs_field_set_key_str(f, keylbl, "Upward radiative flux");
    cs_field_pointer_map(CS_ENUMF_(fup), f);

    cs_field_set_key_int(f, keyvis, vis_gg);
    cs_field_set_key_int(f, keylog, 1);

    /* Downward rad flux by band */
    // TODO sould be "class" fields as
    // cs_field_pointer_map_indexed(CS_ENUMF_(rad_cak), irphas, f);
    f = cs_field_create("rad_flux_down",
                        field_type,
                        location_id,
                        rt_params->nwsgg,
                        false);
    cs_field_set_key_str(f, keylbl, "Downward radiative flux");
    cs_field_pointer_map(CS_ENUMF_(fdown), f);

    cs_field_set_key_int(f, keyvis, vis_gg);
    cs_field_set_key_int(f, keylog, 1);

    /* Upward absorption coefficient by band */
    f = cs_field_create("rad_absorption_coeff_up",
                        field_type,
                        location_id,
                        rt_params->nwsgg,
                        false);
    cs_field_pointer_map(CS_ENUMF_(rad_ck_up), f);

    cs_field_set_key_int(f, keyvis, vis_gg);
    cs_field_set_key_int(f, keylog, 1);

    /* Asymmetry factor (for solar only...)  per band */
    f = cs_field_create("asymmetry_factor",
                        field_type,
                        location_id,
                        rt_params->nwsgg,
                        false);

    cs_field_set_key_int(f, keyvis, vis_gg);
    cs_field_set_key_int(f, keylog, 1);

    /* Simple diffusion albedo (for solar only...)  per band */
    f = cs_field_create("simple_diffusion_albedo",
                        field_type,
                        location_id,
                        rt_params->nwsgg,
                        false);

    cs_field_set_key_int(f, keyvis, vis_gg);
    cs_field_set_key_int(f, keylog, 1);

    /* Upward absorption coefficient by band */
    f = cs_field_create("rad_absorption_coeff_down",
                        field_type,
                        location_id,
                        rt_params->nwsgg,
                        false);
    cs_field_pointer_map(CS_ENUMF_(rad_ck_down), f);

    cs_field_set_key_int(f, keyvis, vis_gg);
    cs_field_set_key_int(f, keylog, 1);
  }

  /* Boundary face fields */
  location_id = CS_MESH_LOCATION_BOUNDARY_FACES;

  /* Albedo Fields for atmospheric diFfuse Solar (FS) model */
  if (rt_params->atmo_model & CS_RAD_ATMO_3D_DIFFUSE_SOLAR) {
    f = cs_field_by_name_try("boundary_albedo");
    if (f == NULL) {
      f = cs_field_create("boundary_albedo",
                          field_type,
                          location_id,
                          1,
                          false);
      cs_field_set_key_str(f, keylbl, "Albedo");

      cs_field_set_key_int(f, keyvis, 1);
      cs_field_set_key_int(f, keylog, 1);
    }
  }

  {
    f = cs_field_by_name_try("boundary_temperature");
    if (f == NULL)
      f = cs_parameters_add_boundary_temperature();

    if (!cs_field_is_key_set(f, keylog))
      cs_field_set_key_int(f, keylog, 1);
    if (!cs_field_is_key_set(f, keyvis))
      cs_field_set_key_int(f, keyvis, CS_POST_ON_LOCATION);
  }

  {
    f = cs_field_create("rad_incident_flux",
                        field_type,
                        location_id,
                        1,
                        false);
    cs_field_set_key_str(f, keylbl, "Incident_flux");
    cs_field_pointer_map(CS_ENUMF_(qinci), f);
  }

  if (rt_params->imoadf >= 1 || rt_params->imfsck >= 1
      || rt_params->atmo_model != CS_RAD_ATMO_3D_NONE) {
    f = cs_field_create("spectral_rad_incident_flux",
                        field_type,
                        location_id,
                        rt_params->nwsgg,
                        false);
    cs_field_set_key_str(f, keylbl, "Spectral_incident_flux");
    cs_field_pointer_map(CS_ENUMF_(qinsp), f);

    cs_field_set_key_int(f, keyvis, vis_gg);
    cs_field_set_key_int(f, keylog, 1);
  }

  {
    f = cs_field_create("wall_thermal_conductivity",
                        field_type,
                        location_id,
                        1,
                        false);
    cs_field_set_key_str(f, keylbl, "Th_conductivity");
    cs_field_pointer_map(CS_ENUMF_(xlam), f);
  }

  {
    f = cs_field_create("wall_thickness",
                        field_type,
                        location_id,
                        1,
                        false);
    cs_field_set_key_str(f, keylbl, "Thickness");
    cs_field_pointer_map(CS_ENUMF_(epa), f);
  }

  {
    f = cs_field_by_name_try("emissivity");
    if (f == NULL)
      f = cs_field_create("emissivity",
                          field_type,
                          location_id,
                          1,
                          false);
    cs_field_set_key_str(f, keylbl, "Emissivity");
    cs_field_pointer_map(CS_ENUMF_(emissivity), f);
  }

  {
    f = cs_field_create("rad_net_flux",
                        field_type,
                        location_id,
                        1,
                        false);
    cs_field_set_key_str(f, keylbl, "Net_flux");
    cs_field_pointer_map(CS_ENUMF_(fnet), f);
  }

  {
    f = cs_field_create("rad_convective_flux",
                        field_type,
                        location_id,
                        1,
                        false);
    cs_field_set_key_str(f, keylbl, "Convective_flux");
    cs_field_pointer_map(CS_ENUMF_(fconv), f);
  }

  {
    f = cs_field_create("rad_exchange_coefficient",
                        field_type,
                        location_id,
                        1,
                        false);
    cs_field_set_key_str(f, keylbl, "Convective_exch_coef");
    cs_field_pointer_map(CS_ENUMF_(hconv), f);
  }
}

/*----------------------------------------------------------------------------*/

END_C_DECLS
