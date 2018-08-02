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
#include <qdatetime.h>


// CTK includes
#include "ctkMessageBox.h"
#include <ctkVTKWidgetsUtils.h>

// VTK includes
#include "vtkNew.h"
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include "vtkVector.h"
#include "vtkVectorOperators.h"
#include <vtksys/SystemTools.hxx>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkRenderLargeImage.h>

// SlicerQt includes
#include "qSlicerApplication.h"
#include "qSlicerIOManager.h"
#include "qSlicerFileDialog.h"
#include "qMRMLSceneModelHierarchyModel.h"
#include "qSlicerPlannerModuleWidget.h"
#include "ui_qSlicerPlannerModuleWidget.h"
#include "qMRMLSortFilterProxyModel.h"
#include "qSlicerLayoutManager.h"
#include "qMRMLThreeDView.h"
#include "qMRMLThreeDWidget.h"
#include "qMRMLUtils.h"
#include "vtkPNGWriter.h"

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
#include "vtkMassProperties.h"
#include "vtkCleanPolyData.h"
#include "vtkTriangleFilter.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkMRMLLayoutNode.h"
#include "vtkScalarBarWidget.h"
#include <vtkLookupTable.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkScalarBarActor.h>
#include "vtkMRMLColorTableNode.h"
#include "vtkMRMLColorNode.h"

// Slicer CLI includes
#include <qSlicerCoreApplication.h>
#include <qSlicerModuleManager.h>
#include "qSlicerAbstractCoreModule.h"
//#include <qSlicerCLIModule.h>
#include <vtkSlicerCLIModuleLogic.h>

// Self
#include "qMRMLPlannerModelHierarchyModel.h"
#include "vtkSlicerPlannerLogic.h"
#include "ButtonItemDelegate.h"


//STD includes
#include <vector>
#include <sstream>
#include <array>

#define D(x) std::cout << x << std::endl;



//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerPlannerModuleWidgetPrivate: public Ui_qSlicerPlannerModuleWidget
{
public:
  qSlicerPlannerModuleWidgetPrivate();
  void fireDeleteChildrenWarning() const;

  void tagModels(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);
  void untagModels(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);
  void createTransformsIfNecessary(
    vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);
  vtkMRMLLinearTransformNode* createTransformNode(
    vtkMRMLScene* scene, vtkMRMLNode* refNode);
  vtkMRMLLinearTransformNode* getTransformNode(
    vtkMRMLScene* scene, vtkMRMLNode* refNode) const;
  void removeTransformNode(vtkMRMLScene* scene, vtkMRMLNode* nodeRef);
  void removeTransforms(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);

  void createPlanesIfNecessary(
    vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);
  void updatePlanesFromModel(vtkMRMLScene* scene, vtkMRMLModelNode*) const;
  void removePlanes(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* refNode);

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
  void hardenTransforms(bool hardenLinearOnly);
  void clearTransforms();
  void hideTransforms();

  //Cutting Variables
  vtkWeakPointer<vtkMRMLModelHierarchyNode> HierarchyNode;
  vtkWeakPointer<vtkMRMLModelHierarchyNode> StagedHierarchyNode;
  QStringList HideChildNodeTypes;
  vtkWeakPointer<vtkMRMLNode> TemplateReferenceNode;
  vtkWeakPointer<vtkMRMLNode> CurrentCutNode;
  vtkSmartPointer<vtkMRMLNode> StagedCutNode1;
  vtkSmartPointer<vtkMRMLNode> StagedCutNode2;
  bool cuttingActive;
  vtkWeakPointer<vtkSlicerCLIModuleLogic> splitLogic;
  vtkWeakPointer<vtkSlicerPlannerLogic> logic;
  vtkSmartPointer<vtkMRMLCommandLineModuleNode> cmdNode;

  //Bending Variables
  std::array<vtkSmartPointer<vtkMRMLMarkupsFiducialNode>, 2> BendPoints;
  std::vector<std::string> PointsToRemove;
  vtkWeakPointer<vtkMRMLNode> CurrentBendNode;
  vtkSmartPointer<vtkPoints> Fiducials;
  vtkSmartPointer<vtkPolyData> BendingData;
  double BendMagnitude;
  bool bendingActive;
  bool bendingOpen;
  bool placingActive;
  bool BendDoubleSide;
  bool BendASide;
  vtkWeakPointer<vtkMRMLScene> scene;

  //move
  bool moveActive;

  //Bending methods
  int beginPlacement(vtkMRMLScene* scene, int id);
  void endPlacement();
  int ActivePoint;
  void computeAndSetSourcePoints(vtkMRMLScene* scene);
  void computeTransform(vtkMRMLScene* scene);
  void clearControlPoints(vtkMRMLScene* scene);
  void clearBendingData(vtkMRMLScene* scene);

  //Metrics Variables
  std::vector<vtkSmartPointer<vtkMRMLModelNode>> modelIterator;
  vtkWeakPointer<vtkSlicerCLIModuleLogic> distanceLogic;
  vtkSmartPointer<vtkMRMLTableNode> modelMetricsTable;
  bool PreOpSet;
  bool cliFreeze;

  //Metrics methods
  void prepScalarComputation(vtkMRMLScene* scene);
  void setScalarVisibility(bool visible);
  vtkSmartPointer<vtkScalarBarWidget> ScalarBarWidget;
  vtkSmartPointer<vtkScalarBarActor> ScalarBarActor;

  //Methods and vars for instruction saving
  std::array<std::string, 4> ActionInProgress;
  std::vector<std::array<std::string, 4>> RecordedActions;
  std::vector<vtkSmartPointer<vtkImageData>> ActionScreenshots;
  QString RootDirectory;
  QString SaveDirectory;
  QString InstructionFile;
  int NumberOfSavedActions;
  int NumberOfWrittenActions;
  std::string generateInstruction(std::array<std::string, 4> action, int index);
  std::string generatePNGFilename(std::array<std::string, 4> action, int index);
  void recordActionInProgress();
  void writeOutActions();
  void setUpSaveFiles();
  bool savingActive;
  bool waitingOnScreenshot;
  vtkSmartPointer<vtkImageData> grabScreenshot();
  void savePNGImage(vtkImageData*, std::array<std::string, 4> action, int index);
  void clearSavingData();
};

//-----------------------------------------------------------------------------
// qSlicerPlannerModuleWidgetPrivate methods

void qSlicerPlannerModuleWidgetPrivate::clearSavingData()
{
  this->ActionInProgress.fill("");
  this->RecordedActions.clear();
  this->ActionScreenshots.clear();
  this->RootDirectory = "";
  this->SaveDirectory = "";
  this->InstructionFile = "";
  this->NumberOfSavedActions = 0;
  this->NumberOfWrittenActions = 0;
  this->savingActive = false;
  this->waitingOnScreenshot = false;
}

void qSlicerPlannerModuleWidgetPrivate::savePNGImage(vtkImageData* image, std::array<std::string, 4> action, int index)
{
  QDir saveDir = QDir(this->SaveDirectory);
  QString pngFile = saveDir.absoluteFilePath(this->generatePNGFilename(action, index).c_str());

  vtkSmartPointer<vtkPNGWriter> writer =
    vtkSmartPointer<vtkPNGWriter>::New();
  writer->SetInputData(image);
  writer->SetFileName(pngFile.toStdString().c_str());
  writer->Write();
}

vtkSmartPointer<vtkImageData> qSlicerPlannerModuleWidgetPrivate::grabScreenshot()
{
  vtkNew<vtkImageData> newImageData;
  QWidget* widget = 0;
  vtkRenderWindow* renderWindow = 0;
  // Create a screenshot of the first 3DView
  
  qMRMLThreeDView* threeDView = qSlicerApplication::application()->layoutManager()->threeDWidget(0)->threeDView();
  widget = threeDView;
  renderWindow = threeDView->renderWindow(); 

  double scaleFactor = 1;

  if (!qFuzzyCompare(scaleFactor, 1.0) )
  {
    // use off screen rendering to magnifiy the VTK widget's image without interpolation
    vtkRenderer *renderer = renderWindow->GetRenderers()->GetFirstRenderer();
    vtkNew<vtkRenderLargeImage> renderLargeImage;
    renderLargeImage->SetInput(renderer);
    renderLargeImage->SetMagnification(scaleFactor);
    renderLargeImage->Update();
    newImageData.GetPointer()->DeepCopy(renderLargeImage->GetOutput());
  }  
  else
  {
    // no scaling, or for not just the 3D window
    QImage screenShot = ctk::grabVTKWidget(widget);

    if (!qFuzzyCompare(scaleFactor, 1.0))
    {
      // Rescale the image which gets saved
      QImage rescaledScreenShot = screenShot.scaled(screenShot.size().width() * scaleFactor,
        screenShot.size().height() * scaleFactor);

      // convert the scaled screenshot from QPixmap to vtkImageData
      qMRMLUtils::qImageToVtkImageData(rescaledScreenShot,
        newImageData.GetPointer());
    }
    else
    {
      // convert the screenshot from QPixmap to vtkImageData
      qMRMLUtils::qImageToVtkImageData(screenShot,
        newImageData.GetPointer());
    }
  }
  // save the screen shot image to this class
  vtkSmartPointer<vtkImageData> screenshot = newImageData.GetPointer();
  return screenshot;
}

