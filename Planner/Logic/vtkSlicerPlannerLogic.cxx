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

// VTK includes
#include <vtkNew.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
#include <vtkAppendPolyData.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlannerLogic);

//----------------------------------------------------------------------------
//Constructor
vtkSlicerPlannerLogic::vtkSlicerPlannerLogic()
{
  this->SkullWrappedPreOP = NULL;
  this->HealthyBrain = NULL;
  this->splitLogic = NULL;
  this->wrapperLogic = NULL;
  this->preOPICV = 0;
  this->healthyBrainICV = 0;
  this->currentICV = 0;
  this->TempMerged = NULL;
  this->TempWrapped = NULL;
  this->CurrentModel = NULL;
}

//----------------------------------------------------------------------------
//Destructor
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

//----------------------------------------------------------------------------
//Set logic for Shrink Wrap CLI
void vtkSlicerPlannerLogic::setWrapperLogic(vtkSlicerCLIModuleLogic* logic)
{
  this->wrapperLogic = logic;
}

//----------------------------------------------------------------------------
//Create reference model form current hierarhcy state
vtkMRMLCommandLineModuleNode* vtkSlicerPlannerLogic::createPreOPModels(vtkMRMLModelHierarchyNode* HierarchyNode)
{
  if (this->SkullWrappedPreOP)
  {
    this->GetMRMLScene()->RemoveNode(this->SkullWrappedPreOP);
    this->SkullWrappedPreOP = NULL;
  }
  
  std::string name;
  name = HierarchyNode->GetName();
  name += " - Merged";
  this->TempMerged = this->mergeModel(HierarchyNode, name);
  this->TempMerged->GetDisplayNode()->SetVisibility(0);
  name = HierarchyNode->GetName();
  name += " - Wrapped";
  return this->wrapModel(this->TempMerged, name, vtkSlicerPlannerLogic::PreOP);
}

//----------------------------------------------------------------------------
//Get the pre-op ICV
double vtkSlicerPlannerLogic::getPreOPICV()
{
  if (this->SkullWrappedPreOP)
  {
    this->preOPICV = this->computeICV(this->SkullWrappedPreOP);
  }
  
  return this->preOPICV;
}

//----------------------------------------------------------------------------
//Create wrapped model from current hierarchy
vtkMRMLCommandLineModuleNode*  vtkSlicerPlannerLogic::createCurrentModel(vtkMRMLModelHierarchyNode* HierarchyNode)
{
  if (this->CurrentModel)
  {
    this->GetMRMLScene()->RemoveNode(this->CurrentModel);
    this->CurrentModel = NULL;
  }
  std::string name;
  name = HierarchyNode->GetName();
  name += " - Temp Merge";
  this->TempMerged = this->mergeModel(HierarchyNode, name);
  this->TempMerged->GetDisplayNode()->SetVisibility(0);
  name = HierarchyNode->GetName();
  name += " - Current Wrapped";
  return this->wrapModel(this->TempMerged, name, vtkSlicerPlannerLogic::Current);
}

//----------------------------------------------------------------------------
//Get the current ICV
double vtkSlicerPlannerLogic::getCurrentICV()
{
  if (this->CurrentModel)
  {
    this->currentICV = this->computeICV(this->CurrentModel);
  }
  return this->currentICV;
}

//----------------------------------------------------------------------------
//Create wrapped version of brain model input
vtkMRMLCommandLineModuleNode* vtkSlicerPlannerLogic::createHealthyBrainModel(vtkMRMLModelNode* model)
{
  if (this->HealthyBrain)
  {
    this->GetMRMLScene()->RemoveNode(this->HealthyBrain);
    this->HealthyBrain = NULL;
  }

  std::string name;
  name = model->GetName();
  name += " - Wrapped";
  return wrapModel(model, name, vtkSlicerPlannerLogic::Brain);
}

//----------------------------------------------------------------------------
//Get brain ICV
double vtkSlicerPlannerLogic::getHealthyBrainICV()
{
  if (this->HealthyBrain)
  {
    this->healthyBrainICV = this->computeICV(this->HealthyBrain);
  }
  return this->healthyBrainICV;
}

