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
#include <vtkMRMLSubjectHierarchyNode.h>
#include <vtkMRMLModelStorageNode.h>
#include <vtkMRMLModelDisplayNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkMassProperties.h>
#include <vtkTriangleFilter.h>
#include <vtkAppendPolyData.h>
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include "vtkMath.h"
#include "vtkCutter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPolyDataNormals.h"
#include "vtkCleanPolyData.h"
#include "vtkPolyDataPointSampler.h"
#include "vtkDecimatePro.h"
#include "vtkMatrix4x4.h"
#include "vtkVertexGlyphFilter.h"

// STD includes
#include <cassert>
#include <sstream>


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerPlannerLogic);

//----------------------------------------------------------------------------
//Constructor
vtkSlicerPlannerLogic::vtkSlicerPlannerLogic()
{
  this->SkullWrappedPreOP = NULL;
  this->HealthyBrain = NULL;
  this->BoneTemplate = NULL;
  this->splitLogic = NULL;
  this->wrapperLogic = NULL;
  this->preOPICV = 0;
  this->healthyBrainICV = 0;
  this->currentICV = 0;
  this->templateICV = 0;
  this->TempMerged = NULL;
  this->TempWrapped = NULL;
  this->CurrentModel = NULL;
  this->SourcePoints = NULL;
  this->SourcePointsDense = NULL;
  this->TargetPoints = NULL;
  this->Fiducials = NULL;
  this->cellLocator = NULL;
  this->bendMode = Double;
  this->bendSide = A;
  this->BendingPlane = NULL;
  this->BendingPlaneLocator = NULL;
  this->bendInitialized = false;
  this->BendingPolyData = NULL;
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
bool vtkSlicerPlannerLogic::DeleteHierarchyChildren(vtkMRMLNode* node)
{
  vtkMRMLHierarchyNode* hNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if(!hNode)
  {
    vtkErrorMacro("DeleteHierarchyChildren: Not a hierarchy node.");
    return false;
  }
  if(!this->GetMRMLScene())
  {
    vtkErrorMacro("DeleteHierarchyChildren: No scene defined on this class");
    return false;
  }

  // first off, set up batch processing mode on the scene
  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);

  // get all the children nodes
  std::vector< vtkMRMLHierarchyNode*> allChildren;
  hNode->GetAllChildrenNodes(allChildren);

  // and loop over them
  for(unsigned int i = 0; i < allChildren.size(); ++i)
  {
    vtkMRMLNode* associatedNode = allChildren[i]->GetAssociatedNode();
    if(associatedNode)
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
void vtkSlicerPlannerLogic::SetMRMLSceneInternal(vtkMRMLScene* newScene)
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
//Create reference model from current hierarchy state
vtkSmartPointer<vtkMRMLCommandLineModuleNode> vtkSlicerPlannerLogic::createPreOPModels(vtkIdType hierarchyID)
{
  if(this->SkullWrappedPreOP)
  {
    this->GetMRMLScene()->RemoveNode(this->SkullWrappedPreOP);
    this->SkullWrappedPreOP = NULL;
  }

  vtkMRMLScene* scene = this->GetMRMLScene();
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  vtkMRMLNode* hierarchyNode = shNode->GetItemDataNode(hierarchyID);

  std::string name;
  name = hierarchyNode->GetName();
  name += " - Merged";
  this->TempMerged = this->mergeModel(hierarchyID, name);
  this->TempMerged->GetDisplayNode()->SetVisibility(0);
  name = hierarchyNode->GetName();
  name += " - Wrapped";
  return this->wrapModel(this->TempMerged, name, vtkSlicerPlannerLogic::PreOP);
}

//----------------------------------------------------------------------------
//Get the pre-op ICV
double vtkSlicerPlannerLogic::getPreOPICV()
{
  if(this->SkullWrappedPreOP)
  {
    this->preOPICV = this->computeICV(this->SkullWrappedPreOP);
  }

  return this->preOPICV;
}

//----------------------------------------------------------------------------
//Create wrapped model from current hierarchy
vtkSmartPointer<vtkMRMLCommandLineModuleNode>  vtkSlicerPlannerLogic::createCurrentModel(vtkIdType hierarchyID)
{
  if(this->CurrentModel)
  {
    this->GetMRMLScene()->RemoveNode(this->CurrentModel);
    this->CurrentModel = NULL;
  }

  vtkMRMLScene* scene = this->GetMRMLScene();
  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);
  vtkMRMLNode* hierarchyNode = shNode->GetItemDataNode(hierarchyID);

  std::string name;
  name = hierarchyNode->GetName();
  name += " - Temp Merge";
  this->TempMerged = this->mergeModel(hierarchyID, name);
  this->TempMerged->GetDisplayNode()->SetVisibility(0);
  name = hierarchyNode->GetName();
  name += " - Current Wrapped";
  return this->wrapModel(this->TempMerged, name, vtkSlicerPlannerLogic::Current);
}

//----------------------------------------------------------------------------
//Get the current ICV
double vtkSlicerPlannerLogic::getCurrentICV()
{
  if(this->CurrentModel)
  {
    this->currentICV = this->computeICV(this->CurrentModel);
  }
  return this->currentICV;
}

//----------------------------------------------------------------------------
//Create wrapped version of brain model input
vtkSmartPointer<vtkMRMLCommandLineModuleNode> vtkSlicerPlannerLogic::createHealthyBrainModel(vtkMRMLModelNode* model)
{
  if(this->HealthyBrain)
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
  if(this->HealthyBrain)
  {
    this->healthyBrainICV = this->computeICV(this->HealthyBrain);
  }
  return this->healthyBrainICV;
}

//----------------------------------------------------------------------------
//Create wrapped version of bone template input
vtkSmartPointer<vtkMRMLCommandLineModuleNode> vtkSlicerPlannerLogic::createBoneTemplateModel(vtkMRMLModelNode* model)
{
  if (this->BoneTemplate)
  {
    this->GetMRMLScene()->RemoveNode(this->BoneTemplate);
    this->BoneTemplate = NULL;
  }

  std::string name;
  name = model->GetName();
  name += " - Wrapped";
  return wrapModel(model, name, vtkSlicerPlannerLogic::Template);
}

//----------------------------------------------------------------------------
//Get template ICV
double vtkSlicerPlannerLogic::getTemplateICV()
{
  if (this->BoneTemplate)
  {
    this->templateICV = this->computeICV(this->BoneTemplate);
  }
  return this->templateICV;
}

//----------------------------------------------------------------------------
//Merge hierarchy into a single model
vtkMRMLModelNode* vtkSlicerPlannerLogic::mergeModel(vtkIdType hierarchyID, std::string name)
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

  vtkMRMLSubjectHierarchyNode* shNode = vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(scene);

  std::vector<vtkIdType> children;
  std::vector<vtkIdType>::const_iterator it;
  shNode->GetItemChildren(hierarchyID, children, true);
  for(it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast(shNode->GetItemDataNode(*it));

    if(childModel)
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
  return (areaFilter->GetVolume() / 1000);   //convert to cm^3
}

//----------------------------------------------------------------------------
//Create shrink wrapped version of a model
vtkSmartPointer<vtkMRMLCommandLineModuleNode> vtkSlicerPlannerLogic::wrapModel(vtkMRMLModelNode* model, std::string name, int dest)
{
  vtkNew<vtkMRMLModelNode> wrappedModel;
  //wrappedModel->HideFromEditorsOn();
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

  switch(dest)
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
  case vtkSlicerPlannerLogic::Template:
    this->BoneTemplate = wrappedModel.GetPointer();

  }

  //CLI setup
  this->wrapperLogic->SetMRMLScene(this->GetMRMLScene());
  vtkSmartPointer<vtkMRMLCommandLineModuleNode> cmdNode = this->wrapperLogic->CreateNodeInScene();
  cmdNode->SetParameterAsString("inputModel", model->GetID());
  cmdNode->SetParameterAsString("outputModel", wrappedModel->GetID());
  cmdNode->SetParameterAsString("phires", "150");
  cmdNode->SetParameterAsString("thetares", "150");
  this->wrapperLogic->Apply(cmdNode, true);
  return cmdNode;
}

