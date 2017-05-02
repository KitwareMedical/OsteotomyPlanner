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

// MRML includes
#include <vtkMRMLMarkupsPlaneNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlannerLogic);

//----------------------------------------------------------------------------
vtkSlicerPlannerLogic::vtkSlicerPlannerLogic()
{
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
vtkMRMLNode* vtkSlicerPlannerLogic::GetSelectionNode()
{
  if (!this->GetMRMLScene())
    {
    return NULL;
    }

  // try the application logic first
  vtkMRMLApplicationLogic *mrmlAppLogic = this->GetMRMLApplicationLogic();
  return mrmlAppLogic ?
    (mrmlAppLogic->GetSelectionNode() ? mrmlAppLogic->GetSelectionNode() : NULL) : NULL;
}

//---------------------------------------------------------------------------
void vtkSlicerPlannerLogic::ObserveMRMLScene()
{
  if (!this->GetMRMLScene())
    {
    return;
    }
  // add known markup types to the selection node
  vtkMRMLSelectionNode *selectionNode =
    vtkMRMLSelectionNode::SafeDownCast(this->GetSelectionNode());
  if (selectionNode)
    {
    // got into batch process mode so that an update on the mouse mode tool
    // bar is triggered when leave it
    this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);

    selectionNode->AddNewPlaceNodeClassNameToList(
      "vtkMRMLMarkupsPlaneNode", ":/Icons/MarkupsPlaneMouseModePlace.png", "Plane");

    // trigger an upate on the mouse mode toolbar
    this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);
    }

 this->Superclass::ObserveMRMLScene();
}

//-----------------------------------------------------------------------------
void vtkSlicerPlannerLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
  vtkMRMLScene *scene = this->GetMRMLScene();

  // Nodes
  vtkNew<vtkMRMLMarkupsPlaneNode> planes;
  scene->RegisterNodeClass(planes.GetPointer());
}

//---------------------------------------------------------------------------
void vtkSlicerPlannerLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}
