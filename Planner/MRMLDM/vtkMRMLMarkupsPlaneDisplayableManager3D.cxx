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

// MarkupsModule/MRML includes
#include <vtkMRMLMarkupsPlaneNode.h>
#include <vtkMRMLMarkupsDisplayNode.h>

// MarkupsModule/MRMLDisplayableManager includes
#include "vtkMRMLMarkupsPlaneDisplayableManager3D.h"

// MRML includes
#include <vtkMRMLApplicationLogic.h>
#include <vtkMRMLDisplayableManagerGroup.h>
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLMarkupsClickCounter.h>
#include <vtkMRMLModelDisplayableManager.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLSelectionNode.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkEventBroker.h>
#include <vtkInteractorStyle.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointHandleRepresentation3D.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

// STD includes
#include <sstream>
#include <string>

//---------------------------------------------------------------------------
vtkStandardNewMacro (vtkMRMLMarkupsPlaneDisplayableManager3D);

//---------------------------------------------------------------------------
class vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
{
public:
  vtkInternal(vtkMRMLMarkupsPlaneDisplayableManager3D* external);
  ~vtkInternal();

  typedef std::vector<vtkImplicitPlaneWidget2*> WidgetListType;
  struct Pipeline
    {
    WidgetListType Widgets;
    };

  typedef std::map<vtkMRMLMarkupsDisplayNode*, Pipeline* > PipelinesCacheType;
  PipelinesCacheType DisplayPipelines;

  typedef std::map<vtkMRMLMarkupsPlaneNode*, std::set<vtkMRMLMarkupsDisplayNode*> >
    MarkupToDisplayCacheType;
  MarkupToDisplayCacheType MarkupToDisplayNodes;

  typedef std::map <vtkImplicitPlaneWidget2*, vtkMRMLMarkupsDisplayNode* > WidgetToPipelineMapType;
  WidgetToPipelineMapType WidgetMap;

  // Node
  void AddNode(vtkMRMLMarkupsPlaneNode* displayableNode);
  void RemoveNode(vtkMRMLMarkupsPlaneNode* displayableNode);
  void UpdateDisplayableNode(vtkMRMLMarkupsPlaneNode *node);

  // Display Nodes
  void AddDisplayNode(vtkMRMLMarkupsPlaneNode*, vtkMRMLMarkupsDisplayNode*);
  void UpdateDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode);
  void UpdateDisplayNodePipeline(vtkMRMLMarkupsDisplayNode*, Pipeline*);
  void RemoveDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode);
  void SetTransformDisplayProperty(vtkMRMLMarkupsDisplayNode *displayNode, vtkActor* actor);

  // Widget
  void UpdateWidgetFromNode(vtkMRMLMarkupsDisplayNode*, vtkMRMLMarkupsPlaneNode*, Pipeline*);
  void UpdateNodeFromWidget(vtkImplicitPlaneWidget2*);
  void UpdateActiveNode();

  // Observations
  void AddObservations(vtkMRMLMarkupsPlaneNode* node);
  void RemoveObservations(vtkMRMLMarkupsPlaneNode* node);

  void AddObservationsToInteractionNode(vtkMRMLInteractionNode*);
  void RemoveObservationsFromInteractionNode(vtkMRMLInteractionNode*);

  // Helper functions
  bool UseDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode);
  void ClearDisplayableNodes();
  void ClearPipeline(Pipeline* pipeline);
  vtkImplicitPlaneWidget2* CreatePlaneWidget() const;
  std::vector<int> EventsToObserve() const;
  void StopInteraction();

public:
  vtkMRMLMarkupsPlaneDisplayableManager3D* External;
  bool AddingNode;
  bool ClickCounter;
  double LastPosition[3];

  vtkSmartPointer<vtkSphereSource> ClickedPointSource;
  vtkSmartPointer<vtkPolyDataMapper> ClickedPointMapper;
  vtkSmartPointer<vtkActor> ClickedPointActor;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::vtkInternal(vtkMRMLMarkupsPlaneDisplayableManager3D * external)
