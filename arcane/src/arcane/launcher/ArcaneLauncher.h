﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* ArcaneLauncher.h                                            (C) 2000-2021 */
/*                                                                           */
/* Classe gérant l'exécution.                                                */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_LAUNCHER_ARCANELAUNCHER_H
#define ARCANE_LAUNCHER_ARCANELAUNCHER_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/launcher/LauncherGlobal.h"

// Les fichiers suivants ne sont pas directement utilisés dans ce '.h'
// mais sont ajoutés pour que le code utilisateur n'ait besoin d'inclure
// que 'ArcaneLauncher.h'.
#include "arcane/utils/ApplicationInfo.h"
#include "arcane/utils/CommandLineArguments.h"
#include "arcane/ApplicationBuildInfo.h"
#include "arcane/DotNetRuntimeInitialisationInfo.h"
#include "arcane/AcceleratorRuntimeInitialisationInfo.h"
#include "arcane/launcher/DirectExecutionContext.h"
#include "arcane/launcher/DirectSubDomainExecutionContext.h"
#include "arcane/launcher/IDirectExecutionContext.h"
#include "arcane/launcher/StandaloneAcceleratorMng.h"

#include <functional>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{
class IMainFactory;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Classe de gestion de l'exécution.
 *
 * La première chose à faire est d'initialiser Arcane en
 * positionnant les arguments via la méthode init() car certains paramètres de la
 * ligne de commande sont utilisés pour remplir les propriétés
 * de applicationInfo() et dotNetRuntimeInitialisationInfo().
 *
 * Il existe ensuite deux possibilités pour exécuter du code:
 * - le comportement classique qui utilise une boucle en temps. Dans
 *   ce cas il suffit d'appeler la méthode run().
 * - l'exécution directe d'une fonction sans passer par la boucle
 *   en temps. Dans ce cas, il faut appeler la méthode runDirect().
 *
 * L'usage est le suivant:
 *
 * \code
 * int main(int* argc,char* argv[])
 * {
 *   ArcaneLauncher::init(CommandLineArguments(&argc,&argv));
 *   auto& app_info = ArcaneLauncher::applicationInfo();
 *   app_info.setCodeName("MyCode");
 *   app_info.setCodeVersion(VersionInfo(1,0,0));
 *   return ArcaneLauncher::run();
 * }
 * \endcode
 *
 */
class ARCANE_LAUNCHER_EXPORT ArcaneLauncher
{
 public:

  /*!
   * \brief Positionne les informations à partir des arguments de la ligne
   * de commande et initialise le lanceur.
   *
   * Cette méthode remplit les valeurs non initialisées
   * de applicationInfo() et dotNetRuntimeInitialisationInfo() avec
   * les paramètres spécifiés dans \a args.
   *
   * Il ne faut appeler cette méthode qu'une seule fois. Les appels supplémentaires
   * génèrent une exception FatalErrorException.
   */
  static void init(const CommandLineArguments& args);

  /*!
   * \brief Indique si init() a déjà été appelé.
   */
  static bool isInitialized();

  /*!
   * \brief Point d'entrée de l'exécutable dans Arcane.
   *
   * Cette méthode appelle initialise l'application, lit le jeu de données
   * et exécute le code suivant la boucle en temps spécifiée dans le jeu de donnée.
   *
   * \retval 0 en cas de succès
   * \return une valeur différente de 0 en cas d'erreur.
   */
  static int run();

 /*!
   * \brief Exécution directe.
   *
   * Initialise l'application et appelle la fonction \a func après l'initialisation
   * Cette méthode ne doit être appelée qu'en exécution séquentielle.
   */
  static int run(std::function<int(DirectExecutionContext&)> func);

 /*!
   * \brief Exécution directe avec création de sous-domaine.
   *
   * Initialise l'application et créé le ou les sous-domaines et appelle
   * la fonction \a func après.
   * Cette méthode permet d'exécuter du code sans passer par les mécanismes
   * de la boucle en temps.
   * Cette méthode permet de gérer automatiquement la création des sous-domaines
   * en fonction des paramètres de lancement (exécution parallèle MPI, multithreading, ...).
   */
  static int run(std::function<int(DirectSubDomainExecutionContext&)> func);

  /*!
   * \brief Positionne la fabrique par défaut pour créer les différents gestionnaires
   *
   * Cette méthode doit être appelée avant run(). L'instance passée en argument doit
   * rester valide durant l'exécution de run().
   */
  static void setDefaultMainFactory(IMainFactory* mf);

 /*!
   * \brief Informations sur l'application.
   *
   * Cette méthode permet de récupérer l'instance de `ApplicationInfo`
   * qui sera utilisée lors de l'appel à run().
   *
   * Pour être prise en compte, ces informations doivent être modifiées
   * avant l'appel à run() ou à runDirect().
   */
  static ApplicationInfo& applicationInfo();

 /*!
   * \brief Informations sur les paramêtre d'exécutions de l'application.
   *
   * Cette méthode permet de récupérer l'instance de `ApplicationBuildInfo`
   * qui sera utilisée lors de l'appel à run().
   *
   * Pour être prise en compte, ces informations doivent être modifiées
   * avant l'appel à run() ou à runDirect().
   */
  static ApplicationBuildInfo& applicationBuildInfo();

  /*!
   * \brief Informations pour l'initialisation du runtime '.Net'.
   *
   * Pour être prise en compte, ces informations doivent être modifiées
   * avant l'appel à run() ou à rundDirect().
   */
  static DotNetRuntimeInitialisationInfo& dotNetRuntimeInitialisationInfo();

  /*!
   * \brief Informations pour l'initialisation des accélerateurs.
   *
   * Pour être prise en compte, ces informations doivent être modifiées
   * avant l'appel à run() ou à rundDirect().
   */
  static AcceleratorRuntimeInitialisationInfo& acceleratorRuntimeInitialisationInfo();

  //! Nom complet du répertoire où se trouve l'exécutable
  static String getExeDirectory();

  /*!
   * \brief Créé une implémentation autonome pour gérer les accélérateurs.
   *
   * Il faut appeler init() avant d'appeler cette méthode.
   */
 static StandaloneAcceleratorMng createStandaloneAcceleratorMng();

 public:

  /*!
   * \deprecated
   */
  ARCCORE_DEPRECATED_2020("Use run(func) instead")
  static int runDirect(std::function<int(IDirectExecutionContext*)> func);

  /*!
   * \deprecated
   */
  ARCCORE_DEPRECATED_2020("Use init(args) instead")
  static void setCommandLineArguments(const CommandLineArguments& args)
  {
    init(args);
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
