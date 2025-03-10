!-------------------------------------------------------------------------------

! This file is part of code_saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2023 EDF S.A.
!
! This program is free software; you can redistribute it and/or modify it under
! the terms of the GNU General Public License as published by the Free Software
! Foundation; either version 2 of the License, or (at your option) any later
! version.
!
! This program is distributed in the hope that it will be useful, but WITHOUT
! ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
! FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
! details.
!
! You should have received a copy of the GNU General Public License along with
! this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
! Street, Fifth Floor, Boston, MA 02110-1301, USA.

!-------------------------------------------------------------------------------

!> \file cfmspr.f90
!> \brief Update the convective mass flux before the velocity prediction step.
!> It is the first step of the compressible algorithm at each time iteration.
!>
!> This function solves the continuity equation in pressure formulation and then
!> updates the density and the mass flux.
!>
!-------------------------------------------------------------------------------

!-------------------------------------------------------------------------------
! Arguments
!______________________________________________________________________________.
!  mode           name          role                                           !
!______________________________________________________________________________!
!> \param[in]     iterns        Navier-Stokes iteration number
!> \param[in]     dt            time step (per cell)
!> \param[in]     vela          velocity value at time step beginning
!_______________________________________________________________________________

subroutine cfmspr &
  (iterns, dt, vela) &
  bind(C, name='cs_compressible_convective_mass_flux')

!===============================================================================
! Module files
!===============================================================================

use, intrinsic :: iso_c_binding

use paramx
use pointe, only: rvoid1, ncetsm, icetsm, smacel
use numvar
use entsor
use optcal
use cstphy
use cstnum
use parall
use period
use ppppar
use ppthch
use ppincl
use mesh
use field
use cs_c_bindings
use cs_cf_bindings

!===============================================================================

implicit none

! Arguments

integer(c_int) ::  iterns

real(c_double) :: dt(ncelet)
real(c_double) :: vela(3, ncelet)

! Local variables

character(len=80) :: chaine
integer          ifac  , iel
integer          init  , inc   , ii, jj
integer          iflmas, iflmab
integer          iphydp, icvflb
integer          imrgrp, nswrgp, imligp, iwarnp
integer          iescap
integer          ivoid(1)
double precision epsrgp, climgp, extrap
double precision sclnor, hint

integer          imucpp
integer          imvis1, f_id0
integer          f_id

double precision normp

double precision rvoid(1)

double precision, allocatable, dimension(:) :: viscf, viscb
double precision, allocatable, dimension(:) :: smbrs, rovsdt
double precision, allocatable, dimension(:) :: w1
double precision, allocatable, dimension(:) :: w7, w8, w9
double precision, allocatable, dimension(:) :: w10
double precision, allocatable, dimension(:) :: wflmas, wflmab, ivolfl, bvolfl
double precision, allocatable, dimension(:) :: wbfa, wbfb
double precision, allocatable, dimension(:) :: dpvar

double precision, allocatable, dimension(:) :: c2

double precision, dimension(:), pointer :: coefaf_p, coefbf_p
double precision, dimension(:), pointer :: imasfl, bmasfl
double precision, dimension(:), pointer :: rhopre, crom, brom
double precision, dimension(:), pointer :: cvar_pr, cvara_pr, cpro_cp, cpro_cv
double precision, dimension(:,:), pointer :: coefau
double precision, dimension(:,:,:), pointer :: coefbu
double precision, dimension(:), pointer :: cpro_divq
double precision, dimension(:), pointer :: cvar_fracv, cvar_fracm, cvar_frace

type(var_cal_opt) :: vcopt_p
type(var_cal_opt), target :: vcopt_loc
type(var_cal_opt), pointer :: p_k_value
type(c_ptr) :: c_k_value

!===============================================================================
! 1. INITIALISATION
!===============================================================================

extrap = 0

! Allocate temporary arrays for the mass resolution
allocate(viscf(nfac), viscb(nfabor))
allocate(smbrs(ncelet), rovsdt(ncelet))
allocate(wflmas(nfac), wflmab(nfabor))
allocate(ivolfl(nfac), bvolfl(nfabor))

allocate(wbfa(nfabor), wbfb(nfabor))

! Allocate work arrays
allocate(w1(ncelet))
allocate(w7(ncelet), w8(ncelet), w9(ncelet))
allocate(w10(ncelet))
allocate(dpvar(ncelet))

call field_get_coefa_v(ivarfl(iu), coefau)
call field_get_coefb_v(ivarfl(iu), coefbu)

! --- Number of computational variable and post for pressure

f_id0 = -1

! --- Mass flux associated to energy
call field_get_key_int(ivarfl(isca(ienerg)), kimasf, iflmas)
call field_get_key_int(ivarfl(isca(ienerg)), kbmasf, iflmab)
call field_get_val_s(iflmas, imasfl)
call field_get_val_s(iflmab, bmasfl)