void qSlicerPlannerModuleWidgetPrivate::writeOutActions()
{
  if (!this->savingActive)
  {
    return;
  }

  while (this->NumberOfWrittenActions < this->NumberOfSavedActions)
  {
    std::array<std::string, 4> action = this->RecordedActions[this->NumberOfWrittenActions];
    QFile file(this->InstructionFile);
    if (file.open(QIODevice::Append))
    {
      QTextStream stream(&file);
      stream << this->generateInstruction(action, this->NumberOfWrittenActions).c_str() << endl << "\t\t" << this->generatePNGFilename(action,
        this->NumberOfWrittenActions).c_str() << endl;
      file.close();
    }
    this->savePNGImage(this->ActionScreenshots[this->NumberOfWrittenActions], action, this->NumberOfWrittenActions);
    this->NumberOfWrittenActions++;
  }
  

}

void qSlicerPlannerModuleWidgetPrivate::setUpSaveFiles()
{
  if (!this->savingActive)
  {
    return;
  }
  
  QDir rootDir = QDir(this->RootDirectory);
  QDir saveDir = QDir(this->SaveDirectory);
  rootDir.mkdir(saveDir.dirName());

  if (!rootDir.exists(this->SaveDirectory))
  {
    std::cout << "Failed to create output folder!" << std::endl;
    this->savingActive = false;
    return;
  }
  
  std::stringstream ssFilename;
  ssFilename << this->HierarchyNode->GetName() << "_Instructions.txt";
  this->InstructionFile = saveDir.absoluteFilePath(ssFilename.str().c_str());
  QFile file(this->InstructionFile);
  if (file.open(QIODevice::ReadWrite))
  {
    QTextStream stream(&file);
    stream << "Osteotomy Planner Instructions for case: " << this->HierarchyNode->GetName() << endl;
    file.close();
  }
}

void qSlicerPlannerModuleWidgetPrivate::recordActionInProgress()
{
    this->RecordedActions.push_back(this->ActionInProgress);
    this->ActionScreenshots.push_back(this->grabScreenshot());
    this->ActionInProgress.fill("");
    this->NumberOfSavedActions++;
    this->writeOutActions();
}
std::string qSlicerPlannerModuleWidgetPrivate::generateInstruction(std::array<std::string, 4> action, int index)
{
    std::stringstream ss;
    ss << "Step " << index << ":\t" << action[1] << " " << action[0];

    if (action[1] == "Cut")
    {
        ss << " into " << action[2] << " and " << action[3];
    }

    return ss.str();
}

// qSlicerPlannerModuleWidgetPrivate methods
std::string qSlicerPlannerModuleWidgetPrivate::generatePNGFilename(std::array<std::string, 4> action, int index)
{
    std::stringstream ss;
    if (this->HierarchyNode)
    {
        ss << this->HierarchyNode->GetName() << "_" << index << "_" << action[1] << "_" << action[0] << ".png";
    }  

    return ss.str();
}

//-----------------------------------------------------------------------------
//Clear fiducials used for bending
void qSlicerPlannerModuleWidgetPrivate::clearControlPoints(vtkMRMLScene* scene)
{
  if(this->BendPoints[0])
  {
    scene->RemoveNode(this->BendPoints[0]);
    this->BendPoints[0] = NULL;
  }
  if(this->BendPoints[1])
  {
    scene->RemoveNode(this->BendPoints[1]);
    this->BendPoints[1] = NULL;
  }

  std::vector<std::string>::iterator it;
  for (it = this->PointsToRemove.begin(); it != this->PointsToRemove.end(); it++) {
      vtkMRMLNode * node = this->scene->GetNodeByID(*it);
      this->scene->RemoveNode(node);
  }
  this->PointsToRemove.clear();
  this->ActivePoint = -1;
  
}

//-----------------------------------------------------------------------------
//Clear data used to compute bending trasforms
void qSlicerPlannerModuleWidgetPrivate::clearBendingData(vtkMRMLScene* scene)
{
  this->Fiducials = NULL;
  this->logic->clearBendingData();

  //reset parent transform to correct node
  vtkSmartPointer<vtkMRMLTransformNode> parentTransform = vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->GetParentTransformNode();
  if (!parentTransform->IsA("vtkMRMLLinearTransformNode"))
  {     
      vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->SetAndObserveTransformNodeID(parentTransform->GetParentTransformNode()->GetID());
      scene->RemoveNode(parentTransform);
      parentTransform = NULL;
  }
  

}

//-----------------------------------------------------------------------------
//Constructor
qSlicerPlannerModuleWidgetPrivate::qSlicerPlannerModuleWidgetPrivate()
{
  this->HierarchyNode = NULL;
  this->StagedHierarchyNode = NULL;
  this->HideChildNodeTypes =
    (QStringList() << "vtkMRMLFiberBundleNode" << "vtkMRMLAnnotationNode");
  this->TemplateReferenceNode = NULL;
  this->CurrentCutNode = NULL;
  this->StagedCutNode1 = NULL;
  this->StagedCutNode2 = NULL;
  this->modelMetricsTable = NULL;
  this->cuttingActive = false;
  this->bendingActive = false;
  this->moveActive = false;
  this->bendingOpen = false;
  this->placingActive = false;
  this->cmdNode = NULL;
  this->PreOpSet = false;
  this->cliFreeze = false;
  this->savingActive = false;
  this->waitingOnScreenshot = false;
  this->scene = NULL;

  this->BendDoubleSide = true;
  this->BendASide = true;

  this->BendPoints[0] = NULL;
  this->BendPoints[1] = NULL;
  this->Fiducials = NULL;
  this->BendMagnitude = 0;
  this->ActivePoint = -1;

  this->ActionInProgress.fill("");
  this->RecordedActions.clear();
  this->ActionScreenshots.clear();
  this->NumberOfSavedActions = 0;
  this->NumberOfWrittenActions = 0;
  this->RootDirectory = "";
  this->InstructionFile = "";
  this->SaveDirectory = "";

  qSlicerAbstractCoreModule* splitModule =
    qSlicerCoreApplication::application()->moduleManager()->module("SplitModel");

  this->splitLogic =
    vtkSlicerCLIModuleLogic::SafeDownCast(splitModule->logic());

  qSlicerAbstractCoreModule* distanceModule =
    qSlicerCoreApplication::application()->moduleManager()->module("OsteotomyModelToModelDistance");
  this->distanceLogic =
    vtkSlicerCLIModuleLogic::SafeDownCast(distanceModule->logic());


  this->ScalarBarWidget = vtkSmartPointer<vtkScalarBarWidget>::New();
  this->ScalarBarActor = vtkSmartPointer<vtkScalarBarActor>::New();
  this->ScalarBarWidget->SetScalarBarActor(this->ScalarBarActor);
  this->ScalarBarActor->SetOrientationToVertical();
  this->ScalarBarActor->SetNumberOfLabels(11);
  this->ScalarBarActor->SetTitle("Distances (mm)");

  // it's a 2d actor, position it in screen space by percentages
  this->ScalarBarActor->SetPosition(0.1, 0.1);
  this->ScalarBarActor->SetWidth(0.1);
  this->ScalarBarActor->SetHeight(0.8);

}


