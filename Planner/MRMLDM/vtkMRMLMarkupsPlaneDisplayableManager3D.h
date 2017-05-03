/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __vtkMRMLMarkupsPlaneDisplayableManager3D_h
#define __vtkMRMLMarkupsPlaneDisplayableManager3D_h

// MarkupsModule includes
#include "vtkSlicerPlannerModuleMRMLDisplayableManagerExport.h"

// MarkupsModule/MRMLDisplayableManager includes
#include <vtkMRMLAbstractThreeDViewDisplayableManager.h>

class vtkMRMLMarkupsPlaneNode;
class vtkSlicerViewerWidget;
class vtkMRMLMarkupsDisplayNode;

/// \ingroup Slicer_QtModules_Markups
class VTK_SLICER_PLANNER_MODULE_MRMLDISPLAYABLEMANAGER_EXPORT vtkMRMLMarkupsPlaneDisplayableManager3D :
    public vtkMRMLAbstractThreeDViewDisplayableManager
{
public:

  static vtkMRMLMarkupsPlaneDisplayableManager3D *New();
  vtkTypeMacro(
    vtkMRMLMarkupsPlaneDisplayableManager3D,
    vtkMRMLAbstractThreeDViewDisplayableManager);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMRMLMarkupsPlaneDisplayableManager3D();
  virtual ~vtkMRMLMarkupsPlaneDisplayableManager3D();

  virtual void UnobserveMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void ProcessMRMLNodesEvents(
    vtkObject* caller, unsigned long event, void* callData);

  virtual void SetMRMLSceneInternal(vtkMRMLScene* scene);
  virtual void OnInteractorStyleEvent(int eventid);

  virtual void SetRenderer(vtkRenderer* newRenderer);

  /// Update Actors based on transforms in the scene
  virtual void UpdateFromMRML();

  virtual void OnMRMLSceneStartClose();
  virtual void OnMRMLSceneEndClose();

  virtual void OnMRMLSceneEndBatchProcess();

  /// Initialize the displayable manager
  virtual void Create();

  virtual void ProcessWidgetsEvents(
    vtkObject* caller, unsigned long event, void* callData);

private:
  vtkMRMLMarkupsPlaneDisplayableManager3D(
    const vtkMRMLMarkupsPlaneDisplayableManager3D&); // Not implemented
  void operator=(const vtkMRMLMarkupsPlaneDisplayableManager3D&); // Not Implemented

  class vtkInternal;
  vtkInternal* Internal;
  friend class vtkInternal;
};

#endif
