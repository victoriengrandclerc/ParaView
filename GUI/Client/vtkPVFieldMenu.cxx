/*=========================================================================

  Program:   ParaView
  Module:    vtkPVFieldMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVFieldMenu.h"

#include "vtkArrayMap.txx"
#include "vtkDataSet.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVIndexWidgetProperty.h"
#include "vtkPVInputMenu.h"
#include "vtkPVInputProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSource.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVFieldMenu);
vtkCxxRevisionMacro(vtkPVFieldMenu, "1.13");


vtkCxxSetObjectMacro(vtkPVFieldMenu, InputMenu, vtkPVInputMenu);


int vtkPVFieldMenuCommand(ClientData cd, Tcl_Interp *interp,
                          int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVFieldMenu::vtkPVFieldMenu()
{
  this->CommandFunction = vtkPVFieldMenuCommand;
  
  this->InputMenu = NULL;
  this->Label = vtkKWLabel::New();
  this->FieldMenu = vtkKWOptionMenu::New();
  this->Value = vtkDataSet::POINT_DATA_FIELD;  
  
  this->Property = NULL;
  this->PropertyInitialized = 0;
}

//----------------------------------------------------------------------------
vtkPVFieldMenu::~vtkPVFieldMenu()
{
  this->Label->Delete();
  this->Label = NULL;
  this->FieldMenu->Delete();
  this->FieldMenu = NULL;

  this->SetInputMenu(NULL);
  
  this->SetProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
  if (this->Value == vtkDataSet::POINT_DATA_FIELD)
    {
    os << indent << "Value: Point Data. \n";
    }
  if (this->Value == vtkDataSet::CELL_DATA_FIELD)
    {
    os << indent << "Value: Cell Data. \n";
    }
}

//----------------------------------------------------------------------------
vtkPVInputProperty* vtkPVFieldMenu::GetInputProperty()
{
  if (this->PVSource == NULL)
    {
    return NULL;
    }

  // Should we get the input name from the input menu?
  return this->PVSource->GetInputProperty("Input");
}


//----------------------------------------------------------------------------
void vtkPVFieldMenu::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());


  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Label->SetLabel("Attribute Mode");
  this->Label->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->FieldMenu->SetParent(this);
  this->FieldMenu->Create(app, "");
  this->FieldMenu->SetBalloonHelpString(
    "Select whether to operate on point or cell data");
  this->Script("pack %s -side left", this->FieldMenu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::SetValue(int field)
{
  if (field == this->Value)
    {
    return;
    }

  if (field == vtkDataSet::POINT_DATA_FIELD)
    {
    this->FieldMenu->SetValue("Point Data");
    }
  else if (field == vtkDataSet::CELL_DATA_FIELD)
    {
    this->FieldMenu->SetValue("Cell Data");
    }
  
  this->Value = field;
  this->ModifiedCallback();
  this->Update();
}

//----------------------------------------------------------------------------
vtkPVDataSetAttributesInformation* vtkPVFieldMenu::GetFieldInformation()
{
  vtkPVSource *input;

  if (this->InputMenu == NULL)
    {
    return NULL;
    }
  input = this->InputMenu->GetCurrentValue();
  if (input == NULL)
    {
    return NULL;
    }

  switch (this->Value)
    {
    case vtkDataSet::DATA_OBJECT_FIELD:
      vtkErrorMacro("We do not handle data object fields yet.");
      return NULL;
    case vtkDataSet::POINT_DATA_FIELD:
      return input->GetDataInformation()->GetPointDataInformation();
      break;
    case vtkDataSet::CELL_DATA_FIELD:
      return input->GetDataInformation()->GetCellDataInformation();
      break;
    }

  vtkErrorMacro("Unknown field.");
  return NULL; 
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::AcceptInternal(vtkClientServerID sourceID)
{
  if (sourceID.ID &&
      (this->Value == vtkDataSet::POINT_DATA_FIELD ||
       this->Value == vtkDataSet::CELL_DATA_FIELD))
    {
    this->Property->SetVTKSourceID(sourceID);
    this->Property->SetIndex(this->Value);
    this->Property->AcceptInternal();
    }
  
  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVFieldMenu::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue "
        << this->Value << endl;
}


//----------------------------------------------------------------------------
void vtkPVFieldMenu::ResetInternal()
{
  // Get the selected array form the VTK filter.
//  this->Script("%s SetValue [%s GetAttributeMode]",
//               this->GetTclName(), 
//               sourceTclName);
  this->SetValue(this->Property->GetIndex());

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
  // Do we really need to update?
  // What causes dependent widgets like ArrayMenu to update?
  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);

  if (sourceID.ID == 0)
    {
    vtkErrorMacro("Sanity check failed. ");
    return;
    }

  *file << "  [$pvTemp" << sourceID 
        << " GetProperty AttributeMode] SetElements1 " << this->Value << endl;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::Update()
{
  int cellFlag, pointFlag;
  vtkPVInputProperty* inProp = this->GetInputProperty();

  this->FieldMenu->ClearEntries();
  if (this->InputMenu == NULL || inProp == NULL )
    {
    // Add both.
    this->FieldMenu->AddEntryWithCommand("Point Data", this,
                                         "SetValue 1");
    this->FieldMenu->AddEntryWithCommand("Cell Data", this,
                                         "SetValue 2");
    this->FieldMenu->SetCurrentEntry("Point Data");
    this->vtkPVWidget::Update();
    return;
    }  

  vtkPVSource* pvs = this->InputMenu->GetCurrentValue();
  if (pvs == NULL)
    {
    return;
    }
  vtkPVDataInformation* dataInfo = pvs->GetDataInformation();
  if (dataInfo == NULL)
    {
    return;
    }
  
  pointFlag = cellFlag = 0;
  if (inProp->GetIsValidField(vtkDataSet::POINT_DATA_FIELD,
                              dataInfo->GetPointDataInformation()))
    {
    this->FieldMenu->AddEntryWithCommand("Point Data", this,
                                         "SetValue 1");
    pointFlag = 1;
    }

  if (inProp->GetIsValidField(vtkDataSet::CELL_DATA_FIELD,
                              dataInfo->GetCellDataInformation()))
    {
    this->FieldMenu->AddEntryWithCommand("Cell Data", this,
                                         "SetValue 2");
    cellFlag = 1;
    }
  if (! cellFlag && ! pointFlag)
    {
    vtkErrorMacro("No valid fields.");
    this->FieldMenu->SetCurrentEntry("None");
    }
  if (! cellFlag)
    {
    this->FieldMenu->SetCurrentEntry("Point Data");
    this->Value = vtkDataSet::POINT_DATA_FIELD;
    }
  else if (! pointFlag)
    {
    this->FieldMenu->SetCurrentEntry("Cell Data");
    this->Value = vtkDataSet::CELL_DATA_FIELD;
    }
  else if (this->Value == vtkDataSet::POINT_DATA_FIELD)
    {
    this->FieldMenu->SetCurrentEntry("Point Data");
    }
  else
    {
    this->FieldMenu->SetCurrentEntry("Cell Data");
    }

  if (!this->PropertyInitialized)
    {
    this->Property->SetIndex(this->Value);
    this->PropertyInitialized = 1;
    }
  
  // This updates any array menu dependent on this widget.
  this->vtkPVWidget::Update();
}


//----------------------------------------------------------------------------
vtkPVFieldMenu* vtkPVFieldMenu::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVFieldMenu::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
// It looks like I could leave this for the superclass.!!!!!!!!!!!!!!!
vtkPVWidget* vtkPVFieldMenu::ClonePrototypeInternal(vtkPVSource* pvSource,
                                vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* pvWidget = 0;
  // Check if a clone of this widget has already been created
  if ( map->GetItem(this, pvWidget) != VTK_OK )
    {
    // If not, create one and add it to the map
    pvWidget = this->NewInstance();
    map->SetItem(this, pvWidget);
    // Now copy all the properties
    this->CopyProperties(pvWidget, pvSource, map);

    vtkPVFieldMenu* pvfm = vtkPVFieldMenu::SafeDownCast(pvWidget);
    if (!pvfm)
      {
      vtkErrorMacro("Internal error. Could not downcast pointer.");
      pvWidget->Delete();
      return 0;
      }
    }
  else
    {
    // Increment the reference count. This is necessary
    // to make the behavior same whether a widget is created
    // or returned from the map. Always call Delete() after
    // cloning.
    pvWidget->Register(this);
    }

  return pvWidget;
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  
  vtkPVFieldMenu* pvamm = vtkPVFieldMenu::SafeDownCast(clone);
  if (pvamm)
    {
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvamm->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to vtkPVAttributeMenu.");
    }

}

//----------------------------------------------------------------------------
int vtkPVFieldMenu::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
    
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if(!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }
  
  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  if (!ime)
    {
    vtkErrorMacro("Couldn't find InputMenu element " << input_menu);
    return 0;
    }
  
  vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();
    
  return 1;
}

void vtkPVFieldMenu::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVIndexWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    this->Property->SetVTKCommand("SetAttributeMode");
    }
}

vtkPVWidgetProperty* vtkPVFieldMenu::GetProperty()
{
  return this->Property;
}

vtkPVWidgetProperty* vtkPVFieldMenu::CreateAppropriateProperty()
{
  return vtkPVIndexWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVFieldMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->FieldMenu);
  this->PropagateEnableState(this->InputMenu);
}
