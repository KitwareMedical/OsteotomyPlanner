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

#include "ShrinkWrapCLP.h"

// VTK includes
#include <vtkAppendPolyData.h>
#include <vtkClipPolyData.h>
#include <vtkDebugLeaks.h>
#include <vtkExtractSurface.h>
#include <vtkFeatureEdges.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPlaneCollection.h>
#include <vtkPolyDataNormals.h>
#include <vtkPolyDataReader.h>
#include <vtkPolyDataWriter.h>
#include <vtkStripper.h>
#include <vtkTriangleFilter.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLPolyDataWriter.h>
#include <vtkVersion.h>
#include <vtksys/SystemTools.hxx>
#include <vtkFillHolesFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkPointData.h>

#include <vtkSphereSource.h>
#include <vtkSmoothPolyDataFilter.h>


int main( int argc, char * argv[] )
{
  PARSE_ARGS;
  vtkSmartPointer<vtkSphereSource> sphereSource =
    vtkSmartPointer<vtkSphereSource>::New();
  sphereSource->SetRadius(350);
  sphereSource->SetPhiResolution(phires);
  sphereSource->SetThetaResolution(thetares);
  sphereSource->Update();

  

  
    vtkSmartPointer<vtkXMLPolyDataReader> reader =
      vtkSmartPointer<vtkXMLPolyDataReader>::New();
    reader->SetFileName(inputModel.c_str());
    reader->Update();
    vtkSmartPointer<vtkPolyData> polydata = reader->GetOutput();
  

  

  vtkSmartPointer<vtkSmoothPolyDataFilter> smoothFilter =
    vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
  smoothFilter->SetInputConnection(0, sphereSource->GetOutputPort());
  smoothFilter->SetInputConnection(1, reader->GetOutputPort());
  smoothFilter->Update();

  vtkSmartPointer<vtkXMLPolyDataWriter> writer =
    vtkSmartPointer<vtkXMLPolyDataWriter>::New();
  writer->SetFileName(outputModel.c_str());
  writer->SetInputConnection(smoothFilter->GetOutputPort());
  writer->Write();

  return EXIT_SUCCESS;
  
}