//-----------------------------------------------------------------------------
//Complete placement of current fiducial
void qSlicerPlannerModuleWidgetPrivate::endPlacement()
{
  if (!this->placingActive)
  {
    return;
  }
    //check that point in close to (i.e. on surface of) model to bend
    //if not, retrigger placing and give it another go  
  
  vtkVector3d point;
  double posa[3];
  this->BendPoints[this->ActivePoint]->GetNthFiducialPosition(0, posa);
  point.SetX(posa[0]);
  point.SetY(posa[1]);
  point.SetZ(posa[2]);  

  double dist = this->logic->getDistanceToModel(point, vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->GetPolyData());
  if (dist > 1.0)
  {
      
      //this->scene->RemoveNode(this->BendPoints[this->ActivePoint]);
      this->BendPoints[this->ActivePoint]->GetDisplayNode()->SetVisibility(0);
      this->PointsToRemove.push_back(this->BendPoints[this->ActivePoint]->GetID());
      BendPoints[this->ActivePoint] = NULL;
      this->beginPlacement(this->scene, this->ActivePoint);
      
  }
  else 
  {
      this->MovingPointAButton->setEnabled(true);
      this->MovingPointBButton->setEnabled(true);
      this->placingActive = false;
      this->ActivePoint = -1;
      std::vector<std::string>::iterator it;
      for (it = this->PointsToRemove.begin(); it != this->PointsToRemove.end(); it++) {
          vtkMRMLNode * node = this->scene->GetNodeByID(*it);
          this->scene->RemoveNode(node);
      }
      this->PointsToRemove.clear();
  } 
  
}

//-----------------------------------------------------------------------------
//Compute source points from fiducials
void qSlicerPlannerModuleWidgetPrivate::computeAndSetSourcePoints(vtkMRMLScene* scene)
{
  this->Fiducials = vtkSmartPointer<vtkPoints>::New();
  
  double posa[3];
  double posb[3];
  this->BendPoints[0]->GetNthFiducialPosition(0, posa);
  this->BendPoints[1]->GetNthFiducialPosition(0, posb);
  this->Fiducials->InsertNextPoint(posa[0], posa[1], posa[2]);
  this->Fiducials->InsertNextPoint(posb[0], posb[1], posb[2]);
  

  this->logic->initializeBend(this->Fiducials, vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode));
  
  vtkNew<vtkCleanPolyData> clean;
  vtkNew<vtkTriangleFilter> triangulate;
  clean->SetInputData(vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->GetPolyData());
  clean->Update();
  triangulate->SetInputData(clean->GetOutput());
  triangulate->Update();
  this->BendingData = triangulate->GetOutput();
  vtkNew<vtkMassProperties> area;
  area->SetInputData(this->BendingData);

  area->Update();
  
  std::stringstream surfaceAreaSstr;
  surfaceAreaSstr << area->GetSurfaceArea();
  const std::string& surfaceAreaString= surfaceAreaSstr.str();
  this->AreaBeforeBending->setText(surfaceAreaString.c_str());
}


