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

subroutine lwcini &
!================

 ( nvar   , nscal  ,                                              &
   dt     , rtp    , propce )

!===============================================================================
! FONCTION :
! --------

! INITIALISATION DES VARIABLES DE CALCUL
!    POUR LA PHYSIQUE PARTICULIERE : COMBUSTION GAZ MODELE LWC
!    PENDANT DE USINIV.F

! Cette routine est appelee en debut de calcul (suite ou non)
!     avant le debut de la boucle en temps

! Elle permet d'INITIALISER ou de MODIFIER (pour les calculs suite)
!     les variables de calcul,
!     les valeurs du pas de temps


! On dispose ici de ROM et VISCL initialises par RO0 et VISCL0
!     ou relues d'un fichier suite
! On ne dispose des variables VISCLS, CP (quand elles sont
!     definies) que si elles ont pu etre relues dans un fichier
!     suite de calcul

! LA MODIFICATION DES PROPRIETES PHYSIQUES (ROM, VISCL, VISCLS, CP)
!     SE FERA EN STANDARD DANS LE SOUS PROGRAMME PPPHYV
!     ET PAS ICI

! Arguments
!__________________.____._____.________________________________________________.
! name             !type!mode ! role                                           !
!__________________!____!_____!________________________________________________!
! nvar             ! i  ! <-- ! total number of variables                      !
! nscal            ! i  ! <-- ! total number of scalars                        !
! dt(ncelet)       ! tr ! <-- ! valeur du pas de temps                         !
! rtp              ! tr ! <-- ! variables de calcul au centre des              !
! (ncelet,*)       !    !     !    cellules                                    !
! propce(ncelet, *)! ra ! <-- ! physical properties at cell centers            !
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
use optcal
use cstphy
use cstnum
use entsor
use parall
use period
use ppppar
use ppthch
use coincl
use cpincl
use ppincl
use mesh
use field

!===============================================================================

implicit none

integer          nvar   , nscal

double precision dt(ncelet), rtp(ncelet,*), propce(ncelet,*)

! Local variables

character*80     chaine
integer          iel, mode, igg, izone
integer          iscal, ivar, ii
double precision hinit, coefg(ngazgm), hair, tinitk
double precision sommqf, sommqt, sommq, tentm, fmelm
double precision valmax, valmin, xkent, xeent, d2s3

! NOMBRE DE PASSAGES DANS LA ROUTINE

integer          ipass
data             ipass /0/
save             ipass

!===============================================================================

!===============================================================================
! 1.  INITIALISATION VARIABLES LOCALES
!===============================================================================

ipass = ipass + 1


do igg = 1, ngazgm
  coefg(igg) = zero
enddo

d2s3 = 2.d0/3.d0

!===============================================================================
! 2. INITIALISATION DES INCONNUES :
!      UNIQUEMENT SI ON NE FAIT PAS UNE SUITE
!===============================================================================

if ( isuite.eq.0 ) then

! ---> Initialisation au 1er passage avec de l'air a TINITK
!                                    ======================

  if ( ipass.eq.1 ) then

! ----- Temperature du melange : air a TINITK
    tinitk = t0

! ----- Enthalpie de l'air a TINITK
    if ( ippmod(icolwc).eq.1 .or. ippmod(icolwc).eq.3 .or.        &
         ippmod(icolwc).eq.5  ) then
      coefg(1) = zero
      coefg(2) = 1.d0
      coefg(3) = zero
      mode     = -1
      call cothht                                                 &
      !==========
        ( mode   , ngazg , ngazgm  , coefg  ,                     &
          npo    , npot   , th     , ehgazg ,                     &
          hair   , tinitk )
    endif

! ----- On en profite pour initialiser FRMEL et TGF
!       CAR on n'a pas encore vu usebuc.F

    frmel = zero
    tgf   = 300.d0

! ---- Initialisation de k et epsilon

    xkent = 1.d-10
    xeent = 1.d-10

    do iel = 1, ncel

! ---- TURBULENCE

      if (itytur.eq.2) then

        rtp(iel,ik)  = xkent
        rtp(iel,iep) = xeent

      elseif (itytur.eq.3) then

        rtp(iel,ir11) = d2s3*xkent
        rtp(iel,ir22) = d2s3*xkent
        rtp(iel,ir33) = d2s3*xkent
        rtp(iel,ir12) = 0.d0
        rtp(iel,ir13) = 0.d0
        rtp(iel,ir23) = 0.d0
        rtp(iel,iep)  = xeent

      elseif (iturb.eq.50) then

        rtp(iel,ik)   = xkent
        rtp(iel,iep)  = xeent
        rtp(iel,iphi) = d2s3
        rtp(iel,ifb)  = 0.d0

      elseif (iturb.eq.60) then

        rtp(iel,ik)   = xkent
        rtp(iel,iomg) = xeent/cmu/xkent

      elseif (iturb.eq.70) then

        rtp(iel,inusa) = cmu*xkent**2/xeent

      endif

