cmake_minimum_required(VERSION 3.1)

project(OsteotomyPlanner)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://github.com/KitwareMedical/OsteotomyPlanner/wiki")
set(EXTENSION_CATEGORY "Osteotomy Planning")
set(EXTENSION_CONTRIBUTORS "Sam Horvath (Kitware Inc.), Johan Andruejol (Kitware Inc.)")
set(EXTENSION_DESCRIPTION "Osteotomy Planner is an extension designed to help the planning of bone surgeries.")
set(EXTENSION_ICONURL "https://raw.githubusercontent.com/KitwareMedical/OsteotomyPlanner/master/Planner/Resources/Icons/Planner.png")
set(EXTENSION_SCREENSHOTURLS "https://raw.githubusercontent.com/KitwareMedical/OsteotomyPlanner/master/Screenshots/Screenshot.jpg")
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
add_subdirectory(SplitModel)
add_subdirectory(Planner)
add_subdirectory(ShrinkWrap)
add_subdirectory(OsteotomyModelToModelDistance)
add_subdirectory(ReplayPlan)

#-----------------------------------------------------------------------------
set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${CMAKE_BINARY_DIR};${EXTENSION_NAME};ALL;/")
#set(CPACK_INSTALL_CMAKE_PROJECTS "${CPACK_INSTALL_CMAKE_PROJECTS};${Foo_DIR};Foo;RuntimeLibraries;/")
include(${Slicer_EXTENSION_CPACK})
