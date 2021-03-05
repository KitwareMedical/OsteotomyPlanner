import vtk, qt, ctk, slicer

class ModelHistory:

  def __init__(self, folderID=-1):
    self.history = [] #List of states, each state is a list of nodes
    self.future = []
    self.maximumSavedStates = 10
    self.lastRestoredState = 0 
    self.cachedState = None
    self.folder = folderID
    print("Model History")

  def hasHistory(self):
    return not len(self.history) == 0
  
  def __del__(self):
    self.clearHistory()
    print("Destructed")
  
  def setFolder(self, itemID):
    if itemID == self.folder:
      return
    self.folder = itemID
    self.clearHistory()
  
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
    if self.maximumSavedStates < 1:
      return
    self.removeAllNextStates()
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

    shNode.GetItemChildren(self.folder, children) # Add a third argument with value True for recursive query
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
    for state in self.future:
      self.deleteState(state)
    self.future = []
    

  def removeAllObsoleteStates(self):
    while (len(self.history) > self.maximumSavedStates) and self.history:
      state = self.history.pop(0)
      self.lastRestoredState -= 1
      self.deleteState(state)
  
  def deleteState(self, state):
    for node in state:
      self.removeNode(node)  

  def restorePreviousState(self):
    if self.lastRestoredState < 1:
      print("There are no previous state available for restore")   
      return

    if len(self.history) < self.lastRestoredState:
      print("There are no previous state available for restore")   
      return

    #restore state, then update index
    restoredState = self.history.pop()
    futureState = []
    
    #drop all of the current nodes
    children = vtk.vtkIdList()
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)

    shNode.GetItemChildren(self.folder, children) # Add a third argument with value True for recursive query
    for i in range(children.GetNumberOfIds()):
      child = children.GetId(i)
      childNode = shNode.GetItemDataNode(child)
      if self.isNodeCurrent(childNode):
        self.archiveNode(childNode)
        futureState.append(childNode)
    self.future.append(futureState)
    for model in restoredState:
      self.restoreNode(model)

    self.lastRestoredState = len(self.history)

  def restoreNextState(self):

    if 0 == len(self.future):
      print("No next state available to restore")
      return

    #restore state, then update index
    restoredState = self.future.pop()
    pastState = []
    
    #drop all of the current nodes
    children = vtk.vtkIdList()
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)

    shNode.GetItemChildren(self.folder, children) # Add a third argument with value True for recursive query
    for i in range(children.GetNumberOfIds()):
      child = children.GetId(i)
      childNode = shNode.GetItemDataNode(child)
      if self.isNodeCurrent(childNode):
        self.archiveNode(childNode)
        pastState.append(childNode)
    self.history.append(pastState)
    
    for model in restoredState:
      self.restoreNode(model)

    self.lastRestoredState = len(self.history)

  def isRestorePreviousStateAvailable(self):
    return not (self.lastRestoredState < 1)

  def isRestoreNextStateAvailable(self):
    return not (0 == len(self.future))
  
  
  def clearHistory(self):
    for state in self.history:
      self.deleteState(state)

    for state in self.future:
      self.deleteState(state)

    self.history = []
    self.future = []
    self.lastRestoredState = 0

  def getNumberOfStates(self):
    return len(self.history)
  
  def cloneNode(self, nodeToClone):
    # Clone the node
    shNode = slicer.vtkMRMLSubjectHierarchyNode.GetSubjectHierarchyNode(slicer.mrmlScene)
    itemIDToClone = shNode.GetItemByDataNode(nodeToClone)
    clonedItemID = slicer.modules.subjecthierarchy.logic().CloneSubjectHierarchyItem(shNode, itemIDToClone)
    clonedNode = shNode.GetItemDataNode(clonedItemID)
    clonedNode.SetName(nodeToClone.GetName())
    return clonedNode