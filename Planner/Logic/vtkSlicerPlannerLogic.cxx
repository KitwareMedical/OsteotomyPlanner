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

// Planner Logic includes
#include "vtkSlicerPlannerLogic.h"

// Slicer CLI includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include "qSlicerAbstractCoreModule.h"
#include <qSlicerCLIModule.h>
#include <vtkSlicerCLIModuleLogic.h>

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLModelHierarchyNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlannerLogic);

//----------------------------------------------------------------------------
vtkSlicerPlannerLogic::vtkSlicerPlannerLogic()
{
  this->SkullBonesReference = NULL;
  this->SkullWrappedReference = NULL;
  this->splitLogic = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerPlannerLogic::~vtkSlicerPlannerLogic()
{
}

//-----------------------------------------------------------------------------
const char* vtkSlicerPlannerLogic::DeleteChildrenWarningSettingName()
{
  return "Planner/DeleteChildrenWarning";
}

//----------------------------------------------------------------------------
bool vtkSlicerPlannerLogic::DeleteHierarchyChildren(vtkMRMLNode *node)
{
  vtkMRMLHierarchyNode* hNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (!hNode)
    {
    vtkErrorMacro("DeleteHierarchyChildren: Not a hierarchy node.");
    return false;
    }
  if (!this->GetMRMLScene())
    {
    vtkErrorMacro("DeleteHierarchyChildren: No scene defined on this class");
    return false;
    }

  // first off, set up batch processing mode on the scene
  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);

  // get all the children nodes
  std::vector< vtkMRMLHierarchyNode *> allChildren;
  hNode->GetAllChildrenNodes(allChildren);

  // and loop over them
  for (unsigned int i = 0; i < allChildren.size(); ++i)
    {
    vtkMRMLNode *associatedNode = allChildren[i]->GetAssociatedNode();
    if (associatedNode)
      {
      this->GetMRMLScene()->RemoveNode(associatedNode);
      }
    this->GetMRMLScene()->RemoveNode(allChildren[i]);
    }
  // end batch processing
  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);

  return true;
}

//----------------------------------------------------------------------------
void vtkSlicerPlannerLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerPlannerLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  Superclass::SetMRMLSceneInternal(newScene);
}

//---------------------------------------------------------------------------
void vtkSlicerPlannerLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}
/**
void vtkSlicerPlannerLogic::setSplitLogic(vtkSlicerCLIModuleLogic* logic)
{
  this->splitLogic = logic;
}
**/
void vtkSlicerPlannerLogic::setWrapperLogic(vtkSlicerCLIModuleLogic* logic)
{
  this->wrapperLogic = logic;
}



std::map<std::string, double> vtkSlicerPlannerLogic::computeBoneAreas(vtkMRMLModelHierarchyNode* hierarchy)
{
  std::cout << "Computing Bone stuff" << std::endl;
  std::map<std::string, double> surfaceAreas;
  
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  std::vector<vtkMRMLModelNode*> models;

  vtkNew<vtkMassProperties> areaFilter;
  vtkNew<vtkTriangleFilter> triFilter;

  hierarchy->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());
    
    if (childModel)
    {
      triFilter->SetInputData(childModel->GetPolyData());
      triFilter->Update();
      areaFilter->SetInputData(triFilter->GetOutput());
      areaFilter->Update();
      surfaceAreas[childModel->GetName()] = (areaFilter->GetSurfaceArea()) / 200;  // 2 to get one side of bone.  100 to convert to cm^2
    }
  }
  std::cout << "Computing Bone stuff - finished" << std::endl;
  return surfaceAreas;
}


void vtkSlicerPlannerLogic::createReferenceModels()
{
  int i = 1;
}