! ----- Fraction massique de fuel et sa variance

      rtp(iel,isca(iyfm)) = fmax
      rtp(iel,isca(iyfp2m)) = zero

! ----- Fraction de melange et sa variance

      rtp(iel,isca(ifm))   = fmax
      rtp(iel,isca(ifp2m)) = zero

      if ( ippmod(icolwc).ge. 2) then
        rtp(iel,isca(icoyfp))   = zero
      endif

! ----- Enthalpie du melange

      if ( ippmod(icolwc).eq.1 .or. ippmod(icolwc).eq.3 .or.      &
          ippmod(icolwc).eq.5) then
        rtp(iel,isca(iscalt)) = hair
      endif

    enddo

! ---> Initialisation au 2eme passage

  else if ( ipass.eq.2 ) then

! ----- Calculs preliminaires : Fraction de melange, T, H
!     (la valeur NOZAPM est utilisee pour inclure les aspects parall)
    sommqf = zero
    sommq  = zero
    sommqt = zero
    do izone = 1, nozapm
      sommqf = sommqf + qimp(izone)*fment(izone)
      sommqt = sommqt + qimp(izone)*tkent(izone)
      sommq  = sommq  + qimp(izone)
    enddo

    if(abs(sommq).gt.epzero) then
      fmelm = sommqf / sommq
      tentm = sommqt / sommq
    else
      fmelm = zero
      tentm = t0
    endif

! ----- Enthalpie du melange HINIT
    if ( ippmod(icolwc).eq.1 .or. ippmod(icolwc).eq.3 .or.        &
        ippmod(icolwc).eq.5  ) then
      coefg(1) = fmelm
      coefg(2) = (1.d0-fmelm)
      coefg(3) = zero
      mode     = -1
      call cothht                                                 &
      !==========
        ( mode   , ngazg , ngazgm  , coefg  ,                     &
          npo    , npot   , th     , ehgazg ,                     &
          hinit  , tentm )
    endif


! ----- On donne la main a l'utilisateur

    call cs_user_initialization &
    !==========================
  ( nvar   , nscal  ,                                            &
    dt     , rtp    , propce )

! ----- En periodique et en parallele,
!       il faut echanger ces initialisations (qui sont en fait dans RTPA)

    if (irangp.ge.0.or.iperio.eq.1) then
      call synsca(rtp(1,isca(iyfm)))
      !==========
      call synsca(rtp(1,isca(iyfp2m)))
        !==========
      call synsca(rtp(1,isca(ifm)))
      !==========
      call synsca(rtp(1,isca(ifp2m)))
      !==========

      if ( ippmod(icolwc).ge.2) then
        call synsca(rtp(1,isca(icoyfp)))
        !==========
      endif

      if (ippmod(icolwc).eq.1 .or. ippmod(icolwc).eq.3 .or.       &
          ippmod(icolwc).eq.5 ) then
        call synsca(rtp(1,isca(iscalt)))
        !==========
      endif

    endif


!      Impressions de controle

    write(nfecra,2000)

    do ii  = 1, nscapp
      iscal = iscapp(ii)
      ivar  = isca(iscal)
      valmax = -grand
      valmin =  grand
      do iel = 1, ncel
        valmax = max(valmax,rtp(iel,ivar))
        valmin = min(valmin,rtp(iel,ivar))
      enddo
      call field_get_label(ivarfl(ivar), chaine)
      if (irangp.ge.0) then
        call parmin(valmin)
        !==========
        call parmax(valmax)
        !==========
      endif
      write(nfecra,2010)chaine(1:8),valmin,valmax
    enddo

    write(nfecra,2020)

  endif

endif

!----
! FORMATS
!----


 2000 format(                                                           &
'                                                             ',/,&
' ----------------------------------------------------------- ',/,&
'                                                             ',/,&
'                                                             ',/,&
' ** INITIALISATION DES VARIABLES PROPRES AU GAZ (FL PRE LWC) ',/,&
'    -------------------------------------------------------- ',/,&
'           2eme PASSAGE                                      ',/,&
' ---------------------------------                           ',/,&
'  Variable  Valeur min  Valeur max                           ',/,&
' ---------------------------------                           '  )

 2010 format(                                                           &
 2x,     a8,      e12.4,      e12.4                              )

 2020 format(                                                           &
' ---------------------------------                           ',/)

!----
! FIN
!----

return
end subroutine
