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

#ifndef __qMRMLPlannerModelHierarchyModel_h
#define __qMRMLPlannerModelHierarchyModel_h


#include "qSlicerPlannerModuleWidgetsExport.h"

// MRMLWidgets includes
#include "qMRMLSceneDisplayableModel.h"
//#include "qMRMLSceneModelHierarchyModel.h"
class qMRMLPlannerModelHierarchyModelPrivate;

class Q_SLICER_QTMODULES_PLANNER_WIDGETS_EXPORT qMRMLSceneModelHierarchyModel : public qMRMLSceneDisplayableModel
{
  Q_OBJECT

public:
  typedef qMRMLSceneDisplayableModel Superclass;
  qMRMLSceneModelHierarchyModel(QObject *parent=nullptr):Superclass(parent){}
  ~qMRMLSceneModelHierarchyModel() override{}

private:
  Q_DISABLE_COPY(qMRMLSceneModelHierarchyModel);
};


class Q_SLICER_QTMODULES_PLANNER_WIDGETS_EXPORT qMRMLPlannerModelHierarchyModel
  : public qMRMLSceneModelHierarchyModel
{
  Q_OBJECT
  /// \todo
  /// A value of -1 (default) hides the column
  Q_PROPERTY(int transformVisibilityColumn
             READ transformVisibilityColumn
             WRITE setTransformVisibilityColumn)

  Q_PROPERTY(int planesVisibilityColumn
             READ planesVisibilityColumn
             WRITE setPlanesVisibilityColumn)

  Q_PROPERTY(int cutButtonColumn
             READ cutButtonColumn
             WRITE setCutButtonColumn)

  Q_PROPERTY(int bendButtonColumn
             READ bendButtonColumn
             WRITE setBendButtonColumn)

public:
  typedef qMRMLSceneModelHierarchyModel Superclass;
  qMRMLPlannerModelHierarchyModel(QObject* parent = 0);
  virtual ~qMRMLPlannerModelHierarchyModel();

  int transformVisibilityColumn()const;
  void setTransformVisibilityColumn(int column);

  int planesVisibilityColumn()const;
  void setPlanesVisibilityColumn(int column);

  int cutButtonColumn()const;
  void setCutButtonColumn(int column);

  int bendButtonColumn()const;
  void setBendButtonColumn(int column);

  static const char* transformDisplayReferenceRole();
  static const char* planesReferenceRole();

  void setPlaneVisibility(vtkMRMLNode* node, bool visible);
  void setTransformVisibility(vtkMRMLNode* node, bool visible);


signals:
    void transformOn(vtkMRMLNode* node);

protected:
  qMRMLPlannerModelHierarchyModel(qMRMLPlannerModelHierarchyModelPrivate* pimpl,
                                  QObject* parent = 0);

  /// Reimplemented to listen to the displayable DisplayModifiedEvent event for
  /// visibility check state changes.
  virtual void observeNode(vtkMRMLNode* node);
  virtual QFlags<Qt::ItemFlag> nodeFlags(vtkMRMLNode* node, int column)const;
  void updateItemDataFromNode(QStandardItem* item, vtkMRMLNode* node, int column);
  void updateNodeFromItemData(vtkMRMLNode* node, QStandardItem* item);

  virtual int maxColumnId()const;

protected slots:
  void onReferenceChangedEvent(vtkObject*);

protected:
  QScopedPointer<qMRMLPlannerModelHierarchyModelPrivate> d_ptr;
  vtkMRMLNode* activeNode;

private:
  Q_DECLARE_PRIVATE(qMRMLPlannerModelHierarchyModel);
  Q_DISABLE_COPY(qMRMLPlannerModelHierarchyModel);
};

#endif
