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
#include <vtkMRMLModelStorageNode.h>
#include <vtkMRMLModelDisplayNode.h>
#include <vtkMRMLTransformNode.h>

#include <QProgressBar.h>

// VTK includes
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
#include <vtkAppendPolyData.h>
#include <vtkCommand.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlannerLogic);

//----------------------------------------------------------------------------
vtkSlicerPlannerLogic::vtkSlicerPlannerLogic()
{
  this->SkullBonesPreOP = NULL;
  this->SkullWrappedPreOP = NULL;
  this->HealthyBrain = NULL;
  this->splitLogic = NULL;
  this->wrapperLogic = NULL;
  this->mergeLogic = NULL;
  this->preOPICV = 0;
  this->healthyBrainICV = 0;
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

void vtkSlicerPlannerLogic::setMergeLogic(vtkSlicerCLIModuleLogic* logic)
{
  this->mergeLogic = logic;
}



std::map<std::string, double> vtkSlicerPlannerLogic::computeBoneAreas(vtkMRMLModelHierarchyNode* hierarchy)
{
  std::cout << "Computing Bone stuff" << std::endl;
  std::map<std::string, double> surfaceAreas;
  
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;

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


void vtkSlicerPlannerLogic::createPreOPModels(vtkMRMLModelHierarchyNode* HierarchyNode)
{
  if (this->SkullBonesPreOP)
  {
    this->GetMRMLScene()->RemoveNode(this->SkullBonesPreOP);
    this->SkullBonesPreOP = NULL;
  }

  if (this->SkullWrappedPreOP)
  {
    this->GetMRMLScene()->RemoveNode(this->SkullWrappedPreOP);
    this->SkullWrappedPreOP = NULL;
  }
  
  std::cout << "creating new reference models" << std::endl;
  std::string name;
  name = HierarchyNode->GetName();
  name += " - Merged";
  vtkMRMLModelNode* mergedModel = this->mergeModel(HierarchyNode, name);
  mergedModel->GetDisplayNode()->SetVisibility(0);
  this->SkullBonesPreOP = mergedModel;
  name = HierarchyNode->GetName();
  name += " - Wrapped";
  vtkMRMLModelNode* wrappedModel = this->wrapModel(mergedModel, name);
  wrappedModel->GetDisplayNode()->SetVisibility(0);
  this->SkullWrappedPreOP = wrappedModel;
  this->preOPICV = this->computeICV(this->SkullWrappedPreOP);

  
}

vtkMRMLModelNode* vtkSlicerPlannerLogic::wrapModel(vtkMRMLModelNode* model, std::string name)
{
  vtkNew<vtkMRMLModelNode> wrappedModel;
  vtkMRMLScene* scene = this->GetMRMLScene();
  wrappedModel->SetScene(scene);
  wrappedModel->SetName(name.c_str());
  vtkNew<vtkMRMLModelDisplayNode> dnode;
  vtkNew<vtkMRMLModelStorageNode> snode;
  wrappedModel->SetAndObserveDisplayNodeID(dnode->GetID());
  wrappedModel->SetAndObserveStorageNodeID(snode->GetID());
  scene->AddNode(dnode.GetPointer());
  scene->AddNode(snode.GetPointer());
  scene->AddNode(wrappedModel.GetPointer());

  //CLI setup
  this->wrapperLogic->SetMRMLScene(this->GetMRMLScene());
  vtkMRMLCommandLineModuleNode* cmdNode = this->wrapperLogic->CreateNodeInScene();
  cmdNode->SetParameterAsString("inputModel", model->GetID());
  cmdNode->SetParameterAsString("outputModel", wrappedModel->GetID());
  cmdNode->SetParameterAsString("PhiRes", "20");
  cmdNode->SetParameterAsString("ThetaRes", "20");
  this->wrapperLogic->ApplyAndWait(cmdNode, true);
  scene->RemoveNode(cmdNode);
  return wrappedModel.GetPointer();
  
}
vtkMRMLModelNode* vtkSlicerPlannerLogic::mergeModel(vtkMRMLModelHierarchyNode* HierarchyNode, std::string name)
{
  
  vtkNew<vtkMRMLModelNode> mergedModel;
  vtkNew<vtkAppendPolyData> filter;
  vtkMRMLScene* scene = this->GetMRMLScene();
  mergedModel->SetScene(scene);
  mergedModel->SetName(name.c_str());
  vtkNew<vtkMRMLModelDisplayNode> dnode;
  vtkNew<vtkMRMLModelStorageNode> snode;
  mergedModel->SetAndObserveDisplayNodeID(dnode->GetID());
  mergedModel->SetAndObserveStorageNodeID(snode->GetID());
  scene->AddNode(dnode.GetPointer());
  scene->AddNode(snode.GetPointer());
  scene->AddNode(mergedModel.GetPointer());

  
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  HierarchyNode->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if (childModel)
    {
      filter->AddInputData(childModel->GetPolyData());
      
    }
  }

  filter->Update();
  mergedModel->SetAndObservePolyData(filter->GetOutput());
  mergedModel->SetAndObserveDisplayNodeID(dnode->GetID());

  return mergedModel.GetPointer();
}

double vtkSlicerPlannerLogic::computeICV(vtkMRMLModelNode* model)
{
  vtkNew<vtkMassProperties> areaFilter;
  vtkNew<vtkTriangleFilter> triFilter;
  triFilter->SetInputData(model->GetPolyData());
  triFilter->Update();
  areaFilter->SetInputData(triFilter->GetOutput());
  areaFilter->Update();
  return (areaFilter->GetVolume() / 1000 );  //convert to cm^3
}

double  vtkSlicerPlannerLogic::getPreOPICV()
{
  return this->preOPICV;
}

double  vtkSlicerPlannerLogic::getCurrentICV(vtkMRMLModelHierarchyNode* HierarchyNode)
{
  std::cout << "Computing ICV" << std::endl;
  //this->hardenTransforms(HierarchyNode);
  std::string name;
  name = HierarchyNode->GetName();
  name += " - Temp Merge";
  vtkMRMLModelNode* mergedModel = this->mergeModel(HierarchyNode, name);
  name = HierarchyNode->GetName();
  name += " - Temp Wrapped";
  vtkMRMLModelNode* wrappedModel = this->wrapModel(mergedModel, name);
  wrappedModel->GetDisplayNode()->SetVisibility(0);
  mergedModel->GetDisplayNode()->SetVisibility(0);
  double volume = this->computeICV(wrappedModel);
  this->GetMRMLScene()->RemoveNode(mergedModel);
  mergedModel = NULL;
  this->GetMRMLScene()->RemoveNode(wrappedModel);
  wrappedModel = NULL;

  return volume;
}

void vtkSlicerPlannerLogic::hardenTransforms(vtkMRMLModelHierarchyNode* hierarchy)
{
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  hierarchy->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if (childModel)
    {
      int m = childModel->StartModify();
      childModel->HardenTransform();
      vtkNew<vtkMRMLTransformNode> newTransform;
      vtkMRMLTransformNode* transform = vtkMRMLTransformNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(childModel->GetTransformNodeID()));
      childModel->EndModify(m);
    }
  }
}

void vtkSlicerPlannerLogic::setProgressBar(QProgressBar* bar)
{
  this->progressBar = bar;
}


void vtkSlicerPlannerLogic::createHealthyBrainModel(vtkMRMLModelNode* model)
{
  if (this->HealthyBrain)
  {
    this->GetMRMLScene()->RemoveNode(this->HealthyBrain);
    this->HealthyBrain = NULL;
  }

  std::string name;
  name = model->GetName();
  name += " - Wrapped";
  vtkMRMLModelNode* wrappedModel = this->wrapModel(model, name);
  wrappedModel->GetDisplayNode()->SetVisibility(0);
  this->HealthyBrain = wrappedModel;
  this->healthyBrainICV = this->computeICV(this->HealthyBrain);
}

double vtkSlicerPlannerLogic::getHealthyBrainICV()
{
  return this->healthyBrainICV;
}
