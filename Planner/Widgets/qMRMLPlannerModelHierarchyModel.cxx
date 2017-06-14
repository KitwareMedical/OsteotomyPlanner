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
#include <vtkMRMLMarkupsPlanesNode.h>
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
    vtkMRMLScene* scene,vtkMRMLNode* node) const;
  vtkMRMLMarkupsPlanesNode* planesNode(
    vtkMRMLScene* scene,vtkMRMLNode* node) const;

  int TransformVisibilityColumn;
  int PlanesVisibilityColumn;
};

//------------------------------------------------------------------------------
qMRMLPlannerModelHierarchyModelPrivate::qMRMLPlannerModelHierarchyModelPrivate()
{
  this->TransformVisibilityColumn = -1;
  this->PlanesVisibilityColumn = -1;
}

//------------------------------------------------------------------------------
vtkMRMLTransformDisplayNode* qMRMLPlannerModelHierarchyModelPrivate
::transformDisplayNode(vtkMRMLScene* scene, vtkMRMLNode* node) const
{
  if (!node)
    {
    return NULL;
    }

  vtkMRMLTransformDisplayNode* display =
    vtkMRMLTransformDisplayNode::SafeDownCast(node->GetNodeReference(
      qMRMLPlannerModelHierarchyModel::transformDisplayReferenceRole()));
  if (!display)
    {
    node->SetNodeReferenceID(
      qMRMLPlannerModelHierarchyModel::transformDisplayReferenceRole(), NULL);
    }
  return display;
}

//------------------------------------------------------------------------------
vtkMRMLMarkupsPlanesNode* qMRMLPlannerModelHierarchyModelPrivate
::planesNode(vtkMRMLScene* scene, vtkMRMLNode* node) const
{
  if (!node)
    {
    return NULL;
    }

  vtkMRMLMarkupsPlanesNode* planes =
    vtkMRMLMarkupsPlanesNode::SafeDownCast(node->GetNodeReference(
      qMRMLPlannerModelHierarchyModel::planesReferenceRole()));
  if (!planes)
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
  if (displayableHierarchyNode)
    {
    std::vector<vtkMRMLHierarchyNode*> children;
    displayableHierarchyNode->GetAllChildrenNodes(children);
    std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
    for (it = children.begin(); it != children.end(); ++it)
      {
      if (vtkMRMLTransformableNode::SafeDownCast((*it)->GetAssociatedNode()))
        {
        return true;
        }
      }
    }
  return false;
}

//----------------------------------------------------------------------------
//------------------------------------------------------------------------------
qMRMLPlannerModelHierarchyModel::qMRMLPlannerModelHierarchyModel(QObject *vparent)
  : Superclass(vparent)
  , d_ptr( new qMRMLPlannerModelHierarchyModelPrivate )
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
  return "Planner/PlanesID";
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel::observeNode(vtkMRMLNode* node)
{
  this->Superclass::observeNode(node);
  if (node->IsA("vtkMRMLModelHierarchyNode") || node->IsA("vtkMRMLModelNode"))
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
  if (display || (!display && !node))
    {
    qvtkConnect(display, vtkCommand::ModifiedEvent, this, SLOT(onMRMLNodeModified(vtkObject*)));
    }
  vtkMRMLMarkupsPlanesNode* markup =
    d->planesNode(this->mrmlScene(), vtkMRMLNode::SafeDownCast(object));
  if (markup || (!markup && !node))
    {
    qvtkConnect(markup, vtkCommand::ModifiedEvent, this, SLOT(onMRMLNodeModified(vtkObject*)));
    }
}

//------------------------------------------------------------------------------
QFlags<Qt::ItemFlag> qMRMLPlannerModelHierarchyModel
::nodeFlags(vtkMRMLNode* node, int column)const
{
  Q_D(const qMRMLPlannerModelHierarchyModel);
  QFlags<Qt::ItemFlag> flags = this->Superclass::nodeFlags(node, column);

  vtkMRMLTransformableNode* transformable = vtkMRMLTransformableNode::SafeDownCast(node);
  vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);

  if (column == this->transformVisibilityColumn() &&
    (transformable || d->hasTransformableNodeChildren(node)))
    {
    flags |= Qt::ItemIsUserCheckable;
    }
  else if (column == this->planesVisibilityColumn() && model)
    {
    flags |= Qt::ItemIsUserCheckable;
    }
  return flags;
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel
::updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column)
{
  Q_D(qMRMLPlannerModelHierarchyModel);

  if (column == this->transformVisibilityColumn())
    {
    vtkMRMLTransformDisplayNode* display =
      d->transformDisplayNode(this->mrmlScene(), node);
    if (display)
      {
      item->setToolTip("Transform");
      item->setCheckState(
        display->GetEditorVisibility() ? Qt::Checked : Qt::Unchecked);
      }
    }
  else if (column == this->planesVisibilityColumn())
    {
    vtkMRMLMarkupsPlanesNode* planes = d->planesNode(this->mrmlScene(), node);
    if (planes)
      {
      bool visible = false;
      for (int i = 0; i < planes->GetNumberOfMarkups(); ++i)
        {
        if (planes->GetNthMarkupAssociatedNodeID(i).compare(node->GetID()) == 0)
          {
          visible = planes->GetNthMarkupVisibility(i);
          break;
          }
        }
      item->setToolTip("Show cutting plane");
      item->setCheckState(visible ? Qt::Checked : Qt::Unchecked);
      }
    }
  this->Superclass::updateItemDataFromNode(item, node, column);
}

//------------------------------------------------------------------------------
void qMRMLPlannerModelHierarchyModel
::updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item)
{
  Q_D(qMRMLPlannerModelHierarchyModel);
  if (item->column() == this->transformVisibilityColumn())
    {
    vtkMRMLTransformDisplayNode* display =
      d->transformDisplayNode(this->mrmlScene(), node);
    if (display)
      {
      display->SetEditorVisibility(
        item->checkState() == Qt::Checked ? true : false);
      display->UpdateEditorBounds();
      
      vtkMRMLModelNode* model = vtkMRMLModelNode::SafeDownCast(node);
      if (model)
      {
        vtkMRMLDisplayNode* display = model->GetDisplayNode();
        if (display)
        {
          int wasModifying = display->StartModify();

          if (item->checkState() == Qt::Checked ? true : false)
          {
            display->SetOpacity(0.5);
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
  else if (item->column() == this->planesVisibilityColumn())
    {
    vtkMRMLMarkupsPlanesNode* planes = d->planesNode(this->mrmlScene(), node);
    if (planes)
      {
      for (int i = 0; i < planes->GetNumberOfMarkups(); ++i)
        {
        if (planes->GetNthMarkupAssociatedNodeID(i).compare(node->GetID()) == 0)
          {
          planes->SetNthMarkupVisibility(i,
            item->checkState() == Qt::Checked ? true : false);
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
int qMRMLPlannerModelHierarchyModel::maxColumnId()const
{
  Q_D(const qMRMLPlannerModelHierarchyModel);
  int maxId = this->Superclass::maxColumnId();
  maxId = qMax(maxId, d->TransformVisibilityColumn);
  maxId = qMax(maxId, d->PlanesVisibilityColumn);
  return maxId;
}