//-----------------------------------------------------------------------------
//Compute thin plate spline transform based on source and target points
void qSlicerPlannerModuleWidgetPrivate::computeTransform(vtkMRMLScene* scene)
{
  //Get direct parent transform of model
  //  
  vtkNew<vtkMRMLTransformNode> bendTemp;
  vtkSmartPointer<vtkMRMLTransformNode> BendingTransformNode;
  vtkSmartPointer<vtkMRMLTransformNode> parentTransform = vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->GetParentTransformNode();
  if (parentTransform->IsA("vtkMRMLLinearTransformNode"))
  {
      //Add bending transform node to scene
      BendingTransformNode = bendTemp.GetPointer();
      scene->AddNode(BendingTransformNode);
      //BendingTransformNode->CreateDefaultDisplayNodes();
      BendingTransformNode->SetAndObserveTransformNodeID(parentTransform->GetID());
  }
  else if (parentTransform->IsA("vtkMRMLTransformNode"))
  {
      BendingTransformNode = parentTransform;
  }  
  
  if(this->BendDoubleSide)
  {
    this->logic->setBendType(vtkSlicerPlannerLogic::Double);
  }
  else
  {
    this->logic->setBendType(vtkSlicerPlannerLogic::Single);
  }

  if(this->BendASide)
  {
    this->logic->setBendSide(vtkSlicerPlannerLogic::A);
  }
  else
  {
    this->logic->setBendSide(vtkSlicerPlannerLogic::B);
  }
  vtkSmartPointer<vtkThinPlateSplineTransform> tps = this->logic->getBendTransform(this->BendMagnitude);
  BendingTransformNode->SetAndObserveTransformToParent(tps);
  vtkMRMLModelNode::SafeDownCast(this->CurrentBendNode)->SetAndObserveTransformNodeID(BendingTransformNode->GetID());

  //
  vtkNew<vtkTransformPolyDataFilter> transform;
  transform->SetTransform(tps);
  transform->SetInputData(this->BendingData);
  transform->Update();
  vtkNew<vtkMassProperties> area;
  area->SetInputData(transform->GetOutput());

  area->Update();
  
  std::stringstream surfaceAreaSstr;
  surfaceAreaSstr << area->GetSurfaceArea();
  const std::string& surfaceAreaString= surfaceAreaSstr.str();
  this->AreaAfterBending->setText(surfaceAreaString.c_str());  
  
}
//-----------------------------------------------------------------------------
//Initialize placement of a fiducial
int qSlicerPlannerModuleWidgetPrivate::beginPlacement(vtkMRMLScene* scene, int id)
{

  vtkNew<vtkMRMLMarkupsFiducialNode> fiducial;

  if (this->BendPoints[id])
  {
      scene->RemoveNode(this->BendPoints[id]);
      BendPoints[id] = NULL;
  }
  this->BendPoints[id] = fiducial.GetPointer();
  std::string name = "BendPoint" + std::to_string(id);
  this->BendPoints[id]->SetName(name.c_str());
  scene->AddNode(this->BendPoints[id]);
  this->BendPoints[id]->CreateDefaultDisplayNodes();
  vtkMRMLMarkupsDisplayNode* disp = vtkMRMLMarkupsDisplayNode::SafeDownCast(this->BendPoints[id]->GetDisplayNode());
  disp->SetTextScale(0.1);
  disp->SetGlyphScale(5);
  this->placingActive = true;
  vtkMRMLInteractionNode* interaction = qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode();
  vtkMRMLSelectionNode* selection = qSlicerCoreApplication::application()->applicationLogic()->GetSelectionNode();
  selection->SetReferenceActivePlaceNodeClassName("vtkMRMLMarkupsFiducialNode");
  selection->SetActivePlaceNodeID(this->BendPoints[id]->GetID());
  interaction->SetCurrentInteractionMode(vtkMRMLInteractionNode::Place);
  
  
  
  //deactivate buttons
  this->MovingPointAButton->setEnabled(false);
  this->MovingPointBButton->setEnabled(false);
  this->ActivePoint = id;
  return EXIT_SUCCESS;
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
::removeTransforms(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if (!hierarchy)
  {
    return;
  }

  this->removeTransformNode(scene, hierarchy);

  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  hierarchy->GetAllChildrenNodes(children);
  for (it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());
    if (childModel)
    {
      this->removeTransformNode(scene, childModel);
    }
  }

}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate
::createTransformsIfNecessary(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if(!hierarchy)
  {
    return;
  }

  vtkMRMLLinearTransformNode* transform = this->getTransformNode(scene, hierarchy);
  if(!transform)
  {
    transform = this->createTransformNode(scene, hierarchy);
  }

  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  hierarchy->GetAllChildrenNodes(children);
  for(it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());
    if(childModel)
    {
      vtkMRMLLinearTransformNode* childTransform = this->getTransformNode(scene, childModel);
      if(!childTransform)
      {
        childTransform = this->createTransformNode(scene, childModel);
        childModel->SetAndObserveTransformNodeID(childTransform->GetID());
      }
      childTransform->SetAndObserveTransformNodeID(transform->GetID());
    }
  }

}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate
::tagModels(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if (!hierarchy)
  {
    return;
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
      childModel->SetAttribute("PlannerRole", "HierarchyMember");
    }
  }

  std::vector<vtkMRMLNode*> nodes;
  std::vector<vtkMRMLNode*>::const_iterator itN;
  scene->GetNodesByClass("vtkMRMLModelNode", nodes);
  for (itN = nodes.begin(); itN != nodes.end(); ++itN)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*itN));
    if (childModel)
    {
      if (!childModel->GetAttribute("PlannerRole"))
      {
        childModel->SetAttribute("PlannerRole", "NonMember");
      }
    }
  }

}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate
::untagModels(vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if (!hierarchy)
  {
    return;
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
      childModel->RemoveAttribute("PlannerRole");
    }
  }

  std::vector<vtkMRMLNode*> nodes;
  std::vector<vtkMRMLNode*>::const_iterator itN;
  scene->GetNodesByClass("vtkMRMLModelNode", nodes);
  for (itN = nodes.begin(); itN != nodes.end(); ++itN)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*itN));
    if (childModel)
    {
      if (childModel->GetAttribute("PlannerRole"))
      {
        childModel->RemoveAttribute("PlannerRole");
      }
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
  if(transform)
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
  if(plane)
  {
    scene->RemoveNode(plane);
  }
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::createPlanesIfNecessary(
  vtkMRMLScene* scene, vtkMRMLModelHierarchyNode* hierarchy)
{
  if(!hierarchy)
  {
    return;
  }
  int count = 0;
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  std::vector<vtkMRMLModelNode*> models;
  hierarchy->GetAllChildrenNodes(children);
  for(it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());
    if(childModel)
    {
      vtkMRMLMarkupsPlanesNode* childPlane = this->getPlaneNode(scene, childModel);
      if(!childPlane)
      {
        childPlane = this->createPlaneNode(scene, childModel);
        this->updatePlanesFromModel(scene, childModel);
      }

    }
  }


}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::removePlanes(
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
      this->removePlaneNode(scene, childModel);

    }
  }


}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidgetPrivate::updatePlanesFromModel(vtkMRMLScene* scene,
    vtkMRMLModelNode* model) const
{
  if(!model)
  {
    return;
  }
  vtkMRMLMarkupsPlanesNode* plane = this->getPlaneNode(scene, model);

  plane->RemoveAllMarkups();

  double bounds[6];
  model->GetRASBounds(bounds);
  double min[3], max[3];
  double boundsPadding = 0.5;
  min[0] = bounds[0] - boundsPadding * (bounds[1] - bounds[0]);
  min[1] = bounds[2] - boundsPadding * (bounds[3] - bounds[2]);
  min[2] = bounds[4] - boundsPadding * (bounds[5] - bounds[4]);
  max[0] = bounds[1] + boundsPadding * (bounds[1] - bounds[0]);
  max[1] = bounds[3] + boundsPadding * (bounds[3] - bounds[2]);
  max[2] = bounds[5] + boundsPadding * (bounds[5] - bounds[4]);

  double origin[3];
  for(int i = 0; i < 3; ++i)
  {
    origin[i] = min[i] + (max[i] - min[i]) / 2;
  }

  // For the normal, pick the bounding largest size
  double largest = 0;
  int largetDimension = 0;
  for(int i = 0; i < 3; ++i)
  {
    double size = fabs(max[i] - min[i]);
    if(size > largest)
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
  if(model)
  {
    vtkMRMLDisplayNode* display = model->GetDisplayNode();
    if(display)
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
  if(model)
  {
    vtkMRMLDisplayNode* display = model->GetDisplayNode();
    if(display)
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
  if(success && loadedNodes->GetNumberOfItems() > 0)
  {
    return vtkMRMLNode::SafeDownCast(loadedNodes->GetItemAsObject(0));
  }
  return NULL;
}


//-----------------------------------------------------------------------------
//Create and display temporary cut
void qSlicerPlannerModuleWidgetPrivate::previewCut(vtkMRMLScene* scene)
{
  if(!this->HierarchyNode)
  {
    this->cuttingActive = false;
    return;
  }

  this->hideTransforms();
  this->hardenTransforms(false);

  //Create nodes
  vtkSmartPointer<vtkMRMLModelNode> splitNode1 = vtkSmartPointer<vtkMRMLModelNode>::New();
  vtkSmartPointer<vtkMRMLModelNode> splitNode2 = vtkSmartPointer<vtkMRMLModelNode>::New();

  std::stringstream name1;
  std::stringstream name2;

  name1 << this->CurrentCutNode->GetName() << "_cut1";
  name2 << this->CurrentCutNode->GetName() << "_cut2";

  this->ActionInProgress[0] = this->CurrentCutNode->GetName();
  this->ActionInProgress[1] = "Cut";
  this->ActionInProgress[2] = name1.str();
  this->ActionInProgress[3] = name2.str();


  splitNode1->SetName(name1.str().c_str());
  splitNode2->SetName(name2.str().c_str());

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
  scene->AddNode(splitNode1);
  scene->AddNode(splitNode2);

  this->splitModel(vtkMRMLModelNode::SafeDownCast(this->CurrentCutNode), splitNode1,
                   splitNode2, scene);

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
  this->StagedCutNode1 = splitNode1;
  this->StagedCutNode2 = splitNode2;

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
  vtkMRMLModelHierarchyNode* hnode = vtkMRMLModelHierarchyNode::SafeDownCast(
                                       vtkMRMLHierarchyNode::GetAssociatedHierarchyNode(node->GetScene(), node->GetID()));

  if(hnode)
  {
    scene->RemoveNode(hnode);
  }
  if(node)
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

  vtkSmartPointer<vtkMRMLCommandLineModuleNode> cmdNode = this->splitLogic->CreateNodeInScene();
  vtkMRMLMarkupsPlanesNode* plane = this->getPlaneNode(scene, inputNode);

  double normal[3];
  double origin[3];
  plane->GetNthPlaneNormal(0, normal);
  plane->GetNthPlaneOrigin(0, origin);



  cmdNode->SetParameterAsString("Model", inputNode->GetID());
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
  if(model)
  {
    vtkMRMLDisplayNode* display = model->GetDisplayNode();
    if(display)
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
  for(it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if(childModel)
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
  for(it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if(childModel)
    {
      childModel->GetDisplayNode()->SetScalarVisibility(visible);
      this->VTKScalarBar->setDisplay(visible);

      if (visible)
      {
        childModel->GetDisplayNode()->SetActiveScalarName("Absolute");
        childModel->GetDisplayNode()->SetScalarRangeFlag(vtkMRMLDisplayNode::UseManualScalarRange);
        childModel->GetDisplayNode()->SetScalarRange(this->LUTRangeWidget->minimumValue(), this->LUTRangeWidget->maximumValue());
        const char *colorNodeID = "vtkMRMLColorTableNodeFileColdToHotRainbow.txt";
        childModel->GetDisplayNode()->SetAndObserveColorNodeID(colorNodeID);
        vtkMRMLColorNode* colorNode = childModel->GetDisplayNode()->GetColorNode();
        vtkMRMLColorTableNode *colorTableNode = vtkMRMLColorTableNode::SafeDownCast(colorNode);
        colorNode->GetLookupTable()->SetRange(this->LUTRangeWidget->minimumValue(), this->LUTRangeWidget->maximumValue());
        this->ScalarBarActor->SetLookupTable(colorTableNode->GetLookupTable());
      }      
    }
  }
}


//-----------------------------------------------------------------------------
//Hide all transforms in the current hierarchy
void qSlicerPlannerModuleWidgetPrivate::hideTransforms()
{
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  this->HierarchyNode->GetAllChildrenNodes(children);
  for(it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if(childModel)
    {
      this->sceneModel()->setTransformVisibility(childModel, false);
    }
  }
}


//-----------------------------------------------------------------------------
//Harden all transforms in the current hierarchy
void qSlicerPlannerModuleWidgetPrivate::hardenTransforms(bool hardenLinearOnly)
{
  std::vector<vtkMRMLHierarchyNode*> children;
  std::vector<vtkMRMLHierarchyNode*>::const_iterator it;
  this->HierarchyNode->GetAllChildrenNodes(children);
  for(it = children.begin(); it != children.end(); ++it)
  {
    vtkMRMLModelNode* childModel =
      vtkMRMLModelNode::SafeDownCast((*it)->GetAssociatedNode());

    if(childModel)
    {
      int m = childModel->StartModify();
      vtkMRMLTransformNode* transformNode = childModel->GetParentTransformNode();
      vtkMRMLMarkupsPlanesNode* planeNode = this->getPlaneNode(childModel->GetScene(), childModel);
      int m2 = planeNode->StartModify();
      if(!transformNode)
      {
        // already in the world coordinate system
        return;
      }
      if(transformNode->IsTransformToWorldLinear())
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
        if (!hardenLinearOnly)
        {
          this->updatePlanesFromModel(childModel->GetScene(), childModel);
          childModel->ApplyTransform(transformNode->GetTransformToParent());
          transformNode->SetAndObserveTransformToParent(NULL);
          vtkNew<vtkMatrix4x4> hardeningMatrix;
          hardeningMatrix->Identity();
          transformNode->SetMatrixTransformFromParent(hardeningMatrix.GetPointer());
        }
      }
      planeNode->EndModify(m2);
      childModel->EndModify(m);
    }
  }
}

//-----------------------------------------------------------------------------
//Harden all transforms in the current hierarchy
void qSlicerPlannerModuleWidgetPrivate::clearTransforms()
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
        hardeningMatrix->Identity();
        transformNode->SetMatrixTransformFromParent(hardeningMatrix.GetPointer());
      }
      else
      {
        // non-linear transform hardening
          this->updatePlanesFromModel(childModel->GetScene(), childModel);
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
  : Superclass(_parent)
  , d_ptr(new qSlicerPlannerModuleWidgetPrivate)
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

  d->scene = this->mrmlScene();

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
  sceneModel->setCutButtonColumn(5);
  sceneModel->setHeaderData(5, Qt::Horizontal, "Cut");
  sceneModel->setBendButtonColumn(6);
  sceneModel->setHeaderData(6, Qt::Horizontal, "Bend");
  // use lazy update instead of responding to scene import end event
  sceneModel->setLazyUpdate(true);


  d->ModelHierarchyTreeView->setHeaderHidden(false);
  d->ModelHierarchyTreeView->header()->setStretchLastSection(false);
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->nameColumn(), QHeaderView::Stretch);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->expandColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->colorColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->opacityColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->transformVisibilityColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->cutButtonColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setResizeMode(sceneModel->bendButtonColumn(), QHeaderView::ResizeToContents);
#else
  d->ModelHierarchyTreeView->header()->setSectionResizeMode(sceneModel->nameColumn(), QHeaderView::Stretch);
  d->ModelHierarchyTreeView->header()->setSectionResizeMode(sceneModel->expandColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setSectionResizeMode(sceneModel->colorColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setSectionResizeMode(sceneModel->opacityColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setSectionResizeMode(sceneModel->transformVisibilityColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setSectionResizeMode(sceneModel->cutButtonColumn(), QHeaderView::ResizeToContents);
  d->ModelHierarchyTreeView->header()->setSectionResizeMode(sceneModel->bendButtonColumn(), QHeaderView::ResizeToContents);
#endif

  d->ModelHierarchyTreeView->sortFilterProxyModel()->setHideChildNodeTypes(d->HideChildNodeTypes);
  d->ModelHierarchyTreeView->sortFilterProxyModel()->invalidate();
  d->ModelHierarchyTreeView->setDragEnabled(false);
  
  ButtonItemDelegate* cuts = new ButtonItemDelegate(d->ModelHierarchyTreeView, qApp->style()->standardPixmap(QStyle::SP_DialogCloseButton));
  ButtonItemDelegate* bends = new ButtonItemDelegate(d->ModelHierarchyTreeView, qApp->style()->standardPixmap(QStyle::SP_DialogOkButton));
  
  d->ModelHierarchyTreeView->setItemDelegateForColumn(sceneModel->cutButtonColumn(), cuts);
  d->ModelHierarchyTreeView->setItemDelegateForColumn(sceneModel->bendButtonColumn(), bends);



  QIcon loadIcon =
    qSlicerApplication::application()->style()->standardIcon(QStyle::SP_DialogOpenButton);
  d->TemplateReferenceOpenButton->setIcon(loadIcon);
  d->ModelHierarchyNodeComboBox->setNoneEnabled(true);
  
  qMRMLSortFilterProxyModel* filterModel4 = d->TemplateReferenceNodeComboBox->sortFilterProxyModel();
  filterModel4->addAttribute("vtkMRMLModelNode", "PlannerRole", "NonMember");

  //Scalar Bar setup
  d->VTKScalarBar->setScalarBarWidget(d->ScalarBarWidget);
  qSlicerApplication * app = qSlicerApplication::application();
  if (app && app->layoutManager())
  {
    qMRMLThreeDView* threeDView = app->layoutManager()->threeDWidget(0)->threeDView();
    vtkRenderer* activeRenderer = app->layoutManager()->activeThreeDRenderer();
    if (activeRenderer)
    {
      d->ScalarBarWidget->SetInteractor(activeRenderer->GetRenderWindow()->GetInteractor());
    }
    connect(d->VTKScalarBar, SIGNAL(modified()), threeDView, SLOT(scheduleRender()));
  }
  d->VTKScalarBar->setVisible(false);
  d->VTKScalarBar->setDisplay(false);
  d->ShowsScalarsCheckbox->setEnabled(false);

  // Connect
  this->connect(d->SaveDirectoryButton, SIGNAL(directoryChanged(const QString &)), this, SLOT(saveDirectoryChanged(const QString &)));
  this->connect(sceneModel, SIGNAL(transformOn(vtkMRMLNode*)), this, SLOT(transformActivated(vtkMRMLNode*)));
  this->connect(
    d->ModelHierarchyNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(setCurrentNode(vtkMRMLNode*)));
  this->connect(
    d->ModelHierarchyNodeComboBox, SIGNAL(nodeAboutToBeRemoved(vtkMRMLNode*)),
    this, SLOT(onCurrentNodeAboutToBeRemoved(vtkMRMLNode*)));

  this->connect(
    d->TemplateReferenceNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)),
    this, SLOT(updateTemplateReferenceNode(vtkMRMLNode*)));

  this->connect(
    d->CutPreviewButton, SIGNAL(clicked()), this, SLOT(previewCutButtonClicked()));
  this->connect(
    d->CutConfirmButton, SIGNAL(clicked()), this, SLOT(confirmCutButtonClicked()));
  this->connect(
    d->CutCancelButton, SIGNAL(clicked()), this, SLOT(cancelCutButtonClicked()));

  this->connect(
    d->ConfirmMoveButton, SIGNAL(clicked()), this, SLOT(confirmMoveButtonClicked()));
  this->connect(
    d->CancelMoveButton, SIGNAL(clicked()), this, SLOT(cancelMoveButtonClicked()));

  this->connect(d->ComputeMetricsButton, SIGNAL(clicked()), this, SLOT(onComputeButton()));
  this->connect(d->SetPreOp, SIGNAL(clicked()), this, SLOT(onSetPreOP()));
  this->connect(
    d->MovingPointAButton, SIGNAL(clicked()), this, SLOT(placeFiducialButtonClicked()));
  this->connect(
    d->MovingPointBButton, SIGNAL(clicked()), this, SLOT(placeFiducialButtonClicked()));
  this->connect(
    d->InitButton, SIGNAL(clicked()), this, SLOT(initBendButtonClicked()));  
  this->connect(
    d->HardenBendButton, SIGNAL(clicked()), this, SLOT(finshBendClicked()));
  this->connect(
    d->CancelBendButton, SIGNAL(clicked()), this, SLOT(cancelBendButtonClicked()));
  this->connect(
    d->ComputeScalarsButton, SIGNAL(clicked()), this, SLOT(computeScalarsClicked()));
  this->connect(
    d->TemplateVisibilityCheckbox, SIGNAL(stateChanged(int)), this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->ShowsScalarsCheckbox, SIGNAL(stateChanged(int)), this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->LUTRangeWidget, SIGNAL(valuesChanged(double, double)), this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->TemplateReferenceColorPickerButton, SIGNAL(colorChanged(QColor)),
    this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->TemplateReferenceOpacitySliderWidget, SIGNAL(valueChanged(double)),
    this, SLOT(updateMRMLFromWidget()));
  this->connect(
    d->TemplateReferenceOpenButton, SIGNAL(clicked()),
    this, SLOT(onOpenTemplateReference()));

  this->connect(d->BendMagnitudeSlider, SIGNAL(valueChanged(double)), this, SLOT(bendMagnitudeSliderUpdated()));
  this->connect(d->DoubleSidedButton, SIGNAL(toggled(bool)), this, SLOT(updateBendButtonClicked()));
  this->connect(d->ASideButton, SIGNAL(toggled(bool)), this, SLOT(updateBendButtonClicked()));
  this->connect(d->BSideButton, SIGNAL(toggled(bool)), this, SLOT(updateBendButtonClicked()));
  this->connect(d->FinishButton, SIGNAL(clicked()), this, SLOT(finishPlanButtonClicked()));

  this->connect(cuts, SIGNAL(buttonIndexClicked(const QModelIndex &)), this, SLOT(modelCallback(const QModelIndex &)));
  this->connect(bends, SIGNAL(buttonIndexClicked(const QModelIndex &)), this, SLOT(modelCallback(const QModelIndex &)));
  this->connect(d->EnableSavingCheckbox, SIGNAL(toggled(bool)), this, SLOT(enabledSavingCheckboxToggled(bool)));
  this->connect(d->ScreenshotButton, SIGNAL(clicked()), this, SLOT(takeScreenshotButtonClicked()));

  this->updateWidgetFromMRML();
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

  this->qvtkReconnect(
    this->mrmlScene(), vtkMRMLScene::StartCloseEvent,
    this, SLOT(finishPlanButtonClicked()));
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget
::onNodeAddedEvent(vtkObject* scene, vtkObject* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  Q_UNUSED(scene);
  vtkMRMLModelHierarchyNode* hNode =
    vtkMRMLModelHierarchyNode::SafeDownCast(node);
  if(!hNode || hNode->GetHideFromEditors())
  {
    return;
  }

  // OnNodeAddedEvent is here to make sure that the combobox is populated
  // too after a node is added to the scene, because the tree view will be
  // and they need to match.
  if(!d->HierarchyNode)
  {
    if(this->mrmlScene()->IsBatchProcessing())
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
  if(transform && !this->mrmlScene()->IsClosing())
  {
    this->updateWidgetFromMRML();
  }

}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::onSceneUpdated()
{
  Q_D(qSlicerPlannerModuleWidget);
  this->disconnect(this, SLOT(onSceneUpdated()));
  if(!d->HierarchyNode && d->StagedHierarchyNode != d->HierarchyNode)
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
  if(hierarchy && hierarchy == d->HierarchyNode)
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

  //activate for non-null hierarchy
  if (d->HierarchyNode)
  {    
    d->MetricsCollapsibleButton->setEnabled(true);
    d->FinishButton->setEnabled(true);
    d->ModelHierarchyNodeComboBox->setEnabled(false);
    d->SetPreOp->setEnabled(true);    
  }
  
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
  //keeps hierarchy models out of other drop down boxes
  d->tagModels(this->mrmlScene(), d->HierarchyNode);

  

  //Disabled due to list view button usage
  //Comboboxes are effectively display only 
  d->CutPreviewButton->setEnabled(false);
  
  
  //set based on cutting/bending state
  d->BendingMenu->setVisible(d->bendingOpen);
  d->CuttingMenu->setVisible(d->cuttingActive);
  d->MoveMenu->setVisible(d->moveActive);
  d->ScreenshotMenu->setVisible(d->waitingOnScreenshot);
  d->ScreenshotMenu->setEnabled(d->waitingOnScreenshot);
  d->CutConfirmButton->setEnabled(d->cuttingActive);
  d->CutCancelButton->setEnabled(d->cuttingActive);
  d->CutPreviewButton->setEnabled(d->cuttingActive);
  d->BendMagnitudeSlider->setEnabled(d->bendingActive);
  d->CancelBendButton->setEnabled(d->bendingOpen);
  d->HardenBendButton->setEnabled(d->bendingActive);
  d->SaveDirectoryButton->setEnabled(!d->savingActive);

  bool performingAction = d->cuttingActive || d->bendingOpen || d->moveActive || d->waitingOnScreenshot;

  
  //non action sections
  d->MetricsCollapsibleButton->setEnabled(!performingAction);
  d->ReferencesCollapsibleButton->setEnabled(!performingAction);
  d->ModelHierarchyTreeView->setEnabled(!performingAction);
  if (performingAction)
  {
      d->ReferencesCollapsibleButton->setCollapsed(true);
      d->MetricsCollapsibleButton->setCollapsed(true);
  }
  d->FinishButton->setEnabled(!performingAction);

  //Pre-op state 
  

  d->SetPreOp->setDisabled(d->PreOpSet);
  d->updateWidgetFromReferenceNode(
    d->TemplateReferenceNodeComboBox->currentNode(),
    d->TemplateReferenceColorPickerButton,
    d->TemplateReferenceOpacitySliderWidget);

  //Bending init button
  if(d->BendPoints[0] && d->BendPoints[1] && d->CurrentBendNode && !d->placingActive)
  {
    d->InitButton->setEnabled(true);
    d->BendingInfoLabel->setText("You can move the points after they are placed by clicking and dragging in the"
        " 3D view, or by using the Place button again. Click 'Begin Bending' once you are satisfied with the positioning.");
  }
  else
  {
    d->InitButton->setEnabled(false);
    d->BendingInfoLabel->setText("Place Point A and Point B to define the bending axis (line you want the model to bend around).");
  }

  if (d->bendingActive)
  {
      d->BendingInfoLabel->setText("You can adjust the magnitude of the bend with the slider."
        " You can also select which side of the model you want to bend (or both sides).  Click 'Confirm' to finalize");
  }

  //Sort out radio buttons  
  d->BendDoubleSide = d->DoubleSidedButton->isChecked();
  d->BendASide = d->ASideButton->isChecked();

  if (!this->plannerLogic()->getWrappedBoneTemplateModel())
  {
    d->ReferenceRadioButton->setChecked(false);
    d->ReferenceRadioButton->setEnabled(false);
  }
  else
  {
    d->ReferenceRadioButton->setEnabled(true);
  }

  //Freeze UI if needed
  if (d->cliFreeze)
  {
      d->ReferencesCollapsibleButton->setEnabled(false);
      d->MetricsCollapsibleButton->setEnabled(false);
      d->FinishButton->setEnabled(false);
      d->ModelHierarchyTreeView->setEnabled(false);
      d->ScreenshotMenu->setEnabled(false);
  }

  //Saving buttons
  d->SaveDirectoryButton->setEnabled(d->savingActive && !d->PreOpSet);
  d->EnableSavingCheckbox->setEnabled(!d->savingActive || !d->PreOpSet);

  d->SavingToLabel->setVisible(d->savingActive);
  d->SavingLocationLabel->setVisible(d->savingActive); 


  //Deactivate everything for null hierarchy or for pre op state not set
  if (!d->HierarchyNode || !d->PreOpSet)
  {
    d->ReferencesCollapsibleButton->setEnabled(false);
    d->MetricsCollapsibleButton->setEnabled(false);
    d->FinishButton->setEnabled(false);
    d->ModelHierarchyNodeComboBox->setEnabled(true);
    d->ModelHierarchyTreeView->setEnabled(false);
    d->BendingMenu->setVisible(false);
    d->CuttingMenu->setVisible(false);
    d->ScreenshotMenu->setVisible(false);
  }
  if (!d->HierarchyNode)
  {
    d->SaveDirectoryButton->setEnabled(false);
    d->EnableSavingCheckbox->setEnabled(false);
    d->SetPreOp->setEnabled(false);
  }

}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateCurrentCutNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  this->qvtkReconnect(d->CurrentCutNode, node,
                      vtkCommand::ModifiedEvent,
                      this, SLOT(updateWidgetFromMRML()));
  this->qvtkReconnect(d->CurrentCutNode, node,
                      vtkMRMLDisplayableNode::DisplayModifiedEvent,
                      this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));
  d->CurrentCutNode = node;
  if (node)
  {
      d->hideTransforms();

  }

  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateTemplateReferenceNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  this->qvtkReconnect(d->TemplateReferenceNode, node, vtkCommand::ModifiedEvent,
                      this, SLOT(updateWidgetFromMRML()));
  this->qvtkReconnect(d->TemplateReferenceNode, node,
                      vtkMRMLDisplayableNode::DisplayModifiedEvent,
                      this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));

  if(node && node != d->TemplateReferenceNode)
  {
    d->cliFreeze = true;
    d->cmdNode = this->plannerLogic()->createBoneTemplateModel(vtkMRMLModelNode::SafeDownCast(node));
    qvtkReconnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(finishWrap()));    
    d->MetricsProgress->setCommandLineModuleNode(d->cmdNode);
    d->TemplateVisibilityCheckbox->setEnabled(true);
  }
  d->TemplateReferenceNode = node;

  this->updateMRMLFromWidget();
}

//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::updateMRMLFromWidget()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->updateReferenceNodeFromWidget(
    d->TemplateReferenceNodeComboBox->currentNode(),
    d->TemplateReferenceColorPickerButton->color(),
    d->TemplateReferenceOpacitySliderWidget->value());

  //set visibility on scalars
  if(d->HierarchyNode)
  {
    d->setScalarVisibility(d->ShowsScalarsCheckbox->isChecked());
  }

  
  if(d->TemplateReferenceNode)
  {
    vtkMRMLModelNode::SafeDownCast(d->TemplateReferenceNode)->GetDisplayNode()->SetVisibility(d->TemplateVisibilityCheckbox->isChecked());
  }
}


