/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLabeledToggle.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLabeledToggle.h"

#include "vtkArrayMap.txx"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVIndexWidgetProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLabeledToggle);
vtkCxxRevisionMacro(vtkPVLabeledToggle, "1.27");

//----------------------------------------------------------------------------
vtkPVLabeledToggle::vtkPVLabeledToggle()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->CheckButton = vtkKWCheckButton::New();
  this->CheckButton->SetParent(this);
  this->DefaultValue = 0;
  this->Property = NULL;
}

//----------------------------------------------------------------------------
vtkPVLabeledToggle::~vtkPVLabeledToggle()
{
  this->CheckButton->Delete();
  this->CheckButton = NULL;
  this->Label->Delete();
  this->Label = NULL;
  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->GetApplication() && !this->BalloonHelpInitialized )
    {
    this->Label->SetBalloonHelpString(this->BalloonHelpString);
    this->CheckButton->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Create(vtkKWApplication *pvApp)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("LabeledToggle already created");
    return;
    }
  this->SetApplication(pvApp);
  
  const char* wname;
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  this->Label->Create(pvApp, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());
  
  // Now the check button
  this->CheckButton->Create(pvApp, "");
  this->CheckButton->SetCommand(this, "ModifiedCallback");
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left", this->CheckButton->GetWidgetName());
  
  this->SetState(this->Property->GetIndex());
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetState(int val)
{
  int oldVal;
  
  oldVal = this->CheckButton->GetState();
  if (val == oldVal)
    {
    return;
    }

  this->CheckButton->SetState(val);
  
  if (!this->AcceptCalled)
    {
    this->Property->SetIndex(val);
    }
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Disable()
{
  this->Script("%s configure -state disabled", 
               this->CheckButton->GetWidgetName());
  // TCL 8.2 does not allow to disable a label. Use the checkbutton's
  // color to make it look like disabled.
  this->Script("%s configure -foreground [%s cget -disabledforeground]", 
               this->Label->GetWidgetName(),
               this->CheckButton->GetWidgetName());
}

//---------------------------------------------------------------------------
void vtkPVLabeledToggle::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetState "
        << this->GetState() << endl;
}

//-----------------------------------------------------------------------------
void vtkPVLabeledToggle::SaveInBatchScript(ofstream *file)
{
  *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
        <<  " GetProperty " << this->VariableName << "] SetElements1 "
        << this->Property->GetIndex() << endl;
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::AcceptInternal(vtkClientServerID sourceID)
{
  this->ModifiedFlag = 0;
  this->Property->SetIndex(this->GetState());
  this->Property->SetVTKSourceID(sourceID);
  this->Property->AcceptInternal();
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  this->SetState(this->Property->GetIndex());

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//----------------------------------------------------------------------------
vtkPVLabeledToggle* vtkPVLabeledToggle::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVLabeledToggle::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::CopyProperties(vtkPVWidget* clone, 
                                        vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVLabeledToggle* pvlt = vtkPVLabeledToggle::SafeDownCast(clone);
  if (pvlt)
    {
    const char* label = this->Label->GetLabel();
    pvlt->Label->SetLabel(label);

    if (label && label[0] &&
        (pvlt->TraceNameState == vtkPVWidget::Uninitialized ||
         pvlt->TraceNameState == vtkPVWidget::Default) )
      {
      pvlt->SetTraceName(label);
      pvlt->SetTraceNameState(vtkPVWidget::SelfInitialized);
      }
    pvlt->SetDefaultValue(this->DefaultValue);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVLabeledToggle.");
    }
}

//----------------------------------------------------------------------------
int vtkPVLabeledToggle::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->Label->SetLabel(label);
    }
  else
    {
    this->Label->SetLabel(this->VariableName);
    }

  const char* defaultValue = element->GetAttribute("default_value");
  if (defaultValue)
    {
    this->DefaultValue = atoi(defaultValue);
    }
  else
    {
    vtkErrorMacro("No default value is specified. Setting to 1");
    this->DefaultValue = 1;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetLabel(const char *str) 
{
  this->Label->SetLabel(str); 
  if (str && str[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(str);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
}

//----------------------------------------------------------------------------
const char* vtkPVLabeledToggle::GetLabel() 
{ 
  return this->Label->GetLabel();
}

//----------------------------------------------------------------------------
int vtkPVLabeledToggle::GetState() 
{ 
  return this->CheckButton->GetState(); 
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVIndexWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    char *cmd = new char[strlen(this->VariableName)+4];
    sprintf(cmd, "Set%s", this->VariableName);
    this->Property->SetVTKCommand(cmd);
    this->Property->SetIndex(this->DefaultValue);
    delete [] cmd;
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVLabeledToggle::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVLabeledToggle::CreateAppropriateProperty()
{
  return vtkPVIndexWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->CheckButton);
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