//----------------------------------------------------------------------------
//Finish up wrapper CLI
void vtkSlicerPlannerLogic::finishWrap(vtkMRMLCommandLineModuleNode* cmdNode)
{
  vtkMRMLModelNode* node = vtkMRMLModelNode::SafeDownCast(this->GetMRMLScene()->GetNodeByID(cmdNode->GetParameterAsString("outputModel")));
  node->GetDisplayNode()->SetVisibility(0);
  node->GetDisplayNode()->SetActiveScalarName("Normals");
  this->GetMRMLScene()->RemoveNode(cmdNode);
  node->SetAttribute("PlannerRole", "WrappedModel");


  if(this->TempMerged)
  {
    this->GetMRMLScene()->RemoveNode(this->TempMerged);
    this->TempMerged = NULL;
  }

  if(this->TempWrapped)
  {
    this->GetMRMLScene()->RemoveNode(this->TempWrapped);
    this->TempWrapped = NULL;
  }
}

//----------------------------------------------------------------------------
//Fill table node with metrics
void vtkSlicerPlannerLogic::fillMetricsTable(vtkIdType hierarchyID, vtkMRMLTableNode* modelMetricsTable)
{
  double preOpVolume;
  double currentVolume;
  double brainVolume;
  double templateVolume;
  vtkMRMLSubjectHierarchyNode* shNode =
    vtkMRMLSubjectHierarchyNode::GetSubjectHierarchyNode(this->GetMRMLScene());
  if(vtkMRMLSubjectHierarchyNode::INVALID_ITEM_ID != hierarchyID)
  {
    preOpVolume = this->getPreOPICV();
    currentVolume = this->getCurrentICV();
    templateVolume = this->getTemplateICV();

    modelMetricsTable->RemoveAllColumns();
    std::string modelTableName = "Model Metrics - ";
    modelTableName += shNode->GetItemName(hierarchyID);
    modelMetricsTable->SetName(modelTableName.c_str());

    modelMetricsTable->AddColumn();
    vtkAbstractArray* col2 = modelMetricsTable->AddColumn();
    col2->SetName("Bone Template");
    vtkAbstractArray* col3 = modelMetricsTable->AddColumn();
    col3->SetName("Initial State");
    vtkAbstractArray* col4 = modelMetricsTable->AddColumn();
    col4->SetName("Current");
    modelMetricsTable->SetUseColumnNameAsColumnHeader(true);
    modelMetricsTable->SetUseFirstColumnAsRowHeader(true);
    modelMetricsTable->SetLocked(true);

    modelMetricsTable->AddEmptyRow();
    modelMetricsTable->SetCellText(0, 0, "ICV\n cm^3");
    
   
    std::stringstream templateVolumeSstr;
    templateVolumeSstr << templateVolume;
    const std::string& templateVolumeString = templateVolumeSstr.str();
    modelMetricsTable->SetCellText(0, 1, templateVolumeString.c_str());
    
    std::stringstream preOpVolumeSstr;
    preOpVolumeSstr << preOpVolume;
    const std::string& preOpVolumeString = preOpVolumeSstr.str();
    modelMetricsTable->SetCellText(0, 2, preOpVolumeString.c_str());
    
    std::stringstream currentVolumeSstr;
    currentVolumeSstr << currentVolume;
    const std::string& currentVolumeString = currentVolumeSstr.str();
    modelMetricsTable->SetCellText(0, 3, currentVolumeString.c_str());
    
  }
}