//-----------------------------------------------------------------------------
void qSlicerPlannerModuleWidget::onOpenTemplateReference()
{
  Q_D(qSlicerPlannerModuleWidget);
  vtkMRMLNode* model = d->openReferenceDialog();
  if(model)
  {
    d->TemplateReferenceNodeComboBox->setCurrentNode(model);
  }
}

//-----------------------------------------------------------------------------
//View current cut results
void qSlicerPlannerModuleWidget::previewCutButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);

  if(d->cuttingActive)
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
  if (d->savingActive)
  {
    d->waitingOnScreenshot = true;
  }
  else
  {
    d->recordActionInProgress();
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Cancel current cutting action
void qSlicerPlannerModuleWidget::cancelCutButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->cancelCut(this->mrmlScene());
  d->cuttingActive = false;
  d->ActionInProgress.fill("");
  d->sceneModel()->setPlaneVisibility(d->CurrentCutNode, false);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Activate the placing of a specific fiducial
void qSlicerPlannerModuleWidget::placeFiducialButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  QObject* obj = sender();
  int success;
  if(obj == d->MovingPointAButton)
  {
    success = d->beginPlacement(this->mrmlScene(), 0);
  }
  if(obj == d->MovingPointBButton)
  {
    success = d->beginPlacement(this->mrmlScene(), 1);
  }
  if (success != EXIT_SUCCESS)
  {
      return;
  }

  d->scene = this->mrmlScene();
  qvtkReconnect(qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode(), vtkMRMLInteractionNode::EndPlacementEvent, this, SLOT(cancelFiducialButtonClicked()));
  return;
}

