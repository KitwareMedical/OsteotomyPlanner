/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QStyle>

#include <QColor>

// Slicer includes
#include "qSlicerApplication.h"
#include "qMRMLPlannerModelHierarchyModel.h"

// MRML includes
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLDisplayableHierarchyNode.h>
#include <vtkMRMLMarkupsPlaneNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLTransformDisplayNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLTransformableNode.h>

//------------------------------------------------------------------------------
class qMRMLPlannerModelHierarchyModelPrivate
{
public:
  qMRMLPlannerModelHierarchyModelPrivate();

  bool hasTransformableNodeChildren(vtkMRMLNode* node) const;

  vtkMRMLTransformDisplayNode* transformDisplayNode(
    vtkMRMLScene* scene, vtkMRMLNode* node) const;
  vtkMRMLMarkupsPlaneNode* planesNode(
    vtkMRMLScene* scene, vtkMRMLNode* node) const;

  int TransformVisibilityColumn;
  int PlanesVisibilityColumn;
  int CutButtonColumn;
  int BendButtonColumn;
};

//------------------------------------------------------------------------------
qMRMLPlannerModelHierarchyModelPrivate::qMRMLPlannerModelHierarchyModelPrivate()
{
  this->TransformVisibilityColumn = -1;
  this->PlanesVisibilityColumn = -1;
  this->CutButtonColumn = -1;
  this->BendButtonColumn = -1;
}

//------------------------------------------------------------------------------
vtkMRMLTransformDisplayNode* qMRMLPlannerModelHierarchyModelPrivate
::transformDisplayNode(vtkMRMLScene* scene, vtkMRMLNode* node) const
{
  if(!node)
  {
    return NULL;
  }

  vtkMRMLTransformDisplayNode* display =
    vtkMRMLTransformDisplayNode::SafeDownCast(node->GetNodeReference(
          qMRMLPlannerModelHierarchyModel::transformDisplayReferenceRole()));
  if(!display)
  {
    node->SetNodeReferenceID(
      qMRMLPlannerModelHierarchyModel::transformDisplayReferenceRole(), NULL);
  }
  return display;
}

//------------------------------------------------------------------------------
vtkMRMLMarkupsPlaneNode* qMRMLPlannerModelHierarchyModelPrivate
::planesNode(vtkMRMLScene* scene, vtkMRMLNode* node) const
{
  if(!node)
  {
    return NULL;
  }

  vtkMRMLMarkupsPlaneNode* planes =
    vtkMRMLMarkupsPlaneNode::SafeDownCast(node->GetNodeReference(
          qMRMLPlannerModelHierarchyModel::planesReferenceRole()));
  if(!planes)
  {
    node->SetNodeReferenceID(
      qMRMLPlannerModelHierarchyModel::planesReferenceRole(), NULL);
  }
  return planes;
}

//------------------------------------------------------------------------------
bool qMRMLPlannerModelHierarchyModelPrivate
::hasTransformableNodeChildren(vtkMRMLNode* node) const
{
  vtkMRMLDisplayableHierarchyNode* displayableHierarchyNode
    = vtkMRMLDisplayableHierarchyNode::SafeDownCast(node);
  if(displayableHierarchyNode)
  {
    std::vector<vtkMRMLHierarchyNode*> children;
    displayableHierarchyNode->GetAllChildrenNodes(children);
    std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
    for(it = children.begin(); it != children.end(); ++it)
    {
      if(vtkMRMLTransformableNode::SafeDownCast((*it)->GetAssociatedNode()))
      {
        return true;
      }
    }
  }
  return false;
}

//----------------------------------------------------------------------------
//------------------------------------------------------------------------------
qMRMLPlannerModelHierarchyModel::qMRMLPlannerModelHierarchyModel(QObject* vparent)
  : Superclass(vparent)
  , d_ptr(new qMRMLPlannerModelHierarchyModelPrivate)
  , activeNode(NULL)
{
}

//------------------------------------------------------------------------------
qMRMLPlannerModelHierarchyModel::~qMRMLPlannerModelHierarchyModel()
{
}

//------------------------------------------------------------------------------
const char* qMRMLPlannerModelHierarchyModel::transformDisplayReferenceRole()
{
  return "Planner/TransformDisplayID";
}

