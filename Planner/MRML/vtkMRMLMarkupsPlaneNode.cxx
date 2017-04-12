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
#include "vtkMRMLMarkupsDisplayNode.h"
#include "vtkMRMLMarkupsPlaneNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkBoundingBox.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

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
                                      double p1x, double p1y, double p1z,
                                      double p2x, double p2y, double p2z,
                                      std::string label)
{
  Markup markup;
  markup.Label = label;
  this->InitMarkup(&markup);

  markup.points.push_back(vtkVector3d(x,y,z));
  markup.points.push_back(vtkVector3d(p1x,p1y,p1z));
  markup.points.push_back(vtkVector3d(p2x,p2y,p2z));
  return this->AddMarkup(markup);
}

//-------------------------------------------------------------------------
int vtkMRMLMarkupsPlaneNode::AddPlaneFromArray(double origin[3],
                                               double p1[3],
                                               double p2[3],
                                               std::string label)
{
  return this->AddPlane(
    origin[0], origin[1], origin[2],
    p1[0], p1[1], p1[2],
    p2[0], p2[1], p2[2],
    label);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneOrigin(int n, double pos[3])
{
  this->GetMarkupPoint(n, 0, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneP1(int n, double pos[3])
{
  this->GetMarkupPoint(n, 1, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneP2(int n, double pos[3])
{
  this->GetMarkupPoint(n, 2, pos);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneOrigin(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, 0, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneP1(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, 1, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneP2(int n, double x, double y, double z)
{
  this->SetMarkupPoint(n, 2, x, y, z);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneOriginFromArray(int n, double pos[3])
{
  this->SetNthPlaneOrigin(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneP1FromArray(int n, double pos[3])
{
  this->SetNthPlaneP1(n, pos[0], pos[1], pos[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneP2FromArray(int n, double pos[3])
{
  this->SetNthPlaneP2(n, pos[0], pos[1], pos[2]);
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
  this->SetMarkupPointWorld(n, 0, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneP1WorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(n, 1, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::SetNthPlaneP2WorldCoordinates(int n, double coords[4])
{
  this->SetMarkupPointWorld(n, 2, coords[0], coords[1], coords[2]);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneOriginWorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, 0, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneP1WorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, 1, coords);
}

//-------------------------------------------------------------------------
void vtkMRMLMarkupsPlaneNode::GetNthPlaneP2WorldCoordinates(int n, double coords[4])
{
  this->GetMarkupPointWorld(n, 2, coords);
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
    this->GetNthPlaneP1WorldCoordinates(i, tmp);
    box.AddPoint(tmp);

    this->GetNthPlaneP2WorldCoordinates(i, tmp);
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
    this->GetNthPlaneP1(i, tmp);
    box.AddPoint(tmp);

    this->GetNthPlaneP2(i, tmp);
    box.AddPoint(tmp);
    }
  box.GetBounds(bounds);
}