//-----------------------------------------------------------------------------
//Cancel the current bend and reset
void qSlicerPlannerModuleWidget::cancelBendButtonClicked()
{
  qvtkDisconnect(qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode(), vtkMRMLInteractionNode::EndPlacementEvent, this, SLOT(cancelFiducialButtonClicked()));
  Q_D(qSlicerPlannerModuleWidget);
  d->BendMagnitude = 0;
  d->BendMagnitudeSlider->setValue(0);
  if(d->bendingActive)
  {
    d->computeTransform(this->mrmlScene());
  }
  if (d->placingActive)
  {
    vtkMRMLInteractionNode* interaction = qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode();
    interaction->SetCurrentInteractionMode(vtkMRMLInteractionNode::ViewTransform);
  }
  d->clearControlPoints(this->mrmlScene());
  d->clearBendingData(this->mrmlScene());
  d->placingActive = false;
  d->bendingActive = false;
  d->bendingOpen = false;
  d->ActionInProgress.fill("");
  this->updateWidgetFromMRML();

}

//-----------------------------------------------------------------------------
//Update active model for bending from hierarchy
void qSlicerPlannerModuleWidget::updateCurrentBendNode(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  this->qvtkReconnect(d->CurrentBendNode, node,
                      vtkCommand::ModifiedEvent,
                      this, SLOT(updateWidgetFromMRML()));
  this->qvtkReconnect(d->CurrentBendNode, node,
                      vtkMRMLDisplayableNode::DisplayModifiedEvent,
                      this, SLOT(updateWidgetFromMRML(vtkObject*, vtkObject*)));

  d->CurrentBendNode = node;
  if(node)
  {
    d->hideTransforms();

  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Initialize bend transform based on current fiducials
void qSlicerPlannerModuleWidget::initBendButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->hideTransforms();
  d->hardenTransforms(true);
  d->computeAndSetSourcePoints(this->mrmlScene());
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
  this->updateBendButtonClicked();
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Finish and harden current bending action
void qSlicerPlannerModuleWidget::finshBendClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  
  d->hardenTransforms(false);
  d->BendMagnitude = 0;
  d->BendMagnitudeSlider->setValue(0);
  d->clearControlPoints(this->mrmlScene());
  d->clearBendingData(this->mrmlScene());
  d->bendingActive = false;
  d->bendingOpen = false;
  qvtkDisconnect(qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode(), vtkMRMLInteractionNode::EndPlacementEvent, this, SLOT(cancelFiducialButtonClicked()));
  if (d->savingActive)
  {
    d->waitingOnScreenshot = true;
  }
  else
  {
    d->recordActionInProgress();
  }
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
//Cancel placement of current fiducial
void qSlicerPlannerModuleWidget::cancelFiducialButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);  
  //qvtkDisconnect(qSlicerCoreApplication::application()->applicationLogic()->GetInteractionNode(), vtkMRMLInteractionNode::EndPlacementEvent, this, SLOT(cancelFiducialButtonClicked()));
  d->endPlacement();
  this->updateWidgetFromMRML();

}