//------------------------------------------------------------------------------
const char* qMRMLPlannerModelHierarchyModel::planesReferenceRole()
{
  return "Planner/PlaneID";
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel::observeNode(vtkMRMLNode* node)
{
  // this->Superclass::observeNode(node);
  Q_D(const qMRMLPlannerModelHierarchyModel);

  if(node->IsA("vtkMRMLModelHierarchyNode") || node->IsA("vtkMRMLModelNode"))
  {
    qvtkConnect(node, vtkMRMLNode::ReferenceAddedEvent,
                this, SLOT(onReferenceChangedEvent(vtkObject*)));
    qvtkConnect(node, vtkMRMLNode::ReferenceModifiedEvent,
                this, SLOT(onReferenceChangedEvent(vtkObject*)));
    qvtkConnect(node, vtkMRMLNode::ReferenceRemovedEvent,
                this, SLOT(onReferenceChangedEvent(vtkObject*)));


  }
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel::onReferenceChangedEvent(vtkObject* object)
{
  Q_D(const qMRMLPlannerModelHierarchyModel);
  vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(object);

  vtkMRMLTransformDisplayNode* display =
    d->transformDisplayNode(this->mrmlScene(), vtkMRMLNode::SafeDownCast(object));
  if(display || (!display && !node))
  {
    qvtkConnect(display, vtkCommand::ModifiedEvent, this, SLOT(onMRMLNodeModified(vtkObject*)));
  }
  vtkMRMLMarkupsPlaneNode* markup =
    d->planesNode(this->mrmlScene(), vtkMRMLNode::SafeDownCast(object));
  if(markup || (!markup && !node))
  {
    qvtkConnect(markup, vtkCommand::ModifiedEvent, this, SLOT(onMRMLNodeModified(vtkObject*)));

  }
}

//------------------------------------------------------------------------------
QFlags<Qt::ItemFlag> qMRMLPlannerModelHierarchyModel
::subjectHierarchyItemFlags(vtkIdType itemID, int column)const
{
  Q_D(const qMRMLPlannerModelHierarchyModel);
  QFlags<Qt::ItemFlag> flags = this->Superclass::subjectHierarchyItemFlags(itemID, column);

  // consider using QStandardItem * itemFromSubjectHierarchyItem (vtkIdType itemID, int column=0) const
  // This is a patch to make things compile only.
  vtkMRMLNode* node;

  vtkMRMLTransformableNode* transformable = vtkMRMLTransformableNode::SafeDownCast(node);
  vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);

  if(column == this->transformVisibilityColumn() &&
      (transformable || d->hasTransformableNodeChildren(node)))
  {
    flags |= Qt::ItemIsUserCheckable;
  }
  else if(column == this->planesVisibilityColumn() && model)
  {
    flags |= Qt::NoItemFlags;
  }
  else if (column == this->cutButtonColumn() && model)
  {
      flags |= Qt::ItemIsUserCheckable;
  }
  else if (column == this->bendButtonColumn() && model)
  {
	  flags |= Qt::ItemIsUserCheckable;
  }
  return flags;
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel
::updateItemDataFromSubjectHierarchyItem(QStandardItem* item, vtkIdType shItemID, int column)
{
  Q_D(qMRMLPlannerModelHierarchyModel);

  // consider using QStandardItem * itemFromSubjectHierarchyItem (vtkIdType itemID, int column=0) const
  // This is a patch to make things compile only.
  vtkMRMLNode* node;

  vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);
  if(column == this->transformVisibilityColumn())
  {
    vtkMRMLTransformDisplayNode* display =
      d->transformDisplayNode(this->mrmlScene(), node);
    if(display)
    {
      item->setToolTip("Transform");
      item->setCheckState(
        display->GetEditorVisibility() ? Qt::Checked : Qt::Unchecked);
    }
  }
  else if(column == this->planesVisibilityColumn())
  {
    vtkMRMLMarkupsPlaneNode* planes = d->planesNode(this->mrmlScene(), node);
    if(planes)
    {
      
      bool visible = false;
      for(int i = 0; i < planes->GetNumberOfControlPoints(); ++i)
      {
        if(planes->GetNthControlPointAssociatedNodeID(i).compare(node->GetID()) == 0)
        {
          visible = planes->GetNthControlPointVisibility(i);
          break;
        }
      }
      item->setToolTip("Show cutting plane");
      item->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
    }
  }
  
  this->Superclass::updateItemDataFromSubjectHierarchyItem(item, shItemID, column);
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel
::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  Q_D(qMRMLPlannerModelHierarchyModel);
  if(item->column() == this->transformVisibilityColumn())
  {
    vtkMRMLTransformDisplayNode* display =
      d->transformDisplayNode(this->mrmlScene(), node);
    if(display)
    {
      display->SetEditorVisibility(
        item->checkState() == Qt::Checked ? true : false);
      display->UpdateEditorBounds();

      vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);
      if(model)
      {
        vtkMRMLDisplayNode* display = model->GetDisplayNode();
        if(display)
        {
          int wasModifying = display->StartModify();

          if(item->checkState() == Qt::Checked ? true : false)
          {
            display->SetOpacity(0.5);
            emit transformOn(node);
          }
          else
          {
            display->SetOpacity(1);

          }
          display->EndModify(wasModifying);
        }
      }
    }
  }
  else if(item->column() == this->planesVisibilityColumn())
  {
    vtkMRMLMarkupsPlaneNode* planes = d->planesNode(this->mrmlScene(), node);
    if(planes)
    {
      for(int i = 0; i < planes->GetNumberOfControlPoints(); ++i)
      {
        if(planes->GetNthControlPointAssociatedNodeID(i).compare(node->GetID()) == 0)
        {
          planes->SetNthControlPointVisibility(i, item->checkState() == Qt::Checked ? true : false);
          break;
        }
      }
    }
  }
  
  return this->Superclass::updateNodeFromItemData(node, item);
}