call field_get_val_s(icrom, crom)
call field_get_val_s(ibrom, brom)
call field_get_val_prev_s(icrom, rhopre)

call field_get_val_s(ivarfl(ipr), cvar_pr)
call field_get_val_prev_s(ivarfl(ipr), cvara_pr)


call field_get_label(ivarfl(ipr), chaine)

call field_get_key_struct_var_cal_opt(ivarfl(ipr), vcopt_p)

if (ippmod(icompf).gt.1) then
  call field_get_val_s(ivarfl(isca(ifracv)), cvar_fracv)
  call field_get_val_s(ivarfl(isca(ifracm)), cvar_fracm)
  call field_get_val_s(ivarfl(isca(ifrace)), cvar_frace)
else
  cvar_fracv => rvoid1
  cvar_fracm => rvoid1
  cvar_frace => rvoid1
endif

if(vcopt_p%iwarni.ge.1) then
  write(nfecra,1000) chaine(1:8)
endif

call field_get_coefaf_s(ivarfl(ipr), coefaf_p)
call field_get_coefbf_s(ivarfl(ipr), coefbf_p)

if (icp.ge.0) then
  call field_get_val_s(icp, cpro_cp)
else
  cpro_cp => rvoid1
endif

if (icv.ge.0) then
  call field_get_val_s(icv, cpro_cv)
else
  cpro_cv => rvoid1
endif

! Computation of the boundary coefficients for the pressure gradient
! recontruction in accordance with the diffusion boundary coefficients
! (coefaf_p, coefbf_p)

do ifac = 1, nfabor
  iel = ifabor(ifac)
  hint = dt(iel) / distb(ifac)

  call set_neumann_scalar                 &
     ( wbfa(ifac)    , coefaf_p(ifac),    &
       wbfb(ifac)    , rvoid(1)      ,    &
       coefaf_p(ifac), hint )
enddo

!===============================================================================
! 2. SOURCE TERMS
!===============================================================================

! --> Initialization

do iel = 1, ncel
  smbrs(iel) = 0.d0
enddo
do iel = 1, ncel
  rovsdt(iel) = 0.d0
enddo


!     MASS SOURCE TERM
!     ================

if (ncetsm.gt.0) then
  do ii = 1, ncetsm
    iel = icetsm(ii)
    smbrs(iel) = smbrs(iel) + smacel(ii,ipr)*cell_f_vol(iel)
  enddo
endif


!     UNSTEADY TERM
!     =============

! --- Calculation of the square of sound velocity c2.
!     Pressure is an unsteady variable in this algorithm
!     Varpos has been modified for that.

allocate(c2(ncelet))

call cs_cf_thermo_c_square(cpro_cp, cpro_cv, cvar_pr, crom, &
                           cvar_fracv, cvar_fracm, cvar_frace, c2, ncel)

do iel = 1, ncel
  rovsdt(iel) = rovsdt(iel) + vcopt_p%istat*(cell_f_vol(iel)/(dt(iel)*c2(iel)))
enddo

!===============================================================================
! 3. "MASS FLUX" AND FACE "VISCOSITY" CALCULATION
!===============================================================================

! computation of the "convective flux" for the density

! volumic flux (u + dt f)
call cfmsfp(iterns, dt, vela, ivolfl, bvolfl)

! mass flux at internal faces (upwind scheme for the density)
! (negative because added to RHS)
do ifac = 1, nfac
  ii = ifacel(1,ifac)
  jj = ifacel(2,ifac)
  wflmas(ifac) = -0.5d0*                                                       &
                 ( crom(ii)*(ivolfl(ifac)+abs(ivolfl(ifac)))                   &
                 + crom(jj)*(ivolfl(ifac)-abs(ivolfl(ifac))))
enddo

! mass flux at boundary faces
! (negative because added to RHS)
do ifac = 1, nfabor
  iel = ifabor(ifac)
  wflmab(ifac) = -brom(ifac)*bvolfl(ifac)
enddo

init = 0
call divmas(init,wflmas,wflmab,smbrs)

call field_get_id_try("predicted_vel_divergence", f_id)
if (f_id.ge.0) then
  call field_get_val_s(f_id, cpro_divq)
  do iel = 1, ncel
    cpro_divq(iel) = smbrs(iel)
  enddo
endif

! (Delta t)_ij is calculated as the "viscosity" associated to the pressure
imvis1 = 1

call viscfa                                                                    &
( imvis1 ,                                                                     &
  dt     ,                                                                     &
  viscf  , viscb  )

!===============================================================================
! 4. SOLVING
!===============================================================================

imrgrp = vcopt_p%imrgra
nswrgp = vcopt_p%nswrgr
imligp = vcopt_p%imligr
iescap = 0
imucpp = 0
iwarnp = vcopt_p%iwarni
epsrgp = vcopt_p%epsrgr
climgp = vcopt_p%climgr
icvflb = 0
normp = -1.d0

