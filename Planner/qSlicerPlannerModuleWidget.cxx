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

// Qt includes
#include <QDebug>
#include <QMessageBox>
#include <QSettings>

// CTK includes
#include "ctkMessageBox.h"

// VTK includes
#include "vtkNew.h"
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include "vtkVector.h"
#include "vtkVectorOperators.h"

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerIOManager.h"
#include "qSlicerFileDialog.h"
#include "qMRMLSceneModelHierarchyModel.h"
#include "qSlicerPlannerModuleWidget.h"
#include "ui_qSlicerPlannerModuleWidget.h"

// Slicer
#include "vtkMRMLDisplayableHierarchyLogic.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsPlanesNode.h"
#include "vtkMRMLModelHierarchyNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTransformDisplayNode.h"
#include "vtkMRMLStorageNode.h"
#include "vtkMRMLModelDisplayNode.h"
#include "vtkMRMLModelStorageNode.h"
#include "vtkMRMLTableNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLInteractionNode.h"
#include "vtkMRMLSelectionNode.h"
#include "vtkThinPlateSplineTransform.h"
#include <vtkPointData.h>

// Slicer CLI includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include "qSlicerAbstractCoreModule.h"
#include <qSlicerCLIModule.h>
#include <vtkSlicerCLIModuleLogic.h>

// Self
#include "qMRMLPlannerModelHierarchyModel.h"
#include "vtkSlicerPlannerLogic.h"

//STD includes
#include <vector>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlannerModuleWidgetPrivate: public Ui_qSlicerPlannerModuleWidget
{
public:
  qSlicerPlannerModuleWidgetPrivate();
  void fireDeleteChildrenWarning() const;

  void createTransformsIfNecessary(
    vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);
  vtkMRMLLinearTransformNode* createTransformNode(
    vtkMRMLScene* scene, vtkMRMLNode* refNode);
  vtkMRMLLinearTransformNode* getTransformNode(
    vtkMRMLScene* scene, vtkMRMLNode* refNode) const;
  void removeTransformNode(vtkMRMLScene* scene, vtkMRMLNode* nodeRef);

  void createPlanesIfNecessary(
    vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);
  void updatePlanesFromModel(vtkMRMLScene* scene, vtkMRMLModelNode*) const;
  
  vtkMRMLMarkupsPlanesNode* createPlaneNode(vtkMRMLScene* scene, vtkMRMLNode* refNode);
  void removePlaneNode(vtkMRMLScene* scene, vtkMRMLNode* nodeRef);
  vtkMRMLMarkupsPlanesNode* getPlaneNode(vtkMRMLScene* scene, vtkMRMLNode* refNode) const;

  qMRMLPlannerModelHierarchyModel* sceneModel() const;
  void updateWidgetFromReferenceNode(
    vtkMRMLNode* node,
    ctkColorPickerButton* button,
    qMRMLSliderWidget* slider) const;
  void updateReferenceNodeFromWidget(
    vtkMRMLNode* node, QColor color, double opacity) const;
  vtkMRMLNode* openReferenceDialog() const;

  //Cutting methods
  void previewCut(vtkMRMLScene* scene);
  void adjustCut(vtkMRMLScene* scene);
  void completeCut(vtkMRMLScene* scene);
  void cancelCut(vtkMRMLScene* scene);
  void deleteModel(vtkMRMLModelNode* node, vtkMRMLScene* scene);
  void splitModel(vtkMRMLModelNode* inputNode, vtkMRMLModelNode* split1, vtkMRMLModelNode* split2, vtkMRMLScene* scene);
  void applyRandomColor(vtkMRMLModelNode* node);
  void hardenTransforms();

  //Cutting Variables
  vtkMRMLModelHierarchyNode* HierarchyNode;
  vtkMRMLModelHierarchyNode* StagedHierarchyNode;
  QStringList HideChildNodeTypes;
  vtkMRMLNode* BrainReferenceNode;
  vtkMRMLNode* TemplateReferenceNode;
  vtkMRMLNode* CurrentCutNode;
  vtkMRMLNode* StagedCutNode1;
  vtkMRMLNode* StagedCutNode2;
  bool cuttingActive;
  vtkSlicerCLIModuleLogic* splitLogic;
  vtkSlicerPlannerLogic* logic;
  vtkMRMLCommandLineModuleNode* cmdNode;

  //Bending Variables
  vtkMRMLMarkupsFiducialNode* FixedPointA;
  vtkMRMLMarkupsFiducialNode* FixedPointB;
  vtkMRMLMarkupsFiducialNode* MovingPointA;
  vtkMRMLMarkupsFiducialNode* MovingPointB;
  vtkMRMLMarkupsFiducialNode* PlacingNode;
  vtkMRMLNode* CurrentBendNode;
  vtkSmartPointer<vtkPoints> Fiducials;
  double BendMagnitude;
  bool bendingActive;
  bool placingActive;

  //Bending methods
  void beginPlacement(vtkMRMLScene* scene, int id);
  void endPlacement();
  void computeAndSetSourcePoints();
  void computeTransform(vtkMRMLScene* scene);
  void clearControlPoints(vtkMRMLScene* scene);
  void clearBendingData();

  //Metrics Variables
  std::vector<vtkMRMLModelNode*> modelIterator;
  vtkSlicerCLIModuleLogic* distanceLogic;
  vtkMRMLTableNode* modelMetricsTable;

  //Metrics methods
  void prepScalarComputation(vtkMRMLScene* scene);
  void setScalarVisibility(bool visible);
};

//-----------------------------------------------------------------------------
// qSlicerPlannerModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
//Clear fiducials used for bending
void qSlicerPlannerModuleWidgetPrivate::clearControlPoints(vtkMRMLScene* scene)
{
  if (this->FixedPointA)
  {
    scene->RemoveNode(this->FixedPointA);
    FixedPointA = NULL;
  }
  if (this->FixedPointB)
  {
    scene->RemoveNode(this->FixedPointB);
    FixedPointB = NULL;
  }
  if (this->MovingPointA)
  {
    scene->RemoveNode(this->MovingPointA);
    MovingPointA = NULL;
  }
  if (this->MovingPointB)
  {
    scene->RemoveNode(this->MovingPointB);
    MovingPointB = NULL;
  }
  if (this->PlacingNode)
  {
    scene->RemoveNode(this->PlacingNode);
    PlacingNode = NULL;
  }
}

//-----------------------------------------------------------------------------
//Clear data used to compute bending trasforms
void qSlicerPlannerModuleWidgetPrivate::clearBendingData()
{
  this->Fiducials = NULL;
  this->logic->clearBendingData();
}

