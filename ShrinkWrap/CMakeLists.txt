
#-----------------------------------------------------------------------------
set(MODULE_NAME ShrinkWrap)

#-----------------------------------------------------------------------------

#
# VTK
#
find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

#
# SlicerExecutionModel
#
find_package(SlicerExecutionModel REQUIRED)
include(${SlicerExecutionModel_USE_FILE})

#-----------------------------------------------------------------------------
SEMMacroBuildCLI(
  NAME ${MODULE_NAME}
  TARGET_LIBRARIES ${VTK_LIBRARIES}
  INCLUDE_DIRECTORIES
    ${vtkITK_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIRS}
  )

#-----------------------------------------------------------------------------
if(BUILD_TESTING)
  #add_subdirectory(Testing)
endif()
