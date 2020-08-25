import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# PlannerTesting
#

class PlannerTesting(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "PlannerTesting" # TODO make this more human readable by adding spaces
    self.parent.categories = ["Testing"]
    self.parent.dependencies = ["Planner"]
    self.parent.contributors = ["Johan Andruejol (Kitware Inc.)"] # replace with "Firstname Lastname (Organization)"
    self.parent.helpText = """Test for the Planner Module GUI"""
    self.parent.acknowledgementText = """TODO""" # replace with organization, grant and thanks.

#
# PlannerTestingWidget
#
# No widget

#
# PlannerTestingLogic
#

class PlannerTestingLogic(ScriptedLoadableModuleLogic):
  pass

class PlannerTestingTest(ScriptedLoadableModuleTest):
  """
  Test case for the Planner Module GUI
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def plannerWidget(self):
    widget = slicer.modules.planner.widgetRepresentation()
    self.assertIsNotNone(widget)
    return widget

  def widget(self, name):
    widget = slicer.util.findChild(self.plannerWidget(), name)
    self.assertIsNotNone(widget)
    return widget

  def loadModelHierarchy(self, numberOfModelHierarchies=1):
    ''' Loads a scene with a model hierachy in it.
        Check that the number of model hierachies in the scene is equal to the
        given numberOfModelHierarchies.
        The (numberOfModelHierarchies - 1)th model hierarchy is returned.
    '''
    moduleName = 'Planner'
    scriptedModulesPath = os.path.dirname(slicer.util.modulePath(moduleName))
    path = os.path.join(scriptedModulesPath, 'Resources', 'Testing', 'ModelHierachyScene.mrb')
    slicer.util.loadScene(path)

    collection = slicer.mrmlScene.GetNodesByClass('vtkMRMLSubjectHierarchyNode')
    self.assertEquals(collection.GetNumberOfItems(), numberOfModelHierarchies)

    legsModel = collection.GetItemAsObject(numberOfModelHierarchies - 1)
    self.assertIsNotNone(legsModel)
    self.assertIs(legsModel, slicer.vtkMRMLSubjectHierarchyNode)
    return legsModel

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_ModuleHierachyTreeView()

  def test_ModuleHierachyTreeView(self):
    """ Test the module hierachy tree view
    """
    modelHierarchyTreeView = self.widget('SubjectHierarchyTreeView')
    modelHierarchyComboBox = self.widget('SubjectHierarchyComboBox')

    # Test base state
    self.assertIsNone(modelHierarchyComboBox.GetCurrentNode())
    self.assertIsNone(modelHierarchyTreeView.currentNode())
    self.assertIsNone(modelHierarchyTreeView.rootNode())

    # Load model hierarchy, test it's set properly
    legsModel = self.loadModelHierarchy(1)
    self.plannerWidget().setCurrentNode(legsModel)

    self.assertEquals(modelHierarchyComboBox.GetCurrentNode(), legsModel)
    self.assertEquals(modelHierarchyTreeView.currentNode(), None)
    self.assertEquals(modelHierarchyTreeView.rootNode(), legsModel)

    # Test other model hierarchy
    legsModel2 = self.loadModelHierarchy(2)
    self.plannerWidget().setCurrentNode(legsModel2)

    self.assertEquals(modelHierarchyComboBox.GetCurrentNode(), legsModel2)
    self.assertEquals(modelHierarchyTreeView.currentNode(), None)
    self.assertEquals(modelHierarchyTreeView.rootNode(), legsModel2)

    # Remove both and test again
    slicer.mrmlScene.RemoveNode(legsModel)
    self.assertEquals(modelHierarchyComboBox.GetCurrentNode(), legsModel2)
    self.assertEquals(modelHierarchyTreeView.currentNode(), None)
    self.assertEquals(modelHierarchyTreeView.rootNode(), legsModel2)

    slicer.mrmlScene.RemoveNode(legsModel2)
    self.assertIsNone(modelHierarchyComboBox.GetCurrentNode())
    self.assertIsNone(modelHierarchyTreeView.currentNode())
    self.assertIsNone(modelHierarchyTreeView.rootNode())