//-----------------------------------------------------------------------------
//Begin wrapping of current model for metrics
void qSlicerPlannerModuleWidget::onComputeButton()
{
  Q_D(qSlicerPlannerModuleWidget);

  if(!d->modelMetricsTable)
  {
    vtkNew<vtkMRMLTableNode> table2;
    d->modelMetricsTable = table2.GetPointer();
    this->mrmlScene()->AddNode(d->modelMetricsTable);
    d->ModelMetrics->setMRMLTableNode(d->modelMetricsTable);
  }

  if(d->HierarchyNode)
  {
    std::vector<vtkMRMLHierarchyNode*> children;

    d->HierarchyNode->GetAllChildrenNodes(children);
    if(children.size() > 0)
    {
      d->hardenTransforms(false);
      std::cout << "Wrapping Current Model" << std::endl;
      d->cmdNode = this->plannerLogic()->createCurrentModel(d->HierarchyNode);
      qvtkReconnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(launchMetrics()));
      d->MetricsProgress->setCommandLineModuleNode(d->cmdNode);
    }
  }
}

//-----------------------------------------------------------------------------
//Create a pre-op model from the wrap of the current state
void qSlicerPlannerModuleWidget::onSetPreOP()
{
  Q_D(qSlicerPlannerModuleWidget);
  if(d->HierarchyNode)
  {
    std::vector<vtkMRMLHierarchyNode*> children;
    qSlicerApplication::application()->layoutManager()->setLayout(vtkMRMLLayoutNode::SlicerLayoutOneUp3DView);
    d->HierarchyNode->GetAllChildrenNodes(children);
    if(children.size() > 0)
    {
      d->SetPreOp->setEnabled(false);
      d->PreOpSet = true;
      d->cliFreeze = true;
      if (d->savingActive)
      {
        d->setUpSaveFiles();
      }
      d->ActionInProgress[0] = d->HierarchyNode->GetName();
      d->ActionInProgress[1] = "Initial State";
      if (d->savingActive)
      {
        d->waitingOnScreenshot = true;
      }
      else
      {
        d->recordActionInProgress();
      }
      
      d->cmdNode = this->plannerLogic()->createPreOPModels(d->HierarchyNode);
      qvtkReconnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(finishWrap()));

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
  vtkMRMLModelNode* distanceReference;

  if (d->InitialStateRadioButton->isChecked())
  {
    distanceReference = this->plannerLogic()->getWrappedPreOpModel();
  }
  else
  {
    distanceReference = this->plannerLogic()->getWrappedBoneTemplateModel();
  }  
    
  if (d->HierarchyNode && distanceReference)
  {
    d->hardenTransforms(false);
    d->ComputeScalarsButton->setEnabled(false);
    d->cliFreeze = true;
    this->updateWidgetFromMRML();
    d->prepScalarComputation(this->mrmlScene());
    d->cmdNode = d->distanceLogic->CreateNodeInScene();
    this->runModelDistance(distanceReference);
  }
}

//-----------------------------------------------------------------------------
//Catch end of model to model CLI and run next
void qSlicerPlannerModuleWidget::finishDistance()
{
  Q_D(qSlicerPlannerModuleWidget);
  if(d->cmdNode->GetStatus() == vtkMRMLCommandLineModuleNode::Completed)
  {
    vtkMRMLModelNode* distanceNode = vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetNodeByID(d->cmdNode->GetParameterAsString("vtkOutput")));
    int m = d->modelIterator.back()->StartModify();
    d->modelIterator.back()->SetAndObservePolyData(distanceNode->GetPolyData());
    d->modelIterator.back()->EndModify(m);
    this->mrmlScene()->RemoveNode(distanceNode);
    distanceNode = NULL;
    d->modelIterator.pop_back();
    this->runModelDistance(vtkMRMLModelNode::SafeDownCast(this->mrmlScene()->GetNodeByID(d->cmdNode->GetParameterAsString("vtkFile2"))));
  }
}

