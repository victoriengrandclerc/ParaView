/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVInteractorStyleControl.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVInteractorStyleControl.h"

#include "vtkArrayMap.txx"
#include "vtkArrayMapIterator.txx"
#include "vtkCollection.h"
#include "vtkCollectionIterator.h"
#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVCameraManipulator.h"
#include "vtkPVInteractorStyle.h"
#include "vtkPVPushButton.h"
#include "vtkPVScale.h"
#include "vtkPVWidget.h"
#include "vtkString.h"
#include "vtkTclUtil.h"
#include "vtkVector.txx"

//------------------------------------------------------------------------------
vtkStandardNewMacro( vtkPVInteractorStyleControl );
vtkCxxSetObjectMacro(vtkPVInteractorStyleControl,ManipulatorCollection,
                     vtkCollection);

vtkPVInteractorStyleControl::vtkPVInteractorStyleControl()
{
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->LabeledFrame->SetParent(this);
  
  int cc;

  for ( cc = 0; cc < 6; cc ++ )
    {
    this->Labels[cc] = vtkKWLabel::New();
    }
  for ( cc = 0; cc < 9; cc ++ )
    {
    this->Menus[cc] = vtkKWOptionMenu::New();
    }

  this->Manipulators = vtkPVInteractorStyleControl::ManipulatorMap::New();
  this->Widgets = vtkPVInteractorStyleControl::WidgetsMap::New();
  this->Arguments = vtkPVInteractorStyleControl::MapStringToArrayStrings::New();

  this->ManipulatorCollection = 0;
  this->DefaultManipulator = 0;
  this->RegisteryName = 0;

  this->ArgumentsFrame = vtkKWFrame::New();

  this->CurrentManipulator = 0;
}

//------------------------------------------------------------------------------
vtkPVInteractorStyleControl::~vtkPVInteractorStyleControl()
{
  this->StoreRegistery();
  int cc;
  if ( this->LabeledFrame )
    {
    this->LabeledFrame->Delete();
    }
  for ( cc = 0; cc < 6; cc ++ )
    {
    this->Labels[cc]->Delete();
    }
  for ( cc = 0; cc < 9; cc ++ )
    {
    this->Menus[cc]->Delete();
    }
  this->Manipulators->Delete();
  this->Widgets->Delete();
  this->Arguments->Delete();
  this->SetManipulatorCollection(0);
  this->SetDefaultManipulator(0);
  this->SetRegisteryName(0);
  
  this->ArgumentsFrame->Delete();
}

//------------------------------------------------------------------------------
void vtkPVInteractorStyleControl::AddManipulator(const char* name, 
                                                 vtkPVCameraManipulator* object)
{
  this->Manipulators->SetItem(name, object);
}

//------------------------------------------------------------------------------
void vtkPVInteractorStyleControl::UpdateMenus()
{
  if ( this->Application )
    {
    this->ReadRegistery();
    vtkPVInteractorStyleControl::ManipulatorMapIterator* it 
      = this->Manipulators->NewIterator();
    int cc;
    for ( cc = 0; cc < 9; cc ++ )
      {
      this->Menus[cc]->ClearEntries();
      char command[100];
      it->InitTraversal();
      while ( !it->IsDoneWithTraversal() )
        {
        const char* name = 0;
        it->GetKey(name);
        sprintf(command, "SetCurrentManipulator %d {%s}", cc, name);
        this->Menus[cc]->AddEntryWithCommand(name, this, command);
        it->GoToNextItem();
        }
      if ( this->GetManipulator(cc) == 0 && this->DefaultManipulator )
        {
        this->SetCurrentManipulator(cc, this->DefaultManipulator);
        }
      }
    it->Delete();
    }

  if ( this->ArgumentsFrame->IsCreated() )
    {
    this->Script("catch { eval pack forget [ pack slaves %s ] }",
                 this->ArgumentsFrame->GetWidgetName());

    vtkPVInteractorStyleControl::WidgetsMap::IteratorType *it
      = this->Widgets->NewIterator();
    it->InitTraversal();
    while(!it->IsDoneWithTraversal())
      {
      const char* name = 0;
      vtkPVWidget *widget = 0;
      if ( it->GetData(widget) == VTK_OK && widget &&
           it->GetKey(name) == VTK_OK && name )
        {
        if ( !widget->IsCreated() )
          {
          widget->SetParent(this->ArgumentsFrame);
          widget->Create(this->Application);
          ostrstream str;
          str << "ChangeArgument " << name << " " 
              << widget->GetTclName() << ends;
          widget->SetModifiedCommand(this->GetTclName(), str.str());
          str.rdbuf()->freeze(0);
          
          char manipulator[100];
          char buffer[100];
          sprintf(manipulator, "Manipulator%s", name);
          if ( this->Application->GetRegisteryValue(2, "RunTime", manipulator,
                                                    buffer) &&
               *buffer > 0 )
            {
            vtkPVScale *sc = vtkPVScale::SafeDownCast(widget);
            if ( sc )
              {
              this->Script("%s SetValue %s", sc->GetTclName(),
                           buffer);
              }
            }
          }
        this->Script("pack %s -fill y -expand true -side top",
                     widget->GetWidgetName());
        }
      it->GoToNextItem();
      }
    it->Delete();
    }
}

