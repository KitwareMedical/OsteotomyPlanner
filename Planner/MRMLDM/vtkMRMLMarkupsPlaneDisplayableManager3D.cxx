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
#include <vtkMRMLInteractionNode.h>
#include <vtkMRMLScene.h>
#include <vtkMRMLViewNode.h>

// VTK includes
#include <vtkAbstractWidget.h>
#include <vtkCallbackCommand.h>
#include <vtkEventBroker.h>
#include <vtkInteractorStyle.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkRenderer.h>
#include <vtkImplicitPlaneWidget2.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkSmartPointer.h>

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

  struct Pipeline
    {
    std::vector<vtkImplicitPlaneWidget2*> Widgets;
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

  // Observations
  void AddObservations(vtkMRMLMarkupsPlaneNode* node);
  void RemoveObservations(vtkMRMLMarkupsPlaneNode* node);

  // Helper functions
  bool UseDisplayNode(vtkMRMLMarkupsDisplayNode* displayNode);
  void ClearDisplayableNodes();
  void ClearPipeline(Pipeline* pipeline);
  vtkImplicitPlaneWidget2* CreatePlaneWidget() const;
  std::vector<int> EventsToObserve() const;

private:
  vtkMRMLMarkupsPlaneDisplayableManager3D* External;
  bool AddingNode;
};

//---------------------------------------------------------------------------
// vtkInternal methods

//---------------------------------------------------------------------------
vtkMRMLMarkupsPlaneDisplayableManager3D::vtkInternal
::vtkInternal(vtkMRMLMarkupsPlaneDisplayableManager3D * external)
: External(external)
, AddingNode(false)
{
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

    if (visible)
      {
      vtkImplicitPlaneRepresentation* rep =
        widget->GetImplicitPlaneRepresentation();

      // origin
      double origin[3];
      planeNode->GetNthPlaneOrigin(n, origin);
      rep->SetOrigin(origin);

      // normal
      double normal[3];
      planeNode->GetNthPlaneNormal(n, normal);
      rep->SetNormal(normal);

      double bounds[6];
      planeNode->GetNthPlaneBoundMinimum(n, bounds);
      planeNode->GetNthPlaneBoundMaximum(n, &(bounds[2]));
      rep->SetWidgetBounds(bounds);
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
  planeNode->SetNthPlaneBoundMinimumFromArray(index, bounds);
  planeNode->SetNthPlaneBoundMaximumFromArray(index, &(bounds[2]));
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
::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (!node || !node->IsA("vtkMRMLMarkupsPlaneNode"))
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