//----------------------------------------------------------------------------
//Merge hierarchy into a single model
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

//----------------------------------------------------------------------------
//Compute the ICV of a model
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

//----------------------------------------------------------------------------
//Create shrink wrapped version of a model
vtkMRMLCommandLineModuleNode* vtkSlicerPlannerLogic::wrapModel(vtkMRMLModelNode* model, std::string name, int dest)
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
  
  switch (dest)
  {
    case vtkSlicerPlannerLogic::Current:
      this->CurrentModel = wrappedModel.GetPointer();
      break;
    case vtkSlicerPlannerLogic::PreOP:
      this->SkullWrappedPreOP = wrappedModel.GetPointer();
      break;
    case vtkSlicerPlannerLogic::Brain:
      this->HealthyBrain = wrappedModel.GetPointer();
      break;
    
  }

  //CLI setup
  this->wrapperLogic->SetMRMLScene(this->GetMRMLScene());
  vtkMRMLCommandLineModuleNode* cmdNode = this->wrapperLogic->CreateNodeInScene();
  cmdNode->SetParameterAsString("inputModel", model->GetID());
  cmdNode->SetParameterAsString("outputModel", wrappedModel->GetID());
  cmdNode->SetParameterAsString("PhiRes", "20");
  cmdNode->SetParameterAsString("ThetaRes", "20");
  this->wrapperLogic->Apply(cmdNode, true);
  return cmdNode;
}

//----------------------------------------------------------------------------
//Finish up wrapper CLI
void vtkSlicerPlannerLogic::finishWrap(vtkMRMLCommandLineModuleNode* cmdNode)
{
  vtkMRMLModelNode* node = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(cmdNode->GetParameterAsString("outputModel")));
  node->GetDisplayNode()->SetVisibility(0);
  this->GetMRMLScene()->RemoveNode(cmdNode);

  if (this->TempMerged)
  {
    this->GetMRMLScene()->RemoveNode(this->TempMerged);
    this->TempMerged = NULL;
  }

  if (this->TempWrapped)
  {
    this->GetMRMLScene()->RemoveNode(this->TempWrapped);
    this->TempWrapped = NULL;
  }
}
//----------------------------------------------------------------------------
//Fill table node with metrics
void vtkSlicerPlannerLogic::fillMetricsTable(vtkMRMLModelHierarchyNode* HierarchyNode, vtkMRMLTableNode* modelMetricsTable)
{
  double preOpVolume;
  double currentVolume;
  double brainVolume;
  if (HierarchyNode)
  {
    preOpVolume = this->getPreOPICV();
    brainVolume = this->getHealthyBrainICV();
    currentVolume = this->getCurrentICV();

    modelMetricsTable->RemoveAllColumns();
    std::string modelTableName = "Model Metrics - ";
    modelTableName += HierarchyNode->GetName();
    modelMetricsTable->SetName(modelTableName.c_str());

    vtkAbstractArray* col0 = modelMetricsTable->AddColumn();
    vtkAbstractArray* col1 = modelMetricsTable->AddColumn();
    col1->SetName("Healthy Brain");
    vtkAbstractArray* col2 = modelMetricsTable->AddColumn();
    col2->SetName("Pre Op");
    vtkAbstractArray* col3 = modelMetricsTable->AddColumn();
    col3->SetName("Current");
    modelMetricsTable->SetUseColumnNameAsColumnHeader(true);
    modelMetricsTable->SetUseFirstColumnAsRowHeader(true);
    modelMetricsTable->SetLocked(true);

    int r1 = modelMetricsTable->AddEmptyRow();
    modelMetricsTable->SetCellText(0, 0, "ICV\n cm^3");
    modelMetricsTable->SetCellText(0, 1, std::to_string(brainVolume).c_str());
    modelMetricsTable->SetCellText(0, 2, std::to_string(preOpVolume).c_str());
    modelMetricsTable->SetCellText(0, 3, std::to_string(currentVolume).c_str());
  }
}

