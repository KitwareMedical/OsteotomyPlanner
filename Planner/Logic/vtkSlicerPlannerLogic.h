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

// .NAME vtkSlicerPlannerLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerPlannerLogic_h
#define __vtkSlicerPlannerLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"
#include "vtkMRMLModelNode.h"

// MRML includes

// STD includes
#include <cstdlib>
#include <vector>
#include <map>

#include "vtkSlicerPlannerModuleLogicExport.h"
#include <vtkSlicerCLIModuleLogic.h>



/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_PLANNER_MODULE_LOGIC_EXPORT vtkSlicerPlannerLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerPlannerLogic *New();
  vtkTypeMacro(vtkSlicerPlannerLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  static const char* DeleteChildrenWarningSettingName();

  // Delete all the children of the given hierarchy node.
  bool DeleteHierarchyChildren(vtkMRMLNode* node);

  //void setSplitLogic(vtkSlicerCLIModuleLogic* logic);
  void setWrapperLogic(vtkSlicerCLIModuleLogic* logic);
  void setMergeLogic(vtkSlicerCLIModuleLogic* logic);
  std::map<std::string, double> computeBoneAreas(vtkMRMLModelHierarchyNode* HierarchyNode);
  void createPreOPModels(vtkMRMLModelHierarchyNode* HierarchyNode);
  void createHealthyBrainModel(vtkMRMLModelNode* brain);
  double getPreOPICV();
  double getHealthyBrainICV();
  double getCurrentICV(vtkMRMLModelHierarchyNode* HierarchyNode);


protected:
  vtkSlicerPlannerLogic();
  virtual ~vtkSlicerPlannerLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  virtual void UpdateFromMRMLScene();

private:
  
  vtkSlicerCLIModuleLogic* splitLogic;
  vtkSlicerCLIModuleLogic* wrapperLogic;
  vtkSlicerCLIModuleLogic* mergeLogic;
  vtkSlicerPlannerLogic(const vtkSlicerPlannerLogic&); // Not implemented
  void operator=(const vtkSlicerPlannerLogic&); // Not implemented
  vtkMRMLModelNode* wrapModel(vtkMRMLModelNode* model, std::string Name);
  vtkMRMLModelNode* mergeModel(vtkMRMLModelHierarchyNode* HierarchyNode, std::string name);
  double computeICV(vtkMRMLModelNode* model);
  vtkMRMLModelNode* SkullBonesPreOP;
  vtkMRMLModelNode* SkullWrappedPreOP;
  vtkMRMLModelNode* HealthyBrain;
  double preOPICV;
  double healthyBrainICV;
  void hardenTransforms(vtkMRMLModelHierarchyNode* HierarchyNode);
};

#endif
