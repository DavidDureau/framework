﻿cmake_minimum_required(VERSION 3.15)

# ----------------------------------------------------------------------------
# Indique que le projet n'a pas de language
# Cela est indispensable pour que CMake ne recherche pas les
# compilateurs avant d'avoir inclus les fichiers de configuration.
Project(ArcaneFrameworkFull LANGUAGES NONE)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

if(POLICY CMP0111)
  cmake_policy(SET CMP0111 NEW)
endif()

# ----------------------------------------------------------------------------
# Macro pour positionner la variable 'var_name' dans le cache
# avec sa valeur actuelle ou la valeur 'var_value' si elle n'a
# pas de valeur.
macro(arcane_set_if_unset var_name var_value var_type description)
  if (NOT DEFINED ${var_name})
    set(${var_name} ${var_value})
  endif()
  set(${var_name} ${${var_name}} CACHE ${var_type} "${description}" FORCE)
  option(${var_name} "${description}")
endmacro()

# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------

arcane_set_if_unset(ARCANE_WANT_NUMERICS TRUE BOOL "Compile numerical library")
arcane_set_if_unset(ARCANE_WANT_TOTALVIEW TRUE BOOL "Compile total view display")
arcane_set_if_unset(ARCANE_DEFAULT_PARTITIONER Metis STRING "Default partitioner")

arcane_set_if_unset(ARCANE_BUILD_TYPE Release STRING "Arcane build type")
arcane_set_if_unset(ARCANE_WANT_LIBXML2 TRUE BOOL "Use libxml2")
arcane_set_if_unset(ARCANE_WANT_LEGACY_CONNECTIVITY FALSE BOOL "Use legacy item connectivity accessor")
arcane_set_if_unset(ARCANE_WANT_CHECK FALSE BOOL "Compilation with additional check (array bounds, ...)")

# Force l'utilisation de bibliothèques dynamiques
set(BUILD_SHARED_LIBS TRUE)

set(ARCFRAMEWORK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

arcane_set_if_unset(Arccon_ROOT ${ARCFRAMEWORK_ROOT}/arccon PATH "Arccon root path")
arcane_set_if_unset(AXLSTAR_ROOT ${ARCFRAMEWORK_ROOT}/axlstar PATH "Axlstar root")
arcane_set_if_unset(Arccore_ROOT ${ARCFRAMEWORK_ROOT}/arccore PATH "Arccore root")
arcane_set_if_unset(ARCDEPENDENCIES_ROOT ${ARCFRAMEWORK_ROOT}/dependencies PATH "Arcdependencies root")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

enable_testing()

configure_file(nuget.config.in ${CMAKE_BINARY_DIR}/nuget.config @ONLY)

add_subdirectory(${ARCFRAMEWORK_ROOT}/arcane arcane)

# ----------------------------------------------------------------------------
# Recopie les packages nuget de 'dependencies' dans le répertoire
# contenant les exemples. On recopie les packages nécessaires pour
# 'netstandard2.0'.
#
set(DEPENDENCIES_NUGET ${ARCDEPENDENCIES_ROOT}/nuget_fallback)
set(SAMPLES_NUGET_DIR ${CMAKE_INSTALL_PREFIX}/samples/_nuget_fallback)
install(FILES ${DEPENDENCIES_NUGET}/microsoft.netcore.platforms.1.1.0.nupkg DESTINATION ${SAMPLES_NUGET_DIR})
install(FILES ${DEPENDENCIES_NUGET}/netstandard.library.2.0.3.nupkg DESTINATION ${SAMPLES_NUGET_DIR})

# ----------------------------------------------------------------------------
# Local Variables:
# tab-width: 2
# indent-tabs-mode: nil
# coding: utf-8-with-signature
# End:
