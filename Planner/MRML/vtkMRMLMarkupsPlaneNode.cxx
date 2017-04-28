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


// MRML includes
#include <vtkMRMLMarkupsDisplayNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// Planner includes
#include "vtkMRMLMarkupsPlaneNode.h"

// STD includes
#include <sstream>

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMarkupsPlaneNode);


//----------------------------------------------------------------------------
vtkMRMLMarkupsPlaneNode::vtkMRMLMarkupsPlaneNode()
{

}

//----------------------------------------------------------------------------
vtkMRMLMarkupsPlaneNode::~vtkMRMLMarkupsPlaneNode()
{

}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of,nIndent);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::ReadXMLAttributes(const char** atts)
{
  Superclass::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}

//-----------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::ProcessMRMLEvents ( vtkObject *caller,
                                           unsigned long event,
                                           void *callData )
{
  Superclass::ProcessMRMLEvents(caller, event, callData);
}

//----------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os,indent);
}

//-------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLMarkupsPlaneNode::CreateDefaultStorageNode()
{
  return Superclass::CreateDefaultStorageNode();
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::CreateDefaultDisplayNodes()
{
  if (this->GetDisplayNode() != NULL &&
      vtkMRMLMarkupsDisplayNode::SafeDownCast(this->GetDisplayNode()) != NULL)
    {
    // display node already exists
    return;
    }
  if (this->GetScene()==NULL)
    {
    vtkErrorMacro(
      "vtkMRMLMarkupsPlaneNode::CreateDefaultDisplayNodes failed: scene is invalid");
    return;
    }
  vtkNew<vtkMRMLMarkupsDisplayNode> dispNode;
  this->GetScene()->AddNode(dispNode.GetPointer());
  this->SetAndObserveDisplayNodeID(dispNode->GetID());
}

//-------------------------------------------------------------------------
vtkMRMLMarkupsDisplayNode *vtkMRMLMarkupsPlaneNode::GetMarkupsDisplayNode()
{
  vtkMRMLDisplayNode *displayNode = this->GetDisplayNode();
  if (displayNode &&
      displayNode->IsA("vtkMRMLMarkupsDisplayNode"))
    {
    return vtkMRMLMarkupsDisplayNode::SafeDownCast(displayNode);
    }
  return NULL;
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsPlaneNode::AddPlane(double x, double y, double z,
                                      double nx, double ny, double nz,
                                      double xmin, double ymin, double zmin,
                                      double xmax, double ymax, double zmax,
                                      std::string label)
{
  Markup markup;
  markup.Label = label;
  this->InitMarkup(&markup);

  markup.points.resize(4);
  markup.points[vtkMRMLMarkupsPlaneNode::ORIGIN_INDEX] = vtkVector3d(x,y,z);
  markup.points[vtkMRMLMarkupsPlaneNode::NORMAL_INDEX] = vtkVector3d(nx,ny,nz);
  markup.points[vtkMRMLMarkupsPlaneNode::BOUND_MIN_INDEX] = vtkVector3d(xmin,ymin,zmin);
  markup.points[vtkMRMLMarkupsPlaneNode::BOUND_MAX_INDEX] = vtkVector3d(xmax,ymax,zmax);
  return this->AddMarkup(markup);
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsPlaneNode::AddPlaneFromArray(double origin[3],
                                               double normal[3],
                                               double min[3],
                                               double max[3],
                                               std::string label)
{
  return this->AddPlane(
    origin[0], origin[1], origin[2],
    normal[0], normal[1], normal[2],
    min[0], min[1], min[2],
    max[0], max[1], max[2],
    label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneOrigin(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::ORIGIN_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneNormal(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::NORMAL_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneBoundMinimum(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::BOUND_MIN_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneBoundMaximum(int n, double pos[3])
{
  this->GetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::BOUND_MAX_INDEX, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneOrigin(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::ORIGIN_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneNormal(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::NORMAL_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneBoundMinimum(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::BOUND_MIN_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneBoundMaximum(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, vtkMRMLMarkupsPlaneNode::BOUND_MAX_INDEX, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneOriginFromArray(int n, double pos[3])
{
  this->SetNthPlaneOrigin(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneNormalFromArray(int n, double pos[3])
{
  this->SetNthPlaneNormal(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode
::SetNthPlaneBoundMinimumFromArray(int n, double pos[3])
{
  this->SetNthPlaneBoundMinimum(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode
::SetNthPlaneBoundMaximumFromArray(int n, double pos[3])
{
  this->SetNthPlaneBoundMaximum(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsPlaneNode::GetNthPlaneSelected(int n)
{
  return this->GetNthMarkupSelected(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneSelected(int n, bool flag)
{
  this->SetNthMarkupSelected(n, flag);
}

//-------------------------------------------------------------------------
bool vtkMRMLMarkupsPlaneNode::GetNthPlaneVisibility(int n)
{
  return this->GetNthMarkupVisibility(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneVisibility(int n, bool flag)
{
  this->SetNthMarkupVisibility(n, flag);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsPlaneNode::GetNthPlaneLabel(int n)
{
  return this->GetNthMarkupLabel(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneLabel(int n, std::string label)
{
  this->SetNthMarkupLabel(n, label);
}

//-------------------------------------------------------------------------
std::string vtkMRMLMarkupsPlaneNode::GetNthPlaneAssociatedNodeID(int n)
{
  return this->GetNthMarkupAssociatedNodeID(n);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneAssociatedNodeID(int n, const char* id)
{
  this->SetNthMarkupAssociatedNodeID(n, std::string(id));
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneOriginWorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(
    n, vtkMRMLMarkupsPlaneNode::ORIGIN_INDEX, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneOriginWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, vtkMRMLMarkupsPlaneNode::ORIGIN_INDEX, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneBoundMinimumWorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(
    n, vtkMRMLMarkupsPlaneNode::BOUND_MIN_INDEX, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneBoundMinimumWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, vtkMRMLMarkupsPlaneNode::BOUND_MIN_INDEX, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneBoundMaximumWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, vtkMRMLMarkupsPlaneNode::BOUND_MAX_INDEX, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneBoundMaximumWorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(
    n, vtkMRMLMarkupsPlaneNode::BOUND_MAX_INDEX, coords[0], coords[1], coords[2]);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetRASBounds(double bounds[6])
{
  vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfMarkups = this->GetNumberOfMarkups();
  if (numberOfMarkups == 0)
    {
    return;
    }

  for (int i = 0; i < numberOfMarkups; i++)
    {
    double tmp[4];
    this->GetNthPlaneBoundMinimumWorldCoordinates(i, tmp);
    box.AddPoint(tmp);
    this->GetNthPlaneBoundMaximumWorldCoordinates(i, tmp);
    box.AddPoint(tmp);
    }
  box.GetBounds(bounds);
}

//---------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetBounds(double bounds[6])
{
   vtkBoundingBox box;
  box.GetBounds(bounds);

  int numberOfMarkups = this->GetNumberOfMarkups();
  if (numberOfMarkups == 0)
    {
    return;
    }

  for (int i = 0; i < numberOfMarkups; i++)
    {
    double tmp[3];
    this->GetNthPlaneBoundMinimum(i, tmp);
    box.AddPoint(tmp);
    this->GetNthPlaneBoundMaximum(i, tmp);
    box.AddPoint(tmp);
    }
  box.GetBounds(bounds);
}
