﻿# Ce fichier contient un ensemble de fonctions et macros pour Arcane

# ----------------------------------------------------------------------------
# Macro pour faire un find_package() de manière optionnel.
#
# Le but est de pouvoir désactiver automatiquement tous les packages
# optionnel si demandé. Cela est utile par exemple si on veut garantir
# qu'on ne va pas ajouter des packages trouvés automatiquement mais
# non demandés.
#
# Si la variable ARCANE_NO_DEFAULT_PACKAGE vaut TRUE, alors les packages
# non requis seront indisponibles par défaut. Pour que ces packages
# soient trouvés, il faut qu'ils soient dans la liste
# ARCANE_REQUIRED_PACKAGE_LIST et dans ce cas le package est requis.
#
# NOTE: il faut que ce soit une macro et pas une fonction sinon les
# résultats du find_package() ne sont pas transmis à l'appelant.
macro(arcane_find_package package_name)
  set(__arcane_disable_package false)
  set(__arcane_required_package false)
  if (${package_name} IN_LIST ARCANE_REQUIRED_PACKAGE_LIST)
    set(__arcane_required_package true)
  endif()
  if (ARCANE_NO_DEFAULT_PACKAGE)
    # Si pas de package par défaut, alors il faut explicitement lister
    # tous les packages dans la variable ARCANE_REQUIRED_PACKAGE_LIST
    if (${package_name} IN_LIST ARCANE_REQUIRED_PACKAGE_LIST)
      set(__arcane_required_package true)
    else()
      message(STATUS "Disabling package '${package_name}' because variable ARCANE_NO_DEFAULT_PACKAGE is set")
      set(__arcane_disable_package true)
    endif()
  endif()
  if(__arcane_disable_package)
    set(CMAKE_DISABLE_FIND_PACKAGE_${package_name} TRUE)
  endif()
  find_package(${package_name} ${ARGN})
  if(__arcane_required_package)
    if(NOT ${package_name}_FOUND)
      message(FATAL_ERROR "Required package '${package_name}' is not available")
    endif()
  endif()
endmacro()

