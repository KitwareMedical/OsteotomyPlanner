import os
import unittest
import logging
import vtk, qt, ctk, slicer
import numpy as np
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin
from ModelHistory.ModelHistory import ModelHistory as ModelHistory
from vtk.util.numpy_support import vtk_to_numpy as vtk_to_numpy
from scipy.spatial.transform import Rotation as R

#
# OsteotomyPlanner
#

class OsteotomyPlanner(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "OsteotomyPlanner"  # TODO: make this more human readable by adding spaces
    self.parent.categories = ["Surface Models"]  # TODO: set categories (folders where the module shows up in the module selector)
    self.parent.dependencies = []  # TODO: add here list of module names that this module requires
    self.parent.contributors = ["Sam Horvath (Kitware Inc.)"]  # TODO: replace with "Firstname Lastname (Organization)"
    # TODO: update with short description of the module and a link to online module documentation
    self.parent.helpText = """
Osteotomy Planner v2.0
"""
    # TODO: replace with organization, grant and thanks
    self.parent.acknowledgementText = """
This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc., Andras Lasso, PerkLab,
and Steve Pieper, Isomics, Inc. and was partially funded by NIH grant 3P41RR013218-12S1.
"""

    # Additional initialization step after application startup is complete

#
# Register sample data sets in Sample Data module
#

# def registerSampleData():
#   """
#   Add data sets to Sample Data module.
#   """
#   # It is always recommended to provide sample data for users to make it easy to try the module,
#   # but if no sample data is available then this method (and associated startupCompeted signal connection) can be removed.

#   import SampleData
#   iconsPath = os.path.join(os.path.dirname(__file__), 'Resources/Icons')

#   # To ensure that the source code repository remains small (can be downloaded and installed quickly)
#   # it is recommended to store data sets that are larger than a few MB in a Github release.

#   # OsteotomyPlanner1
#   SampleData.SampleDataLogic.registerCustomSampleDataSource(
#     # Category and sample name displayed in Sample Data module
#     category='OsteotomyPlanner',
#     sampleName='OsteotomyPlanner1',
#     # Thumbnail should have size of approximately 260x280 pixels and stored in Resources/Icons folder.
#     # It can be created by Screen Capture module, "Capture all views" option enabled, "Number of images" set to "Single".
#     thumbnailFileName=os.path.join(iconsPath, 'OsteotomyPlanner1.png'),
#     # Download URL and target file name
#     uris="https://github.com/Slicer/SlicerTestingData/releases/download/SHA256/998cb522173839c78657f4bc0ea907cea09fd04e44601f17c82ea27927937b95",
#     fileNames='OsteotomyPlanner1.nrrd',
#     # Checksum to ensure file integrity. Can be computed by this command:
#     #  import hashlib; print(hashlib.sha256(open(filename, "rb").read()).hexdigest())
#     checksums = 'SHA256:998cb522173839c78657f4bc0ea907cea09fd04e44601f17c82ea27927937b95',
#     # This node name will be used when the data set is loaded
#     nodeNames='OsteotomyPlanner1'
#   )

#   # OsteotomyPlanner2
#   SampleData.SampleDataLogic.registerCustomSampleDataSource(
#     # Category and sample name displayed in Sample Data module
#     category='OsteotomyPlanner',
#     sampleName='OsteotomyPlanner2',
#     thumbnailFileName=os.path.join(iconsPath, 'OsteotomyPlanner2.png'),
#     # Download URL and target file name
#     uris="https://github.com/Slicer/SlicerTestingData/releases/download/SHA256/1a64f3f422eb3d1c9b093d1a18da354b13bcf307907c66317e2463ee530b7a97",
#     fileNames='OsteotomyPlanner2.nrrd',
#     checksums = 'SHA256:1a64f3f422eb3d1c9b093d1a18da354b13bcf307907c66317e2463ee530b7a97',
#     # This node name will be used when the data set is loaded
#     nodeNames='OsteotomyPlanner2'
#   )

#
# OsteotomyPlannerWidget
#

class OsteotomyPlannerWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
  """Uses ScriptedLoadableModuleWidget base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent=None):
    """
    Called when the user opens the module the first time and the widget is initialized.
    """
    ScriptedLoadableModuleWidget.__init__(self, parent)
    VTKObservationMixin.__init__(self)  # needed for parameter node observation
    self.logic = None
    self._parameterNode = None
    self._updatingGUIFromParameterNode = False

  def setup(self):
    """
    Called when the user opens the module the first time and the widget is initialized.
    """
    ScriptedLoadableModuleWidget.setup(self)

    # Load widget from .ui file (created by Qt Designer).
    # Additional widgets can be instantiated manually and added to self.layout.
    uiWidget = slicer.util.loadUI(self.resourcePath('UI/OsteotomyPlanner.ui'))
    self.layout.addWidget(uiWidget)
    self.ui = slicer.util.childWidgetVariables(uiWidget)

    # Set scene in MRML widgets. Make sure that in Qt designer the top-level qMRMLWidget's
    # "mrmlSceneChanged(vtkMRMLScene*)" signal in is connected to each MRML widget's.
    # "setMRMLScene(vtkMRMLScene*)" slot.
    uiWidget.setMRMLScene(slicer.mrmlScene)

    # Create logic class. Logic implements all computations that should be possible to run
    # in batch mode, without a graphical user interface.
    self.logic = OsteotomyPlannerLogic()

    # Connections

    # These connections ensure that we update parameter node when scene is closed
    self.addObserver(slicer.mrmlScene, slicer.mrmlScene.StartCloseEvent, self.onSceneStartClose)
    self.addObserver(slicer.mrmlScene, slicer.mrmlScene.EndCloseEvent, self.onSceneEndClose)

    #Set up SubjectHieratchy combo
    nodeTypes = ['vtkMRMLFolderDisplayNode']
    levelFilters = [slicer.vtkMRMLSubjectHierarchyConstants().GetDICOMLevelPatient(), slicer.vtkMRMLSubjectHierarchyConstants().GetDICOMLevelStudy(), slicer.vtkMRMLSubjectHierarchyConstants().GetSubjectHierarchyLevelFolder()]
    self.ui.SubjectHierarchyComboBox.setNodeTypes(nodeTypes)
    self.ui.SubjectHierarchyComboBox.setLevelFilter(levelFilters)
    self.ui.SubjectHierarchyComboBox.connect("currentItemChanged(vtkIdType)", self.onFolderChanged)
    self.ui.SubjectHierarchyTreeView.connect("currentItemChanged(vtkIdType)", self.onViewItemChanged)
    self.ui.SubjectHierarchyTreeView.transformColumnVisible = False
    self.ui.SubjectHierarchyTreeView.idColumnVisible = False
    self.ui.AllModelsSubjectHierarchyTreeView.transformColumnVisible = False
    self.ui.AllModelsSubjectHierarchyTreeView.idColumnVisible = False
    self.ui.AllModelsSubjectHierarchyTreeView.sortFilterProxyModel().setExcludeItemAttributeNamesFilter(['NodeInPlanner'])



    #Selection 
    self.ui.SubjectHierarchyTreeView.setSelectionMode(qt.QAbstractItemView().ExtendedSelection)
    self.ui.ActionsWidget.setCurrentWidget(self.ui.MenuWidget)
    self.ui.MenuWidget.enabled = False

    self.ui.MoveButton.clicked.connect(self.beginMove)
    moveIcon = qt.QIcon(":/Icons/Transform.png")
    self.ui.MoveButton.setIcon(moveIcon)
    self.ui.MoveCancelButton.clicked.connect(self.endMove)
    self.ui.MoveConfirmButton.clicked.connect(self.confirmMove)

    self.ui.CurveCutButton.clicked.connect(self.beginCurveCut)
    curveCutIcon = qt.QIcon(":/Icons/CurveCut.png")
    self.ui.CurveCutButton.setIcon(curveCutIcon)
    self.ui.PlaceCurveButton.clicked.connect(self.placeManualCurve)
    self.ui.PreviewCurveCutButton.clicked.connect(self.previewCurveCut)
    self.ui.CurveCutConfirmButton.clicked.connect(self.confirmCurveCut)
    self.ui.CurveCutCancelButton.clicked.connect(self.endCurveCut)

    self.ui.SplitButton.clicked.connect(self.beginSplit)
    splitIcon = qt.QIcon(":/Icons/DynamicModeler.png")
    self.ui.SplitButton.setIcon(splitIcon)
    self.ui.SplitCancelButton.clicked.connect(self.endSplit)
    self.ui.ManualPlaneButton.clicked.connect(self.placeManualPlane)
    self.ui.ClearPlaneButton.clicked.connect(self.clearPlanes)
    self.ui.AutoPlaneButton.clicked.connect(self.placeAutoPlane)
    self.ui.PreviewSplitButton.clicked.connect(self.previewSplit)
    self.ui.SplitConfirmButton.clicked.connect(self.confirmSplit)

    self.ui.RemoveButton.clicked.connect(self.beginRemove)
    removeIcon = qt.QIcon(":/Icons/Medium/SlicerEditCut.png")
    self.ui.RemoveButton.setIcon(removeIcon)
    self.ui.RemoveConfirmButton.clicked.connect(self.confirmRemove)
    self.ui.RemoveCancelButton.clicked.connect(self.endRemove)

    self.ui.BendButton.clicked.connect(self.beginBend)
    BendIcon = qt.QIcon(self.resourcePath('Icons/Bending.png'))
    self.ui.BendButton.setIcon(BendIcon)
    self.ui.BendPlaceAxisButton.clicked.connect(self.placeBendAxis)
    self.ui.BendInitializeButton.clicked.connect(self.initializeBend)
    self.ui.BendConfirmButton.clicked.connect(self.confirmBend)
    self.ui.BendCancelButton.clicked.connect(self.endBend)
    self.ui.BendRadiusSlider.setMinimum(0)
    self.ui.BendRadiusSlider.setMaximum(100)
    self.ui.BendRadiusSlider.setSliderPosition(10)
    self.ui.BendRadiusSlider.valueChanged.connect(self.updateBendTransform)
    self.ui.BendMagnitudeSlider.setMinimum(-15)
    self.ui.BendMagnitudeSlider.setMaximum(15)
    self.ui.BendMagnitudeSlider.valueChanged.connect(self.updateBendTransform)
    self.ui.BendSideACheckBox.stateChanged.connect(self.updateBendTransform)
    self.ui.BendSideBCheckBox.stateChanged.connect(self.updateBendTransform)

    self.ui.FinishButton.clicked.connect(self.finishPlan)
    finishIcon = qt.QIcon(":/Icons/AnnotationOkDone.png")
    self.ui.FinishButton.setIcon(finishIcon)
    self.ui.RedoButton.clicked.connect(self.restoreNextState)
    redoIcon = qt.QIcon(":/Icons/Medium/SlicerRedo.png")
    self.ui.RedoButton.setIcon(redoIcon)
    self.ui.UndoButton.clicked.connect(self.restorePreviousState)
    undoIcon = qt.QIcon(":/Icons/Medium/SlicerUndo.png")
    self.ui.UndoButton.setIcon(undoIcon)
    # self.ui.RedoButton.visible = False

    self.ui.HideModelsButton.clicked.connect(self.onHideModels)
    self.ui.ShowAsTemplateButton.clicked.connect(self.onShowModelAsTemplate)
    self.ui.ComputeDistanceButton.clicked.connect(self.onComputeDistance)

    self.transform = None
    self.activeNodes = []
    self.curve = None
    self.splitPlanes = []
    self.splitModels = []
    self.bendAxis = None
    self.bendAxisNode = None
    self.bendPlane = None
    self.bendMaxRadius = 0.0
    self.bendTPS = None
    self.bendDownSample = None
    self.actionInProgress = False
    self.modelHistory = ModelHistory()
    self.activeFolder = None 

    # Make sure parameter node is initialized (needed for module reload)
    self.initializeParameterNode()
    self.resolveStateButtons()

    self.ui.TabWidget.setCurrentWidget(self.ui.ActionTab)
    self.onTabChanged(self.ui.TabWidget.indexOf(self.ui.ActionTab))
    self.ui.TabWidget.currentChanged.connect(self.onTabChanged)

  
  # Split Action
  def beginSplit(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.SplitWidget)
    self.ui.PreviewSplitButton.enabled = False
    self.ui.SplitConfirmButton.enabled = False
    self.ui.SplitLabel.text = ''
    self.beginAction()
  
  def placeManualPlane(self):
    interactionNode = slicer.app.applicationLogic().GetInteractionNode()
    selectionNode = slicer.app.applicationLogic().GetSelectionNode()
    selectionNode.SetReferenceActivePlaceNodeClassName("vtkMRMLMarkupsPlaneNode")
    planeNode = slicer.vtkMRMLMarkupsPlaneNode()
    slicer.mrmlScene.AddNode(planeNode)
    planeNode.CreateDefaultDisplayNodes()
    selectionNode.SetActivePlaceNodeID(planeNode.GetID())
    interactionNode.SetCurrentInteractionMode(interactionNode.Place)
    self.splitPlanes.append(planeNode)
    self.ui.PreviewSplitButton.enabled = True

  def placeAutoPlane(self):
    planeNode = slicer.vtkMRMLMarkupsPlaneNode()
    slicer.mrmlScene.AddNode(planeNode)
    planeNode.CreateDefaultDisplayNodes() 
    self.splitPlanes.append(planeNode)

    bounds = [0,0,0,0,0,0]
    self.activeNodes[0].GetRASBounds(bounds)
    for activeNode in self.activeNodes:
      currentBound = [0,0,0,0,0,0]
      activeNode.GetRASBounds(currentBound)
      bounds = [min(bounds[0], currentBound[0]),
                max(bounds[1], currentBound[1]),
                min(bounds[2], currentBound[2]),
                max(bounds[3], currentBound[3]),
                min(bounds[4], currentBound[4]),
                max(bounds[5], currentBound[5])]

    # TODO: Clean this up to be clearer about what is being calculated, maybe add a function
    center = [((bounds[1] - bounds[0]) / 2) + bounds[0], ((bounds[3] - bounds[2]) / 2) + bounds[2], ((bounds[5] - bounds[4]) / 2) + bounds[4]]
    p2 = [((bounds[1] - bounds[0]) / 2) + bounds[0]+(bounds[1] - bounds[0])*1.5/2, ((bounds[3] - bounds[2]) / 2) + bounds[2], ((bounds[5] - bounds[4]) / 2) + bounds[4]]
    p3 = [((bounds[1] - bounds[0]) / 2) + bounds[0], ((bounds[3] - bounds[2]) / 2) + bounds[2]+(bounds[3] - bounds[2])*1.5/2, ((bounds[5] - bounds[4]) / 2) + bounds[4]]

    planeNode.AddControlPoint(vtk.vtkVector3d(center))
    planeNode.AddControlPoint(vtk.vtkVector3d(p2))
    planeNode.AddControlPoint(vtk.vtkVector3d(p3))
    self.ui.PreviewSplitButton.enabled = True

  def previewSplit(self):
    self.clearSplitModels()
    dynamicModelerNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLDynamicModelerNode")
    dynamicModelerNode.SetToolName("Plane cut")
    dynamicModelerNode.SetAttribute("OperationType", self.ui.OperationTypeComboBox.currentText)
    dynamicModelerNode.SetAttribute("CapSurface", str(int(self.ui.CapModelCheckBox.isChecked())))
    for plane in self.splitPlanes:
      dynamicModelerNode.AddNodeReferenceID("PlaneCut.InputPlane", plane.GetID())

    for activeNode in self.activeNodes:
      positiveModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode", activeNode.GetName() + '-positive')
      positiveModel.CreateDefaultDisplayNodes()
      positiveModel.GetDisplayNode().SetColor(1.0,0,0)
      negativeModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode", activeNode.GetName() + '-negative')
      negativeModel.CreateDefaultDisplayNodes()
      negativeModel.GetDisplayNode().SetColor(0,0,1.0)

      dynamicModelerNode.SetNodeReferenceID("PlaneCut.InputModel", activeNode.GetID())
      dynamicModelerNode.SetNodeReferenceID("PlaneCut.OutputPositiveModel", positiveModel.GetID())
      dynamicModelerNode.SetNodeReferenceID("PlaneCut.OutputNegativeModel", negativeModel.GetID())
      slicer.modules.dynamicmodeler.logic().RunDynamicModelerTool(dynamicModelerNode)

      self.splitModels.append(positiveModel)
      self.splitModels.append(negativeModel)

    for splitModel in self.splitModels:
      if self.isModelValid(splitModel) is False:
        self.ui.SplitLabel.text = 'Failed - check planes'
        slicer.mrmlScene.RemoveNode(dynamicModelerNode)
        return

    self.ui.SplitConfirmButton.enabled = True
    self.ui.SplitLabel.text = 'Success'
    slicer.mrmlScene.RemoveNode(dynamicModelerNode)
      
  def confirmSplit(self):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    folderItem = self.ui.SubjectHierarchyComboBox.currentItem()
    selectItem = None
    for model in self.splitModels:
      item = shNode.GetItemByDataNode(model)
      shNode.SetItemParent(item, folderItem)
      selectItem = item
    self.splitModels = []
    for activeNode in self.activeNodes:
      self.removeNode(activeNode)
    #need to force selection of something
    self.ui.SubjectHierarchyTreeView.setCurrentItem(selectItem)
    self.confirmAction()
    self.endSplit()

  def endSplit(self):
    self.clearPlanes()
    self.clearSplitModels()
    self.endAction()  
  
  def clearSplitModels(self):
    for model in self.splitModels:
      self.removeNode(model)
    self.splitModels = []

  def clearPlanes(self):
    for plane in self.splitPlanes:
      slicer.mrmlScene.RemoveNode(plane)
    self.splitPlanes = []

  # Curve action
  def beginCurveCut(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.CurveCutWidget)
    self.ui.PlaceCurveButton.text = "Place curve"
    self.ui.CurveCutLabel.text = ''
    self.ui.PreviewCurveCutButton.enabled = False
    self.ui.CurveCutConfirmButton.enabled = False
    self.beginAction()

  def placeManualCurve(self):
    self.clearCurve()
    interactionNode = slicer.app.applicationLogic().GetInteractionNode()
    selectionNode = slicer.app.applicationLogic().GetSelectionNode()
    selectionNode.SetReferenceActivePlaceNodeClassName("vtkMRMLMarkupsCurveNode")
    curveNode = slicer.vtkMRMLMarkupsCurveNode()
    slicer.mrmlScene.AddNode(curveNode)
    curveNode.CreateDefaultDisplayNodes() 
    selectionNode.SetActivePlaceNodeID(curveNode.GetID())
    interactionNode.SetCurrentInteractionMode(interactionNode.Place)
    self.curve = curveNode
    self.ui.PreviewCurveCutButton.enabled = True

  def previewCurveCut(self):
    self.clearSplitModels()
    dynamicModelerNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLDynamicModelerNode")
    dynamicModelerNode.SetToolName("Curve cut")
    dynamicModelerNode.AddNodeReferenceID("CurveCut.InputCurve", self.curve.GetID())

    for activeNode in self.activeNodes:
      insideModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode", activeNode.GetName() + '-inside')
      insideModel.CreateDefaultDisplayNodes()
      insideModel.GetDisplayNode().SetColor(1.0,0,0)
      outsideModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode", activeNode.GetName() + '-outside')
      outsideModel.CreateDefaultDisplayNodes()
      outsideModel.GetDisplayNode().SetColor(0,0,1.0)

      dynamicModelerNode.SetNodeReferenceID("CurveCut.InputModel", activeNode.GetID())
      dynamicModelerNode.SetNodeReferenceID("CurveCut.OutputInside", insideModel.GetID())
      dynamicModelerNode.SetNodeReferenceID("CurveCut.OutputOutside", outsideModel.GetID())
      slicer.modules.dynamicmodeler.logic().RunDynamicModelerTool(dynamicModelerNode)

      self.splitModels.append(insideModel)
      self.splitModels.append(outsideModel)

    for splitModel in self.splitModels:
      if self.isModelValid(splitModel) is False:
        self.ui.CurveCutLabel.text = 'Failed - check curve'
        slicer.mrmlScene.RemoveNode(dynamicModelerNode)
        return

    self.ui.CurveCutConfirmButton.enabled = True
    self.ui.CurveCutLabel.text = 'Success'
    slicer.mrmlScene.RemoveNode(dynamicModelerNode)
   
  def confirmCurveCut(self):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    folderItem = self.ui.SubjectHierarchyComboBox.currentItem()
    selectItem = None
    for model in self.splitModels:
      item = shNode.GetItemByDataNode(model)
      shNode.SetItemParent(item, folderItem)
      selectItem = item
    self.splitModels = []
    for activeNode in self.activeNodes:
      self.removeNode(activeNode)
    #need to force selection of something
    self.ui.SubjectHierarchyTreeView.setCurrentItem(selectItem)
    self.confirmAction()
    self.endCurveCut()
  
  def endCurveCut(self):
    self.clearCurve()
    self.clearSplitModels()
    self.endAction()

  def clearCurve(self):
    if self.curve is not None:
      self.removeNode(self.curve)

  
  # Move action
  def beginMove(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.MoveWidget)
    self.beginAction()
    self.createMoveTransform()

  def createMoveTransform(self):
    self.transform = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLLinearTransformNode')
    self.transform.CreateDefaultDisplayNodes()
    display = self.transform.GetDisplayNode()
    for activeNode in self.activeNodes:
      activeNode.SetAndObserveTransformNodeID(self.transform.GetID())
    display.UpdateEditorBounds()
    display.EditorScalingEnabledOff()
    display.EditorVisibilityOn()
    self.ui.MRMLTransformSlidersTranslation.setMRMLTransformNode(self.transform)
    self.ui.MRMLTransformSlidersRotation.setMRMLTransformNode(self.transform)

  
  def confirmMove(self):
    logic = slicer.vtkSlicerTransformLogic()
    for activeNode in self.activeNodes:
      logic.hardenTransform(activeNode)
    self.confirmAction()
    self.endMove()

  def endMove(self):
    for activeNode in self.activeNodes:
      activeNode.SetAndObserveTransformNodeID(None)
    slicer.mrmlScene.RemoveNode(self.transform)
    self.ui.MRMLTransformSlidersTranslation.reset()
    self.ui.MRMLTransformSlidersRotation.reset()
    self.ui.MRMLTransformSlidersTranslation.setMRMLTransformNode(None)
    self.ui.MRMLTransformSlidersRotation.setMRMLTransformNode(None)
    self.transform = None
    self.endAction()

  #Remove action
  def beginRemove(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.RemoveWidget)
    self.beginAction()

  def confirmRemove(self):
    for activeNode in self.activeNodes:
      self.removeNode(activeNode)
    self.confirmAction()
    self.endRemove()

  def endRemove(self):
    self.endAction()

  # Bend action
  def beginBend(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.BendWidget)
    self.ui.BendInitializeButton.enabled = False
    self.ui.BendRadiusSlider.enabled = False
    self.ui.BendMagnitudeSlider.enabled = False
    self.beginAction()
    if len(self.activeNodes) != 1:
      print("Warning: multiple models are selected, please choose only one.")
      self.endAction()

  def placeBendAxis(self):
    interactionNode = slicer.app.applicationLogic().GetInteractionNode()
    selectionNode = slicer.app.applicationLogic().GetSelectionNode()
    selectionNode.SetReferenceActivePlaceNodeClassName("vtkMRMLMarkupsLineNode")
    lineNode = slicer.vtkMRMLMarkupsLineNode()
    slicer.mrmlScene.AddNode(lineNode)
    lineNode.CreateDefaultDisplayNodes()
    selectionNode.SetActivePlaceNodeID(lineNode.GetID())
    interactionNode.SetCurrentInteractionMode(interactionNode.Place)
    self.bendAxisNode = lineNode
    self.ui.BendInitializeButton.enabled = True

  def initializeBend(self):
    # In case bend axis is moved for re-initialization
    if self.transform is not None:
      slicer.mrmlScene.RemoveNode(self.transform)
      self.transform = None
      self.activeNodes[0].SetAndObserveTransformNodeID(None)

    # Check parent transform
    parentTransform = self.activeNodes[0].GetParentTransformNode()
    if parentTransform is not None:
      print("Bending initialization failed: parent transform is not None," +
            " please harden existing transform and initialize bending again.")
      return

    # Create transform
    self.transform = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLTransformNode')
    self.bendTPS = vtk.vtkThinPlateSplineTransform()
    self.bendTPS.SetSigma(.0001)
    self.bendTPS.SetBasisToR()
    self.transform.SetAndObserveTransformToParent(self.bendTPS)
    self.activeNodes[0].SetAndObserveTransformNodeID(self.transform.GetID())

    # Create bend plane
    normals = vtk.vtkPolyDataNormals()
    normals.ComputePointNormalsOn()
    normals.ComputeCellNormalsOn()
    normals.AutoOrientNormalsOn()
    normals.SetInputData(self.activeNodes[0].GetPolyData())
    normals.Update()

    cellLocator = vtk.vtkCellLocator()
    cellLocator.SetDataSet(normals.GetOutput())
    cellLocator.BuildLocator()

    A = np.array(self.bendAxisNode.GetNthControlPointPositionVector(0))
    B = np.array(self.bendAxisNode.GetNthControlPointPositionVector(1))
    AB = B - A
    ABMid = A + 0.5 * AB
    self.bendAxis = AB / np.linalg.norm(AB)

    cellId = vtk.mutable(0)
    cellLocator.FindClosestPoint(ABMid.tolist(), [0.0, 0.0, 0.0], cellId, vtk.mutable(0), vtk.mutable(0))
    normal = np.array(normals.GetOutput().GetCellData().GetNormals().GetTuple(cellId))
    self.bendPlane = vtk.vtkPlane()
    self.bendPlane.SetOrigin(ABMid)
    self.bendPlane.SetNormal(np.cross(self.bendAxis, normal))

    # Create down-sampling point set
    verts = vtk.vtkVertexGlyphFilter()
    verts.SetInputData(normals.GetOutput())
    verts.Update()
    clean = vtk.vtkCleanPolyData()
    clean.SetInputData(verts.GetOutput())
    # TODO: use some metrics to calculate tolerance
    clean.SetTolerance(0.07)
    clean.Update()
    self.bendDownSample = clean.GetOutput().GetPoints()  # vtkPoints

    # Update max radius
    self.bendMaxRadius = 0.0
    for i in range(self.bendDownSample.GetNumberOfPoints()):
      radius = self.bendPlane.EvaluateFunction(self.bendDownSample.GetPoint(i))
      if abs(radius) > self.bendMaxRadius:
        self.bendMaxRadius = abs(radius)
    print(str(self.bendDownSample.GetNumberOfPoints()) + ' points\n')
    print('Max radius:' + str(self.bendMaxRadius))

    self.ui.BendRadiusSlider.enabled = True
    self.ui.BendMagnitudeSlider.enabled = True
    self.updateBendTransform()

  def updateBendTransform(self):
    # Check status
    radius = self.ui.BendRadiusSlider.sliderPosition * self.bendMaxRadius / 100.0
    magnitude = self.ui.BendMagnitudeSlider.sliderPosition / 10.0
    sideA = self.ui.BendSideACheckBox.isChecked()
    sideB = self.ui.BendSideBCheckBox.isChecked()

    # Compute Transform
    sourcePoints = vtk.vtkPoints()
    targetPoints = vtk.vtkPoints()
    for i in range(self.bendDownSample.GetNumberOfPoints()):
      sourcePoint = self.bendDownSample.GetPoint(i)
      if self.bendPlane.EvaluateFunction(sourcePoint) > radius:
        sourcePoints.InsertNextPoint(sourcePoint)
        if sideA:
          r = R.from_rotvec(magnitude * self.bendAxis)
          center = np.array(self.bendPlane.GetOrigin())
          targetPoint = (r.as_matrix() @ (np.array(sourcePoint) - center)) + center
          targetPoints.InsertNextPoint(targetPoint.tolist())
        else:
          targetPoints.InsertNextPoint(sourcePoint)
      if self.bendPlane.EvaluateFunction(sourcePoint) < -radius:
        sourcePoints.InsertNextPoint(sourcePoint)
        if sideB:
          r = R.from_rotvec(-magnitude * self.bendAxis)
          center = np.array(self.bendPlane.GetOrigin())
          targetPoint = (r.as_matrix() @ (np.array(sourcePoint) - center)) + center
          targetPoints.InsertNextPoint(targetPoint.tolist())
        else:
          targetPoints.InsertNextPoint(sourcePoint)

    self.bendTPS.SetSourceLandmarks(sourcePoints)
    self.bendTPS.SetTargetLandmarks(targetPoints)
    self.bendTPS.Update()

  def confirmBend(self):
    logic = slicer.vtkSlicerTransformLogic()
    logic.hardenTransform(self.activeNodes[0])
    self.confirmAction()
    self.endBend()

  def endBend(self):
    self.activeNodes[0].SetAndObserveTransformNodeID(None)
    slicer.mrmlScene.RemoveNode(self.transform)
    slicer.mrmlScene.RemoveNode(self.bendAxisNode)
    self.transform = None
    self.bendAxisNode = None
    self.bendAxis = None
    self.bendPlane = None
    self.bendMaxRadius = 0.0
    self.bendTPS = None
    self.bendDownSample = None
    self.endAction()

  #General
  def onShowModelAsTemplate(self):
    item = self.ui.AllModelsSubjectHierarchyTreeView.currentItem()
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    node = shNode.GetItemDataNode(item)
    if node is not None:
      node.SetDisplayVisibility(True)
      node.GetDisplayNode().SetOpacity(0.2)

  
  def onHideModels(self):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    modelNodes = slicer.util.getNodesByClass('vtkMRMLModelNode')
    for node in modelNodes:         
      item = shNode.GetItemByDataNode(node)
      if shNode.GetItemAttribute(item, 'NodeInPlanner') == '':
        node.SetDisplayVisibility(False)

  def onComputeDistance(self):
    referenceItem = self.ui.AllModelsSubjectHierarchyTreeView.currentItem()
    subjectItem = self.ui.SubjectHierarchyTreeView.currentItem()
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    subjectNode = shNode.GetItemDataNode(subjectItem)
    referenceNode = shNode.GetItemDataNode(referenceItem)
    if subjectNode is not None and referenceNode is not None:

      distanceFilter = vtk.vtkDistancePolyDataFilter()
      distanceFilter.SetInputData(0, subjectNode.GetPolyData())
      distanceFilter.SetInputData(1, referenceNode.GetPolyData())
      distanceFilter.SignedDistanceOff()
      distanceFilter.Update()

      # update polydata
      vtkDistanceArray = distanceFilter.GetOutput().GetPointData().GetScalars('Distance')
      subjectNode.GetPolyData().GetPointData().SetActiveScalars('Distance')
      subjectNode.GetPolyData().GetPointData().SetScalars(vtkDistanceArray)
      displayNode = subjectNode.GetDisplayNode()
      displayNode.SetActiveScalar('Distance', vtk.vtkAssignAttribute.POINT_DATA)
      displayNode.SetAndObserveColorNodeID('Rainbow')
      displayNode.SetScalarVisibility(True)

      # update metric table
      table = self.ui.DistanceTableWidget
      distanceData = vtk_to_numpy(vtkDistanceArray)

      table.setRowCount(3)
      table.setColumnCount(2)
      table.setHorizontalHeaderLabels([' Metric ', ' Value '])
      table.setColumnWidth(0, 100)
      table.setColumnWidth(1, 200)
      horizontalHeader = table.horizontalHeader()
      horizontalHeader.setStretchLastSection(True)
      table.verticalHeader().setVisible(True)

      minMetricLabel = qt.QLabel('min')
      minMetricLabel.setAlignment(0x84)
      table.setCellWidget(0, 0, minMetricLabel)
      minValueLabel = qt.QLabel("{:.6e}".format(distanceData.min()))
      minValueLabel.setAlignment(0x84)
      table.setCellWidget(0, 1, minValueLabel)

      maxMetricLabel = qt.QLabel('max')
      maxMetricLabel.setAlignment(0x84)
      table.setCellWidget(1, 0, maxMetricLabel)
      maxValueLabel = qt.QLabel("{:.6e}".format(distanceData.max()))
      maxValueLabel.setAlignment(0x84)
      table.setCellWidget(1, 1, maxValueLabel)

      meanMetricLabel = qt.QLabel('mean')
      meanMetricLabel.setAlignment(0x84)
      table.setCellWidget(2, 0, meanMetricLabel)
      meanValueLabel = qt.QLabel("{:.6e}".format(distanceData.mean()))
      meanValueLabel.setAlignment(0x84)
      table.setCellWidget(2, 1, meanValueLabel)

  def isModelValid(self, model):
    if model.GetPolyData() is not None:
      if model.GetPolyData().GetNumberOfPoints() > 0:
        return True
    return False
  
  def onViewItemChanged(self, item):
    itemList = vtk.vtkIdList()
    self.ui.SubjectHierarchyTreeView.currentItems(itemList)
    self.activeNodes = []
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    if itemList.GetNumberOfIds() is not 0:
      for i in range(itemList.GetNumberOfIds()):
        node = shNode.GetItemDataNode(itemList.GetId(i))
        if (node is None) or (node.IsA('vtkMRMLModelNode') is False):
          self.ui.MenuWidget.enabled = False
          return
        self.activeNodes.append(node)
    else:
      self.ui.MenuWidget.enabled = False
      return
    self.ui.MenuWidget.enabled = True

  def onFolderChanged(self, item):
    self.ui.SubjectHierarchyTreeView.setMRMLScene(slicer.mrmlScene)
    self.ui.SubjectHierarchyTreeView.setRootItem(item)
    self.ui.SubjectHierarchyTreeView.expandItem(item)
    self.ui.SubjectHierarchyTreeView.enabled = True
    self.modelHistory = ModelHistory(item)
    self.untagModels()
    self.activeFolder = self.ui.SubjectHierarchyComboBox.currentItem()
    self.tagModels()

  def removeNode(self,node):
    slicer.mrmlScene.RemoveNode(node)

  def restoreNextState(self):
    self.modelHistory.restoreNextState()
    self.resolveStateButtons()
  
  def restorePreviousState(self):
    self.modelHistory.restorePreviousState()
    self.resolveStateButtons()
  
  def finishPlan(self):
    self.untagModels()
    self.modelHistory.clearHistory()
    self.resolveStateButtons()
    self.ui.SubjectHierarchyComboBox.setCurrentItem(-1)
    self.ui.SubjectHierarchyTreeView.enabled = False 

  def resolveStateButtons(self):
    self.ui.UndoButton.enabled = self.modelHistory.isRestorePreviousStateAvailable() and not self.actionInProgress
    self.ui.RedoButton.enabled = self.modelHistory.isRestoreNextStateAvailable() and not self.actionInProgress
    self.ui.SubjectHierarchyComboBox.enabled = not self.modelHistory.hasHistory() and not self.actionInProgress
    self.ui.FinishButton.enabled = not self.actionInProgress
    self.onViewItemChanged(self.ui.SubjectHierarchyTreeView.currentItem())
    # print(self.modelHistory.history)
    # print(self.modelHistory.future)
    # print(self.modelHistory.lastRestoredState)

  def onTabChanged(self, index):

    if index == self.ui.TabWidget.indexOf(self.ui.ActionTab):
      self.ui.AllModelsSubjectHierarchyTreeView.visible = False

    if index == self.ui.TabWidget.indexOf(self.ui.ReferenceTab):
     self.ui.AllModelsSubjectHierarchyTreeView.visible = True

  def tagModel(self, node):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    item = shNode.GetItemByDataNode(node)
    parent = shNode.GetItemParent(item)
    shNode.RemoveItemAttribute(item, 'NodeInPlanner')
    if parent == self.ui.SubjectHierarchyComboBox.currentItem() and not parent == shNode.GetSceneItemID():
      print('Tagging ' + node.GetName())
      shNode.SetItemAttribute(item, 'NodeInPlanner', 'True')

  def untagModel(self, node):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    item = shNode.GetItemByDataNode(node)
    shNode.RemoveItemAttribute(item, 'NodeInPlanner')

  def tagModels(self):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    item = self.ui.SubjectHierarchyComboBox.currentItem()
    shNode.SetItemAttribute(item, 'NodeInPlanner', 'True')
    modelNodes = slicer.util.getNodesByClass('vtkMRMLModelNode')
    for node in modelNodes:         
      self.tagModel(node)
    self.ui.AllModelsSubjectHierarchyTreeView.sortFilterProxyModel().setExcludeItemAttributeNamesFilter(['NodeInPlanner'])

  def untagModels(self):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    if self.activeFolder is not None:
      shNode.RemoveItemAttribute(self.activeFolder, 'NodeInPlanner')
    modelNodes = slicer.util.getNodesByClass('vtkMRMLModelNode')
    for node in modelNodes:         
      self.untagModel(node)    
    self.ui.AllModelsSubjectHierarchyTreeView.sortFilterProxyModel().setExcludeItemAttributeNamesFilter(['NodeInPlanner'])

  # Generic Action
  def beginAction(self):
    self.ui.SubjectHierarchyTreeView.setSelectionMode(qt.QAbstractItemView().NoSelection)
    self.ui.MenuWidget.enabled = False
    self.modelHistory.cacheState()
    self.actionInProgress = True
    self.resolveStateButtons()

  def confirmAction(self):
    self.modelHistory.saveState()  

  def endAction(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.MenuWidget)
    self.ui.SubjectHierarchyTreeView.setSelectionMode(qt.QAbstractItemView().ExtendedSelection)
    self.ui.MenuWidget.enabled = True
    self.actionInProgress = False
    interactionNode = slicer.app.applicationLogic().GetInteractionNode()    
    interactionNode.SetCurrentInteractionMode(interactionNode.ViewTransform)
    self.resolveStateButtons()

  
  def cleanup(self):
    """
    Called when the application closes and the module widget is destroyed.
    """
    self.removeObservers()

  def enter(self):
    """
    Called each time the user opens this module.
    """
    # Make sure parameter node exists and observed
    self.initializeParameterNode()

  def exit(self):
    """
    Called each time the user opens a different module.
    """
    # Do not react to parameter node changes (GUI wlil be updated when the user enters into the module)
    self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self.updateGUIFromParameterNode)

  def onSceneStartClose(self, caller, event):
    """
    Called just before the scene is closed.
    """
    # Parameter node will be reset, do not use it anymore
    self.splitPlanes = []
    self.splitModels = []
    self.activeNodes = []
    self.transform = None
    self.setParameterNode(None)
    self.clearHistory()

  def onSceneEndClose(self, caller, event):
    """
    Called just after the scene is closed.
    """
    # If this module is shown while the scene is closed then recreate a new parameter node immediately
    if self.parent.isEntered:
      self.initializeParameterNode()

  def initializeParameterNode(self):
    """
    Ensure parameter node exists and observed.
    """
    # Parameter node stores all user choices in parameter values, node selections, etc.
    # so that when the scene is saved and reloaded, these settings are restored.

    self.setParameterNode(self.logic.getParameterNode())

    # Select default input nodes if nothing is selected yet to save a few clicks for the user
    if not self._parameterNode.GetNodeReference("InputVolume"):
      firstVolumeNode = slicer.mrmlScene.GetFirstNodeByClass("vtkMRMLScalarVolumeNode")
      if firstVolumeNode:
        self._parameterNode.SetNodeReferenceID("InputVolume", firstVolumeNode.GetID())

  def setParameterNode(self, inputParameterNode):
    """
    Set and observe parameter node.
    Observation is needed because when the parameter node is changed then the GUI must be updated immediately.
    """

    if inputParameterNode:
      self.logic.setDefaultParameters(inputParameterNode)

    # Unobserve previously selected parameter node and add an observer to the newly selected.
    # Changes of parameter node are observed so that whenever parameters are changed by a script or any other module
    # those are reflected immediately in the GUI.
    if self._parameterNode is not None:
      self.removeObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self.updateGUIFromParameterNode)
    self._parameterNode = inputParameterNode
    if self._parameterNode is not None:
      self.addObserver(self._parameterNode, vtk.vtkCommand.ModifiedEvent, self.updateGUIFromParameterNode)

    # Initial GUI update
    self.updateGUIFromParameterNode()

  def updateGUIFromParameterNode(self, caller=None, event=None):
    """
    This method is called whenever parameter node is changed.
    The module GUI is updated to show the current state of the parameter node.
    """

    if self._parameterNode is None or self._updatingGUIFromParameterNode:
      return

    # Make sure GUI changes do not call updateParameterNodeFromGUI (it could cause infinite loop)
    self._updatingGUIFromParameterNode = True

    

    # All the GUI updates are done
    self._updatingGUIFromParameterNode = False

  def updateParameterNodeFromGUI(self, caller=None, event=None):
    """
    This method is called when the user makes any change in the GUI.
    The changes are saved into the parameter node (so that they are restored when the scene is saved and loaded).
    """

    if self._parameterNode is None or self._updatingGUIFromParameterNode:
      return

    wasModified = self._parameterNode.StartModify()  # Modify all properties in a single batch

    self._parameterNode.SetNodeReferenceID("InputVolume", self.ui.inputSelector.currentNodeID)
    self._parameterNode.SetNodeReferenceID("OutputVolume", self.ui.outputSelector.currentNodeID)
    self._parameterNode.SetParameter("Threshold", str(self.ui.imageThresholdSliderWidget.value))
    self._parameterNode.SetParameter("Invert", "true" if self.ui.invertOutputCheckBox.checked else "false")
    self._parameterNode.SetNodeReferenceID("OutputVolumeInverse", self.ui.invertedOutputSelector.currentNodeID)

    self._parameterNode.EndModify(wasModified)

  def onApplyButton(self):
    """
    Run processing when user clicks "Apply" button.
    """
    try:

      # Compute output
      self.logic.process(self.ui.inputSelector.currentNode(), self.ui.outputSelector.currentNode(),
        self.ui.imageThresholdSliderWidget.value, self.ui.invertOutputCheckBox.checked)

      # Compute inverted output (if needed)
      if self.ui.invertedOutputSelector.currentNode():
        # If additional output volume is selected then result with inverted threshold is written there
        self.logic.process(self.ui.inputSelector.currentNode(), self.ui.invertedOutputSelector.currentNode(),
          self.ui.imageThresholdSliderWidget.value, not self.ui.invertOutputCheckBox.checked, showResult=False)

    except Exception as e:
      slicer.util.errorDisplay("Failed to compute results: "+str(e))
      import traceback
      traceback.print_exc()


#
# OsteotomyPlannerLogic
#

class OsteotomyPlannerLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual
  computation done by your module.  The interface
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget.
  Uses ScriptedLoadableModuleLogic base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self):
    """
    Called when the logic class is instantiated. Can be used for initializing member variables.
    """
    ScriptedLoadableModuleLogic.__init__(self)

  def setDefaultParameters(self, parameterNode):
    """
    Initialize parameter node with default settings.
    """
    if not parameterNode.GetParameter("Threshold"):
      parameterNode.SetParameter("Threshold", "100.0")
    if not parameterNode.GetParameter("Invert"):
      parameterNode.SetParameter("Invert", "false")

  def process(self, inputVolume, outputVolume, imageThreshold, invert=False, showResult=True):
    """
    Run the processing algorithm.
    Can be used without GUI widget.
    :param inputVolume: volume to be thresholded
    :param outputVolume: thresholding result
    :param imageThreshold: values above/below this threshold will be set to 0
    :param invert: if True then values above the threshold will be set to 0, otherwise values below are set to 0
    :param showResult: show output volume in slice viewers
    """

    if not inputVolume or not outputVolume:
      raise ValueError("Input or output volume is invalid")

    import time
    startTime = time.time()
    logging.info('Processing started')

    # Compute the thresholded output volume using the "Threshold Scalar Volume" CLI module
    cliParams = {
      'InputVolume': inputVolume.GetID(),
      'OutputVolume': outputVolume.GetID(),
      'ThresholdValue' : imageThreshold,
      'ThresholdType' : 'Above' if invert else 'Below'
      }
    cliNode = slicer.cli.run(slicer.modules.thresholdscalarvolume, None, cliParams, wait_for_completion=True, update_display=showResult)
    # We don't need the CLI module node anymore, remove it to not clutter the scene with it
    slicer.mrmlScene.RemoveNode(cliNode)

    stopTime = time.time()
    logging.info('Processing completed in {0:.2f} seconds'.format(stopTime-startTime))

#
# OsteotomyPlannerTest
#

class OsteotomyPlannerTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  Uses ScriptedLoadableModuleTest base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear()

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_OsteotomyPlanner1()

  def test_OsteotomyPlanner1(self):
    """ Ideally you should have several levels of tests.  At the lowest level
    tests should exercise the functionality of the logic with different inputs
    (both valid and invalid).  At higher levels your tests should emulate the
    way the user would interact with your code and confirm that it still works
    the way you intended.
    One of the most important features of the tests is that it should alert other
    developers when their changes will have an impact on the behavior of your
    module.  For example, if a developer removes a feature that you depend on,
    your test should break so they know that the feature is needed.
    """

    self.delayDisplay("Starting the test")

    # Get/create input data

    import SampleData
    registerSampleData()
    inputVolume = SampleData.downloadSample('OsteotomyPlanner1')
    self.delayDisplay('Loaded test data set')

    inputScalarRange = inputVolume.GetImageData().GetScalarRange()
    self.assertEqual(inputScalarRange[0], 0)
    self.assertEqual(inputScalarRange[1], 695)

    outputVolume = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLScalarVolumeNode")
    threshold = 100

    # Test the module logic

    logic = OsteotomyPlannerLogic()

    # Test algorithm with non-inverted threshold
    logic.process(inputVolume, outputVolume, threshold, True)
    outputScalarRange = outputVolume.GetImageData().GetScalarRange()
    self.assertEqual(outputScalarRange[0], inputScalarRange[0])
    self.assertEqual(outputScalarRange[1], threshold)

    # Test algorithm with inverted threshold
    logic.process(inputVolume, outputVolume, threshold, False)
    outputScalarRange = outputVolume.GetImageData().GetScalarRange()
    self.assertEqual(outputScalarRange[0], inputScalarRange[0])
    self.assertEqual(outputScalarRange[1], inputScalarRange[1])

    self.delayDisplay('Test passed')
