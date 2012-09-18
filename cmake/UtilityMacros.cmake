
# author Ivan Kreshchenko
# date 09.06.2011


# modified macros from Delta3D
MACRO(LINK_WITH_VARIABLES TRGTNAME)
  FOREACH(varname ${ARGN})
    IF(${varname}_RELEASE)
      IF(${varname}_DEBUG)
        TARGET_LINK_LIBRARIES(${TRGTNAME} optimized "${${varname}_RELEASE}" debug "${${varname}_DEBUG}")
      ELSE(${varname}_DEBUG)
        TARGET_LINK_LIBRARIES(${TRGTNAME} optimized "${${varname}_RELEASE}" debug "${${varname}_RELEASE}" )
      ENDIF(${varname}_DEBUG)
    ELSE(${varname}_RELEASE)
      IF(${varname}_DEBUG)
        TARGET_LINK_LIBRARIES(${TRGTNAME} optimized "${${varname}}" debug "${${varname}_DEBUG}")
      ELSE(${varname}_DEBUG)
        TARGET_LINK_LIBRARIES(${TRGTNAME} optimized "${${varname}}" debug "${${varname}}" )
      ENDIF(${varname}_DEBUG)
    ENDIF(${varname}_RELEASE)
  ENDFOREACH(varname)
ENDMACRO(LINK_WITH_VARIABLES TRGTNAME)

# see http://www.vtk.org/Wiki/CMake_RPATH_handling
MACRO(ADD_TO_RPATH_IF_NOT_SYSTEM DIRECTORY)
	LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${DIRECTORY}" isSystemDir)
	IF("${isSystemDir}" STREQUAL "-1")
		SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:${DIRECTORY}")
	ENDIF("${isSystemDir}" STREQUAL "-1")
ENDMACRO(ADD_TO_RPATH_IF_NOT_SYSTEM)