# ----------------------------------------------------------------------------
# Macro pour ajouter des fichiers 'axl' à la cible \a target.
#
# Par exemple:
#
# arcane_add_axl_files(arcane_std RELATIVE_PATH arcane/std
#   FILES
#   ArcaneVerifier
#   ArcanePostProcessing
#   ArcaneCheckpoint
# )
#
# Les noms des fichiers ne doivent pas contenir l'extension 'axl'.
#
# L'option COPY_PATH permet de spécifier un répertoire où les fichiers
# 'axl' seront copiés. Si cette option est spécifiée, les fichiers
# seront aussi installés dans share/axl lors de l'installation.
#
# Les répertoires contenant les fichiers axl générés sont aussi
# ajoutés à la liste des includes.
#
# NOTE: pour l'instant, cette macro ne supporte pas la génération des
# fichiers pour le C# (l'option LANGUAGE n'est pas utilisée).
#
macro(arcane_add_axl_files target)

  set(options        )
  set(oneValueArgs   COPY_PATH LANGUAGE INPUT_PATH RELATIVE_PATH)
  set(multiValueArgs FILES)

  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(rel_path ${ARGS_RELATIVE_PATH})
  set(_OUT_AXL_DIR ${CMAKE_BINARY_DIR}/${rel_path})
  file(MAKE_DIRECTORY ${_OUT_AXL_DIR})
  if (NOT ARGS_INPUT_PATH)
    set(ARGS_INPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  # Pour être sur de construire les axl avant notre cible.
  # Cela est nécessaire car CMake ne gère pas dépendances sur les
  # fichiers entre les sous-répertoires
  # (utile uniquement si on compile Axlstar en même temps que Arcane)
  if (TARGET dotnet_axl_depend)
    add_dependencies(${target} dotnet_axl_depend)
  endif()
  # Cette variable contient la liste des répertoires dans lesquels
  # seront générés les 'axl'.
  set(_INCLUDES_TO_ADD)
  foreach(iaxl ${ARGS_FILES})
    #message(STATUS "FOREACH i='${iaxl}'")
    set(_AXL_FILE ${ARGS_INPUT_PATH}/${rel_path}/${iaxl}.axl)
    set(_CUSTOM_ARGS)
    # _TOCOPY_AXL_FILE est le nom du fichier 'axl' suffixé par 'rel_path' et où
    # les '/' sont remplacés par des '_'.
    # Par exemple, si \a rel_path vaut 'arcane/std' et le fichier est 'ArcaneVerifier',
    # alors la valeur sera 'ArcaneVerifier_arcane_std.axl'
    set(_AXL_RELATIVE_FILE ${rel_path}/${iaxl})
    get_filename_component(_AXL_RELATIVE_PATH ${_AXL_RELATIVE_FILE} DIRECTORY)
    get_filename_component(_AXL_ONLY_FILE ${_AXL_RELATIVE_FILE} NAME)
    string(REPLACE "/" "_" _TOCOPY_AXL_FILE ${_AXL_ONLY_FILE}/${_AXL_RELATIVE_PATH}.axl)
    #message(STATUS "_TOCOPY_AXL_FILE='${_TOCOPY_AXL_FILE}")
    if (ARGS_COPY_PATH)
      list(APPEND _CUSTOM_ARGS "--copy")
      list(APPEND _CUSTOM_ARGS "${ARGS_COPY_PATH}/${_TOCOPY_AXL_FILE}")
    endif()
    set(_OUT_AXL_PATH ${CMAKE_BINARY_DIR}/${_AXL_RELATIVE_PATH})
    set(_OUT_AXL_FILE ${_OUT_AXL_PATH}/${_AXL_ONLY_FILE}_axl.h)
    file(MAKE_DIRECTORY ${_OUT_AXL_PATH})
    add_custom_command(OUTPUT ${_OUT_AXL_FILE}
      DEPENDS ${_AXL_FILE} ${AXLSTAR_AXL2CC_TARGETDEPEND}
      COMMAND ${ARCANE_AXL2CC} ARGS -i ${rel_path} -o ${_OUT_AXL_PATH} ${_CUSTOM_ARGS} ${_AXL_FILE}
      )
    set_source_files_properties(${_OUT_AXL_FILE} PROPERTIES GENERATED TRUE)
    # Ajoute le fichier généré à la liste des sources. Cela n'est utile
    # que pour les projets tels que CLion ou Visual Studio.
    target_sources(${target} PRIVATE ${_OUT_AXL_FILE})
    if (ARGS_COPY_PATH)
      # TODO: pouvoir changer le chemin d'installation.
      install(FILES ${ARGS_COPY_PATH}/${_TOCOPY_AXL_FILE} DESTINATION share/axl)
    endif()
    # Ajoute répertoire où se trouve les 'axl' à la cible
    list(APPEND _INCLUDES_TO_ADD ${_OUT_AXL_PATH})
  endforeach()
  # Ajoute les répertoires de génération à la cible
  if (_INCLUDES_TO_ADD)
    list(REMOVE_DUPLICATES _INCLUDES_TO_ADD)
    target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${_INCLUDES_TO_ADD}>)
  endif()
endmacro()

# ----------------------------------------------------------------------------
# Fonction pour positionner le chemin d'installation d'une cible
# dans le même répertoire quel que soit son type (library/executable)
# et la configuration (debug, release, ...).
# Par défaut, le répertoire est '${CMAKE_BINARY_DIR}/lib'
#
function(arcane_target_set_standard_path target)
  set(options        )
  set(oneValueArgs   PATH)
  set(multiValueArgs)

  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if (ARGS_PATH)
    set(_libpath ${ARGS_PATH})
  else()
    set(_libpath ${CMAKE_BINARY_DIR}/lib)
  endif()
  message(STATUS "arcane_target_set_standard_path target='${target}'")
  set_target_properties(${target}
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${_libpath}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${_libpath}
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${_libpath}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${_libpath}
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${_libpath}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${_libpath}
    LIBRARY_OUTPUT_DIRECTORY_MINSIZERELEASE ${_libpath}
    RUNTIME_OUTPUT_DIRECTORY_MINSIZERELEASE ${_libpath}
    )
endfunction()

