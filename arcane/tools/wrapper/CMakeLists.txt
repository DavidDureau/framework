﻿# Ce fichier contient les instructions pour générer le wrapper C# de Arcane
# Les opérations sont les suivantes:
# 1. Utiliser Swig pour générer un ensemble de classes C# et un fichier C++ contenant les wrappings
# 2. Compiler le fichier C++
# 3. Compiler les fichiers C#.
#
# Les deux bibliothèques générées sont installées dans le répertoire 'lib'

# ----------------------------------------------------------------------------

include(${CMAKE_CURRENT_SOURCE_DIR}/ArcaneSwigUtils.cmake)
if (NOT ARCANE_HAS_SWIG)
  return()
endif()

message(STATUS "Generating infos for SWIG .NET wrapper")

# ----------------------------------------------------------------------------

install(FILES NumericWrapper.h ArcaneSwigUtils.h DESTINATION include)

# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
# Note: 'core' doit être le premier car les autres wrappers dépendent de lui.

add_subdirectory(core)
add_subdirectory(services)
add_subdirectory(hdf5)
add_subdirectory(launcher)
add_subdirectory(main)

# ----------------------------------------------------------------------------

configure_file(ArcaneSwigConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/ArcaneSwigConfig.cmake @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ArcaneSwigConfig.cmake
  DESTINATION ${ARCANE_CMAKE_CONFIG_DIR})

# ----------------------------------------------------------------------------
# Installe la 'DLL' 'Arcane.Utils'.
# TODO: il faudrait utiliser un mécanisme de publication mais pour cela
# il faudrait générer dans un autre répertoire que 'lib' pour pouvoir
# tout recopier ensuite dans 'lib'.
install(FILES
  ${ARCANE_DOTNET_WRAPPER_INSTALL_DIRECTORY}/Arcane.Utils.dll
  ${ARCANE_DOTNET_WRAPPER_INSTALL_DIRECTORY}/Arcane.Utils.pdb
  DESTINATION lib)

# ----------------------------------------------------------------------------
# Local Variables:
# tab-width: 2
# indent-tabs-mode: nil
# coding: utf-8-with-signature
# End:
