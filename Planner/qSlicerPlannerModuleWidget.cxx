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
#include <vtkMath.h>


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

// Slicer includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include "qSlicerAbstractCoreModule.h"
#include <qSlicerCLIModule.h>

// CropVolume Logic includes
#include <vtkSlicerCLIModuleLogic.h>

// Self
#include "qMRMLPlannerModelHierarchyModel.h"
#include "vtkSlicerPlannerLogic.h"

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
  vtkMRMLMarkupsPlanesNode* createPlanesNode(vtkMRMLScene* scene);
  void updatePlanesFromModel(vtkMRMLModelNode*, int) const;

  qMRMLPlannerModelHierarchyModel* sceneModel() const;
  void previewCut(vtkMRMLScene* scene);
  void adjustCut(vtkMRMLScene* scene);
  void completeCut(vtkMRMLScene* scene);
  void cancelCut(vtkMRMLScene* scene);
  void deleteModel(vtkMRMLModelNode* node, vtkMRMLScene* scene);
  void splitModel(vtkMRMLModelNode* inputNode, vtkMRMLModelNode* split1, vtkMRMLModelNode* split2, vtkMRMLScene* scene);
  int getPlaneIndex(vtkMRMLModelNode* node, vtkMRMLMarkupsPlanesNode* planes);
  void applyRandomColor(vtkMRMLModelNode* node);

  void updateWidgetFromReferenceNode(
    vtkMRMLNode* node,
    ctkColorPickerButton* button,
    qMRMLSliderWidget* slider) const;
  void updateReferenceNodeFromWidget(
    vtkMRMLNode* node, QColor color, double opacity) const;
  vtkMRMLNode* openReferenceDialog() const;

  vtkMRMLModelHierarchyNode* HierarchyNode;
  vtkMRMLModelHierarchyNode* StagedHierarchyNode;
  QStringList HideChildNodeTypes;
  vtkMRMLNode* BrainReferenceNode;
  vtkMRMLNode* TemplateReferenceNode;
  vtkMRMLNode* CurrentCutNode;
  vtkMRMLNode* StagedCutNode1;
  vtkMRMLNode* StagedCutNode2;
  vtkMRMLMarkupsPlanesNode* PlanesNode;
  bool cuttingActive;
};

//-----------------------------------------------------------------------------
// qSlicerPlannerModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
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
  this->PlanesNode = NULL;
  this->cuttingActive = false;
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
void qSlicerPlannerModuleWidgetPrivate::createPlanesIfNecessary(
  vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if (!hierarchy)
    {
    return;
    }

  if (!this->PlanesNode)
    {
    this->PlanesNode = this->createPlanesNode(scene);
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
      models.push_back(childModel);
      }
    }

  if (this->PlanesNode->GetNumberOfMarkups() > models.size())
    {
    this->PlanesNode->RemoveAllMarkups();
    }
  for (size_t i = 0; i < models.size(); ++i)
    {
    this->updatePlanesFromModel(models[i], i);
    }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::updatePlanesFromModel(
  vtkMRMLModelNode* model, int count) const
{
  if (!model)
    {
    return;
    }

  model->SetNodeReferenceID(
    this->sceneModel()->planesReferenceRole(), this->PlanesNode->GetID());

  if (this->PlanesNode->GetNumberOfMarkups() > count)
    {
    return;
    }

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

  this->PlanesNode->AddPlaneFromArray(origin, normal, min, max);
  this->PlanesNode->SetNthMarkupVisibility(count, false);
  this->PlanesNode->SetNthMarkupAssociatedNodeID(count, model->GetID());
}

//-----------------------------------------------------------------------------
vtkMRMLMarkupsPlanesNode* qSlicerPlannerModuleWidgetPrivate
::createPlanesNode(vtkMRMLScene* scene)
{
  Q_ASSERT(scene);
  vtkNew<vtkMRMLMarkupsPlanesNode> newPlanes;
  vtkMRMLMarkupsPlanesNode* planes =
    vtkMRMLMarkupsPlanesNode::SafeDownCast(
      scene->AddNode(newPlanes.GetPointer()));
  QString planesName = "Planner_Planes";
  planes->SetName(planesName.toLatin1());

  vtkNew<vtkMRMLMarkupsDisplayNode> newDisplay;
  vtkMRMLNode* display = scene->AddNode(newDisplay.GetPointer());
  planes->SetAndObserveDisplayNodeID(display->GetID());

  return planes;
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


//split into create child models and splitModel
void qSlicerPlannerModuleWidgetPrivate::previewCut(vtkMRMLScene* scene)
{
  if (!this->HierarchyNode)
  {
    this->cuttingActive = false;
    return;
  }

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
  //dnode1->SetVisibility(1);
  //dnode2->SetVisibility(1);
  

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

//move or combine with previewCut
void qSlicerPlannerModuleWidgetPrivate::adjustCut(vtkMRMLScene* scene)
{
  this->splitModel(vtkMRMLModelNode::SafeDownCast(CurrentCutNode), vtkMRMLModelNode::SafeDownCast(StagedCutNode1),
    vtkMRMLModelNode::SafeDownCast(StagedCutNode2), scene);
}

//move
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

//move
void qSlicerPlannerModuleWidgetPrivate::cancelCut(vtkMRMLScene* scene)
{
  this->deleteModel(vtkMRMLModelNode::SafeDownCast(this->StagedCutNode1), scene);
  this->deleteModel(vtkMRMLModelNode::SafeDownCast(this->StagedCutNode2), scene);
}

//move
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
    scene->RemoveNode(node);
  }
  node = NULL;
}