//----------------------------------------------------------------------------
//Initiaize bending
void vtkSlicerPlannerLogic::initializeBend(vtkPoints* inputFiducials, vtkMRMLModelNode* model)
{
  this->Fiducials = inputFiducials;
  this->ModelToBend = model;

  vtkNew<vtkTriangleFilter> triangulate;
  vtkNew<vtkCleanPolyData> clean;
  vtkNew<vtkPolyDataNormals> normals;
  normals->SetComputePointNormals(1);
  normals->SetComputeCellNormals(1);
  normals->SetAutoOrientNormals(1);
  normals->SetInputData(this->ModelToBend->GetPolyData());
  normals->Update();
  clean->SetInputData(normals->GetOutput());
  clean->Update();
  this->BendingPolyData = clean->GetOutput();

  this->cellLocator = vtkSmartPointer<vtkCellLocator>::New();
  this->cellLocator->SetDataSet(this->BendingPolyData);
  this->cellLocator->BuildLocator();

  this->generateSourcePoints();
  this->bendInitialized =  true;
}

//----------------------------------------------------------------------------
//CReate bend transform based on points and bend magnitude
vtkSmartPointer<vtkThinPlateSplineTransform> vtkSlicerPlannerLogic::getBendTransform(double magnitude)
{
  vtkSmartPointer<vtkThinPlateSplineTransform> transform = vtkSmartPointer<vtkThinPlateSplineTransform>::New();
  if(this->bendInitialized)
  {

    this->TargetPoints = vtkSmartPointer<vtkPoints>::New();
    for(int i = 0; i < this->SourcePointsDense->GetNumberOfPoints(); i++)
    {
      double p[3];
      this->SourcePointsDense->GetPoint(i, p);
      vtkVector3d point = (vtkVector3d)p;
      vtkVector3d bent = point;
      if(this->bendMode == Double)
      {
        bent = this->bendPoint(point, magnitude);
      }
      if(this->bendMode == Single)
      {
        if(this->bendSide == A)
        {
          if(this->BendingPlane->EvaluateFunction(point.GetData())*this->BendingPlane->EvaluateFunction(this->SourcePoints->GetPoint(0)) > 0)
          {
            bent = this->bendPoint(point, magnitude);
          }
        }
        if(this->bendSide == B)
        {
          if(this->BendingPlane->EvaluateFunction(point.GetData())*this->BendingPlane->EvaluateFunction(this->SourcePoints->GetPoint(1)) > 0)
          {
            bent = this->bendPoint(point, magnitude);
          }
        }

      }
      this->TargetPoints->InsertPoint(i, bent.GetData());
    }

    transform->SetSigma(.0001);
    transform->SetBasisToR();
    transform->SetSourceLandmarks(this->SourcePointsDense);
    transform->SetTargetLandmarks(this->TargetPoints);
    transform->Update();
  }
  return transform;
}

