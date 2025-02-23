﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* IExtraGhostCellsBuilder.h                                   (C) 2000-2011 */
/*                                                                           */
/* Interface d'un constructeur de mailles fantômes "extraordinaires"         */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_IEXTRAGHOSTCELLSBUILDER_H
#define ARCANE_IEXTRAGHOSTCELLSBUILDER_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/ArcaneTypes.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Interface d'un constructeur de mailles fantômes "extraordinaires"  
 *
 * Une maille fantôme "extraordinaire" est une maille fantôme ajoutée aux
 * mailles fantômes définies par la connectivité du maillage. En particulier,
 * le calcul des mailles fantômes extraordinaires est effectué à chaque mise
 * à jour du maillage ou équilibrage de charge.
 *
 * NB : rend obsolète le paramètre remove_old_ghost de la méthode endUpdate de IMesh
 *
 */
class IExtraGhostCellsBuilder
{
public:
  
  virtual ~IExtraGhostCellsBuilder() {} //!< Libère les ressources.
  
public:

  /*!
   * \brief Calcul des mailles "extraordinaires" à envoyer
   * Effectue le calcul des mailles "extraordinaires" suivant
   * un algorithme de construction  
   */
  virtual void computeExtraCellsToSend() =0;

  /*!
   * \brief Indices locaux des mailles "extraordinaires" pour envoi
   * Récupère le tableau des mailles "extraordinaires" à destination
   * du sous-domaine \a sid
   */
  virtual IntegerConstArrayView extraCellsToSend(Integer sid) const =0;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  