//move
void qSlicerPlannerModuleWidgetPrivate::splitModel(vtkMRMLModelNode* inputNode, vtkMRMLModelNode* split1, vtkMRMLModelNode* split2, vtkMRMLScene* scene)
{
  qSlicerAbstractCoreModule* splitModule =
    qSlicerCoreApplication::application()->moduleManager()->module("SplitModel");

  vtkSlicerCLIModuleLogic* splitLogic =
    vtkSlicerCLIModuleLogic::SafeDownCast(splitModule->logic());

  splitLogic->SetMRMLScene(scene);

  //Fails in this section
  vtkMRMLCommandLineModuleNode* cmdNode = splitLogic->CreateNodeInScene();
  int planeIndex = this->getPlaneIndex(inputNode, this->PlanesNode);

  double normal[3];
  double origin[3];
  PlanesNode->GetNthPlaneNormal(planeIndex, normal);
  PlanesNode->GetNthPlaneOrigin(planeIndex, origin);


  cmdNode->SetParameterAsString("Model",inputNode->GetID());
  cmdNode->SetParameterAsString("ModelOutput1", split1->GetID());
  cmdNode->SetParameterAsString("ModelOutput2", split2->GetID());

  std::stringstream originStream;
  originStream << std::setprecision(15) << origin[0] << "," << origin[1] << "," << origin[2];
  cmdNode->SetParameterAsString("Origin", originStream.str());

  std::stringstream normalStream;
  normalStream << std::setprecision(15) << normal[0] << "," << normal[1] << "," << normal[2];
  cmdNode->SetParameterAsString("Normal", normalStream.str());


  splitLogic->ApplyAndWait(cmdNode, false);
  scene->RemoveNode(cmdNode);
}

//move
int qSlicerPlannerModuleWidgetPrivate::getPlaneIndex(vtkMRMLModelNode* node, vtkMRMLMarkupsPlanesNode* planes)
{
  for (int i = 0; i < planes->GetNumberOfMarkups(); ++i)
  {
    if (planes->GetNthMarkupAssociatedNodeID(i).compare(node->GetID()) == 0)
    {
      return i;
    }
  }
  return -1;
}

//move
void qSlicerPlannerModuleWidgetPrivate::applyRandomColor(vtkMRMLModelNode* model)
{
  //vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);
  if (model)
  {
    vtkMRMLDisplayNode* display = model->GetDisplayNode();
    if (display)
    {
      int wasModifying = display->StartModify();
      display->SetColor(vtkMath::Random(0.0, 1.0), vtkMath::Random(0.0, 1.0), vtkMath::Random(0.0, 1.0));
      display->EndModify(wasModifying);
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
    d->CutPreviewButton, SIGNAL(clicked()), this, SLOT(previewButtonClicked()));
  this->connect(
    d->CutConfirmButton, SIGNAL(clicked()), this, SLOT(confirmButtonClicked()));
  this->connect(
    d->CutCancelButton, SIGNAL(clicked()), this, SLOT(cancelButtonClicked()));

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

  vtkMRMLLinearTransformNode* transform =
    vtkMRMLLinearTransformNode::SafeDownCast(node);
  if (transform && !this->mrmlScene()->IsClosing())
    {
    this->updateWidgetFromMRML();
    }
  else if (node == d->PlanesNode)
    {
    d->PlanesNode = NULL;
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
  d->createTransformsIfNecessary(this->mrmlScene(), d->HierarchyNode);

  // Create the plane node for the current hierarchy node
  d->createPlanesIfNecessary(this->mrmlScene(), d->HierarchyNode);
  if (d->CurrentCutNode != NULL)
  {
    d->CutPreviewButton->setEnabled(true);
    d->CutPreviewButton->setText("Preview Cut");
    
  }
  else
  {
    d->CutPreviewButton->setEnabled(false);
  }

  if (d->cuttingActive)
  {
    d->CutConfirmButton->setEnabled(true);
    d->CutCancelButton->setEnabled(true);
    d->CurrentCutNodeComboBox->setEnabled(false);
    d->CutPreviewButton->setText("Adjust cut");
  }
  else
  {
    d->CutConfirmButton->setEnabled(false);
    d->CutCancelButton->setEnabled(false);
    d->CurrentCutNodeComboBox->setEnabled(true);
  }


  d->updateWidgetFromReferenceNode(
    d->BrainReferenceNodeComboBox->currentNode(),
    d->BrainReferenceColorPickerButton,
    d->BrainReferenceOpacitySliderWidget);
  d->updateWidgetFromReferenceNode(
    d->TemplateReferenceNodeComboBox->currentNode(),
    d->TemplateReferenceColorPickerButton,
    d->TemplateReferenceOpacitySliderWidget);

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

void qSlicerPlannerModuleWidget::previewButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->cuttingActive)
  {
    d->adjustCut(this->mrmlScene());
  }
  else
  {
    d->previewCut(this->mrmlScene());
    d->cuttingActive = true;
  }
  this->updateWidgetFromMRML();
}

void qSlicerPlannerModuleWidget::confirmButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->completeCut(this->mrmlScene());
  d->cuttingActive = false;
  this->updateWidgetFromMRML();
}

void qSlicerPlannerModuleWidget::cancelButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->cancelCut(this->mrmlScene());
  d->cuttingActive = false;
  this->updateWidgetFromMRML();
}
