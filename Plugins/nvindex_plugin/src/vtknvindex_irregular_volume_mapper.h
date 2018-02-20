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

#ifndef vtknvindex_irregular_volume_mapper_h
#define vtknvindex_irregular_volume_mapper_h

#include <cstdio>
#include <string>

#include <nv/index/icamera.h>
#include <nv/index/icolormap.h>
#include <nv/index/iconfig_settings.h>
#include <nv/index/iscene.h>
#include <nv/index/isession.h>
#include <nv/index/iviewport.h>

#include "vtkCamera.h"
#include "vtkMultiProcessController.h"
#include "vtkUnstructuredGridVolumeMapper.h"

#include "vtknvindex_application.h"
#include "vtknvindex_cluster_properties.h"
#include "vtknvindex_colormap_utility.h"
#include "vtknvindex_performance_values.h"
#include "vtknvindex_scene.h"

class vtknvindex_cluster_properties;
class vtkPKdTree;

// The class vtknvindex_irregular_volume_mapper maps ParaView's data to NVIDIA IndeX's data
// representations.
// All NVIDIA IndeX data preparation, scene creation,
// update and rendering for irregular volumes is triggered by an instance of this class.

class VTK_EXPORT vtknvindex_irregular_volume_mapper : public vtkUnstructuredGridVolumeMapper
{
public:
  static vtknvindex_irregular_volume_mapper* New();
  vtkTypeMacro(vtknvindex_irregular_volume_mapper, vtkUnstructuredGridVolumeMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtknvindex_irregular_volume_mapper();
  ~vtknvindex_irregular_volume_mapper();

  // Shutdown forward loggers, IndeX library and unload libraries.
  void shutdown();

  // Overriding from vtkUnstructuredGridVolumeMapper.
  virtual void Render(vtkRenderer* ren, vtkVolume* vol);

  // Load and setup NVIDIA IndeX library.
  bool initialize_nvindex();

  // Prepare data for the importer.
  bool prepare_data();

  // Volume changed, needs to be reload.
  void volume_changed();

  // Host id from NVIDIA IndeX running on this machine.
  mi::Sint32 get_local_hostid();

  // Cluster properties from Representation.
  void set_cluster_properties(vtknvindex_cluster_properties* cluster_properties);

  // Set ParaView domain subdivision KD-Tree.
  void set_domain_kdtree(vtkPKdTree* kd_tree);

  // Returns true if NVIDIA IndeX is initialized by this mapper.
  bool is_mapper_initialized() { return m_is_mapper_initialized; }

  // Update render canvas.
  void update_canvas(vtkRenderer* ren);

  // The configuration settings needs to be updated on changes applied to the GUI.
  void config_settings_changed();

  // Initialize the mapper.
  bool initialize_mapper(vtkRenderer* ren, vtkVolume* vol);

  // BTX

private:
  vtknvindex_irregular_volume_mapper(const vtknvindex_irregular_volume_mapper&) = delete;
  void operator=(const vtknvindex_irregular_volume_mapper&) = delete;

  bool m_is_mapper_initialized;   // True if mapper is initialized.
  bool m_is_index_initialized;    // True if index library is initialized.
  bool m_is_viewer;               // True if this is viewer node.
  bool m_is_nvindex_rank;         // True if this rank is running NVIDIA IndeX.
  bool m_is_data_prepared;        // True if all the data is ready for the importer.
  bool m_config_settings_changed; // When some parameter changed on the GUI.
  bool m_volume_changed;          // When switching to a different time step.
                                  // or switching between properties.
  std::string m_prev_property;    // volume property that was rendered.

  vtknvindex_application m_application_context;        // NVIDIA IndeX application context.
  vtknvindex_scene m_scene;                            // NVIDIA IndeX scene.
  vtknvindex_cluster_properties* m_cluster_properties; // Cluster properties gathered from ParaView.
  vtknvindex_performance_values m_performance_values;  // Performance values logger.
  vtkMultiProcessController* m_controller;             // MPI controller from ParaView.
  vtkDataArray* m_scalar_array;                        // Scalar array containing actual data.
  vtknvindex_irregular_volume_data m_volume_data;      // Tetrahedral volume data.

  vtkPKdTree* m_kd_tree; // ParaView domain subdivision.
  // ETX
};

#endif