//-----------------------------------------------------------------------------
//Set up model to model distance CLI for next model in list
void qSlicerPlannerModuleWidget::runModelDistance(vtkMRMLModelNode* distRef)
{
  Q_D(qSlicerPlannerModuleWidget);
  if(!d->modelIterator.empty())
  {
    d->distanceLogic->SetMRMLScene(this->mrmlScene());
    vtkNew<vtkMRMLModelNode> temp;
    this->mrmlScene()->AddNode(temp.GetPointer());
    d->cmdNode->SetParameterAsString("vtkFile1", d->modelIterator.back()->GetID());
    d->cmdNode->SetParameterAsString("vtkFile2", distRef->GetID());
    d->cmdNode->SetParameterAsString("vtkOutput", temp->GetID());
    d->cmdNode->SetParameterAsString("distanceType", "absolute_closest_point");
    d->distanceLogic->Apply(d->cmdNode, true);
    qvtkReconnect(d->cmdNode, vtkMRMLCommandLineModuleNode::StatusModifiedEvent, this, SLOT(finishDistance()));
    d->MetricsProgress->setCommandLineModuleNode(d->cmdNode);
  }
  else
  {
    d->cliFreeze = false;
    this->updateWidgetFromMRML();
    d->ComputeScalarsButton->setEnabled(true);
    d->ShowsScalarsCheckbox->setEnabled(true);
    this->mrmlScene()->RemoveNode(d->cmdNode);
    this->updateMRMLFromWidget();
  }
}

//-----------------------------------------------------------------------------
//Catch the end of the WrapModels CLI and clean up
void qSlicerPlannerModuleWidget::finishWrap()
{
  Q_D(qSlicerPlannerModuleWidget);
  if(d->cmdNode->GetStatus() == vtkMRMLCommandLineModuleNode::Completed)
  {
    this->plannerLogic()->finishWrap(d->cmdNode);
    d->cliFreeze = false;
    this->updateWidgetFromMRML();
  }
}

//-----------------------------------------------------------------------------
//Catch end of metrics wrap and being metrics computation
void qSlicerPlannerModuleWidget::launchMetrics()
{
  Q_D(qSlicerPlannerModuleWidget);
  if(d->cmdNode->GetStatus() == vtkMRMLCommandLineModuleNode::Completed)
  {
    this->plannerLogic()->finishWrap(d->cmdNode);
    this->plannerLogic()->fillMetricsTable(d->HierarchyNode, d->modelMetricsTable);
    this->updateWidgetFromMRML();
  }
}

//-----------------------------------------------------------------------------
//Call clean up code for module
void qSlicerPlannerModuleWidget::finishPlanButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);

  
  if (d->HierarchyNode)
  {
    d->hideTransforms();
    d->hardenTransforms(false);
    d->clearControlPoints(this->mrmlScene());
    d->clearSavingData();
    this->plannerLogic()->clearModelsAndData();
    d->PreOpSet = false;
    d->ShowsScalarsCheckbox->setEnabled(false);
  }

  //Clear out hierarchy
  vtkMRMLModelHierarchyNode* tempH = d->HierarchyNode;
  d->ModelHierarchyNodeComboBox->setCurrentNode(NULL);
  d->HierarchyNode = NULL;

  d->removePlanes(this->mrmlScene(), tempH);
  d->removeTransforms(this->mrmlScene(), tempH);
  d->untagModels(this->mrmlScene(), tempH);
  this->updateWidgetFromMRML();
}

void qSlicerPlannerModuleWidget::modelCallback(const QModelIndex &index)
{
    Q_D(qSlicerPlannerModuleWidget);

    if (d->cuttingActive || d->bendingOpen)
    {
        std::cout << "Busy!" << std::endl;
        return;
    }
    
    QModelIndex sourceIndex = d->ModelHierarchyTreeView->sortFilterProxyModel()->mapToSource(index);
    vtkMRMLNode* node = d->ModelHierarchyTreeView->sceneModel()->mrmlNodeFromIndex(sourceIndex);

    if (sourceIndex.column() == 5)
    {
        std::stringstream title;
        title << "Cutting model: " << node->GetName();
        d->CuttingMenu->setTitle(title.str().c_str());
        d->hardenTransforms(false);        
        this->updateCurrentCutNode(node);
        this->previewCutButtonClicked();
    }
    if (sourceIndex.column() == 6)
    {
        std::stringstream title;
        d->ActionInProgress[0] = node->GetName();
        d->ActionInProgress[1] = "Bend";
        title << "Bending model: " << node->GetName();
        d->BendingMenu->setTitle(title.str().c_str());
        d->hardenTransforms(false);        
        d->BendingInfoLabel->setText("Place Point A and Point B to define the bending axis (line you want the model to bend around).");
        this->updateCurrentBendNode(node);
        d->MovingPointAButton->setEnabled(true);
        d->MovingPointBButton->setEnabled(true);
        d->bendingOpen = true;
        this->updateWidgetFromMRML();
    }
    
    
}

void qSlicerPlannerModuleWidget::transformActivated(vtkMRMLNode* node)
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->cuttingActive || d->bendingActive)
  {
    return;
  }
  d->moveActive = true;
  d->ActionInProgress[0] = node->GetName();
  d->ActionInProgress[1] = "Move";
  this->updateWidgetFromMRML();
}

void qSlicerPlannerModuleWidget::confirmMoveButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->cuttingActive || d->bendingActive || !d->moveActive)
  {
    return;
  }
  d->hideTransforms();
  d->hardenTransforms(false);
  d->moveActive = false;
  if (d->savingActive)
  {
    d->waitingOnScreenshot = true;
  }
  else
  {
    d->recordActionInProgress();
  }
  this->updateWidgetFromMRML();

}

void qSlicerPlannerModuleWidget::cancelMoveButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->cuttingActive || d->bendingActive || !d->moveActive)
  {
    return;
  }
  d->hideTransforms();
  d->clearTransforms();
  d->moveActive = false;
  d->ActionInProgress.fill("");
  this->updateWidgetFromMRML();  
}

void qSlicerPlannerModuleWidget::saveDirectoryChanged(const QString &directory)
{
  Q_D(qSlicerPlannerModuleWidget);
  if (d->savingActive)
  {
    return;
  }
  d->RootDirectory = directory;
  d->savingActive = (d->RootDirectory != "");
  if (d->savingActive)
  {
    const QDateTime now = QDateTime::currentDateTime();
    const QString timestamp = now.toString(QLatin1String("yyyyMMdd-hhmmsszzz"));
    std::stringstream ssDirectoryName;
    ssDirectoryName << d->HierarchyNode->GetName() << "_" << timestamp.toStdString();
    std::vector<std::string> pathComponents;
    QDir rootDir = QDir(d->RootDirectory);
    d->SaveDirectory = rootDir.absoluteFilePath(ssDirectoryName.str().c_str());
    d->SavingLocationLabel->setText(d->SaveDirectory);

  }
  
  this->updateWidgetFromMRML();
}

void qSlicerPlannerModuleWidget::enabledSavingCheckboxToggled(bool state)
{
  Q_D(qSlicerPlannerModuleWidget);

  if (state && !d->savingActive)
  {
    d->SaveDirectoryButton->browse();
    std::cout << "Saving to: " << d->RootDirectory.toStdString().c_str() << std::endl;
  }
  if (d->PreOpSet)
  {
    d->setUpSaveFiles();
    d->writeOutActions();
  }
}
void qSlicerPlannerModuleWidget::takeScreenshotButtonClicked()
{
  Q_D(qSlicerPlannerModuleWidget);
  d->recordActionInProgress();
  d->waitingOnScreenshot = false;
  this->updateWidgetFromMRML();
}