//------------------------------------------------------------------------------
void vtkPVInteractorStyleControl::ChangeArgument(const char* name, 
                                                 const char* swidget)
{
  vtkPVInteractorStyleControl::ArrayStrings *strings = 0;
  if ( this->Arguments->GetItem(name, strings) != VTK_OK || !strings )
    {
    cout << "Cannot find arguments..." << endl;
    return;
    }

  int error =0;
  vtkPVWidget *widget = static_cast<vtkPVWidget*>(
    vtkTclGetPointerFromObject(swidget, "vtkPVWidget", 
                               this->Application->GetMainInterp(), error));
  if ( !widget )
    {
    vtkErrorMacro("Change argument called without valid widget");
    return;
    }
  vtkPVScale* scale = vtkPVScale::SafeDownCast(widget);
  vtkPVPushButton* pushButton = vtkPVPushButton::SafeDownCast(widget);
  char* value = 0;
  if ( scale )
    {
    ostrstream str;
    str << "[ " << scale->GetTclName() << " GetValue ]" << ends;
    value = vtkString::Duplicate(str.str());
    str.rdbuf()->freeze(0);
    }
  else if ( pushButton )
    {
    value = vtkString::Duplicate("");
    }
  else
    {
    cout << "Unknown widget" << endl;
    return;
    }

  int found = 0;
  
  vtkPVInteractorStyleControl::ArrayStrings::IteratorType *vit
    = strings->NewIterator();
  vit->InitTraversal();
  while ( !vit->IsDoneWithTraversal() )
    {
    const char* smanipulator = 0;
    if ( vit->GetData(smanipulator) == VTK_OK && smanipulator )
      {
      vtkCollectionIterator *cit = this->ManipulatorCollection->NewIterator();
      cit->InitTraversal();
      while ( !cit->IsDoneWithTraversal() )
        {
        vtkPVCameraManipulator* cman 
          = static_cast<vtkPVCameraManipulator*>(cit->GetObject());
        int mouse = cman->GetButton() - 1;
        int key = 0;
        if ( cman->GetShift() )
          {
          key = 1;
          }
        if ( cman->GetControl() )
          {
          key = 2;
          }
        const char* sman = this->Menus[mouse + key * 3]->GetValue();
        if ( vtkString::Equals(smanipulator, sman) )
          {
          this->CurrentManipulator = cman;
          this->Script("[ %s GetCurrentManipulator ] Set%s %s", 
                       this->GetTclName(), name, value );
          this->CurrentManipulator = 0;
          found = 1;
          }
        cit->GoToNextItem();
        }
      cit->Delete();
      }    
    vit->GoToNextItem();
    }
  vit->Delete();

  if ( found )
    {
    if ( vtkString::Length(value) > 0 )
      {
      const char* val = this->Application->EvaluateString("%s", value);
      char *rname = vtkString::Append("Manipulator", name);      
      this->Application->SetRegisteryValue(2, "RunTime", rname, val);
      delete[] rname;
      }
    }
  delete [] value;
}

//------------------------------------------------------------------------------
void vtkPVInteractorStyleControl::SetCurrentManipulator(
  int mouse, int key, const char* name)
{
  if ( mouse < 0 || mouse > 2 || key < 0 || key > 2 )
    {
    vtkErrorMacro("Setting manipulator to the wrong key or mouse");
    return;
    }
  this->SetCurrentManipulator(mouse + key * 3, name);
}