# ----------------------------------------------------------------------------
# Fonction pour créér une bibliothèque avec des sources.
#
# Par exemple:
#
# arcane_add_library(arcane_std
#   INTUT_PATH ...
#   RELATIVE_PATH arcane/std
#   FILES ArcaneVerifier.cc ArcaneVerifier.h ...
#   AXL_FILES ArcaneVerifier
# )
#
# Chaque fichier spécifié dans ${FILES} doit trouver dans le
# répertoire ${INPUT_PATH}/${RELATIVE_PATH}. FILES peut contenir
# des sous-répertoires. Les fichiers spécifiés dans ${AXL_FILES}
# ne doivent pas contenir l'extension 'axl'.
#
# Les fichiers de FILES qui ont l'extension '.h' ou '.H' seront
# installés dans le répertoire *include* de destination avec un
# chemin relative donné par RELATIVE_PATH. Par convention, les fichiers qui
# sont dans un sous-répertoire de nom 'internal' sont considérés comme
# interne et ne sont pas installés.
function(arcane_add_library target)
  set(options        )
  set(oneValueArgs   INPUT_PATH RELATIVE_PATH)
  set(multiValueArgs FILES AXL_FILES)

  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(rel_path ${ARGS_RELATIVE_PATH})
  set(_FILES)
  #set(_INSTALL_FILES)
  foreach(isource ${ARGS_FILES})
    set(_FILE ${ARGS_INPUT_PATH}/${ARGS_RELATIVE_PATH}/${isource})
    #message(STATUS "FOREACH i='${_FILE}'")
    # Regarde si le fichier est un fichier d'en-tête et si
    # c'est le cas, l'installe automatiquement sauf s'il est
    # dans un sous-répertoire de nom *internal* auquel cas il
    # est considéré comme interne à Arcane.
    string(REGEX MATCH "\.(h|H)$" _OUT_HEADER ${isource})
    if (_OUT_HEADER)
      get_filename_component(_HEADER_RELATIVE_PATH ${isource} DIRECTORY)
      string(REGEX MATCH "internal/" _INTERNAL_HEADER ${isource})
      if (NOT _INTERNAL_HEADER)
        install(FILES ${_FILE} DESTINATION include/${rel_path}/${_HEADER_RELATIVE_PATH})
      endif()
    endif()
    list(APPEND _FILES ${_FILE})
  endforeach()
  #message(STATUS "TARGET=${target} FILES=${_FILES}")
  add_library(${target} ${_FILES})
  target_compile_definitions(${target} PRIVATE ARCANE_COMPONENT_${target})
  set_target_properties(${target}
    PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH 1
    INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib
    )
  arcane_target_set_standard_path(${target})
  if (ARGS_AXL_FILES)
    arcane_add_axl_files(${target}
      INPUT_PATH ${ARGS_INPUT_PATH}
      RELATIVE_PATH ${ARGS_RELATIVE_PATH}
      COPY_PATH ${CMAKE_BINARY_DIR}/share/axl
      FILES ${ARGS_AXL_FILES}
      )
  endif()
endfunction()