//-----------------------------------------------------------------------------
//Constructor
qSlicerPlannerModuleWidgetPrivate::qSlicerPlannerModuleWidgetPrivate()
{
  this->HierarchyNode = NULL;
  this->StagedHierarchyNode = NULL;
  this->HideChildNodeTypes =
    (QStringList() << "vtkMRMLFiberBundleNode" << "vtkMRMLAnnotationNode");
  this->BrainReferenceNode = NULL;
  this->TemplateReferenceNode = NULL;
  this->CurrentCutNode = NULL;
  this->StagedCutNode1 = NULL;
  this->StagedCutNode2 = NULL;
  this->modelMetricsTable = NULL;
  this->cuttingActive = false;
  this->bendingActive = false;
  this->placingActive = false;
  this->cmdNode = NULL;

  this->FixedPointA = NULL;
  this->FixedPointB = NULL;
  this->MovingPointA = NULL;
  this->MovingPointB = NULL;
  this->PlacingNode = NULL;
  this->Fiducials = NULL;
  this->BendMagnitude = 0;
  
  qSlicerAbstractCoreModule* splitModule =
    qSlicerCoreApplication::application()->moduleManager()->module("SplitModel");

  this->splitLogic =
    vtkSlicerCLIModuleLogic::SafeDownCast(splitModule->logic());

  qSlicerAbstractCoreModule* distanceModule =
    qSlicerCoreApplication::application()->moduleManager()->module("ModelToModelDistance");
  this->distanceLogic =
    vtkSlicerCLIModuleLogic::SafeDownCast(distanceModule->logic());
}

//-----------------------------------------------------------------------------
//Complete placement of current fiducial
void qSlicerPlannerModuleWidgetPrivate::endPlacement()
{
  this->FixedPointAButton->setEnabled(true);
  this->FixedPointBButton->setEnabled(true);
  this->MovingPointAButton->setEnabled(true);
  this->MovingPointBButton->setEnabled(true);
  this->placingActive = false;
  this->PlacingNode = NULL;
}

//-----------------------------------------------------------------------------
//Compute source points from fiducials
void qSlicerPlannerModuleWidgetPrivate::computeAndSetSourcePoints()
{
  this->Fiducials = vtkSmartPointer<vtkPoints>::New();
  double posfa[3];
  double posfb[3];
  double posma[3];
  double posmb[3];
  this->FixedPointA->GetNthFiducialPosition(0, posfa);
  this->FixedPointB->GetNthFiducialPosition(0, posfb);
  this->MovingPointA->GetNthFiducialPosition(0, posma);
  this->MovingPointB->GetNthFiducialPosition(0, posmb);
  this->Fiducials->InsertNextPoint(posfa[0], posfa[1], posfa[2]);
  this->Fiducials->InsertNextPoint(posfb[0], posfb[1], posfb[2]);
  this->Fiducials->InsertNextPoint(posma[0], posma[1], posma[2]);
  this->Fiducials->InsertNextPoint(posmb[0], posmb[1], posmb[2]);
  this->logic->initializeBend(this->Fiducials, vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode));
}


//-----------------------------------------------------------------------------
//Compute thin plate spline transform based on source and target points
void qSlicerPlannerModuleWidgetPrivate::computeTransform(vtkMRMLScene* scene)
{
  
  vtkMRMLTransformNode* tnode = vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->GetParentTransformNode();
  vtkNew<vtkMRMLTransformNode> tnodetemp;
  if (!tnode)
  {
    tnode = tnodetemp.GetPointer();
    scene->AddNode(tnode);
    tnode->CreateDefaultDisplayNodes();
  }

  vtkSmartPointer<vtkThinPlateSplineTransform> tps = this->logic->getBendTransform(this->BendMagnitude);
  tnode->SetAndObserveTransformToParent(tps);
  vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->SetAndObserveTransformNodeID(tnode->GetID());
}