//------------------------------------------------------------------------------
void vtkPVInteractorStyleControl::SetCurrentManipulator(
  int pos, const char* name)
{
  this->SetManipulator(pos, name);
  if ( pos < 0 || pos > 8 || !this->ManipulatorCollection )
    {
    return;
    }
  vtkPVCameraManipulator *manipulator = this->GetManipulator(name);
  if ( !manipulator )
    {
    return;
    }

  int mouse = pos % 3;
  int key = static_cast<int>(pos / 3);
  int shift = (key == 1);
  int control = (key == 2);

  vtkCollectionIterator *it = this->ManipulatorCollection->NewIterator();
  it->InitTraversal();

  vtkPVCameraManipulator *clone = 0;
  while(!it->IsDoneWithTraversal())
    {
    vtkPVCameraManipulator* access 
      = static_cast<vtkPVCameraManipulator*>(it->GetObject());
    
    if ( access->GetButton() == mouse+1 &&
         access->GetShift() == shift &&
         access->GetControl() == control )
      {
      if ( vtkString::Equals(access->GetClassName(), 
                             manipulator->GetClassName()) )
        {
        clone = access;
        }
      else
        {
        this->ManipulatorCollection->RemoveItem(access);
        }
      break;
      }

    it->GoToNextItem();
    }  
  it->Delete();
  
  if ( !clone )
    {
    clone = manipulator->NewInstance();
    clone->SetApplication(this->Application);
    this->ManipulatorCollection->AddItem(clone); 
    clone->Delete();
    }
  clone->SetButton(mouse+1);
  clone->SetShift(shift);
  clone->SetControl(control);
}

//------------------------------------------------------------------------------
void vtkPVInteractorStyleControl::SetLabel(const char* label)
{
  if ( this->LabeledFrame && this->Application )
    {
    ostrstream str;
    str << "Camera Control for: " << label << ends;
    this->LabeledFrame->SetLabel(str.str());
    str.rdbuf()->freeze(0);
    }
}

//------------------------------------------------------------------------------
int vtkPVInteractorStyleControl::SetManipulator(int pos, const char* name)
{
  if ( pos < 0 || pos > 8 )
    {
    vtkErrorMacro("There are only 9 possible menus");
    return 0;
    }
  if ( !this->GetManipulator(name) ) 
    {
    vtkErrorMacro("Manipulator: " << name << " does not exist");
    return 0;
    }
  this->Menus[pos]->SetValue(name);
  return 1;
}

//------------------------------------------------------------------------------
int vtkPVInteractorStyleControl::SetManipulator(int mouse, int key, 
                                                const char* name)
{
  if ( mouse < 0 || mouse > 2 || key < 0 || key > 2 )
    {
    vtkErrorMacro("Setting manipulator to the wrong key or mouse");
    return 0;
    }
  return this->SetManipulator(mouse + key * 3, name);
}

//------------------------------------------------------------------------------
vtkPVCameraManipulator* vtkPVInteractorStyleControl::GetManipulator(int pos)
{
  if ( pos < 0 || pos > 8 )
    {
    vtkErrorMacro("There are only 9 possible menus");
    return 0;
    }
  const char* name = this->Menus[pos]->GetValue();
  return this->GetManipulator(name);
}

//------------------------------------------------------------------------------
vtkPVCameraManipulator* 
vtkPVInteractorStyleControl::GetManipulator(int mouse, int key)
{
  if ( mouse < 0 || mouse > 2 || key < 0 || key > 2 )
    {
    vtkErrorMacro("Getting manipulator from the wrong key or mouse");
    return 0;
    }
  return this->GetManipulator(mouse + key * 3);
}

//------------------------------------------------------------------------------
vtkPVCameraManipulator* 
vtkPVInteractorStyleControl::GetManipulator(const char* name)
{
  vtkPVCameraManipulator* manipulator = 0;
  this->Manipulators->GetItem(name, manipulator);
  return manipulator;
}