# ----------------------------------------------------------------------------
# Fonction pour ajouter à la cible 'target' une liste de packages
#
# Usage:
#
#  arcane_add_arccon_packages(target (PRIVATE|PUBLIC) <pkg1> <pkg2> ...)
#
# Par exemple:
#
#  arcane_add_arccon_packages(arcane_std PRIVATE HDF5 TBB)
#
# La liste des packages est sans le préfix 'arccon::'.
#
# En plus d'ajouter la liste des packages, cette fonction
# ajoute aussi en public un argument '-rpath' pour chaque
# répertoire contenant une bibliothèque des packages.
# Cela est nécessaire car actuellement avec CMake (3.11), les
# bibliothèques INTERFACE n'assurent pas la transitivité des
# chemins des bibliothèques dynamiques. Sans cette option, il
# est par exemple possible lors de l'édition de lien de prendre
# une mauvaise bibliothèque notamment s'il y a une version
# installée par le système.
#
# Par exemple, si on a le package HDF5 dans /opt/hdf5 et
# qu'il est aussi fourni par le système dans /usr/lib64.
# Si une bibliothèuq A dépend de manière privée de HDF5, et
# qu'un exécutable B dépend de A, on a:
#
# (1) gcc -o libA.so ... /opt/hdf5/libhdf5.so
# (2) gcc -o B ./libA.so
#
# Dans ce 'gcc' va cherche la version de hdf5 qui est dans
# /usr/lib64. Pour éviter cela, il faut faire:
#
#  gcc -o B -Wl,-rpath,/otp/hdf5 ./libA.so
#
function(arcane_add_arccon_packages target visibility)
  #message(STATUS "Function 'arcane_add_arccon_packages' target=${target} visibility=${visibility} list='${ARGN}'")
  # _RPATH_LIST contiendra la liste des répertoires contenant les bibliothèques des packages
  set(_RPATH_LIST)
  foreach(package ${ARGN})
    if (DEFINED ARCCON_TARGET_${package})
      set(_PKG ${ARCCON_TARGET_${package}})
    else()
      set(_PKG arccon::${package})
    endif()
    #message(STATUS "Check add package '${_PKG}'")
    if (TARGET ${_PKG})
      #message(STATUS "Add package '${_PKG}'")
      target_link_libraries(${target} ${visibility} ${_PKG})

      # Récupère les bibliothèques définies dans le package.
      # TODO: ne traiter que les répertoires correspondants aux bibliothèques
      # dynamiques.
      get_target_property(_LIB ${_PKG} INTERFACE_LINK_LIBRARIES)
      foreach(lib ${_LIB})
        #message(STATUS "LIB::${lib}")
        get_filename_component(_LIB_PATH ${lib} DIRECTORY)
        if (_LIB_PATH)
          list(APPEND _RPATH_LIST ${_LIB_PATH})
        endif()
      endforeach()
    endif()
  endforeach()
  # TODO: regarder plutôt sur quelles plateforme il faut le
  # faire au lieu de juste retirer windows.
  if (NOT WIN32 AND (_RPATH_LIST))
    list(REMOVE_DUPLICATES _RPATH_LIST)
    # Les répertoires systèmes ne doivent pas être dans le rpath
    set(_REMOVED_DIRS "/lib" "/lib64" "/usr/lib" "/usr/lib64")
    list(REMOVE_ITEM _RPATH_LIST ${_REMOVED_DIRS})
    foreach(path ${_RPATH_LIST})
      #message(STATUS "Add rpath '${path}' to target '${target}'")
      target_link_libraries(${target} PUBLIC "-Wl,-rpath,${path}")
    endforeach()
  endif()
endfunction()

# ----------------------------------------------------------------------------
# Macro pour enregistrer une cible qui est une bibliothèque.
#
# Cela permet de positionner aussi les RPATH et le préfix d'installation.
# La cible est ajoutée à la variable ARCANE_LIBRARIES qui permettra donc
# de connaitre l'ensemble des cibles de type bibliothèque définies
# dans Arcane.
#
# La cible est par défaut ajoutée à la cible interface 'arcane_full'.
# sauf si un l'argument OPTIONAL est spécifié. Par exemple:
#   arcane_register_library(toto)
#   arcane_register_library(toto OPTIONAL)
macro(arcane_register_library lib_name)
  install(TARGETS ${lib_name} EXPORT ArcaneTargets DESTINATION lib)
  set(_IS_FULL TRUE)
  if(${ARGC} GREATER 1)
    if ((${ARGV1} STREQUAL "OPTIONAL"))
      set(_IS_FULL FALSE)
    endif()
  endif()
  if (_IS_FULL)
    target_link_libraries(arcane_full INTERFACE ${lib_name})
  endif()
  set(ARCANE_LIBRARIES ${lib_name} ${ARCANE_LIBRARIES} PARENT_SCOPE)
endmacro()

# ----------------------------------------------------------------------------
# Fonction pour créér l'exécutable de test d'un composant 'Arcane'
#
# arcane_add_component_test_executable(component_name
#   FILES ...
# )
#
# Par exemple:
#
# arcane_add_component_test_executable(utils
#   FILES Test1.cc Test2.cc
# )
#
function(arcane_add_component_test_executable component_name)
  set(options)
  set(oneValueArgs)
  set(multiValueArgs FILES)

  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(_EXE_NAME arcane_${component_name}.tests)
  add_executable(${_EXE_NAME} ${ARGS_FILES})

  # Génère les exécutables dans le répertoire 'lib' du projet.
  # Cela permet sous windows de trouver automatiquement les dll des composantes

  set(_exepath ${CMAKE_BINARY_DIR}/lib)
  if (WIN32)
    set_target_properties(${_EXE_NAME}
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_DEBUG ${_exepath}
      RUNTIME_OUTPUT_DIRECTORY_RELEASE ${_exepath}
      RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${_exepath}
      )
  else()
    set_target_properties(${_EXE_NAME}
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${_exepath}
      )
  endif()

endfunction()

# ----------------------------------------------------------------------------
# Local Variables:
# tab-width: 2
# indent-tabs-mode: nil
# coding: utf-8-with-signature
# End:
