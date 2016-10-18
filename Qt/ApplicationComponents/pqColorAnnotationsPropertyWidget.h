/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#ifndef pqColorAnnotationsPropertyWidget_h
#define pqColorAnnotationsPropertyWidget_h

#include "pqApplicationComponentsModule.h"
#include "pqPropertyWidget.h"

class QModelIndex;
class vtkSMPropertyGroup;

/**
* pqColorAnnotationsPropertyWidget is used to edit the Annotations property on the
* "PVLookupTable" proxy. The property group can comprise of two properties,
* \c Annotations and \c IndexedColors.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqColorAnnotationsPropertyWidget :
  public pqPropertyWidget
{
  Q_OBJECT;
  Q_PROPERTY(QList<QVariant> annotations READ annotations WRITE setAnnotations);
  Q_PROPERTY(QList<QVariant> indexedColors READ indexedColors WRITE setIndexedColors);

  typedef pqPropertyWidget Superclass;
public:
  pqColorAnnotationsPropertyWidget(
    vtkSMProxy* proxy, vtkSMPropertyGroup* smgroup, QWidget* parent=0);
  virtual ~pqColorAnnotationsPropertyWidget();

  /**
  * Get/Set the annotations. Annotations are returns as a QList<QVariant>.
  * This is a list generated by flattening 2-tuples where 1st value is the
  * annotated value and second is the annotation text.
  */
  QList<QVariant> annotations() const;
  void setAnnotations(const QList<QVariant>&);

  /**
  * Get/Set the indexed colors. This is a list generated by flattening
  * 3-tuples (r,g,b).
  */
  QList<QVariant> indexedColors() const;
  void setIndexedColors(const QList<QVariant>&);

signals:
  /**
  * Fired when the annotations are changed.
  */
  void annotationsChanged();

  /**
  * Fired when the indexed colors are changed.
  */
  void indexedColorsChanged();

private slots:
  /**
  * slots called when user presses corresponding buttons to add/remove
  * annotations.
  */
  void addAnnotation();
  void removeAnnotation();
  void addActiveAnnotations();
  void addActiveAnnotationsFromVisibleSources();
  void removeAllAnnotations();

  /**
  * called whenever the internal model's data changes. We fire
  * annotationsChanged() or indexedColorsChanged() signals appropriately.
  */
  void onDataChanged(const QModelIndex& topleft, const QModelIndex& btmright);

  /**
  * called when user double-clicks on an item. If the double click is on the
  * 0-th column, we show the color editor to allow editing of the indexed
  * color.
  */
  void onDoubleClicked(const QModelIndex& idx);

  /**
  * pick a preset.
  */
  void choosePreset(const char* presetName=NULL);

  /**
  * save current transfer function as preset.
  */
  void saveAsPreset();

  /**
  * Ensures that the color-swatches for indexedColors are shown only when this
  * is set to true.
  */
  void updateIndexedLookupState();

  /**
  * called when the user edits past the last row.
  */
  void editPastLastRow();
private:
  Q_DISABLE_COPY(pqColorAnnotationsPropertyWidget)

  class pqInternals;
  pqInternals* Internals;
};

#endif
