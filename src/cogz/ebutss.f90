!-------------------------------------------------------------------------------

! This file is part of Code_Saturne, a general-purpose CFD tool.
!
! Copyright (C) 1998-2013 EDF S.A.
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

subroutine ebutss &
!================

 ( iscal  ,                                                       &
   rtpa   ,                                                       &
   smbrs  , rovsdt )

!===============================================================================
! FONCTION :
! ----------

! ROUTINE PHYSIQUE PARTICULIERE : FLAMME PREMELANGE MODELE EBU
!   ON PRECISE LES TERMES SOURCES POUR UN SCALAIRE PP
!   SUR UN PAS DE TEMPS

! ATTENTION : LE TRAITEMENT DES TERMES SOURCES EST DIFFERENT
! ---------   DE CELUI DE USTSSC.F

! ON RESOUT ROVSDT*D(VAR) = SMBRS

! ROVSDT ET SMBRS CONTIENNENT DEJA D'EVENTUELS TERMES SOURCES
!  UTILISATEUR. IL FAUT DONC LES INCREMENTER ET PAS LES
!  ECRASER

! POUR DES QUESTIONS DE STABILITE, ON NE RAJOUTE DANS ROVSDT
!  QUE DES TERMES POSITIFS. IL N'Y A PAS DE CONTRAINTE POUR
!  SMBRS

! DANS LE CAS D'UN TERME SOURCE EN CEXP + CIMP*VAR ON DOIT
! ECRIRE :
!          SMBRS  = SMBRS  + CEXP + CIMP*VAR
!          ROVSDT = ROVSDT + MAX(-CIMP,ZERO)

! ON FOURNIT ICI ROVSDT ET SMBRS (ILS CONTIENNENT RHO*VOLUME)
!    SMBRS en kg variable/s :
!     ex : pour la vitesse            kg m/s2
!          pour les temperatures      kg degres/s
!          pour les enthalpies        Joules/s
!    ROVSDT en kg /s


!-------------------------------------------------------------------------------
!ARGU                             ARGUMENTS
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! iscal            ! i  ! <-- ! scalar number                                  !
! rtpa             ! ra ! <-- ! calculated variables at cell centers           !
!  (ncelet, *)     !    !     !  (at previous time step)                       !
! smbrs(ncelet)    ! tr ! --> ! second membre explicite                        !
! rovsdt(ncelet    ! tr ! --> ! partie diagonale implicite                     !
!__________________!____!_____!________________________________________________!

!     TYPE : E (ENTIER), R (REEL), A (ALPHANUMERIQUE), T (TABLEAU)
!            L (LOGIQUE)   .. ET TYPES COMPOSES (EX : TR TABLEAU REEL)
!     MODE : <-- donnee, --> resultat, <-> Donnee modifiee
!            --- tableau de travail
!===============================================================================

!===============================================================================
! Module files
!===============================================================================

use paramx
use numvar
use entsor
use optcal
use cstphy
use cstnum
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use mesh
use field
!===============================================================================

implicit none

! Arguments

integer          iscal

double precision rtpa(ncelet,*)
double precision smbrs(ncelet), rovsdt(ncelet)

! Local variables

character*80     chaine
integer          ivar, iel

double precision, allocatable, dimension(:) :: w1, w2, w3
double precision, dimension(:), pointer ::  crom
!===============================================================================
!===============================================================================
! 1. INITIALISATION
!===============================================================================

! Allocate temporary arrays
allocate(w1(ncelet), w2(ncelet), w3(ncelet))


! --- Numero du scalaire a traiter : ISCAL

! --- Numero de la variable associee au scalaire a traiter ISCAL
ivar = isca(iscal)

! --- Nom de la variable associee au scalaire a traiter ISCAL
call field_get_label(ivarfl(ivar), chaine)

! --- Numero des grandeurs physiques (voir cs_user_boundary_conditions)
call field_get_val_s(icrom, crom)


!===============================================================================
! 2. PRISE EN COMPTE DES TERMES SOURCES
!===============================================================================

if ( ivar.eq.isca(iygfm) ) then

! ---> Terme source pour la fraction massique moyenne de gaz frais

  if (iwarni(ivar).ge.1) then
    write(nfecra,1000) chaine(1:8)
  endif

! ---> Calcul de K et Epsilon en fonction du modele de turbulence

  if (itytur.eq.2) then

    do iel = 1, ncel
      w1(iel) = rtpa(iel,ik)
      w2(iel) = rtpa(iel,iep)
    enddo

  elseif (itytur.eq.3) then

    do iel = 1, ncel
      w1(iel) = 0.5d0 *( rtpa(iel,ir11)                    &
                        +rtpa(iel,ir22)                    &
                        +rtpa(iel,ir33) )
      w2(iel) = rtpa(iel,iep)
    enddo

  elseif (iturb.eq.50) then

    do iel = 1, ncel
      w1(iel) = rtpa(iel,ik)
      w2(iel) = rtpa(iel,iep)
    enddo

  elseif (iturb.eq.60) then

    do iel = 1, ncel
      w1(iel) = rtpa(iel,ik)
      w2(iel) = cmu*rtpa(iel,ik)*rtpa(iel,iomg)
    enddo

  endif

  do iel = 1, ncel
    if ( w1(iel).gt.epzero .and.                                  &
         w2(iel).gt.epzero       ) then
      w3(iel) = cebu*w2(iel)/w1(iel)                              &
                   *crom(iel)*volume(iel)                &
                   *(1.d0 - rtpa(iel,ivar))
      smbrs(iel) = smbrs(iel) - rtpa(iel,ivar)*w3(iel)
      rovsdt(iel) = rovsdt(iel) + max(w3(iel),zero)
    endif
  enddo

endif

! Free memory
deallocate(w1, w2, w3)

!--------
! FORMATS
!--------

 1000 format(' TERMES SOURCES PHYSIQUE PARTICULIERE POUR LA VARIABLE '  &
       ,a8,/)

!----
! FIN
!----

return

end subroutine
