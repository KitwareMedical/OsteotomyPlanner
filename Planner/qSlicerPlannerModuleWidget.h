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

#ifndef __qSlicerPlannerModuleWidget_h
#define __qSlicerPlannerModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerPlannerModuleExport.h"

class qSlicerPlannerModuleWidgetPrivate;
class vtkMRMLNode;
class vtkSlicerPlannerLogic;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_PLANNER_EXPORT qSlicerPlannerModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPlannerModuleWidget(QWidget *parent=0);
  virtual ~qSlicerPlannerModuleWidget();

  // Shorthand
  vtkSlicerPlannerLogic* plannerLogic() const;

public slots:
  void setCurrentNode(vtkMRMLNode* node);
  virtual void setMRMLScene(vtkMRMLScene* scene);

protected slots:
  /// Tree view slots
  void onNodeAddedEvent(vtkObject* scene, vtkObject* node);
  void onNodeRemovedEvent(vtkObject* scene, vtkObject* node);
  void onCurrentNodeAboutToBeRemoved(vtkMRMLNode* node);
  void onSceneUpdated();

  /// General widget slots
  void updateWidgetFromMRML(vtkObject*, vtkObject*);
  void updateWidgetFromMRML();
  void updateMRMLFromWidget();

  /// References slots
  void updateBrainReferenceNode(vtkMRMLNode* node);
  void updateTemplateReferenceNode(vtkMRMLNode* node);
  void onOpenBrainReference();
  void onOpenTemplateReference();

  //Actions slots
  void updateCurrentCutNode(vtkMRMLNode* node);
  void previewButtonClicked();
  void confirmButtonClicked();
  void cancelButtonClicked();

  // Metrics slots
  void onComputeButton();
  void onSetPreOP();
  
protected:
  virtual void setup();

  QScopedPointer<qSlicerPlannerModuleWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlannerModuleWidget);
  Q_DISABLE_COPY(qSlicerPlannerModuleWidget);
};

#endif
