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

#include "vtkMRMLCommandLineModuleNode.h"

class qSlicerPlannerModuleWidgetPrivate;
class vtkMRMLNode;
class vtkSlicerPlannerLogic;
class vtkMRMLModelNode;
class QModelIndex;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_PLANNER_EXPORT qSlicerPlannerModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:
  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerPlannerModuleWidget(QWidget* parent = 0);
  virtual ~qSlicerPlannerModuleWidget();

  // Shorthand
  vtkSlicerPlannerLogic* plannerLogic() const;

public slots:
  void setCurrentItem(vtkIdType item);
  virtual void setMRMLScene(vtkMRMLScene* scene);

protected slots:
  /// Tree view slots
  void onItemAddedEvent(vtkObject* subjectHierarchy, vtkIdType nodeID);
  void onNodeRemovedEvent(vtkObject* scene, vtkObject* node);
  void onSceneUpdated();

  /// General widget slots
  void updateWidgetFromMRML(vtkObject*, vtkObject*);
  void updateWidgetFromMRML();
  void updateMRMLFromWidget();

  /// References slots
  void updateTemplateReferenceNode(vtkMRMLNode* node);
  void onOpenTemplateReference();

  //Actions slots
  void updateCurrentCutNode(vtkMRMLNode* node);
  void updateCurrentBendNode(vtkMRMLNode* node);
  void previewCutButtonClicked();
  void confirmCutButtonClicked();
  void cancelCutButtonClicked();
  void placeFiducialButtonClicked();
  void cancelFiducialButtonClicked();
  void cancelBendButtonClicked();
  void initBendButtonClicked();
  void updateBendButtonClicked();
  void bendMagnitudeSliderUpdated();
  void finshBendClicked();
  void finishPlanButtonClicked();
  void transformActivated(vtkMRMLNode* node);
  void confirmMoveButtonClicked();
  void cancelMoveButtonClicked();
  void cutCurrentSelectionButtonClicked();
  void bendCurrentSelectionButtonClicked();


  //Saving slots
  void saveDirectoryChanged(const QString &);
  void enabledSavingCheckboxToggled(bool state);
  void takeScreenshotButtonClicked();


  // Metrics slots
  void onComputeButton();
  void onSetPreOP();
  void computeScalarsClicked();

  //CLI slots
  void finishWrap();
  void launchMetrics();
  void finishDistance();
  void runModelDistance(vtkMRMLModelNode* distRef);
  void launchDistance();


protected:
  virtual void setup();

  QScopedPointer<qSlicerPlannerModuleWidgetPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerPlannerModuleWidget);
  Q_DISABLE_COPY(qSlicerPlannerModuleWidget);
};

#endif