: External(external),
  AddingNode(false),
  ClickCounter(0)
{
  this->ClickedPointSource = vtkSmartPointer<vtkSphereSource>::New();
  this->ClickedPointSource->SetCenter(0.0, 0.0, 0.0);
  this->ClickedPointSource->SetRadius(5.0);

  this->ClickedPointMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->ClickedPointMapper->SetInputConnection(
    this->ClickedPointSource->GetOutputPort());

  this->ClickedPointActor = vtkSmartPointer<vtkActor>::New();
  this->ClickedPointActor->SetMapper(this->ClickedPointMapper);
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal::~vtkInternal()
{
  this->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
bool vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::UseDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode)
{
   // allow annotations to appear only in designated viewers
  return displayNode && displayNode->IsDisplayableInView(this->External->GetMRMLViewNode()->GetID());
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::AddNode(vtkMRMLMarkupsPlaneNode* node)
{
  if (this->AddingNode || !node)
    {
    return;
    }

  this->AddingNode = true;

  // Add Display Nodes
  this->AddObservations(node);
  for (int i=0; i< node->GetNumberOfDisplayNodes(); i++)
    {
    vtkMRMLMarkupsDisplayNode* displayNode =
      vtkMRMLMarkupsDisplayNode::SafeDownCast(node->GetNthDisplayNode(i));
    if (this->UseDisplayNode(displayNode))
      {
      this->MarkupToDisplayNodes[node].insert(displayNode);
      this->AddDisplayNode(node, displayNode);
      }
    }

  this->AddingNode = false;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::RemoveNode(vtkMRMLMarkupsPlaneNode* node)
{
  if (!node)
    {
    return;
    }

  vtkInternal::MarkupToDisplayCacheType::iterator displayableIt =
    this->MarkupToDisplayNodes.find(node);
  if(displayableIt == this->MarkupToDisplayNodes.end())
    {
    return;
    }

  std::set<vtkMRMLMarkupsDisplayNode *> displayNodes = displayableIt->second;
  std::set<vtkMRMLMarkupsDisplayNode*>::iterator diter;
  for (diter = displayNodes.begin(); diter != displayNodes.end(); ++diter)
    {
    this->RemoveDisplayNode(*diter);
    }
  this->RemoveObservations(node);
  this->MarkupToDisplayNodes.erase(displayableIt);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::UpdateDisplayableNode(vtkMRMLMarkupsPlaneNode* mNode)
{
  // Update the pipeline for all tracked DisplayableNode
  PipelinesCacheType::iterator pipelinesIter;
  std::set<vtkMRMLMarkupsDisplayNode*> displayNodes = this->MarkupToDisplayNodes[mNode];
  std::set<vtkMRMLMarkupsDisplayNode*>::iterator diter;
  for (diter = displayNodes.begin(); diter != displayNodes.end(); ++diter)
    {
    pipelinesIter = this->DisplayPipelines.find(*diter);
    if (pipelinesIter != this->DisplayPipelines.end())
      {
      Pipeline* pipeline = pipelinesIter->second;
      this->UpdateDisplayNodePipeline(pipelinesIter->first, pipeline);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::ClearPipeline(Pipeline* pipeline)
{
  for (size_t i = 0; i < pipeline->Widgets.size(); ++i)
    {
    vtkImplicitPlaneWidget2* widget = pipeline->Widgets[i];
    this->WidgetMap.erase(widget);
    widget->Delete();
    }
  pipeline->Widgets.clear();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::RemoveDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode)
{
  PipelinesCacheType::iterator actorsIt = this->DisplayPipelines.find(displayNode);
  if(actorsIt == this->DisplayPipelines.end())
    {
    return;
    }
  Pipeline* pipeline = actorsIt->second;
  this->ClearPipeline(pipeline);
  delete pipeline;
  this->DisplayPipelines.erase(actorsIt);
}

//---------------------------------------------------------------------------
vtkImplicitPlaneWidget2*
vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal::CreatePlaneWidget() const
{
  vtkImplicitPlaneWidget2* widget = vtkImplicitPlaneWidget2::New();
  widget->CreateDefaultRepresentation();
  vtkImplicitPlaneRepresentation* rep = widget->GetImplicitPlaneRepresentation();
  rep->SetConstrainToWidgetBounds(0);
  widget->AddObserver(
    vtkCommand::InteractionEvent, this->External->GetWidgetsCallbackCommand());

  widget->SetInteractor(this->External->GetInteractor());
  widget->SetCurrentRenderer(this->External->GetRenderer());
  return widget;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::AddDisplayNode(vtkMRMLMarkupsPlaneNode* mNode, vtkMRMLMarkupsDisplayNode* displayNode)
{
  if (!mNode || !displayNode)
    {
    return;
    }

  // Do not add the display node if it is already associated with a pipeline object.
  // This happens when a node already associated with a display node
  // is copied into an other (using vtkMRMLNode::Copy()) and is added to the scene afterward.
  // Related issue are #3428 and #2608
  PipelinesCacheType::iterator it;
  it = this->DisplayPipelines.find(displayNode);
  if (it != this->DisplayPipelines.end())
    {
    return;
    }

  // Create pipeline
  Pipeline* pipeline = new Pipeline();
  this->DisplayPipelines.insert( std::make_pair(displayNode, pipeline) );

  // Update cached matrices. Calls UpdateDisplayNodePipeline
  this->UpdateDisplayableNode(mNode);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::UpdateDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode)
{
  // If the DisplayNode already exists, just update.
  //   otherwise, add as new node
  if (!displayNode)
    {
    return;
    }
  PipelinesCacheType::iterator it;
  it = this->DisplayPipelines.find(displayNode);
  if (it != this->DisplayPipelines.end())
    {
    this->UpdateDisplayNodePipeline(displayNode, it->second);
    }
  else
    {
    this->AddNode(
      vtkMRMLMarkupsPlaneNode::SafeDownCast(displayNode->GetDisplayableNode()));
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::UpdateActiveNode()
{
  vtkMRMLSelectionNode* selectionNode = this->External->GetSelectionNode();
  vtkMRMLMarkupsPlaneNode* planesNode =
    vtkMRMLMarkupsPlaneNode::SafeDownCast(
      this->External->GetMRMLScene()->GetNodeByID(
        selectionNode ? selectionNode->GetActivePlaceNodeID() : NULL));

  if (!planesNode)
    {
    planesNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(
      this->External->GetMRMLScene()->AddNode(vtkMRMLMarkupsPlaneNode::New()));
    planesNode->Delete();
    }

  // ModelDisplayableManager is expected to be instantiated !
  vtkMRMLModelDisplayableManager * modelDisplayableManager =
    vtkMRMLModelDisplayableManager::SafeDownCast(
      this->External->GetMRMLDisplayableManagerGroup()->GetDisplayableManagerByClassName(
        "vtkMRMLModelDisplayableManager"));
  assert(modelDisplayableManager);

  vtkRenderWindowInteractor* interactor = this->External->GetInteractor();
  double x = interactor->GetEventPosition()[0];
  double y = interactor->GetEventPosition()[1];

  double windowHeight = interactor->GetRenderWindow()->GetSize()[1];
  double yNew = windowHeight - y - 1;

  double worldCoordinates[4];
  if (modelDisplayableManager->Pick(x,yNew))
    {
    double* pickedWorldCoordinates = modelDisplayableManager->GetPickedRAS();
    worldCoordinates[0] = pickedWorldCoordinates[0];
    worldCoordinates[1] = pickedWorldCoordinates[1];
    worldCoordinates[2] = pickedWorldCoordinates[2];
    worldCoordinates[3] = 1;
    }
  else
    {
    // we could not pick so just convert to world coordinates
    vtkInteractorObserver::ComputeDisplayToWorld(
      this->External->GetRenderer(),x,y,0,worldCoordinates);
    }

  if (this->ClickCounter == 1) // 2nd click (click counter==1) for the normal
    {
    double normal[3];
    vtkMath::Subtract(worldCoordinates, this->LastPosition, normal);
    double distance = vtkMath::Normalize(normal);

    planesNode->AddPlane(
      this->LastPosition[0], this->LastPosition[1], this->LastPosition[2],
      normal[0], normal[1], normal[2],
      this->LastPosition[0] - distance, this->LastPosition[1] - distance, this->LastPosition[2] - distance,
      this->LastPosition[0] + distance, this->LastPosition[1] + distance, this->LastPosition[2] + distance
      );

    this->StopInteraction();
    }
  else // First click (click counter==0) for the origin
    {
    this->ClickedPointActor->SetPosition(worldCoordinates);
    this->ClickedPointActor->SetVisibility(1);
    for (int i = 0; i < 3; ++i)
      {
      this->LastPosition[i] = worldCoordinates[i];
      }
    ++this->ClickCounter;
    }
  this->External->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal::StopInteraction()
{
  vtkMRMLInteractionNode* interactionNode =
    this->External->GetInteractionNode();
  interactionNode->SwitchToViewTransformMode();
  this->ClickedPointActor->SetVisibility(0);
  this->ClickCounter = 0;
  this->External->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::UpdateWidgetFromNode(vtkMRMLMarkupsDisplayNode* displayNode,
                       vtkMRMLMarkupsPlaneNode* planeNode,
                       Pipeline* pipeline)
{
  size_t numberOfWidgets =  pipeline->Widgets.size();
  // If we need to remove plane widgets, we don't know which one
  // were deleted, so remove all of them.
  if (planeNode && planeNode->GetNumberOfMarkups() < numberOfWidgets)
    {
    this->ClearPipeline(pipeline);
    }
  // Add any missing plane widget.
  if (planeNode && planeNode->GetNumberOfMarkups() > numberOfWidgets)
    {
    for (int i = numberOfWidgets; i < planeNode->GetNumberOfMarkups(); ++i)
      {
      vtkImplicitPlaneWidget2* widget = this->CreatePlaneWidget();
      pipeline->Widgets.push_back(widget);
      this->WidgetMap[widget] = displayNode;
      }
    }

  for (size_t n = 0; n < pipeline->Widgets.size(); ++n)
    {
    vtkImplicitPlaneWidget2* widget = pipeline->Widgets[n];
    bool visible = planeNode ? planeNode->GetNthMarkupVisibility(n) : false;
    widget->SetEnabled(visible);

    int processEvents = planeNode->GetLocked() ? 0 :
       (planeNode->GetNthMarkupLocked(n) ? 0 : 1);
    widget->SetProcessEvents(processEvents);

    if (visible)
      {
      vtkImplicitPlaneRepresentation* rep =
        widget->GetImplicitPlaneRepresentation();

      // origin
      double origin[4];
      planeNode->GetNthPlaneOriginWorldCoordinates(n, origin);
      rep->SetOrigin(origin);

      // normal
      double normal[4];
      planeNode->GetNthPlaneNormal(n, normal);
      rep->SetNormal(normal);

      double min[4], max[4];
      planeNode->GetNthPlaneBoundMinimumWorldCoordinates(n, min);
      planeNode->GetNthPlaneBoundMaximumWorldCoordinates(n, max);
      rep->SetWidgetBounds(min[0], max[0], min[1], max[1], min[2], max[2]);
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::UpdateNodeFromWidget(vtkImplicitPlaneWidget2* widget)
{
  assert(widget);
  vtkMRMLMarkupsDisplayNode* displayNode = this->WidgetMap.at(widget);
  assert(displayNode);
  vtkMRMLMarkupsPlaneNode* planeNode =
    vtkMRMLMarkupsPlaneNode::SafeDownCast(displayNode->GetDisplayableNode());

  vtkImplicitPlaneRepresentation* rep = widget->GetImplicitPlaneRepresentation();

  Pipeline* pipeline = this->DisplayPipelines[displayNode];
  size_t index =
    std::find(pipeline->Widgets.begin(), pipeline->Widgets.end(), widget)
    - pipeline->Widgets.begin();

  int wasModifying = planeNode->StartModify();

  // origin
  double origin[3];
  rep->GetOrigin(origin);
  planeNode->SetNthPlaneOriginFromArray(index, origin);

  // normal
  double normal[3];
  rep->GetNormal(normal);
  planeNode->SetNthPlaneNormalFromArray(index, normal);

  double bounds[6];
  rep->GetWidgetBounds(bounds);
  planeNode->SetNthPlaneBoundMinimum(index, bounds[0], bounds[2], bounds[4]);
  planeNode->SetNthPlaneBoundMaximum(index, bounds[1], bounds[3], bounds[5]);

  planeNode->EndModify(wasModifying);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::UpdateDisplayNodePipeline(
  vtkMRMLMarkupsDisplayNode* displayNode, Pipeline* pipeline)
{
  if (!displayNode || !pipeline)
    {
    return;
    }

  vtkMRMLMarkupsPlaneNode* planeNode =
    vtkMRMLMarkupsPlaneNode::SafeDownCast(displayNode->GetDisplayableNode());

  this->UpdateWidgetFromNode(displayNode, planeNode, pipeline);
}

//---------------------------------------------------------------------------
std::vector<int> vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::EventsToObserve() const
{
  std::vector<int> events;
  events.push_back(vtkMRMLDisplayableNode::DisplayModifiedEvent);
  events.push_back(vtkMRMLMarkupsPlaneNode::MarkupAddedEvent);
  events.push_back(vtkMRMLMarkupsPlaneNode::MarkupRemovedEvent);
  events.push_back(vtkMRMLMarkupsPlaneNode::NthMarkupModifiedEvent);
  events.push_back(vtkMRMLMarkupsPlaneNode::PointModifiedEvent);
  events.push_back(vtkCommand::ModifiedEvent);
  return events;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::AddObservations(vtkMRMLMarkupsPlaneNode* node)
{
  std::vector<int> events = this->EventsToObserve();
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  for (size_t i = 0; i < events.size(); ++i)
    {
    if (!broker->GetObservationExist(node, events[i], this->External, this->External->GetMRMLNodesCallbackCommand()))
      {
      broker->AddObservation(node, events[i], this->External, this->External->GetMRMLNodesCallbackCommand());
      }
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::RemoveObservations(vtkMRMLMarkupsPlaneNode* node)
{
  vtkEventBroker* broker = vtkEventBroker::GetInstance();
  vtkEventBroker::ObservationVector observations;
  std::vector<int> events = this->EventsToObserve();
  for (size_t i = 0; i < events.size(); ++i)
    {
    observations = broker->GetObservations(node, events[i], this->External, this->External->GetMRMLNodesCallbackCommand() );
    }
  broker->RemoveObservations(observations);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::AddObservationsToInteractionNode(vtkMRMLInteractionNode* interactionNode)
{
  if (!interactionNode)
    {
    return;
    }

  vtkNew<vtkIntArray> interactionEvents;
  interactionEvents->InsertNextValue(vtkMRMLInteractionNode::InteractionModeChangedEvent);
  interactionEvents->InsertNextValue(vtkMRMLInteractionNode::EndPlacementEvent);
  this->External->GetMRMLNodesObserverManager()->AddObjectEvents(
    interactionNode, interactionEvents.GetPointer());
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::RemoveObservationsFromInteractionNode(vtkMRMLInteractionNode* interactionNode)
{
  this->External->GetMRMLNodesObserverManager()->RemoveObjectEvents(interactionNode);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal::ClearDisplayableNodes()
{
  while(this->MarkupToDisplayNodes.size() > 0)
    {
    this->RemoveNode(this->MarkupToDisplayNodes.begin()->first);
    }
}

//---------------------------------------------------------------------------
// vtkMRMLMarkupsPlaneDisplayableManager3D methods
//---------------------------------------------------------------------------
vtkMRMLMarkupsPlaneDisplayableManager3D::vtkMRMLMarkupsPlaneDisplayableManager3D()
{
  this->Internal = new vtkInternal(this);
}

//---------------------------------------------------------------------------
vtkMRMLMarkupsPlaneDisplayableManager3D::~vtkMRMLMarkupsPlaneDisplayableManager3D()
{
  delete this->Internal;
  this->Internal=NULL;
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::PrintSelf ( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf ( os, indent );
  os << indent << "vtkMRMLMarkupsPlaneDisplayableManager3D: "
     << this->GetClassName() << "\n";
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D
::SetMRMLSceneInternal(vtkMRMLScene* newScene)
{
  Superclass::SetMRMLSceneInternal(newScene);

  if (newScene)
    {
    this->Internal->AddObservationsToInteractionNode(
      this->GetInteractionNode());
    }
  else
    {
    this->Internal->RemoveObservationsFromInteractionNode(
      this->GetInteractionNode());
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D
::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node)
    {
    return;
    }

  if (node->IsA("vtkMRMLInteractionNode"))
    {
    this->Internal->AddObservationsToInteractionNode(
      vtkMRMLInteractionNode::SafeDownCast(node));
    return;
    }

  if (!node->IsA("vtkMRMLMarkupsPlaneNode"))
    {
    return;
    }

  // Escape if the scene a scene is being closed, imported or connected
  if (this->GetMRMLScene()->IsBatchProcessing())
    {
    this->SetUpdateFromMRMLRequested(1);
    return;
    }

  this->Internal->AddNode(vtkMRMLMarkupsPlaneNode::SafeDownCast(node));
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node
    && (!node->IsA("vtkMRMLMarkupsPlaneNode"))
    && (!node->IsA("vtkMRMLMarkupDisplayNode")))
    {
    return;
    }

  vtkMRMLMarkupsPlaneNode* planeNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(node);
  vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(node);

  bool modified = false;
  if (planeNode)
    {
    this->Internal->RemoveNode(planeNode);
    modified = true;
    }
  else if (displayNode)
    {
    this->Internal->RemoveDisplayNode(displayNode);
    modified = true;
    }
  if (modified)
    {
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D
::ProcessMRMLNodesEvents(vtkObject* caller, unsigned long event, void* callData)
{
  vtkMRMLScene* scene = this->GetMRMLScene();
  if (scene->IsBatchProcessing())
    {
    return;
    }

  vtkMRMLMarkupsPlaneNode* planeNode = vtkMRMLMarkupsPlaneNode::SafeDownCast(caller);
  vtkMRMLMarkupsDisplayNode* displayNode = vtkMRMLMarkupsDisplayNode::SafeDownCast(caller);
  vtkMRMLInteractionNode* interactionNode = vtkMRMLInteractionNode::SafeDownCast(caller);

  if (planeNode)
    {
    displayNode = reinterpret_cast<vtkMRMLMarkupsDisplayNode*>(callData);
    if (displayNode && (event == vtkMRMLDisplayableNode::DisplayModifiedEvent) )
      {
      this->Internal->UpdateDisplayNode(displayNode);
      this->RequestRender();
      }
    else
      {
      this->Internal->UpdateDisplayableNode(planeNode);
      this->RequestRender();
      }
    }
  else if (interactionNode)
    {
    if (event == vtkMRMLInteractionNode::EndPlacementEvent)
      {
      this->Internal->StopInteraction();
      }
    }
  else
    {
    this->Superclass::ProcessMRMLNodesEvents(caller, event, callData);
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::UpdateFromMRML()
{
  this->SetUpdateFromMRMLRequested(0);

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (!scene)
    {
    return;
    }
  this->Internal->ClearDisplayableNodes();

  vtkMRMLMarkupsPlaneNode* mNode = NULL;
  std::vector<vtkMRMLNode *> mNodes;
  int nnodes = scene ? scene->GetNodesByClass("vtkMRMLMarkupsPlaneNode", mNodes) : 0;
  for (int i=0; i < nnodes; i++)
    {
    mNode  = vtkMRMLMarkupsPlaneNode::SafeDownCast(mNodes[i]);
    if (mNode)
      {
      this->Internal->AddNode(mNode);
      }
    }
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D
::OnInteractorStyleEvent(int eventid)
{
  vtkMRMLSelectionNode* selectionNode =
    this->GetMRMLApplicationLogic()->GetSelectionNode();
  if (!selectionNode || !selectionNode->GetActivePlaceNodeClassName())
    {
    return;
    }

  if (strcmp(selectionNode->GetActivePlaceNodeClassName(), "vtkMRMLMarkupsPlaneNode") != 0)
    {
    return;
    }

  if (eventid == vtkCommand::LeftButtonReleaseEvent)
    {
    if (this->GetInteractionNode()->GetCurrentInteractionMode() == vtkMRMLInteractionNode::Place)
      {
      this->Internal->UpdateActiveNode();
      }
    }
  else if (eventid == vtkCommand::RightButtonReleaseEvent)
    {
    this->Internal->StopInteraction();
    }
}


//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::UnobserveMRMLScene()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::OnMRMLSceneStartClose()
{
  this->Internal->ClearDisplayableNodes();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::OnMRMLSceneEndClose()
{
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::OnMRMLSceneEndBatchProcess()
{
  this->SetUpdateFromMRMLRequested(1);
  this->RequestRender();
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D::Create()
{
  Superclass::Create();
  this->SetUpdateFromMRMLRequested(1);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D
::ProcessWidgetsEvents(vtkObject* caller, unsigned long event, void* callData)
{
  Superclass::ProcessWidgetsEvents(caller, event, callData);

  vtkImplicitPlaneWidget2* planeWidget =
    vtkImplicitPlaneWidget2::SafeDownCast(caller);
  if (planeWidget)
    {
    this->Internal->UpdateNodeFromWidget(planeWidget);
    this->RequestRender();
    }
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneDisplayableManager3D
::SetRenderer(vtkRenderer* newRenderer)
{
  Superclass::SetRenderer(newRenderer);
  if (newRenderer)
    {
    newRenderer->AddActor(this->Internal->ClickedPointActor);
    }
}