//----------------------------------------------------------------------------
//Clear all bending data
void vtkSlicerPlannerLogic::clearBendingData()
{
  this->SourcePoints = NULL;
  this->SourcePointsDense = NULL;
  this->TargetPoints = NULL;
  this->Fiducials = NULL;
  this->ModelToBend = NULL;
  this->cellLocator = NULL;
  this->BendingPlane = NULL;
  this->BendingPlaneLocator = NULL;
  this->bendInitialized = false;
}

//----------------------------------------------------------------------------
//Create source points based on fiducials
void vtkSlicerPlannerLogic::generateSourcePoints()
{
  this->SourcePoints = vtkSmartPointer<vtkPoints>::New();
  bool bendingAxis = true;

  //A and B are on the bending line.  C and D are on the bending axis
  vtkVector3d A;
  vtkVector3d B;
  vtkVector3d C;
  vtkVector3d D;
  vtkVector3d CD;
  vtkVector3d AB;

  
  double firstPoint[3];
  double secondPoint[3];

  this->Fiducials->GetPoint(0, firstPoint);
  this->Fiducials->GetPoint(1, secondPoint);

  //we are receive the bending axis as input, must derive bending line
  if (bendingAxis)
  {
      C = (vtkVector3d)firstPoint;
      D = (vtkVector3d)secondPoint;
      CD = D - C;
      vtkVector3d CDMid = C + 0.5*CD;
      vtkVector3d normal = this->getNormalAtPoint(CDMid, this->cellLocator, this->BendingPolyData);
      vtkVector3d bendLine = normal.Cross(CD);
      A = CDMid + bendLine;
      B = CDMid - bendLine;
      AB = B - A;
  }
  else  //we are receiving the bending line as input, must derive bending axis
  {
      A = (vtkVector3d)firstPoint;
      B = (vtkVector3d)secondPoint;
      AB = B - A;
      vtkVector3d ABMid = A + 0.5*AB;
      vtkVector3d normal = this->getNormalAtPoint(ABMid, this->cellLocator, this->BendingPolyData);
      vtkVector3d bendAxis = normal.Cross(AB);
      C = ABMid + bendAxis;
      D = ABMid - bendAxis;
      CD = D - C;
  }
    
  AB.Normalize();
  CD.Normalize();

  vtkSmartPointer<vtkPlane> fixedPlane = this->createPlane(C, D, A, B);
  this->BendingPlane = fixedPlane;
  this->createBendingLocator();

  
  A = this->projectToModel(A);
  B = this->projectToModel(B);

  this->SourcePoints->InsertPoint(0, A.GetData());
  this->SourcePoints->InsertPoint(1, B.GetData());
  this->SourcePoints->InsertPoint(2, C.GetData());
  this->SourcePoints->InsertPoint(3, D.GetData());

  //Compute Next point as the vector defining the bend axis
  vtkVector3d E = A + 0.5 * (B - A);
  vtkVector3d CE = E - C;
  CD = D - C;
  vtkVector3d CF = CE.Dot(CD.Normalized()) * CD.Normalized();

  //Midpoint projected onto te line between the fixed points - Pivot point
  vtkVector3d F = C + CF;
  F = projectToModel(F);
  vtkVector3d FE = E - F;
  vtkVector3d FB = B - F;
  vtkVector3d axis = FE.Cross(FB);
  axis.Normalize();

  //Store beding axis in source points
  this->SourcePoints->InsertPoint(4, axis.GetData());

  //Agressively downsample to create source points
  vtkNew<vtkCleanPolyData> clean;
  vtkNew<vtkVertexGlyphFilter> verts;
  verts->SetInputData(this->BendingPolyData);
  verts->Update();
  clean->SetInputData(verts->GetOutput());
  clean->SetTolerance(0.07);
  clean->Update();
  this->SourcePointsDense = clean->GetOutput()->GetPoints();
}

