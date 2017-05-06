/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $HeadURL: http://svn.slicer.org/Slicer4/trunk/Modules/CLI/OrientImage.cxx $
  Language:  C++
  Date:      $Date: 2007-12-20 18:30:38 -0500 (Thu, 20 Dec 2007) $
  Version:   $Revision: 5310 $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#if defined(_MSC_VER)
#pragma warning ( disable : 4786 )
#endif

#include "SplitModelCLP.h"

// VTK includes
#include <vtkClipPolyData.h>
#include <vtkDebugLeaks.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkVersion.h>
#include <vtksys/SystemTools.hxx>

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  if (Normal.size() != 3 || Origin.size() != 3)
    {
    std::cerr << "Normal and Origin must be of size 3" << std::endl;
    return EXIT_FAILURE;
    }

  vtkDebugLeaks::SetExitError(true);

  vtkSmartPointer<vtkPolyData> polydata;

  // do we have vtk or vtp models?
  std::string extension = vtksys::SystemTools::LowerCase(
    vtksys::SystemTools::GetFilenameLastExtension(Model) );
  if( extension.empty() )
    {
    std::cerr << "Failed to find an extension for " << Model << std::endl;
    return EXIT_FAILURE;
    }

  // read the first poly data
  if( extension == std::string(".vtk") )
    {
    vtkNew<vtkPolyDataReader> reader;
    reader->SetFileName(Model.c_str() );
    reader->Update();
    polydata = reader->GetOutput();
    }
  else if( extension == std::string(".vtp") )
    {
    vtkNew<vtkXMLPolyDataReader> reader;
    reader->SetFileName(Model.c_str() );
    reader->Update();
    polydata = reader->GetOutput();
    }

  // Create clip
  vtkNew<vtkClipPolyData> clip;
  clip->SetValue(0);
  clip->GenerateClippedOutputOn();
  clip->SetInputData(polydata);

  // Create plane
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(Origin[0], Origin[1], Origin[2]);
  plane->SetNormal(Normal[0], Normal[1], Normal[2]);
  clip->SetClipFunction(plane.GetPointer());

  // write the first part
  extension = vtksys::SystemTools::LowerCase( vtksys::SystemTools::GetFilenameLastExtension(ModelOutput1) );
  if( extension.empty() )
    {
    std::cerr << "Failed to find an extension for " << ModelOutput1 << std::endl;
    return EXIT_FAILURE;
    }
  if( extension == std::string(".vtk") )
    {
    vtkNew<vtkPolyDataWriter> writer;
    writer->SetFileName(ModelOutput1.c_str() );
    writer->SetInputConnection(clip->GetOutputPort());
    writer->Write();
    }
  else if( extension == std::string(".vtp") )
    {
    vtkNew<vtkXMLPolyDataWriter> writer;
    writer->SetIdTypeToInt32();
    writer->SetFileName(ModelOutput1.c_str() );
    writer->SetInputConnection(clip->GetOutputPort());
    writer->Write();
    }

  // Negate plane normal and write second part
  plane->SetNormal(-Normal[0], -Normal[1], -Normal[2]);
  clip->SetClipFunction(plane.GetPointer());

  extension = vtksys::SystemTools::LowerCase( vtksys::SystemTools::GetFilenameLastExtension(ModelOutput2) );
  if( extension.empty() )
    {
    std::cerr << "Failed to find an extension for " << ModelOutput2 << std::endl;
    return EXIT_FAILURE;
    }
  if( extension == std::string(".vtk") )
    {
    vtkNew<vtkPolyDataWriter> writer;
    writer->SetFileName(ModelOutput2.c_str() );
    writer->SetInputConnection(clip->GetOutputPort());
    writer->Write();
    }
  else if( extension == std::string(".vtp") )
    {
    vtkNew<vtkXMLPolyDataWriter> writer;
    writer->SetIdTypeToInt32();
    writer->SetFileName(ModelOutput2.c_str() );
    writer->SetInputConnection(clip->GetOutputPort());
    writer->Write();
    }

  return EXIT_SUCCESS;
}