//------------------------------------------------------------------------------
int qMRMLPlannerModelHierarchyModel::transformVisibilityColumn()const
{
  Q_D(const qMRMLPlannerModelHierarchyModel);
  return d->TransformVisibilityColumn;
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel::setTransformVisibilityColumn(int column)
{
  Q_D(qMRMLPlannerModelHierarchyModel);
  d->TransformVisibilityColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLPlannerModelHierarchyModel::planesVisibilityColumn() const
{
  Q_D(const qMRMLPlannerModelHierarchyModel);
  return d->PlanesVisibilityColumn;
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel::setPlanesVisibilityColumn(int column)
{
  Q_D(qMRMLPlannerModelHierarchyModel);
  d->PlanesVisibilityColumn = column;
  this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLPlannerModelHierarchyModel::cutButtonColumn() const
{
    Q_D(const qMRMLPlannerModelHierarchyModel);
    return d->CutButtonColumn;
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel::setCutButtonColumn(int column)
{
    Q_D(qMRMLPlannerModelHierarchyModel);
    d->CutButtonColumn = column;
    this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLPlannerModelHierarchyModel::bendButtonColumn() const
{
    Q_D(const qMRMLPlannerModelHierarchyModel);
    return d->BendButtonColumn;
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel::setBendButtonColumn(int column)
{
    Q_D(qMRMLPlannerModelHierarchyModel);
    d->BendButtonColumn = column;
    this->updateColumnCount();
}

//------------------------------------------------------------------------------
int qMRMLPlannerModelHierarchyModel::maxColumnId()const
{
  Q_D(const qMRMLPlannerModelHierarchyModel);
  int maxId = this->Superclass::maxColumnId();
  maxId = qMax(maxId, d->TransformVisibilityColumn);
  maxId = qMax(maxId, d->PlanesVisibilityColumn);
  maxId = qMax(maxId, d->CutButtonColumn);
  maxId = qMax(maxId, d->BendButtonColumn);
  return maxId;
}

void qMRMLPlannerModelHierarchyModel::setPlaneVisibility(vtkMRMLNode* node, bool visible)
{
  Q_D(qMRMLPlannerModelHierarchyModel);
  //this->itemFromNode(node, d->PlanesVisibilityColumn)->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
  vtkMRMLMarkupsPlaneNode* planes = d->planesNode(this->mrmlScene(), node);
  planes->SetNthControlPointVisibility(0, visible);

}

void qMRMLPlannerModelHierarchyModel::setTransformVisibility(vtkMRMLNode* node, bool visible)
{
  Q_D(qMRMLPlannerModelHierarchyModel);
  this->itemFromNode(node, d->TransformVisibilityColumn)->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
  vtkMRMLTransformDisplayNode* transform = d->transformDisplayNode(this->mrmlScene(), node);
  transform->SetVisibility(visible);

}