//----------------------------------------------------------------------------
//Project a 3D point onto the closest point on the bending model
vtkVector3d vtkSlicerPlannerLogic::projectToModel(vtkVector3d point)
{
  //build locator when model is loaded

  return this->projectToModel(point, this->cellLocator);
}

//----------------------------------------------------------------------------
//Project a 3D point onto the closest point on the bending model, constrained by a plane
vtkVector3d vtkSlicerPlannerLogic::projectToModel(vtkVector3d point, vtkPlane* plane)
{
  vtkNew<vtkCutter> cutter;
  cutter->SetCutFunction(plane);
  cutter->SetInputData(this->BendingPolyData);
  vtkSmartPointer<vtkPolyData> cut;
  cutter->Update();
  cut = cutter->GetOutput();
  return this->projectToModel(point, cut);
}

//----------------------------------------------------------------------------
//Project a 3D point onto the closest point on the specified model
vtkVector3d vtkSlicerPlannerLogic::projectToModel(vtkVector3d point, vtkPolyData* model)
{
  vtkNew<vtkCellLocator> locator;
  vtkNew<vtkTriangleFilter> triangulate;
  triangulate->SetInputData(model);
  triangulate->Update();
  locator->SetDataSet(triangulate->GetOutput());
  locator->BuildLocator();
  return this->projectToModel(point, locator.GetPointer());
}

//----------------------------------------------------------------------------
//Project a 3D point onto the closest point on the model as defined by the provided cell locator
vtkVector3d vtkSlicerPlannerLogic::projectToModel(vtkVector3d point, vtkCellLocator* locator)
{
  double closestPoint[3];//the coordinates of the closest point will be returned here
  double closestPointDist2; //the squared distance to the closest point will be returned here
  vtkIdType cellId; //the cell id of the cell containing the closest point will be returned here
  int subId; //this is rarely used (in triangle strips only, I believe)
  locator->FindClosestPoint(point.GetData(), closestPoint, cellId, subId, closestPointDist2);

  vtkVector3d projection;
  projection.Set(closestPoint[0], closestPoint[1], closestPoint[2]);
  return projection;
}

//----------------------------------------------------------------------------
//Create Plane from two points in plane and two points on normal vector
vtkSmartPointer<vtkPlane> vtkSlicerPlannerLogic::createPlane(vtkVector3d A, vtkVector3d B, vtkVector3d C, vtkVector3d D)
{
  //A and B are in the plane
  //C and D are perp to plane

  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  vtkVector3d AB = B - A;
  vtkVector3d E = A + 0.5 * AB;
  vtkVector3d CD = D - C;
  plane->SetOrigin(E.GetData());
  plane->SetNormal(CD.GetData());
  return plane;
}

