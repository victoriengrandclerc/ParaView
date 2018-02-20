/* Copyright 2018 NVIDIA Corporation. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*  * Neither the name of NVIDIA CORPORATION nor the names of its
*    contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef vtknvindex_irregular_volume_representation_h
#define vtknvindex_irregular_volume_representation_h

#include <mi/math/bbox.h>

#include "vtkPVConfig.h"
#include "vtkPVDataRepresentation.h"

//#if PARAVIEW_VERSION_MAJOR == 5 && PARAVIEW_VERSION_MINOR >= 2
#define PARAVIEW_UGRID_USE_PARTITIONS
//#endif

class vtkColorTransferFunction;
class vtkMultiProcessController;
class vtkOrderedCompositeDistributor;
class vtkPiecewiseFunction;
class vtkPolyDataMapper;
class vtkPVCacheKeeper;
class vtkPVGeometryFilter;
class vtkPVLODVolume;
class vtkPVUpdateSuppressor;
class vtkResampleToImage;
class vtkUnstructuredDataDeliveryFilter;
class vtkUnstructuredGridVolumeMapper;
class vtkVolumeProperty;
class vtkVolumeRepresentationPreprocessor;

class vtknvindex_affinity;
class vtknvindex_config_settings;
class vtknvindex_cluster_properties;
class vtknvindex_irregular_volume_mapper;

// The class vtknvindex_representation represents the base class for distributing and rendering
// of irregular volume data (unstructured volume grids) in NVIDIA IndeX.

class VTK_EXPORT vtknvindex_irregular_volume_representation : public vtkPVDataRepresentation
{
public:
  static vtknvindex_irregular_volume_representation* New();
  vtkTypeMacro(vtknvindex_irregular_volume_representation, vtkPVDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Register a volume mapper with the representation.
  void AddVolumeMapper(const char* name, vtkUnstructuredGridVolumeMapper*);

  // Set the active volume mapper to use.
  virtual void SetActiveVolumeMapper(const char*);
  vtkUnstructuredGridVolumeMapper* GetActiveVolumeMapper();

  // The method vtkAlgorithm::ProcessRequest() is equivalent for all rendering passes.
  // The method is typically called by the vtkView to request meta-data from the
  // representations or ask them to perform certain tasks e.g.
  // PrepareForRendering.
  virtual int ProcessViewRequest(
    vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo);

  // This needs to be called on all instances of vtkGeometryRepresentation when
  // the input is modified. This is essential since the geometry filter does not
  // have any real-input on the client side which messes with the Update
  // requests.
  virtual void MarkModified();

  // Get/Set the visibility for this representation. When the visibility of
  // representation of false, all view passes are ignored.
  // Overridden to propagate to the active representation.
  virtual void SetVisibility(bool val);

  //***************************************************************************
  // Forwarded to vtkVolumeRepresentationPreprocessor
  void SetExtractedBlockIndex(unsigned int index);

  //***************************************************************************
  // Forwarded to vtkResampleToImage
  void SetSamplingDimensions(int dims[3])
  {
    this->SetSamplingDimensions(dims[0], dims[1], dims[2]);
  }

  void SetSamplingDimensions(int xdim, int ydim, int zdim);
  //***************************************************************************
  // Forwarded to Actor.
  void SetOrientation(double, double, double);
  void SetOrigin(double, double, double);
  void SetPickable(int val);
  void SetPosition(double, double, double);
  void SetScale(double, double, double);

  //***************************************************************************
  // Forwarded to vtkVolumeProperty and vtkProperty (when applicable).
  void SetInterpolationType(int val);
  void SetColor(vtkColorTransferFunction* lut);
  void SetScalarOpacity(vtkPiecewiseFunction* pwf);
  void SetScalarOpacityUnitDistance(double val);

  // Provides access to the actor used by this representation.
  vtkPVLODVolume* GetActor() { return this->Actor; }

#ifdef PARAVIEW_UGRID_USE_PARTITIONS
  //@{
  /**
   * Specify whether or not to redistribute the data. The default is false
   * since that is the only way in general to guarantee correct rendering.
   * Can set to true if all rendered data sets are based on the same
   * data partitioning in order to save on the data redistribution.
   */
  vtkSetMacro(UseDataPartitions, bool);
  vtkGetMacro(UseDataPartitions, bool);
//@}

#endif // PARAVIEW_UGRID_USE_PARTITIONS

  //
  // Configuration options set from ParaView GUI.
  // These are stored in vtknvindex_config_settings class.
  //

  // Set subcube size.
  void set_subcube_size(unsigned x, unsigned y, unsigned z);

  // Set subcube border size.
  void set_subcube_border(int border);

  // Set filtering mode.
  void set_filter_mode(int filter_mode);

  // Set pre-integration mode.
  void set_preintegration(bool enable_preint);

  // Set dump internal state of NVIDIA IndeX.
  void set_dump_internal_state(bool is_dump);

  // Set flag to log performance values.
  void set_log_performance(bool is_log);

  // Set region of interest.
  void set_roi_minI(double val);
  void set_roi_maxI(double val);
  vtkGetVector2Macro(m_roi_range_I, double);

  void set_roi_minJ(double val);
  void set_roi_maxJ(double val);
  vtkGetVector2Macro(m_roi_range_J, double);

  void set_roi_minK(double val);
  void set_roi_maxK(double val);
  vtkGetVector2Macro(m_roi_range_K, double);

  // BTX
protected:
  vtknvindex_irregular_volume_representation();
  ~vtknvindex_irregular_volume_representation();

  // Fill input port information.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  virtual int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  // Adds the representation to the view. This is called from
  // vtkView::AddRepresentation(). The sub classes should override this method.
  // Returns true if the addition succeeds.
  virtual bool AddToView(vtkView* view);

  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().  Subclasses should override this method.
  // Returns true if the removal succeeds.
  virtual bool RemoveFromView(vtkView* view);

  // Overridden to check with the vtkPVCacheKeeper to see if the key is cached.
  virtual bool IsCached(double cache_key);

  // Passes on parameters to the active volume mapper.
  virtual void UpdateMapperParameters();

  vtkVolumeRepresentationPreprocessor* Preprocessor;
  vtkPVCacheKeeper* CacheKeeper;

  vtknvindex_irregular_volume_mapper* DefaultMapper;

  vtkVolumeProperty* Property;
  vtkPVLODVolume* Actor;

  double DataBounds[6];

#ifdef PARAVIEW_UGRID_USE_PARTITIONS
  bool UseDataPartitions;
#endif // PARAVIEW_UGRID_USE_PARTITIONS

private:
  vtknvindex_irregular_volume_representation(
    const vtknvindex_irregular_volume_representation&) = delete;
  void operator=(const vtknvindex_irregular_volume_representation&) = delete;

  class vtkInternals;
  vtkInternals* Internals;

  // Region of interest in the GUI.
  double m_roi_range_I[2];
  double m_roi_range_J[2];
  double m_roi_range_K[2];

  vtkResampleToImage* ResampleToImageFilter;
  vtkMultiProcessController* m_controller;           // MPI controller from ParaView.
  vtknvindex_config_settings* m_app_config_settings; // Application side config settings.
  vtknvindex_cluster_properties*
    m_cluster_properties; // Cluster wide properties, refer class documentation.
  mi::math::Bbox_struct<mi::Float32, 3> m_roi_gui; // Region of interest set in the GUI.

  // ETX

  mi::Float32 m_prev_time_step;
};

#endif // vtknvindex_irregular_volume_representation_h
