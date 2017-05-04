cmake_minimum_required(VERSION 3.1)

project(OsteotomyPlanner)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "http://slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/OsteotomyPlanner")
set(EXTENSION_CATEGORY "Planning")
set(EXTENSION_CONTRIBUTORS "Johan Andruejol (Kitware Inc.)")
set(EXTENSION_DESCRIPTION "Osteotomy Planner is an extension design to help the planning of bone surgeries.")
set(EXTENSION_ICONURL "http://www.example.com/Slicer/Extensions/OsteotomyPlanner.png")
set(EXTENSION_SCREENSHOTURLS "http://www.example.com/Slicer/Extensions/OsteotomyPlanner/Screenshots/1.png")
set(EXTENSION_DEPENDS "NA") # Specified as a space separated string, a list or 'NA' if any
set(EXTENSION_BUILD_SUBDIRECTORY inner-build)

set(SUPERBUILD_TOPLEVEL_PROJECT inner)

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})
mark_as_superbuild(Slicer_DIR)

find_package(Git REQUIRED)
mark_as_superbuild(GIT_EXECUTABLE)

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(Planes)
add_subdirectory(Planner)

#-----------------------------------------------------------------------------
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${EXTENSION_NAME};ALL;/")
#set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${Foo_DIR};Foo;RuntimeLibraries;/")
include(${Slicer_EXTENSION_CPACK})
