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

// SlicerQt includes
#include "qMRMLSceneModelHierarchyModel.h"
#include "qSlicerPlannerModuleWidget.h"
#include "ui_qSlicerPlannerModuleWidget.h"

// Slicer
#include "vtkMRMLModelHierarchyNode.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlannerModuleWidgetPrivate: public Ui_qSlicerPlannerModuleWidget
{
public:
  qSlicerPlannerModuleWidgetPrivate();

  vtkMRMLModelHierarchyNode* HierarchyNode;
  QStringList HideChildNodeTypes;
};

//-----------------------------------------------------------------------------
// qSlicerPlannerModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerPlannerModuleWidgetPrivate::qSlicerPlannerModuleWidgetPrivate()
{
  this->HideChildNodeTypes =
    (QStringList() << "vtkMRMLFiberBundleNode" << "vtkMRMLAnnotationNode");
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
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::setup()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->ModelHierarchyTreeView->setSelectionMode(QAbstractItemView::SingleSelection);
  d->ModelHierarchyTreeView->setShowScene(false);
  qMRMLSceneModelHierarchyModel* sceneModel =
    qobject_cast<qMRMLSceneModelHierarchyModel*>(
      d->ModelHierarchyTreeView->sceneModel());
  sceneModel->setIDColumn(-1);
  sceneModel->setExpandColumn(1);
  sceneModel->setColorColumn(2);
  sceneModel->setOpacityColumn(3);

  d->ModelHierarchyTreeView->header()->setStretchLastSection(false);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->nameColumn(), QHeaderView::Stretch);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->expandColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->colorColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->opacityColumn(), QHeaderView::ResizeToContents);

  d->ModelHierarchyTreeView->sortFilterProxyModel()->setHideChildNodeTypes(d->HideChildNodeTypes);
  d->ModelHierarchyTreeView->sortFilterProxyModel()->invalidate();

    // use lazy update instead of responding to scene import end event
  sceneModel->setLazyUpdate(true);

  // Connect
  this->connect(
    d->ModelHierarchyNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(setCurrentNode(vtkMRMLNode*)));
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::setCurrentNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  vtkMRMLModelHierarchyNode* hNode = vtkMRMLModelHierarchyNode::SafeDownCast(node);
  if (!hNode)
    {
    return;
    }
  d->HierarchyNode = hNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerPlannerModuleWidget);
  // Inputs
  d->ModelHierarchyNodeComboBox->setCurrentNode(d->HierarchyNode);

  if (d->HierarchyNode)
    {
    d->ModelHierarchyTreeView->setMRMLScene(this->mrmlScene());
    }
  d->ModelHierarchyTreeView->setRootNode(d->HierarchyNode);
}
