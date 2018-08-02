import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging
import glob

#
# ReplayPlan
#

class ReplayPlan(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "ReplayPlan" # TODO make this more human readable by adding spaces
    self.parent.categories = ["Craniosynostosis"]
    self.parent.dependencies = []
    self.parent.contributors = ["Sam Horvath (Kitware Inc.)"] # replace with "Firstname Lastname (Organization)"
    self.parent.helpText = """ """
    self.parent.helpText += self.getDefaultModuleDocumentationLink()
    self.parent.acknowledgementText = """ """ # replace with organization, grant and thanks.

#
# ReplayPlanWidget
#

class ReplayPlanWidget(ScriptedLoadableModuleWidget):
  """Uses ScriptedLoadableModuleWidget base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)
    # Load main widget from UI file
    scriptedModulesPath = os.path.dirname(slicer.util.modulePath(self.moduleName))
    path = os.path.join(scriptedModulesPath, 'Resources', 'UI', 'ReplayPlanWidget.ui')
    self.widget = slicer.util.loadUI(path)
    assert(self.widget)
    self.layout.addWidget(self.widget)

    #outlets
    self.DirectoryButton = self.findChild("DirectoryButton")
    self.PreviousButton = self.findChild("PreviousButton")
    self.NextButton = self.findChild("NextButton")
    self.CloseButton = self.findChild("CloseButton")
    self.OutputTextBrowser = self.findChild("OutputTextBrowser")

    self.DirectoryButton.directoryChanged.connect(self.planDirectoryChanged)
    self.PreviousButton.clicked.connect(self.previousButtonClicked)
    self.NextButton.clicked.connect(self.nextButtonClicked)
    self.CloseButton.clicked.connect(self.closeButtonClicked)

    self.loadedScreenshots = []
    self.closeButtonClicked()


  def planDirectoryChanged(self, newDirectory):
    files = glob.glob(newDirectory + "/*_Instructions.txt")
    if len(files) == 1:
      self.OutputTextBrowser.setText("Found instructions at: " + str(files[0]))
      self.parseInstructions(files[0])
    else:
      self.OutputTextBrowser.setText("Invalid Directory")

  def parseInstructions(self, instructionsFile):
    with open(instructionsFile) as f:
      lines = f.readlines()
    lines = [x.strip() for x in lines]
    self.CaseTitle = lines[0]
    self.OutputTextBrowser.setText(self.CaseTitle)
    for i in range(0, len(lines)):
      if lines[i].startswith("Step"):
        self.StepDescriptions.append(lines[i])
        self.StepFiles.append(lines[i+1])

    if len(self.StepDescriptions) < 1 or len(self.StepFiles) < 1 or len(self.StepDescriptions) != len(self.StepFiles):
      self.OutputTextBrowser.setText("Bad Instruction File")
      return

    self.TotalSteps = len(self.StepDescriptions)
    self.loadScreenshots()
    layoutManager = slicer.app.layoutManager()
    layoutManager.setLayout(slicer.vtkMRMLLayoutNode.SlicerLayoutOneUpRedSliceView)
    self.nextButtonClicked()

  def nextButtonClicked(self):
    self.CurrentStep += 1
    self.updateStep()

  def previousButtonClicked(self):
    self.CurrentStep -= 1
    self.updateStep()

  def updateStep(self):
    self.OutputTextBrowser.setText(self.StepDescriptions[self.CurrentStep])
    self.OutputTextBrowser.append("")
    self.OutputTextBrowser.append(self.StepFiles[self.CurrentStep])
    self.setSliceViewNode(self.loadedScreenshots[self.CurrentStep])
    self.updateNavButtons()

  def closeButtonClicked(self):
    self.CaseTitle = ""
    self.StepDescriptions = []
    self.StepFiles = []
    self.CurrentStep = -1
    self.TotalSteps = -1
    self.updateNavButtons()
    self.DirectoryButton.directory = ""
    for node in self.loadedScreenshots:
      slicer.mrmlScene.RemoveNode(node)
    self.loadedScreenshots = []


  def findChild(self, name):
    return slicer.util.findChild(self.widget, name=name)

  def updateNavButtons(self):
    self.CloseButton.enabled = self.TotalSteps > 0
    self.PreviousButton.enabled = self.CurrentStep > 0
    self.NextButton.enabled = self.CurrentStep < (self.TotalSteps - 1)

  def loadScreenshots(self):
    for file in self.StepFiles:
      file = self.DirectoryButton.directory + "/" + file
      [success, loadedVolumeNode] = slicer.util.loadVolume(file, returnNode=True)
      self.loadedScreenshots.append(loadedVolumeNode)

  def setSliceViewNode(self, volumeNode):
    applicationLogic = slicer.app.applicationLogic()
    selectionNode = applicationLogic.GetSelectionNode()
    selectionNode.SetActiveVolumeID(volumeNode.GetID())
    applicationLogic.PropagateBackgroundVolumeSelection(0)
    selectionNode.SetActiveLabelVolumeID(None)
    applicationLogic.PropagateLabelVolumeSelection()




#
# ReplayPlanLogic
#

class ReplayPlanLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual
  computation done by your module.  The interface
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget.
  Uses ScriptedLoadableModuleLogic base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def hasImageData(self,volumeNode):
    """This is an example logic method that
    returns true if the passed in volume
    node has valid image data
    """
    if not volumeNode:
      logging.debug('hasImageData failed: no volume node')
      return False
    if volumeNode.GetImageData() is None:
      logging.debug('hasImageData failed: no image data in volume node')
      return False
    return True