//----------------------------------------------------------------------------
//bend point using vector computed from axis
vtkVector3d vtkSlicerPlannerLogic::bendPoint(vtkVector3d point, double magnitude)
{
  double ax[3];
  this->SourcePoints->GetPoint(4, ax);
  vtkVector3d axis = (vtkVector3d)ax;
  vtkVector3d F = projectToModel(point, this->BendingPlaneLocator);
  vtkVector3d AF = F - point;
  vtkVector3d BendingVector;
  if(this->BendingPlane->EvaluateFunction(point.GetData()) < 0)
  {
    BendingVector = AF.Cross(axis);
  }
  else
  {
    BendingVector = axis.Cross(AF);
  }
  vtkVector3d point2 = point + ((magnitude * AF.Norm()) * BendingVector.Normalized());
  vtkVector3d A2F = F - point2;

  //correction factor
  point2 = point2 + (A2F.Norm() - AF.Norm()) * A2F.Normalized();

  return point2;
}
//----------------------------------------------------------------------------
//Create a point locator constrained to the bending axis
void vtkSlicerPlannerLogic::createBendingLocator()
{
  this->BendingPlaneLocator = vtkSmartPointer<vtkCellLocator>::New();

  vtkNew<vtkCutter> cutter;
  cutter->SetCutFunction(this->BendingPlane);
  cutter->SetInputData(this->BendingPolyData);
  vtkSmartPointer<vtkPolyData> cut;
  cutter->Update();
  cut = cutter->GetOutput();

  vtkNew<vtkTriangleFilter> triangulate;
  triangulate->SetInputData(cut);
  triangulate->Update();
  this->BendingPlaneLocator->SetDataSet(triangulate->GetOutput());
  this->BendingPlaneLocator->BuildLocator();
}

//----------------------------------------------------------------------------
//Remove models and clear data
void vtkSlicerPlannerLogic::clearModelsAndData()
{
  this->clearBendingData();
  if (this->SkullWrappedPreOP)
  {
    this->GetMRMLScene()->RemoveNode(this->SkullWrappedPreOP);
    this->SkullWrappedPreOP = NULL;
  }
  if (this->HealthyBrain)
  {
    this->GetMRMLScene()->RemoveNode(this->HealthyBrain);
    this->HealthyBrain = NULL;
  }
  if (this->CurrentModel)
  {
    this->GetMRMLScene()->RemoveNode(this->CurrentModel);
    this->CurrentModel = NULL;
  }
  if (this->BoneTemplate)
  {
    this->GetMRMLScene()->RemoveNode(this->BoneTemplate);
    this->BoneTemplate = NULL;
  }
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

  this->preOPICV = 0;
  this->healthyBrainICV = 0;
  this->currentICV = 0;
  this->templateICV = 0;

}

vtkVector3d vtkSlicerPlannerLogic::getNormalAtPoint(vtkVector3d point, vtkCellLocator* locator, vtkPolyData* model)
{
    double closestPoint[3];//the coordinates of the closest point will be returned here
    double closestPointDist2; //the squared distance to the closest point will be returned here
    vtkIdType cellId; //the cell id of the cell containing the closest point will be returned here
    int subId; //this is rarely used (in triangle strips only, I believe)
    locator->FindClosestPoint(point.GetData(), closestPoint, cellId, subId, closestPointDist2);

    vtkVector3d normal;
    double n[3];
    model->GetCellData()->GetNormals()->GetTuple(cellId, n);
    normal.SetX(n[0]);
    normal.SetY(n[1]);
    normal.SetZ(n[2]);

    return normal;
}

double vtkSlicerPlannerLogic::getDistanceToModel(vtkVector3d point, vtkPolyData* model)
{
    vtkNew<vtkCellLocator> locator;
    vtkNew<vtkTriangleFilter> triangulate;
    triangulate->SetInputData(model);
    triangulate->Update();
    locator->SetDataSet(triangulate->GetOutput());
    locator->BuildLocator();

    double closestPoint[3];//the coordinates of the closest point will be returned here
    double closestPointDist2; //the squared distance to the closest point will be returned here
    vtkIdType cellId; //the cell id of the cell containing the closest point will be returned here
    int subId; //this is rarely used (in triangle strips only, I believe)
    locator->FindClosestPoint(point.GetData(), closestPoint, cellId, subId, closestPointDist2);   
    return std::sqrt(closestPointDist2);
}
