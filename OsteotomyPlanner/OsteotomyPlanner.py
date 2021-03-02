import os
import unittest
import logging
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
from slicer.util import VTKObservationMixin

#
# OsteotomyPlanner
#

class OsteotomyPlanner(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "OsteotomyPlannerv2"  # TODO: make this more human readable by adding spaces
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
    self.ui.SubjectHierarchyTreeView.hideColumn(2)
    # self.ui.SubjectHierarchyTreeView.hideColumn(3)
    self.ui.SubjectHierarchyTreeView.hideColumn(4)
    self.ui.SubjectHierarchyTreeView.hideColumn(5)

    #Selection 
    self.ui.SubjectHierarchyTreeView.setSelectionMode(qt.QAbstractItemView().SingleSelection)
    self.ui.ActionsWidget.setCurrentWidget(self.ui.MenuWidget)
    self.ui.MenuWidget.enabled = False

    self.ui.ActionButton.clicked.connect(self.dummyAction)
    self.ui.MoveButton.clicked.connect(self.beginMove)
    self.ui.SplitButton.clicked.connect(self.beginSplit)
    self.ui.MoveCancelButton.clicked.connect(self.endMove)
    self.ui.MoveConfirmButton.clicked.connect(self.confirmMove)
    self.ui.SplitCancelButton.clicked.connect(self.endSplit)
    self.ui.ManualPlaneButton.clicked.connect(self.placeManualPlane)
    self.ui.ClearPlaneButton.clicked.connect(self.clearPlanes)
    self.ui.AutoPlaneButton.clicked.connect(self.placeAutoPlane)
    self.ui.PreviewSplitButton.clicked.connect(self.previewSplit)
    self.ui.SplitConfirmButton.clicked.connect(self.confirmSplit)

    self.ui.RedoButton.clicked.connect(self.restoreNextState)
    self.ui.UndoButton.clicked.connect(self.restorePreviousState)
    self.ui.RedoButton.visible = False

    self.transform = None
    self.activeNode = None
    self.splitPlanes = []
    self.splitModels = []
    self.history = [] #List of states, each state is a list of nodes
    self.maximumSavedStates = 10
    self.lastRestoredState = 0 
    self.cachedState = None
    

    # Make sure parameter node is initialized (needed for module reload)
    self.initializeParameterNode()
    self.resolveStateButtons()

  def archiveNode(self, node):
    node.HideFromEditorsOn()
    node.SetDisplayVisibility(False)
    node.SetAttribute("History", "yes")

  def restoreNode(self, node):
    node.HideFromEditorsOff()
    node.SetDisplayVisibility(True)
    node.RemoveAttribute("History")

  def removeNode(self,node):
    slicer.mrmlScene.RemoveNode(node)

  def isNodeCurrent(self, node):
    return not node.GetAttribute("History") == "yes"
  
  def saveState(self):
    print("Save state")
    if self.maximumSavedStates < 1:
      return
    # self.removeAllNextStates()
    if self.cachedState is None:
      self.cacheState()
    self.history.append(self.cachedState)
    self.cachedState = None
    self.lastRestoredState = len(self.history)
    self.removeAllObsoleteStates()

  
  def cacheState(self):
    self.clearCachedState()
    self.cachedState = []
    #clone nodes
    children = vtk.vtkIdList()
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)

    shNode.GetItemChildren(self.ui.SubjectHierarchyComboBox.currentItem(), children) # Add a third argument with value True for recursive query
    state = []
    for i in range(children.GetNumberOfIds()):
      child = children.GetId(i)
      childNode = shNode.GetItemDataNode(child)
      if not self.isNodeCurrent(childNode):
        continue
      clonedChild = self.cloneNode(childNode)
      self.archiveNode(clonedChild)
      self.cachedState.append(clonedChild)
  
  def clearCachedState(self):
    if self.cachedState is not None:
      self.deleteState(self.cachedState)
    self.cachedState = None
  
  def removeAllNextStates(self):
    while (len(self.history) > (self.lastRestoredState + 1)) and self.history:
      state = self.history.pop()
      self.deleteState(state)
    

  def removeAllObsoleteStates(self):
    while (len(self.history) > self.maximumSavedStates) and self.history:
      state = self.history.pop(0)
      self.lastRestoredState -= 1
      self.deleteState(state)
  
  def deleteState(self, state):
    print("Deleting state")
    for node in state:
      self.removeNode(node)

  def restoreState(self, stateIndex):
    #restore state, then update index
    restoredState = self.history.pop()
    
    #drop all of the current nodes
    children = vtk.vtkIdList()
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)

    shNode.GetItemChildren(self.ui.SubjectHierarchyComboBox.currentItem(), children) # Add a third argument with value True for recursive query
    for i in range(children.GetNumberOfIds()):
      child = children.GetId(i)
      childNode = shNode.GetItemDataNode(child)
      if self.isNodeCurrent(childNode):
        self.removeNode(childNode)

    for model in restoredState:
      self.restoreNode(model)

    self.lastRestoredState = stateIndex

  def restorePreviousState(self):
    if self.lastRestoredState < 1:
      print("There are no previous state available for restore")   
      return

    if len(self.history) < self.lastRestoredState:
      print("There are no previous state available for restore")   
      return

    stateToRestore = self.lastRestoredState -1
    # if len(self.history) == self.lastRestoredState:
    #   print("Need to save before restore")
    #   self.saveState()
    #   stateToRestore = len(self.history) - 2
    
    self.restoreState(stateToRestore)
    self.resolveStateButtons()

  def restoreNextState(self):

    if self.lastRestoredState + 1 >= len(self.history):
      print("No next state available to restore")
      return

    self.restoreState(self.lastRestoredState + 1)
    self.resolveStateButtons()

  def resolveStateButtons(self):

    self.ui.UndoButton.enabled = self.isRestorePreviousStateAvailable()
    self.ui.RedoButton.enabled = self.isRestoreNextStateAvailable()
    print(self.history)
    print("LastRestored: " + str(self.lastRestoredState))

  
  def isRestorePreviousStateAvailable(self):
    return not (self.lastRestoredState < 1)

  def isRestoreNextStateAvailable(self):
    # return not (self.lastRestoredState + 1 >= len(self.history))
    return False
  
  
  def removeAllStates(self):
    for state in self.history:
      self.deleteState(state)

    self.history = []
    self.lastRestoredState = 0

  def getNumberOfStates(self):
    return len(self.history)
  
  def cloneNode(self, nodeToClone):
    # Clone the node
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    itemIDToClone = shNode.GetItemByDataNode(nodeToClone)
    clonedItemID = slicer.modules.subjecthierarchy.logic().CloneSubjectHierarchyItem(shNode, itemIDToClone)
    clonedNode = shNode.GetItemDataNode(clonedItemID)
    print("Cloning " + nodeToClone.GetID() + " to " + clonedNode.GetID())
    clonedNode.SetName(nodeToClone.GetName())
    return clonedNode
  
  def dummyAction(self):
    self.beginAction()
    self.endAction()
  
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
    self.activeNode.GetRASBounds(bounds)

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
    positiveModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
    positiveModel.CreateDefaultDisplayNodes()
    positiveModel.GetDisplayNode().SetColor(1.0,0,0)
    negativeModel = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLModelNode")
    negativeModel.CreateDefaultDisplayNodes()
    negativeModel.GetDisplayNode().SetColor(0,0,1.0)
    dynamicModelerNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLDynamicModelerNode")
    dynamicModelerNode.SetToolName("Plane cut")
    dynamicModelerNode.SetAttribute("OperationType", self.ui.OperationTypeComboBox.currentText)
    dynamicModelerNode.SetAttribute("CapSurface", str(int(self.ui.CapModelCheckBox.isChecked())))
    dynamicModelerNode.SetNodeReferenceID("PlaneCut.InputModel", self.activeNode.GetID())
    for plane in self.splitPlanes:
      dynamicModelerNode.AddNodeReferenceID("PlaneCut.InputPlane", plane.GetID())
    dynamicModelerNode.SetNodeReferenceID("PlaneCut.OutputPositiveModel", positiveModel.GetID())
    dynamicModelerNode.SetNodeReferenceID("PlaneCut.OutputNegativeModel", negativeModel.GetID())
    slicer.modules.dynamicmodeler.logic().RunDynamicModelerTool(dynamicModelerNode)
    self.splitModels.append(positiveModel)
    self.splitModels.append(negativeModel)
    slicer.mrmlScene.RemoveNode(dynamicModelerNode)
    self.ui.SplitConfirmButton.enabled = True
      
  def confirmSplit(self):
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    folderItem = self.ui.SubjectHierarchyComboBox.currentItem()
    selectItem = None
    for model in self.splitModels:
      item = shNode.GetItemByDataNode(model)
      shNode.SetItemParent(item, folderItem)
      selectItem = item
    self.splitModels = []
    self.removeNode(self.activeNode)
    #need to force selection of something
    self.ui.SubjectHierarchyTreeView.setCurrentItem(selectItem)
    self.saveState()
    self.endSplit()

  def clearSplitModels(self):
    for model in self.splitModels:
      self.removeNode(model)
    self.splitModels = []

  def clearPlanes(self):
    for plane in self.splitPlanes:
      slicer.mrmlScene.RemoveNode(plane)
    self.splitPlanes = []
  
  def beginAction(self):
    self.ui.SubjectHierarchyTreeView.setSelectionMode(qt.QAbstractItemView().NoSelection)
    self.ui.MenuWidget.enabled = False
    self.cacheState()

  def endAction(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.MenuWidget)
    self.ui.SubjectHierarchyTreeView.setSelectionMode(qt.QAbstractItemView().SingleSelection)
    self.ui.MenuWidget.enabled = True
    self.resolveStateButtons()

  def beginMove(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.MoveWidget)
    self.beginAction()
    self.createMoveTransform()

  def beginSplit(self):
    self.ui.ActionsWidget.setCurrentWidget(self.ui.SplitWidget)
    self.ui.PreviewSplitButton.enabled = False
    self.ui.SplitConfirmButton.enabled = False
    self.beginAction()

  def endMove(self):
    self.activeNode.SetAndObserveTransformNodeID(None)
    slicer.mrmlScene.RemoveNode(self.transform)
    self.transform = None
    self.endAction()

  def confirmMove(self):
    logic = slicer.vtkSlicerTransformLogic()
    logic.hardenTransform(self.activeNode)
    self.saveState()
    self.endMove()

  def endSplit(self):
    self.clearPlanes()
    self.clearSplitModels()
    self.endAction()  
  
  def createMoveTransform(self):
    self.transform = slicer.mrmlScene.AddNewNodeByClass('vtkMRMLLinearTransformNode')
    self.transform.CreateDefaultDisplayNodes()
    display = self.transform.GetDisplayNode()
    self.activeNode.SetAndObserveTransformNodeID(self.transform.GetID())
    display.UpdateEditorBounds()
    display.EditorScalingEnabledOff()
    display.EditorVisibilityOn()    
  
  def onViewItemChanged(self, item):
    itemList = vtk.vtkIdList()
    self.ui.SubjectHierarchyTreeView.currentItems(itemList)
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    node =  shNode.GetItemDataNode(item)
    self.ui.MenuWidget.enabled = (node is not None and node.IsA('vtkMRMLModelNode'))
    self.activeNode = node

  def onFolderChanged(self, item):
    self.ui.SubjectHierarchyTreeView.setMRMLScene(slicer.mrmlScene)
    self.ui.SubjectHierarchyTreeView.setRootItem(item)
    self.ui.SubjectHierarchyTreeView.expandItem(item)

  
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
    self.activeNode = None
    self.transform = None
    self.setParameterNode(None)
    self.removeAllStates()

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