//------------------------------------------------------------------------------
void vtkPVInteractorStyleControl::Create(vtkKWApplication *app, const char*)
{
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("ScrollableFrame already created");
    return;
    }

  this->SetApplication(app);
  const char *wname;
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat",wname);
  
  this->LabeledFrame->ShowHideFrameOn();
  this->LabeledFrame->Create(app);
  this->LabeledFrame->SetLabel("Camera Manipulators Control");

  vtkKWFrame *frame = vtkKWFrame::New();
  frame->SetParent(this->LabeledFrame->GetFrame());
  frame->Create(app, 0);
  
  int cc;

  for ( cc = 0; cc < 6; cc ++ )
    {
    this->Labels[cc]->SetParent(frame->GetFrame());
    this->Labels[cc]->Create(app, "");
    }

  for ( cc = 0; cc < 9; cc ++ )
    {
    this->Menus[cc]->SetParent(frame->GetFrame());
    this->Menus[cc]->Create(app, "");
    }

  this->Labels[0]->SetLabel("Left");
  this->Labels[1]->SetLabel("Middle");
  this->Labels[2]->SetLabel("Right");
  this->Labels[3]->SetLabel("Plain");
  this->Labels[4]->SetLabel("Shift");
  this->Labels[5]->SetLabel("Control");

  this->Script("grid x %s %s %s -sticky ew", 
               this->Labels[0]->GetWidgetName(), 
               this->Labels[1]->GetWidgetName(), 
               this->Labels[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew", 
               this->Labels[3]->GetWidgetName(), 
               this->Menus[0]->GetWidgetName(), 
               this->Menus[1]->GetWidgetName(), 
               this->Menus[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew", 
               this->Labels[4]->GetWidgetName(), 
               this->Menus[3]->GetWidgetName(), 
               this->Menus[4]->GetWidgetName(), 
               this->Menus[5]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew", 
               this->Labels[5]->GetWidgetName(), 
               this->Menus[6]->GetWidgetName(), 
               this->Menus[7]->GetWidgetName(), 
               this->Menus[8]->GetWidgetName());
               
  this->Script("grid columnconfigure %s 0 -weight 0", 
               frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2", 
               frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 2", 
               frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 3 -weight 2", 
               frame->GetFrame()->GetWidgetName());
  

  frame->Delete();
  this->Script("pack %s -expand true -fill both -side top", 
               frame->GetWidgetName());
  this->Script("pack %s -expand true -fill both", 
               this->LabeledFrame->GetWidgetName());
  this->UpdateMenus();

  this->ArgumentsFrame->SetParent(this->LabeledFrame->GetFrame());
  this->ArgumentsFrame->Create(this->Application, 0);
  this->Script("pack %s -expand true -fill both -side top", 
               this->ArgumentsFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::ReadRegistery()
{
  if ( !this->Application || !this->RegisteryName )
    {
    vtkErrorMacro("Application and type of Interactor Style Controler have to be defined");
    }
  int cc;
  char manipulator[100];
  char buffer[100];
  for ( cc = 0; cc < 9; cc ++ )
    {
    int mouse = cc % 3;
    int key = static_cast<int>(cc / 3);
    buffer[0] = 0;
    sprintf(manipulator, "ManipulatorT%sM%dK%d", 
            this->RegisteryName, mouse, key);
    if ( this->Application->GetRegisteryValue(2, "RunTime", manipulator,
                                              buffer) &&
         *buffer > 0 &&
         this->GetManipulator(buffer) )
      {
      this->SetCurrentManipulator(mouse, key, buffer);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::StoreRegistery()
{
  if ( !this->Application || !this->RegisteryName )
    {
    vtkErrorMacro("Application and type of Interactor Style Controler have to be defined");
    return;
    }
  int cc;
  char manipulator[100];
  for ( cc = 0; cc < 9; cc ++ )
    {
    int mouse = cc % 3;
    int key = static_cast<int>(cc / 3);
    
    sprintf(manipulator, "ManipulatorT%sM%dK%d", 
            this->RegisteryName, mouse, key);
    this->Application->SetRegisteryValue(2, "RunTime", manipulator,
                                         this->Menus[cc]->GetValue());
    }
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::AddArgument(
  const char* name, const char* manipulator, vtkPVWidget* widget)
{
  if ( !name || !manipulator || !widget )
    {
    vtkErrorMacro("Name, manipulator, or widget not specified");
    return;
    }
  // Add widget to the map
  this->Widgets->SetItem(name, widget);

  // find vector of manipulators that respond to this argument
  vtkPVInteractorStyleControl::ArrayStrings* strings = 0;
  if ( this->Arguments->GetItem(name, strings) != VTK_OK || !strings )
    {
    // If there is none, create it.
    strings = vtkPVInteractorStyleControl::ArrayStrings::New();
    this->Arguments->SetItem(name, strings);
    strings->Delete();
    }
  
  // Now check if this manipulator is already on the list
  vtkIdType res = 0;
  if ( strings->FindItem(manipulator, res) != VTK_OK )
    {
    // if not add it.
    strings->AppendItem(manipulator);
    }
}

//----------------------------------------------------------------------------
void vtkPVInteractorStyleControl::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Frame: " << this->LabeledFrame << endl;
  os << indent << "DefaultManipulator: " << (this->DefaultManipulator?this->DefaultManipulator:"None") << endl;
  os << indent << "ManipulatorCollection: " << this->ManipulatorCollection << endl;
  os << indent << "RegisteryName: " << (this->RegisteryName?this->RegisteryName:"none") << endl;
  os << indent << "CurrentManipulator: " << this->CurrentManipulator << endl;
}