//-----------------------------------------------------------------------------
//Initialize placement of a fiducial
void qSlicerPlannerModuleWidgetPrivate::beginPlacement(vtkMRMLScene* scene, int id)
{
    
  vtkNew<vtkMRMLMarkupsFiducialNode> fiducial;
  if (id == 0)  //Place fiducial A
  {
    if (this->FixedPointA)
    {
      scene->RemoveNode(this->FixedPointA);
      FixedPointA = NULL;
    }
    this->FixedPointA = fiducial.GetPointer();
    this->FixedPointA->SetName("FA");
    this->PlacingNode = this->FixedPointA;
    
  }

  if (id == 1)  //Place fiducial B
  {
    if (this->FixedPointB)
    {
      scene->RemoveNode(this->FixedPointB);
      FixedPointB = NULL;
    }
    this->FixedPointB = fiducial.GetPointer();
    this->FixedPointB->SetName("FB");
    this->PlacingNode = this->FixedPointB;
    
  }

  if (id == 2)  //Place fiducial MA
  {
    if (this->MovingPointA)
    {
      scene->RemoveNode(this->MovingPointA);
      MovingPointA = NULL;
    }
    this->MovingPointA = fiducial.GetPointer();
    this->MovingPointA->SetName("MA");
    this->PlacingNode = this->MovingPointA;
    
  }

  if (id == 3)  //Place fiducial MB
  {
    if (this->MovingPointB)
    {
      scene->RemoveNode(this->MovingPointB);
      MovingPointB = NULL;
    }
    this->MovingPointB = fiducial.GetPointer();
    this->MovingPointB->SetName("MB");
    this->PlacingNode = this->MovingPointB;

  }

  //add new fiducial to scene
  scene->AddNode(this->PlacingNode);
  this->PlacingNode->CreateDefaultDisplayNodes();
  vtkMRMLMarkupsDisplayNode* disp = vtkMRMLMarkupsDisplayNode::SafeDownCast( this->PlacingNode->GetDisplayNode());
  disp->SetTextScale(0.1);
  this->placingActive = true;

  //activate placing
  vtkMRMLInteractionNode* interaction = qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode();
  vtkMRMLSelectionNode* selection = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  selection->SetReferenceActivePlaceNodeClassName("vtkMRMLMarkupsFiducialNode");
  selection->SetActivePlaceNodeID(this->PlacingNode->GetID());
  interaction->SetCurrentInteractionMode(vtkMRMLInteractionNode::Place);

  //deactivate buttons
  this->FixedPointAButton->setEnabled(false);
  this->FixedPointBButton->setEnabled(false);
  this->MovingPointAButton->setEnabled(false);
  this->MovingPointBButton->setEnabled(false);
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::fireDeleteChildrenWarning() const
{
  ctkMessageBox deleteChildrenDialog;
  deleteChildrenDialog.setText(
    "Deleting this model hierarchy will also delete all the model children.");
  deleteChildrenDialog.setWindowTitle("Warning");
  deleteChildrenDialog.setIcon(QMessageBox::Warning);
  deleteChildrenDialog.setStandardButtons(QMessageBox::Ok);
  deleteChildrenDialog.setDontShowAgainSettingsKey(
    vtkSlicerPlannerLogic::DeleteChildrenWarningSettingName());
  deleteChildrenDialog.exec();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate
::createTransformsIfNecessary(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if (!hierarchy)
    {
    return;
    }

  vtkMRMLLinearTransformNode* transform = this->getTransformNode(scene, hierarchy);
  if (!transform)
    {
    transform = this->createTransformNode(scene, hierarchy);
    }

  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  hierarchy->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
    {
      vtkMRMLModelNode* childModel =
        vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());
    if (childModel)
      {
      vtkMRMLLinearTransformNode* childTransform = this->getTransformNode(scene, childModel);
      if (!childTransform)
        {
        childTransform = this->createTransformNode(scene, childModel);
        childModel->SetAndObserveTransformNodeID(childTransform->GetID());
        }
      childTransform->SetAndObserveTransformNodeID(transform->GetID());
      }
    }

}

//-----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* qSlicerPlannerModuleWidgetPrivate
::createTransformNode(vtkMRMLScene* scene, vtkMRMLNode* refNode)
{
  Q_ASSERT(scene);
  vtkNew<vtkMRMLLinearTransformNode> newTransform;
  vtkMRMLLinearTransformNode* transform =
    vtkMRMLLinearTransformNode::SafeDownCast(
      scene->AddNode(newTransform.GetPointer()));
  QString transformName = refNode->GetName();
  transformName += "_Transform";
  transform->SetName(transformName.toLatin1());

  vtkNew<vtkMRMLTransformDisplayNode> newDisplay;
  newDisplay->SetEditorScalingEnabled(false);
  vtkMRMLNode* display = scene->AddNode(newDisplay.GetPointer());
  transform->SetAndObserveDisplayNodeID(display->GetID());

  refNode->SetNodeReferenceID(
    this->sceneModel()->transformDisplayReferenceRole(), display->GetID());
  return transform;
}

//-----------------------------------------------------------------------------
vtkMRMLLinearTransformNode* qSlicerPlannerModuleWidgetPrivate
::getTransformNode(vtkMRMLScene* scene, vtkMRMLNode* refNode) const
{
  vtkMRMLTransformDisplayNode* display =
    vtkMRMLTransformDisplayNode::SafeDownCast(refNode ?
      refNode->GetNodeReference(this->sceneModel()->transformDisplayReferenceRole()) : NULL);
  return display ?
    vtkMRMLLinearTransformNode::SafeDownCast(display->GetDisplayableNode()) : NULL;
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate
::removeTransformNode(vtkMRMLScene* scene, vtkMRMLNode* nodeRef)
{
  vtkMRMLLinearTransformNode* transform = this->getTransformNode(scene, nodeRef);
  if (transform)
    {
    scene->RemoveNode(transform);
    }
}

//-----------------------------------------------------------------------------
vtkMRMLMarkupsPlanesNode* qSlicerPlannerModuleWidgetPrivate::createPlaneNode(
  vtkMRMLScene* scene, vtkMRMLNode* refNode)
{
  Q_ASSERT(scene);
  vtkNew<vtkMRMLMarkupsPlanesNode> newPlanes;
  vtkMRMLMarkupsPlanesNode* planes =
    vtkMRMLMarkupsPlanesNode::SafeDownCast(
    scene->AddNode(newPlanes.GetPointer()));
  QString planesName = refNode->GetName();
  planesName += "Planner_Planes";
  planes->SetName(planesName.toLatin1());

  vtkNew<vtkMRMLMarkupsDisplayNode> newDisplay;
  vtkMRMLNode* display = scene->AddNode(newDisplay.GetPointer());
  planes->SetAndObserveDisplayNodeID(display->GetID());

  refNode->SetNodeReferenceID(
    this->sceneModel()->planesReferenceRole(), planes->GetID());
  /**
  vtkMRMLTransformDisplayNode* display2 =
    vtkMRMLTransformDisplayNode::SafeDownCast(refNode ?
    refNode->GetNodeReference(this->sceneModel()->transformDisplayReferenceRole()) : NULL);
  planes->SetNodeReferenceID(
    this->sceneModel()->transformDisplayReferenceRole(), display2->GetID());
    **/
  //planes->SetAndObserveTransformNodeID(display2->GetID());
  return planes;
}

//-----------------------------------------------------------------------------
vtkMRMLMarkupsPlanesNode* qSlicerPlannerModuleWidgetPrivate
::getPlaneNode(vtkMRMLScene* scene, vtkMRMLNode* refNode) const
{
  vtkMRMLMarkupsPlanesNode* plane =
    vtkMRMLMarkupsPlanesNode::SafeDownCast(refNode ?
    refNode->GetNodeReference(this->sceneModel()->planesReferenceRole()) : NULL);
  return plane;
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate
::removePlaneNode(vtkMRMLScene* scene, vtkMRMLNode* nodeRef)
{
  vtkMRMLMarkupsPlanesNode* plane = this->getPlaneNode(scene, nodeRef);
  if (plane)
  {
    scene->RemoveNode(plane);
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::createPlanesIfNecessary(
  vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if (!hierarchy)
    {
    return;
    }  
  int count = 0;
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  std::vector<vtkMRMLModelNode*> models;
  hierarchy->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
    {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());
    if (childModel)
      {
        vtkMRMLMarkupsPlanesNode* childPlane = this->getPlaneNode(scene, childModel);
        if (!childPlane)
        {
          childPlane = this->createPlaneNode(scene, childModel);
          this->updatePlanesFromModel(scene, childModel);
        }
        
      }
    }

  
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::updatePlanesFromModel(vtkMRMLScene* scene,
  vtkMRMLModelNode* model) const
{
  if (!model)
    {
    return;
    }
  vtkMRMLMarkupsPlanesNode* plane = this->getPlaneNode(scene, model);
  
  plane->RemoveAllMarkups();

  double bounds[6];
  model->GetRASBounds(bounds);
  double min[3], max[3];
  min[0] = bounds[0]; min[1] = bounds[2]; min[2] = bounds[4];
  max[0] = bounds[1]; max[1] = bounds[3]; max[2] = bounds[5];

  double origin[3];
  for (int i = 0; i < 3; ++i)
    {
    origin[i] = min[i] + (max[i] - min[i])/2;
    }

  // For the normal, pick the bounding largest size
  double largest = 0;
  int largetDimension = 0;
  for (int i = 0; i < 3; ++i)
    {
    double size = fabs(max[i] - min[i]);
    if (size > largest)
      {
      largest = size;
      largetDimension = i;
      }
    }
  double normal[3] = {0.0, 0.0, 0.0};
  normal[largetDimension] = 1.0;

  plane->AddPlaneFromArray(origin, normal, min, max);
  plane->SetNthMarkupVisibility(0, false);
  plane->SetNthMarkupAssociatedNodeID(0, model->GetID());
}


//-----------------------------------------------------------------------------
qMRMLPlannerModelHierarchyModel* qSlicerPlannerModuleWidgetPrivate
::sceneModel() const
{
  return qobject_cast<qMRMLPlannerModelHierarchyModel*>(
    this->ModelHierarchyTreeView->sceneModel());
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::updateWidgetFromReferenceNode(
  vtkMRMLNode* node,
  ctkColorPickerButton* button,
  qMRMLSliderWidget* slider) const
{
  vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);
  button->setEnabled(model != NULL);
  slider->setEnabled(model != NULL);
  if (model)
    {
    vtkMRMLDisplayNode* display = model->GetDisplayNode();
    if (display)
      {
      double rgb[3];
      display->GetColor(rgb);

      bool wasBlocking = button->blockSignals(true);
      button->setColor(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));
      button->blockSignals(wasBlocking);

      wasBlocking = slider->blockSignals(true);
      slider->setValue(display->GetOpacity());
      slider->blockSignals(wasBlocking);
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::updateReferenceNodeFromWidget(
  vtkMRMLNode* node, QColor color, double opacity) const
{
  vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);
  if (model)
    {
    vtkMRMLDisplayNode* display = model->GetDisplayNode();
    if (display)
      {
      int wasModifying = display->StartModify();
      display->SetColor(color.redF(), color.greenF(), color.blueF());
      display->SetOpacity(opacity);
      display->EndModify(wasModifying);
      }
    }
}

//-----------------------------------------------------------------------------
vtkMRMLNode* qSlicerPlannerModuleWidgetPrivate::openReferenceDialog() const
{
  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  vtkNew<vtkCollection> loadedNodes;
  bool success = ioManager->openDialog(
    "ModelFile", qSlicerFileDialog::Read,
    qSlicerIO::IOProperties(), loadedNodes.GetPointer());
  if (success && loadedNodes->GetNumberOfItems() > 0)
    {
    return vtkMRMLNode::SafeDownCast(loadedNodes->GetItemAsObject(0));
    }
  return NULL;
}


//-----------------------------------------------------------------------------
//Create and display temporary cut
void qSlicerPlannerModuleWidgetPrivate::previewCut(vtkMRMLScene* scene)
{
  if (!this->HierarchyNode)
  {
    this->cuttingActive = false;
    return;
  }

  this->hardenTransforms();

  //Create nodes
  vtkNew<vtkMRMLModelNode> splitNode1;
  vtkNew<vtkMRMLModelNode> splitNode2;
  splitNode1->SetName("TemporaryBoneA");
  splitNode2->SetName("TemporaryBoneB");

  //Set scene on nodes
  splitNode1->SetScene(scene);
  splitNode2->SetScene(scene);

  //Create display and storage nodes
  vtkNew<vtkMRMLModelDisplayNode> dnode1;
  vtkNew<vtkMRMLModelDisplayNode> dnode2;
  vtkNew<vtkMRMLModelStorageNode> snode1;
  vtkNew<vtkMRMLModelStorageNode> snode2;

  splitNode1->SetAndObserveDisplayNodeID(dnode1->GetID());
  splitNode2->SetAndObserveDisplayNodeID(dnode2->GetID());
  splitNode1->SetAndObserveStorageNodeID(snode1->GetID());
  splitNode2->SetAndObserveStorageNodeID(snode2->GetID());

  //add nodes to scene
  scene->AddNode(dnode1.GetPointer());
  scene->AddNode(dnode2.GetPointer());
  scene->AddNode(snode1.GetPointer());
  scene->AddNode(snode2.GetPointer());
  scene->AddNode(splitNode1.GetPointer());
  scene->AddNode(splitNode2.GetPointer());

  this->splitModel(vtkMRMLModelNode::SafeDownCast(CurrentCutNode), splitNode1.GetPointer(), 
    splitNode2.GetPointer(), scene);

  //add to hierarchy
  vtkNew<vtkMRMLModelHierarchyNode> splitNodeH1;
  vtkNew<vtkMRMLModelHierarchyNode> splitNodeH2;
  splitNodeH1->SetHideFromEditors(1);
  splitNodeH2->SetHideFromEditors(1);
  scene->AddNode(splitNodeH1.GetPointer());
  scene->AddNode(splitNodeH2.GetPointer());
  splitNodeH1->SetParentNodeID(this->HierarchyNode->GetID());
  splitNodeH2->SetParentNodeID(this->HierarchyNode->GetID());
  splitNodeH1->SetModelNodeID(splitNode1->GetID());
  splitNodeH2->SetModelNodeID(splitNode2->GetID());
  this->StagedCutNode1 = splitNode1.GetPointer();
  this->StagedCutNode2 = splitNode2.GetPointer();

  //set random colors on models
  this->applyRandomColor(splitNode1.GetPointer());
  this->applyRandomColor(splitNode2.GetPointer());
}

//-----------------------------------------------------------------------------
//Redo current cut with new inputs
void qSlicerPlannerModuleWidgetPrivate::adjustCut(vtkMRMLScene* scene)
{
  this->splitModel(vtkMRMLModelNode::SafeDownCast(CurrentCutNode), vtkMRMLModelNode::SafeDownCast(StagedCutNode1),
    vtkMRMLModelNode::SafeDownCast(StagedCutNode2), scene);
}

//-----------------------------------------------------------------------------
//Finish current cut
void qSlicerPlannerModuleWidgetPrivate::completeCut(vtkMRMLScene* scene)
{
  //Rename temp bones
  std::stringstream rename1;
  std::stringstream rename2;

  rename1 << this->CurrentCutNode->GetName() << "_cut1";
  rename2 << this->CurrentCutNode->GetName() << "_cut2";
  StagedCutNode1->SetName(rename1.str().c_str());
  StagedCutNode2->SetName(rename2.str().c_str());

  //dump refs from staging vars
  StagedCutNode1 = NULL;
  StagedCutNode2 = NULL;
  
  this->deleteModel(vtkMRMLModelNode::SafeDownCast(this->CurrentCutNode), scene);
}

//-----------------------------------------------------------------------------
//Cancel current cut, resetting to default state
void qSlicerPlannerModuleWidgetPrivate::cancelCut(vtkMRMLScene* scene)
{
  this->deleteModel(vtkMRMLModelNode::SafeDownCast(this->StagedCutNode1), scene);
  this->deleteModel(vtkMRMLModelNode::SafeDownCast(this->StagedCutNode2), scene);
}

//-----------------------------------------------------------------------------
//Delete model and associated nodes from scene
void qSlicerPlannerModuleWidgetPrivate::deleteModel(vtkMRMLModelNode* node, vtkMRMLScene* scene)
{
  vtkMRMLModelHierarchyNode *hnode = vtkMRMLModelHierarchyNode::SafeDownCast(
    vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(node->GetScene(), node->GetID()));

  if (hnode)
  {
    scene->RemoveNode(hnode);
  }
  if (node)
  {
    this->removeTransformNode(scene, node);
    this->removePlaneNode(scene, node);
    scene->RemoveNode(node);
  }
  node = NULL;
}

//-----------------------------------------------------------------------------
//Split of model node into two based on its plane node
void qSlicerPlannerModuleWidgetPrivate::splitModel(vtkMRMLModelNode* inputNode, vtkMRMLModelNode* split1, vtkMRMLModelNode* split2, vtkMRMLScene* scene)
{
  

  this->splitLogic->SetMRMLScene(scene);

  vtkMRMLCommandLineModuleNode* cmdNode = this->splitLogic->CreateNodeInScene();
  vtkMRMLMarkupsPlanesNode* plane = this->getPlaneNode(scene, inputNode);

  double normal[3];
  double origin[3];
  plane->GetNthPlaneNormal(0, normal);
  plane->GetNthPlaneOrigin(0, origin);



  cmdNode->SetParameterAsString("Model",inputNode->GetID());
  cmdNode->SetParameterAsString("ModelOutput1", split1->GetID());
  cmdNode->SetParameterAsString("ModelOutput2", split2->GetID());

  std::stringstream originStream;
  originStream << std::setprecision(15) << origin[0] << "," << origin[1] << "," << origin[2];
  cmdNode->SetParameterAsString("Origin", originStream.str());

  std::stringstream normalStream;
  normalStream << std::setprecision(15) << normal[0] << "," << normal[1] << "," << normal[2];
  cmdNode->SetParameterAsString("Normal", normalStream.str());


  this->splitLogic->ApplyAndWait(cmdNode, false);
  scene->RemoveNode(cmdNode);
}

//-----------------------------------------------------------------------------
//Apply a random color to a model
void qSlicerPlannerModuleWidgetPrivate::applyRandomColor(vtkMRMLModelNode* model)
{
  if (model)
  {
    vtkMRMLDisplayNode* display = model->GetDisplayNode();
    if (display)
    {
      int wasModifying = display->StartModify();
      display->SetColor(vtkMath::Random(0.0, 1.0), vtkMath::Random(0.0, 1.0), vtkMath::Random(0.0, 1.0));
      display->SetScalarVisibility(false);
      display->EndModify(wasModifying);
    }
  }
}

//-----------------------------------------------------------------------------
//Collect info for computing model to model distances
void qSlicerPlannerModuleWidgetPrivate::prepScalarComputation(vtkMRMLScene* scene)
{
    
  std::vector<vtkMRMLHierarchyNode*> children;
  this->HierarchyNode->GetAllChildrenNodes(children);
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  for (it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if (childModel)
    {
      this->modelIterator.push_back(childModel);
    }
  }
  
}

//-----------------------------------------------------------------------------
//Set the scalar visibility on all models in the current hierarchy
void qSlicerPlannerModuleWidgetPrivate::setScalarVisibility(bool visible)
{
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  this->HierarchyNode->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if (childModel)
    {
      childModel->GetDisplayNode()->SetActiveScalarName("Signed");
      childModel->GetDisplayNode()->SetScalarVisibility(visible);
      childModel->GetDisplayNode()->SetScalarRangeFlag(vtkMRMLDisplayNode::ScalarRangeFlagType::UseDataScalarRange);
    }
  }
}

//-----------------------------------------------------------------------------
//Harden all transforms in the current hierarchy
void qSlicerPlannerModuleWidgetPrivate::hardenTransforms()
{
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  this->HierarchyNode->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
    vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if (childModel)
    {
      int m = childModel->StartModify();
      vtkMRMLTransformNode* transformNode = childModel->GetParentTransformNode();   
      vtkMRMLMarkupsPlanesNode* planeNode = this->getPlaneNode(childModel->GetScene(), childModel);
      int m2 = planeNode->StartModify();
      if (!transformNode)
      {
        // already in the world coordinate system
        return;
      }
      if (transformNode->IsTransformToWorldLinear())
      {
        this->updatePlanesFromModel(childModel->GetScene(), childModel);
        vtkNew<vtkMatrix4x4> hardeningMatrix;
        transformNode->GetMatrixTransformToWorld(hardeningMatrix.GetPointer());
        childModel->ApplyTransformMatrix(hardeningMatrix.GetPointer());
        hardeningMatrix->Identity();
        transformNode->SetMatrixTransformFromParent(hardeningMatrix.GetPointer());
      }
      else
      {
        // non-linear transform hardening
        this->updatePlanesFromModel(childModel->GetScene(), childModel);        
        childModel->ApplyTransform(transformNode->GetTransformToParent());
        transformNode->SetAndObserveTransformToParent(NULL);
        vtkNew<vtkMatrix4x4> hardeningMatrix;
        hardeningMatrix->Identity();
        transformNode->SetMatrixTransformFromParent(hardeningMatrix.GetPointer());
      }
      planeNode->EndModify(m2);
      childModel->EndModify(m);
    }
  }
}


//-----------------------------------------------------------------------------
// qSlicerPlannerModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerPlannerModuleWidget::qSlicerPlannerModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerPlannerModuleWidgetPrivate )
{
}

//-----------------------------------------------------------------------------
qSlicerPlannerModuleWidget::~qSlicerPlannerModuleWidget()
{
  Q_D(qSlicerPlannerModuleWidget);
}

//-----------------------------------------------------------------------------
vtkSlicerPlannerLogic* qSlicerPlannerModuleWidget::plannerLogic() const
{
  return vtkSlicerPlannerLogic::SafeDownCast(this->logic());
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::setup()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();
  
  qMRMLPlannerModelHierarchyModel* sceneModel =
    new qMRMLPlannerModelHierarchyModel(this);

  d->logic = this->plannerLogic();
  qSlicerAbstractCoreModule* wrapperModule =
    qSlicerCoreApplication::application()->moduleManager()->module("ShrinkWrap");

  vtkSlicerCLIModuleLogic* wrapperLogic =
    vtkSlicerCLIModuleLogic::SafeDownCast(wrapperModule->logic());
  this->plannerLogic()->setWrapperLogic(wrapperLogic);
  
  d->ModelHierarchyTreeView->setSceneModel(sceneModel, "Planner");
  d->ModelHierarchyTreeView->setSceneModelType("Planner");
  d->ModelHierarchyTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
  sceneModel->setIDColumn(-1);
  sceneModel->setHeaderData(0, Qt::Horizontal, "Node");
  sceneModel->setExpandColumn(1);
  sceneModel->setHeaderData(1, Qt::Horizontal, ""); // Don't know a good descriptor
  sceneModel->setColorColumn(2);
  sceneModel->setHeaderData(2, Qt::Horizontal, "Color");
  sceneModel->setOpacityColumn(3);
  sceneModel->setHeaderData(3, Qt::Horizontal, "Opacity");
  sceneModel->setTransformVisibilityColumn(4);
  sceneModel->setHeaderData(4, Qt::Horizontal, "Transform");
  sceneModel->setPlanesVisibilityColumn(5);
  sceneModel->setHeaderData(5, Qt::Horizontal, "Planes");
  // use lazy update instead of responding to scene import end event
  sceneModel->setLazyUpdate(true);
  

  d->ModelHierarchyTreeView->setHeaderHidden(false);
  d->ModelHierarchyTreeView->header()->setStretchLastSection(false);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->nameColumn(), QHeaderView::Stretch);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->expandColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->colorColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->opacityColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->transformVisibilityColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->planesVisibilityColumn(), QHeaderView::ResizeToContents);

  d->ModelHierarchyTreeView->sortFilterProxyModel()->setHideChildNodeTypes(d->HideChildNodeTypes);
  d->ModelHierarchyTreeView->sortFilterProxyModel()->invalidate();

  QIcon loadIcon =
    qSlicerApplication::application()->style()->standardIcon(QStyle::SP_DialogOpenButton);
  d->BrainReferenceOpenButton->setIcon(loadIcon);
  d->TemplateReferenceOpenButton->setIcon(loadIcon);

  // Connect
  this->connect(
    d->ModelHierarchyNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(setCurrentNode(vtkMRMLNode*)));
  this->connect(
    d->ModelHierarchyNodeComboBox, SIGNAL(nodeAboutToBeRemoved(vtkMRMLNode*)),
    this, SLOT(onCurrentNodeAboutToBeRemoved(vtkMRMLNode*)));

  this->connect(
    d->BrainReferenceNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(updateBrainReferenceNode(vtkMRMLNode*)));
  this->connect(
    d->TemplateReferenceNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(updateTemplateReferenceNode(vtkMRMLNode*)));
  
  this->connect(
    d->CurrentCutNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(updateCurrentCutNode(vtkMRMLNode*)));
  this->connect(
    d->CurrentBendNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(updateCurrentBendNode(vtkMRMLNode*)));
  this->connect(
    d->CutPreviewButton, SIGNAL(clicked()), this, SLOT(previewCutButtonClicked()));
  this->connect(
    d->CutConfirmButton, SIGNAL(clicked()), this, SLOT(confirmCutButtonClicked()));
  this->connect(
    d->CutCancelButton, SIGNAL(clicked()), this, SLOT(cancelCutButtonClicked()));

  this->connect(d->ComputeMetricsButton, SIGNAL(clicked()), this, SLOT(onComputeButton()));
  this->connect(d->SetPreOp, SIGNAL(clicked()), this, SLOT(onSetPreOP()));
  this->connect(
    d->FixedPointAButton, SIGNAL(clicked()), this, SLOT(placeFiducialButtonClicked()));
  this->connect(
    d->FixedPointBButton, SIGNAL(clicked()), this, SLOT(placeFiducialButtonClicked()));
  this->connect(
    d->MovingPointAButton, SIGNAL(clicked()), this, SLOT(placeFiducialButtonClicked()));
  this->connect(
    d->MovingPointBButton, SIGNAL(clicked()), this, SLOT(placeFiducialButtonClicked()));
  this->connect(
    d->InitButton, SIGNAL(clicked()), this, SLOT(initBendButtonClicked()));
  this->connect(
    d->UpdateBendButton, SIGNAL(clicked()), this, SLOT(updateBendButtonClicked()));
  this->connect(
    d->HardenBendButton, SIGNAL(clicked()), this, SLOT(finshBendClicked()));
  this->connect(
    d->CancelBendButton, SIGNAL(clicked()), this, SLOT(cancelBendButtonClicked()));
  this->connect(
    d->ComputeScalarsButton, SIGNAL(clicked()), this, SLOT(computeScalarsClicked()));
  this->connect(
    d->ShowsScalarsCheckbox, SIGNAL(stateChanged(int)), this, SLOT(updateMRMLFromWidget()));

  this->connect(
    d->BrainReferenceColorPickerButton, SIGNAL(colorChanged(QColor)),
    this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->TemplateReferenceColorPickerButton, SIGNAL(colorChanged(QColor)),
    this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->BrainReferenceOpacitySliderWidget, SIGNAL(valueChanged(double)),
    this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->TemplateReferenceOpacitySliderWidget, SIGNAL(valueChanged(double)),
    this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->BrainReferenceOpenButton, SIGNAL(clicked()),
    this, SLOT(onOpenBrainReference()));
  this->connect(
    d->TemplateReferenceOpenButton, SIGNAL(clicked()),
    this, SLOT(onOpenTemplateReference()));

  this->connect(d->BendMagnitudeSlider, SIGNAL(valueChanged(double)), this, SLOT(bendMagnitudeSliderUpdated()));

}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::setCurrentNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  vtkMRMLModelHierarchyNode* hNode = vtkMRMLModelHierarchyNode::SafeDownCast(node);
  d->HierarchyNode = hNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Superclass::setMRMLScene(scene);

  this->qvtkReconnect(
    this->mrmlScene(), vtkMRMLScene::NodeAddedEvent,
    this, SLOT(onNodeAddedEvent(vtkObject*, vtkObject*)));

  this->qvtkReconnect(
    this->mrmlScene(), vtkMRMLScene::NodeRemovedEvent,
    this, SLOT(onNodeRemovedEvent(vtkObject*, vtkObject*)));
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget
::onNodeAddedEvent(vtkObject* scene, vtkObject* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  Q_UNUSED(scene);
  vtkMRMLModelHierarchyNode* hNode =
    vtkMRMLModelHierarchyNode::SafeDownCast(node);
  if (!hNode || hNode->GetHideFromEditors())
    {
    return;
    }

  // OnNodeAddedEvent is here to make sure that the combobox is populated
  // too after a node is added to the scene, because the tree view will be
  // and they need to match.
  if (!d->HierarchyNode)
    {
    if (this->mrmlScene()->IsBatchProcessing())
      {
      // Problem is, during a batch processing, the model is yet up-to-date.
      // So we wait for the sceneUpdated() signal and then do the update.
      d->StagedHierarchyNode = hNode;
      this->connect(
        d->ModelHierarchyTreeView->sceneModel(), SIGNAL(sceneUpdated()),
        this, SLOT(onSceneUpdated()));
      }
    else
      {
      // No problem, just do the update directly.
      this->setCurrentNode(hNode);
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget
::onNodeRemovedEvent(vtkObject* sceneObject, vtkObject* nodeObject)
{
  Q_D(qSlicerPlannerModuleWidget);
  vtkMRMLScene* scene = vtkMRMLScene::SafeDownCast(sceneObject);
  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(nodeObject);

  d->removeTransformNode(scene, node);
  d->removePlaneNode(scene, node);

  vtkMRMLLinearTransformNode* transform =
    vtkMRMLLinearTransformNode::SafeDownCast(node);
  if (transform && !this->mrmlScene()->IsClosing())
    {
    this->updateWidgetFromMRML();
    }
  
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::onSceneUpdated()
{
  Q_D(qSlicerPlannerModuleWidget);
  this->disconnect(this, SLOT(onSceneUpdated()));
  if (!d->HierarchyNode && d->StagedHierarchyNode != d->HierarchyNode)
    {
    this->setCurrentNode(d->StagedHierarchyNode);
    d->StagedHierarchyNode = NULL;
    }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::onCurrentNodeAboutToBeRemoved(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  vtkMRMLModelHierarchyNode* hierarchy =
    vtkMRMLModelHierarchyNode::SafeDownCast(node);
  if (hierarchy && hierarchy == d->HierarchyNode)
    {
    d->fireDeleteChildrenWarning();
    this->plannerLogic()->DeleteHierarchyChildren(hierarchy);
    }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget
::updateWidgetFromMRML(vtkObject* obj1, vtkObject* obj2)
{
  Q_UNUSED(obj1);
  Q_UNUSED(obj2);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerPlannerModuleWidget);
  
  // Inputs
  d->ModelHierarchyNodeComboBox->setCurrentNode(d->HierarchyNode);
  d->ModelHierarchyTreeView->setEnabled(d->HierarchyNode != NULL);
  d->ModelHierarchyTreeView->setRootNode(d->HierarchyNode);
  d->ModelHierarchyTreeView->setCurrentNode(d->HierarchyNode);
  d->ModelHierarchyTreeView->expandAll();

  // Create all the transforms for the current hierarchy node
  //must do first so that there are available for the planes nodes
  d->createTransformsIfNecessary(this->mrmlScene(), d->HierarchyNode);

  // Create the plane node for the current hierarchy node
  d->createPlanesIfNecessary(this->mrmlScene(), d->HierarchyNode);

  //Button enable/disable logic
  //Cutting Intialization
  if (d->CurrentCutNode != NULL)
  {
    d->CutPreviewButton->setEnabled(true);
    d->CutPreviewButton->setText("Preview Cut");
    
  }
  else
  {
    d->CutPreviewButton->setEnabled(false);
  }

  //Active cutting
  if (d->cuttingActive)
  {
    d->CutConfirmButton->setEnabled(true);
    d->CutCancelButton->setEnabled(true);
    d->CurrentCutNodeComboBox->setEnabled(false);
    d->CutPreviewButton->setText("Adjust cut");
    d->BendingCollapsibleButton->setEnabled(false);
  }
  else
  {
    d->CutConfirmButton->setEnabled(false);
    d->CutCancelButton->setEnabled(false);
    d->CurrentCutNodeComboBox->setEnabled(true);
    d->BendingCollapsibleButton->setEnabled(true);
  }

  //Active bending
  if (d->bendingActive)
  {
    d->CuttingCollapsibleButton->setEnabled(false);
    d->CurrentBendNodeComboBox->setEnabled(false);
    d->UpdateBendButton->setEnabled(true);
    d->BendMagnitudeSlider->setEnabled(true);
    d->CancelBendButton->setEnabled(true);
  }
  else
  {
    d->CuttingCollapsibleButton->setEnabled(true);
    d->CurrentBendNodeComboBox->setEnabled(true);
    d->UpdateBendButton->setEnabled(false);
    d->BendMagnitudeSlider->setEnabled(false);
    d->HardenBendButton->setEnabled(false);
    d->CancelBendButton->setEnabled(true);
  }

  //Pre-op state
  if (this->plannerLogic()->getPreOPICV() == 0)
  {
    d->SetPreOp->setText(QString("Need to set Pre Op State! - click"));
  }
  else
  {
    d->SetPreOp->setText("Pre Op State set - click to reset!");
  }

  d->updateWidgetFromReferenceNode(
    d->BrainReferenceNodeComboBox->currentNode(),
    d->BrainReferenceColorPickerButton,
    d->BrainReferenceOpacitySliderWidget);
  d->updateWidgetFromReferenceNode(
    d->TemplateReferenceNodeComboBox->currentNode(),
    d->TemplateReferenceColorPickerButton,
    d->TemplateReferenceOpacitySliderWidget);

  //Bending init button
  if (d->FixedPointA && d->FixedPointB && d->MovingPointA && d->MovingPointB && d->CurrentBendNode)
  {
    d->InitButton->setEnabled(true);
  }
  else
  {
    d->InitButton->setEnabled(false);
  }

}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateBrainReferenceNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  this->qvtkReconnect(d->BrainReferenceNode, node,
    vtkCommand::ModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  this->qvtkReconnect(d->BrainReferenceNode, node,
    vtkMRMLDisplayableNode::DisplayModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  
  if (node && node != d->BrainReferenceNode)
    {
    d->cmdNode =  this->plannerLogic()->createHealthyBrainModel(vtkMRMLModelNode::SafeDownCast(node));
    qvtkConnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(finishWrap()));
    d->MetricsProgress->setCommandLineModuleNode(d->cmdNode);
    }
  d->BrainReferenceNode = node;
  this->updateMRMLFromWidget();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateCurrentCutNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  this->qvtkReconnect(d->CurrentCutNode, node,
    vtkCommand::ModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  this->qvtkReconnect(d->CurrentCutNode, node,
    vtkMRMLDisplayableNode::DisplayModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  d->CurrentCutNode = node;
  
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateTemplateReferenceNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  this->qvtkReconnect(d->TemplateReferenceNode, node, vtkCommand::ModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  this->qvtkReconnect(d->TemplateReferenceNode, node,
    vtkMRMLDisplayableNode::DisplayModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  d->TemplateReferenceNode = node;

  this->updateMRMLFromWidget();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateMRMLFromWidget()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->updateReferenceNodeFromWidget(
    d->BrainReferenceNodeComboBox->currentNode(),
    d->BrainReferenceColorPickerButton->color(),
    d->BrainReferenceOpacitySliderWidget->value());
  d->updateReferenceNodeFromWidget(
    d->TemplateReferenceNodeComboBox->currentNode(),
    d->TemplateReferenceColorPickerButton->color(),
    d->TemplateReferenceOpacitySliderWidget->value());

  //set visibility on scalars
  if (d->HierarchyNode)
  {
    d->setScalarVisibility(d->ShowsScalarsCheckbox->isChecked());
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::onOpenBrainReference()
{
  Q_D(qSlicerPlannerModuleWidget);
  vtkMRMLNode* model = d->openReferenceDialog();
  if (model)
    {
    d->BrainReferenceNodeComboBox->setCurrentNode(model);
    }
}
  
//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::onOpenTemplateReference()
{
  Q_D(qSlicerPlannerModuleWidget);
  vtkMRMLNode* model = d->openReferenceDialog();
  if (model)
    {
    d->TemplateReferenceNodeComboBox->setCurrentNode(model);
    }
}

//-----------------------------------------------------------------------------
//View current cut results
void qSlicerPlannerModuleWidget::previewCutButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);

  if (d->cuttingActive)
  {
    d->adjustCut(this->mrmlScene());
  }
  else
  {
    d->cuttingActive = true;
    d->previewCut(this->mrmlScene());
    
  }
  this->updateWidgetFromMRML();
  d->sceneModel()->setPlaneVisibility(d->CurrentCutNode, true);
}

//-----------------------------------------------------------------------------
//Finish and harden current cutting action
void qSlicerPlannerModuleWidget::confirmCutButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->completeCut(this->mrmlScene());
  d->cuttingActive = false;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Cancel current cutting action
void qSlicerPlannerModuleWidget::cancelCutButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->cancelCut(this->mrmlScene());
  d->cuttingActive = false;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Activate the placing of a specific fiducial
void qSlicerPlannerModuleWidget::placeFiducialButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  QObject* obj = sender();
  
  if (obj == d->FixedPointAButton)
  {
    std::cout << "FixedPointAButton" << std::endl;
    d->beginPlacement(this->mrmlScene(), 0);
  }
  if (obj == d->FixedPointBButton)
  {
    std::cout << "FixedPointBButton" << std::endl;
    d->beginPlacement(this->mrmlScene(), 1);
  }
  if (obj == d->MovingPointAButton)
  {
    std::cout << "MovingPointButton" << std::endl;
    d->beginPlacement(this->mrmlScene(), 2);
  }
  if (obj == d->MovingPointBButton)
  {
    std::cout << "MovingPointButton" << std::endl;
    d->beginPlacement(this->mrmlScene(), 3);
  }

  qvtkConnect(qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode(), vtkMRMLInteractionNode::EndPlacementEvent, this, SLOT(cancelFiducialButtonClicked()));
  return;
}

//-----------------------------------------------------------------------------
//Cancel the current bend and reset
void qSlicerPlannerModuleWidget::cancelBendButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->BendMagnitude = 0;
  d->computeTransform(this->mrmlScene());
  d->clearControlPoints(this->mrmlScene());
  d->clearBendingData();
  d->bendingActive = false;
  this->updateWidgetFromMRML();
  
}

//-----------------------------------------------------------------------------
//Update active model for bending from combobox
void qSlicerPlannerModuleWidget::updateCurrentBendNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  this->qvtkReconnect(d->CurrentBendNode, node,
    vtkCommand::ModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  this->qvtkReconnect(d->CurrentBendNode, node,
    vtkMRMLDisplayableNode::DisplayModifiedEvent,
    this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));

  d->CurrentBendNode = node;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Initialize bend transform based on current fiducials
void qSlicerPlannerModuleWidget::initBendButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->computeAndSetSourcePoints();
  d->bendingActive = true;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Update displayed bend based on current magnitude
void qSlicerPlannerModuleWidget::updateBendButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->computeTransform(this->mrmlScene());
  d->HardenBendButton->setEnabled(true);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Update value for the bend magnitude
void qSlicerPlannerModuleWidget::bendMagnitudeSliderUpdated()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->BendMagnitude = d->BendMagnitudeSlider->value();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Finish and harden current bending action
void qSlicerPlannerModuleWidget::finshBendClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->hardenTransforms();
  d->clearBendingData();  
  d->bendingActive = false;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Cancel placement of current fiducial
void qSlicerPlannerModuleWidget::cancelFiducialButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->endPlacement();
  this->updateWidgetFromMRML();

}

//-----------------------------------------------------------------------------
//Begin wrapping of current model for metrics
void qSlicerPlannerModuleWidget::onComputeButton()
{
  Q_D(qSlicerPlannerModuleWidget);

  if (!d->modelMetricsTable)
  {
    vtkNew<vtkMRMLTableNode> table2;
    d->modelMetricsTable = table2.GetPointer();
    this->mrmlScene()->AddNode(d->modelMetricsTable);
    d->ModelMetrics->setMRMLTableNode(d->modelMetricsTable);
  }

  if (d->HierarchyNode)
  {
    std::vector<vtkMRMLHierarchyNode*> children;

    d->HierarchyNode->GetAllChildrenNodes(children);
    if (children.size() > 0)
    {
      d->hardenTransforms();
      std::cout << "Wrapping Current Model" << std::endl;
      d->cmdNode = this->plannerLogic()->createCurrentModel(d->HierarchyNode);
      qvtkConnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(launchMetrics()));
      d->MetricsProgress->setCommandLineModuleNode(d->cmdNode);
    }
  }
}

//-----------------------------------------------------------------------------
//Create a pre-op model from the wrap of the current state
void qSlicerPlannerModuleWidget::onSetPreOP()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->HierarchyNode)
  {
    std::vector<vtkMRMLHierarchyNode*> children;

    d->HierarchyNode->GetAllChildrenNodes(children);
    if (children.size() > 0)
    {
      d->cmdNode = this->plannerLogic()->createPreOPModels(d->HierarchyNode);
      qvtkConnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(finishWrap()));

      d->MetricsProgress->setCommandLineModuleNode(d->cmdNode);
    }
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Launch the computation of distance scalars for the current hierarchy
void qSlicerPlannerModuleWidget::computeScalarsClicked()
{
  //launch scalar computation
  Q_D(qSlicerPlannerModuleWidget);
  if (d->HierarchyNode && d->BrainReferenceNode)
  {
    d->hardenTransforms();
    d->ComputeScalarsButton->setEnabled(false);
    d->prepScalarComputation(this->mrmlScene());
    d->cmdNode = d->distanceLogic->CreateNodeInScene();
    this->runModelDistance();
  }
}

//-----------------------------------------------------------------------------
//Catch end of model to model CLI and run next
void qSlicerPlannerModuleWidget::finishDistance()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->cmdNode->GetStatus() == vtkMRMLCommandLineModuleNode::Completed)
  {
    vtkMRMLModelNode* distanceNode = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetNodeByID(d->cmdNode->GetParameterAsString("vtkOutput")));
    int m = d->modelIterator.back()->StartModify();
    d->modelIterator.back()->SetAndObservePolyData(distanceNode->GetPolyData());
    d->modelIterator.back()->EndModify(m);
    this->mrmlScene()->RemoveNode(distanceNode);
    distanceNode = NULL;
    d->modelIterator.pop_back();
    this->runModelDistance();
  }
}

//-----------------------------------------------------------------------------
//Set up model to model distance CLI for next model in list
void qSlicerPlannerModuleWidget::runModelDistance()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (!d->modelIterator.empty())
  {
    d->distanceLogic->SetMRMLScene(this->mrmlScene());
    vtkNew<vtkMRMLModelNode> temp;
    this->mrmlScene()->AddNode(temp.GetPointer());
    d->cmdNode->SetParameterAsString("vtkFile1", d->modelIterator.back()->GetID());
    d->cmdNode->SetParameterAsString("vtkFile2", d->BrainReferenceNode->GetID());
    d->cmdNode->SetParameterAsString("vtkOutput", temp->GetID());
    d->cmdNode->SetParameterAsString("distanceType", "closest_point");
    d->distanceLogic->Apply(d->cmdNode, true);
    qvtkConnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(finishDistance()));
    d->MetricsProgress->setCommandLineModuleNode(d->cmdNode);
  }
  else
  {
    d->ComputeScalarsButton->setEnabled(true);
    this->mrmlScene()->RemoveNode(d->cmdNode);
    this->updateMRMLFromWidget();
  }
}

//-----------------------------------------------------------------------------
//Catch the end of the WrapModels CLI and clean up
void qSlicerPlannerModuleWidget::finishWrap()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->cmdNode->GetStatus() == vtkMRMLCommandLineModuleNode::Completed)
  {
    this->plannerLogic()->finishWrap(d->cmdNode);
    this->updateWidgetFromMRML();
  }
}

//-----------------------------------------------------------------------------
//Catch end of metrics wrap and being metrics computation
void qSlicerPlannerModuleWidget::launchMetrics()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->cmdNode->GetStatus() == vtkMRMLCommandLineModuleNode::Completed)
  {
    this->plannerLogic()->finishWrap(d->cmdNode);
    this->plannerLogic()->fillMetricsTable(d->HierarchyNode, d->modelMetricsTable);
    this->updateWidgetFromMRML();
  }
}