vcopt_loc = vcopt_p

vcopt_loc%istat  = -1
vcopt_loc%icoupl = -1
vcopt_loc%idifft = -1
vcopt_loc%iwgrec = 0 ! Warning, may be overwritten if a field
vcopt_loc%blend_st = 0 ! Warning, may be overwritten if a field

p_k_value => vcopt_loc
c_k_value = equation_param_from_vcopt(c_loc(p_k_value))

call cs_equation_iterative_solve_scalar                 &
 ( idtvar , init   ,                                    &
   ivarfl(ipr)     , c_null_char ,                      &
   iescap , imucpp , normp  , c_k_value       ,         &
   cvara_pr        , cvara_pr        ,                  &
   wbfa   , wbfb   , coefaf_p        , coefbf_p       , &
   wflmas , wflmab ,                                    &
   viscf  , viscb  , viscf  , viscb  ,                  &
   rvoid  , rvoid  , rvoid  ,                           &
   icvflb , ivoid  ,                                    &
   rovsdt , smbrs  , cvar_pr, dpvar  ,                  &
   rvoid  , rvoid  )

!===============================================================================
! 5. PRINTINGS AND CLIPPINGS
!===============================================================================

! --- User intervention for a finer management of the bounds and possible
!       corrective treatement.

call cs_cf_check_pressure(cvar_pr, ncel)

! Explicit balance (see cs_equation_iterative_solve_scalar:
! the increment is removed)

if (vcopt_p%iwarni.ge.2) then
  do iel = 1, ncel
    smbrs(iel) = smbrs(iel)                                                    &
                 - vcopt_p%istat*(cell_f_vol(iel)/dt(iel))                     &
                   *(cvar_pr(iel)-cvara_pr(iel))                               &
                   * max(0,min(vcopt_p%nswrsm-2,1))
  enddo
  sclnor = sqrt(cs_gdot(ncel,smbrs,smbrs))
  write(nfecra,1200) chaine(1:8) ,sclnor
endif

!===============================================================================
! 6. COMMUNICATION OF P
!===============================================================================

if (irangp.ge.0.or.iperio.eq.1) then
  call synsca(cvar_pr)
endif

!===============================================================================
! 7. ACOUSTIC MASS FLUX CALCULATION AT THE FACES
!===============================================================================

! mass flux = [dt (grad P).n] + [rho (u + dt f)]

! Computation of [dt (grad P).n] by itrmas
init   = 1
inc    = 1
iphydp = 0
! festx,y,z    = rvoid
! viscf, viscb = arithmetic mean at faces
! viscelx,y,z  = dt
! This flux is stored as the mass flux of the energy

call itrmas                                                                    &
( f_id0  , init   , inc    , imrgrp , nswrgp , imligp ,                        &
  iphydp , 0      , iwarnp ,                                                   &
  epsrgp , climgp , extrap ,                                                   &
  rvoid  ,                                                                     &
  cvar_pr,                                                                     &
  wbfa   , wbfb   ,                                                            &
  coefaf_p        , coefbf_p        ,                                          &
  viscf  , viscb  ,                                                            &
  dt     ,                                                                     &
  imasfl, bmasfl)

! Incrementation of the flux with [rho (u + dt f)].n = wflmas
! (added with a negative sign since wflmas,wflmab was used above
! in the right hand side).
do ifac = 1, nfac
  imasfl(ifac) = imasfl(ifac) - wflmas(ifac)
enddo
do ifac = 1, nfabor
  bmasfl(ifac) = bmasfl(ifac) - wflmab(ifac)
enddo

!===============================================================================
! 8. UPDATING OF THE DENSITY
!===============================================================================

if (igrdpp.gt.0) then

  do iel = 1, ncel
    ! Backup of the current density values
    rhopre(iel) = crom(iel)
    ! Update of density values
    crom(iel) = crom(iel)+(cvar_pr(iel)-cvara_pr(iel))/c2(iel)
  enddo

!===============================================================================
! 9. DENSITY COMMUNICATION
!===============================================================================

  if (irangp.ge.0.or.iperio.eq.1) then
    call synsca(crom)
    call synsca(rhopre)
  endif

endif

deallocate(c2)
deallocate(viscf, viscb)
deallocate(smbrs, rovsdt)
deallocate(wflmas, wflmab)
deallocate(ivolfl, bvolfl)
deallocate(w1)
deallocate(w7, w8, w9)
deallocate(w10)
deallocate(dpvar)
deallocate(wbfa, wbfb)

!--------
! FORMATS
!--------

 1000 format(/,                                                                 &
'   ** RESOLUTION FOR THE VARIABLE ',A8                        ,/,              &
'      ---------------------------                            ',/)
 1200 format(1X,A8,' : EXPLICIT BALANCE = ',E14.5)

!----
! END
!----

return
end subroutine
