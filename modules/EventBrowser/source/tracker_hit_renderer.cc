/* tracker_hit_renderer.cc
 *
 * Copyright (C) 2011 Xavier Garrido <garrido@lal.in2p3.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include <falaise/snemo/view/tracker_hit_renderer.h>
#include <falaise/snemo/view/browser_tracks.h>
#include <falaise/snemo/view/options_manager.h>
#include <falaise/snemo/view/style_manager.h>

#include <falaise/snemo/detector/detector_manager.h>
#include <falaise/snemo/detector/i_root_volume.h>

#include <falaise/snemo/io/event_server.h>

#include <falaise/snemo/datamodels/helix_trajectory_pattern.h>
#include <falaise/snemo/datamodels/line_trajectory_pattern.h>

#include <geomtools/id_mgr.h>

#include <TObjArray.h>
#include <TPolyLine3D.h>
#include <TPolyMarker3D.h>
#include <TMarker3DBox.h>
#include <TColor.h>
#include <TRotation.h>
#include <TMath.h>

namespace snemo {

  namespace visualization {

    namespace view {

      // ctor:
      tracker_hit_renderer::tracker_hit_renderer() :
        base_renderer()
      {
        return;
      }

      // dtor:
      tracker_hit_renderer::~tracker_hit_renderer()
      {
        return;
      }

      void tracker_hit_renderer::push_simulated_hits(const std::string & hit_category_)
      {
        const io::event_record & event = _server->get_event();
        const mctools::simulated_data & sim_data = event.get<mctools::simulated_data>(io::SD_LABEL);

        if (!sim_data.has_step_hits(hit_category_)) {
          DT_LOG_INFORMATION(options_manager::get_instance().get_logging_priority(),
                             "Event has no '" << hit_category_ << "' tracker hits");
          return;
        }

        const mctools::simulated_data::hit_handle_collection_type & hit_collection
          = sim_data.get_step_hits(hit_category_);

        if (hit_collection.empty()) {
          DT_LOG_INFORMATION(options_manager::get_instance().get_logging_priority(),
                             "No tracker hits");
          return;
        }

        // time gradient color
        double hit_start_time = hit_collection.front().get().get_time_start();
        double hit_stop_time  = hit_collection.front().get().get_time_start();

        const bool geiger_with_gradient
          = options_manager::get_instance().get_option_flag(SHOW_GG_TIME_GRADIENT);
        if (geiger_with_gradient) {
          for (mctools::simulated_data::hit_handle_collection_type::const_iterator
                 it_hit = hit_collection.begin();
               it_hit != hit_collection.end(); ++it_hit) {
            hit_start_time = std::min(it_hit->get().get_time_start(), hit_start_time);
            hit_stop_time  = std::max(it_hit->get().get_time_start(), hit_stop_time);
          }
        }

        for (mctools::simulated_data::hit_handle_collection_type::const_iterator
               it_hit = hit_collection.begin();
             it_hit != hit_collection.end(); ++it_hit) {
          const mctools::base_step_hit & a_step = it_hit->get();

          // draw the Geiger avalanche path:
          TPolyLine3D * gg_path = new TPolyLine3D;
          _objects->Add(gg_path);
          gg_path->SetPoint(0,
                            a_step.get_position_start().x(),
                            a_step.get_position_start().y(),
                            a_step.get_position_start().z());
          gg_path->SetPoint(1,
                            a_step.get_position_stop().x(),
                            a_step.get_position_stop().y(),
                            a_step.get_position_stop().z());

          const unsigned int time_percent
            = (unsigned int)((TColor::GetNumberOfColors() - 1)
                             * (a_step.get_time_start() - hit_start_time)
                             / (hit_stop_time - hit_start_time));
          const unsigned int color = geiger_with_gradient ?
            TColor::GetColorPalette(time_percent) : kSpring;
          gg_path->SetLineColor(color);

          // Store this value into cluster properties:
          mctools::base_step_hit * mutable_hit = const_cast<mctools::base_step_hit*>(&(a_step));
          datatools::properties & hit_properties = mutable_hit->grab_auxiliaries();
          const long pixel = TColor::Number2Pixel(color);
          const std::string hex_str = TColor::PixelAsHexString(pixel);
          hit_properties.update(browser_tracks::COLOR_FLAG, hex_str);

          // Retrieve line width from properties if 'hit' is highlighted:
          size_t line_width = style_manager::get_instance().get_mc_line_width();
          if (hit_properties.has_flag(browser_tracks::HIGHLIGHT_FLAG)) line_width = 3;
          //hit_properties.update(browser_tracks::HIGHLIGHT_FLAG, false);
          gg_path->SetLineWidth(line_width);

          // draw circle tangential to the track:
          if (options_manager::get_instance().get_option_flag(SHOW_GG_CIRCLE)) {
            const size_t n_point = 100;
            TRotation dr;
            int cell_axis = 'z';

            const detector::detector_manager & detector_mgr
              = detector::detector_manager::get_instance();
            const std::string & setup_label = detector_mgr.get_setup_label_name();

            if (setup_label == "snemo::tracker_commissioning") {
              cell_axis = 'x';
            }

            if (cell_axis == 'z') {
              dr.RotateZ(2*TMath::Pi()/(double)n_point);
            } else if (cell_axis == 'x') {
              dr.RotateX(2*TMath::Pi()/(double)n_point);
            } else {
              DT_THROW_IF(true, std::logic_error, "Unsupported cell axis !");
            }

            std::vector<geomtools::vector_3d> points;
            points.reserve(n_point);
            if (cell_axis == 'z') {
              TVector3 current_pos((a_step.get_position_start() - a_step.get_position_stop()).x(),
                                   (a_step.get_position_start() - a_step.get_position_stop()).y(),
                                   a_step.get_position_stop().z());
              for (size_t i_point = 0; i_point <= n_point; ++i_point) {
                current_pos *= dr;
                points.push_back(geomtools::vector_3d(current_pos.x() + a_step.get_position_stop().x(),
                                                      current_pos.y() + a_step.get_position_stop().y(),
                                                      current_pos.z()));
              }
            } else if (cell_axis == 'x') {
              TVector3 current_pos(a_step.get_position_stop().x(),
                                   (a_step.get_position_start() - a_step.get_position_stop()).y(),
                                   (a_step.get_position_start() - a_step.get_position_stop()).z());

              for (size_t i_point = 0; i_point <= n_point; ++i_point) {
                current_pos *= dr;
                points.push_back(geomtools::vector_3d(current_pos.x(),
                                                      current_pos.y() + a_step.get_position_stop().y(),
                                                      current_pos.z() + a_step.get_position_stop().z()));
              }
            }

            TPolyLine3D * gg_drift = base_renderer::make_polyline(points);
            _objects->Add(gg_drift);
            gg_drift->SetLineColor(color);
            gg_drift->SetLineWidth(line_width);
          }// end of "show geiger drift circle" condition

          // if(options_manager::get_instance().show_geiger_box())
          //   {
          //     const geomtools::geom_id & drift_cell_gid = a_step.get_geom_id();
          //     detector::detector_manager & detector_mgr
          //       = detector::detector_manager::get_instance();
          //     detector::i_root_volume * volume_hit =
          //       dynamic_cast<detector::i_root_volume *>(detector_mgr.get_volume(drift_cell_gid));

          //     if (!volume_hit)
          //       {
          //         if (verbose)
          //           clog << datatools::utils::io::notice
          //                << "snemo::visualisation::view::"
          //                << "tracker_hit_renderer::push_simulated_hits: "
          //                << "Geiger detector with hit not found ! "
          //                << "check either sngeometry file or style file "
          //                << "in case this detector part is disable" << endl;
          //         continue;
          //       }

          //     if (verbose)
          //       clog << datatools::utils::io::notice
          //            << "snemo::visualisation::view::"
          //            << "tracker_hit_renderer::push_simulated_hits: '"
          //            << drift_cell_gid << "' has geiger hit" << endl;

          //     volume_hit->highlight(/*kOrange*/);
          //     _highlighted_id_.insert(drift_cell_gid);

          //   } // end of "show geiger box" condition

        } // end of step collection

        return;
      }

      void tracker_hit_renderer::push_calibrated_hits()
      {
        const io::event_record & event = _server->get_event();
        const snemo::datamodel::calibrated_data & calib_data
          = event.get<snemo::datamodel::calibrated_data>(io::CD_LABEL);

        if (!calib_data.has_calibrated_tracker_hits()) {
          DT_LOG_INFORMATION(options_manager::get_instance().get_logging_priority(),
                             "Event has no calibrated tracker hits");
          return;
        }

        const snemo::datamodel::calibrated_data::tracker_hit_collection_type & ct_collection
          = calib_data.calibrated_tracker_hits();

        if (ct_collection.empty()) {
          DT_LOG_INFORMATION(options_manager::get_instance().get_logging_priority(),
                             "No calibrated tracker hits");
          return;
        }

        for (snemo::datamodel::calibrated_data::tracker_hit_collection_type::const_iterator
               it_hit = ct_collection.begin();
             it_hit != ct_collection.end(); ++it_hit) {
          const snemo::datamodel::calibrated_tracker_hit & a_hit = it_hit->get();
          make_calibrated_geiger_hit(a_hit, _objects, false);
        }
        return;
      }

      void tracker_hit_renderer::push_clustered_hits()
      {
        const io::event_record & event = _server->get_event();
        const snemo::datamodel::tracker_clustering_data & tracker_clustered_data
          = event.get<snemo::datamodel::tracker_clustering_data>(io::TCD_LABEL);

        const snemo::datamodel::tracker_clustering_data::solution_col_type & cluster_solutions
          = tracker_clustered_data.get_solutions();
        for (snemo::datamodel::tracker_clustering_data::solution_col_type::const_iterator
               isolution = cluster_solutions.begin();
             isolution != cluster_solutions.end(); ++isolution) {
          // Get current tracker solution:
          const snemo::datamodel::tracker_clustering_solution & a_solution = isolution->get();

          // Check solution properties:
          if (a_solution.get_auxiliaries().has_key(browser_tracks::CHECKED_FLAG) &&
              !a_solution.get_auxiliaries().has_flag(browser_tracks::CHECKED_FLAG)) continue;

          // Get clusters stored in the current tracker solution:
          const snemo::datamodel::tracker_clustering_solution::cluster_col_type & clusters
            = a_solution.get_clusters();

          for (snemo::datamodel::tracker_clustering_solution::cluster_col_type::const_iterator
                 icluster = clusters.begin();
               icluster != clusters.end(); ++icluster) {
            // Get current tracker cluster:
            const snemo::datamodel::tracker_cluster & a_cluster = icluster->get();

            // Check cluster properties:
            if (a_cluster.get_auxiliaries().has_key(browser_tracks::CHECKED_FLAG) &&
                !a_cluster.get_auxiliaries().has_flag(browser_tracks::CHECKED_FLAG)) continue;

            // Determine cluster color
            const size_t cluster_color
              = style_manager::get_instance().get_color(std::distance(clusters.begin(), icluster));

            // Store this value into cluster properties:
            snemo::datamodel::tracker_cluster * mutable_cluster = const_cast<snemo::datamodel::tracker_cluster*>(&(a_cluster));
            datatools::properties & cluster_properties = mutable_cluster->grab_auxiliaries();
            const long pixel = TColor::Number2Pixel(cluster_color);
            const std::string hex_str = TColor::PixelAsHexString(pixel);
            cluster_properties.update(browser_tracks::COLOR_FLAG, hex_str);

            // Get tracker hits stored in the current tracker cluster:
            const snemo::datamodel::calibrated_tracker_hit::collection_type & hits = a_cluster.get_hits();

            // Make a gradient color starting from color_solution:
            for (snemo::datamodel::calibrated_tracker_hit::collection_type::const_iterator
                   igg = hits.begin();
                 igg != hits.end(); ++igg) {
              const snemo::datamodel::calibrated_tracker_hit & a_gg_hit = igg->get();

              // Retrieve a mutable reference to calibrated_tracker_hit:
              snemo::datamodel::calibrated_tracker_hit * mutable_hit = const_cast<snemo::datamodel::calibrated_tracker_hit*>(&(a_gg_hit));
              datatools::properties & gg_properties = mutable_hit->grab_auxiliaries();

              // Store current color to be used by calibrated_tracker_hit renderer:
              const long pixel = TColor::Number2Pixel(cluster_color);
              const std::string hex_str = TColor::PixelAsHexString(pixel);
              gg_properties.update(browser_tracks::COLOR_FLAG, hex_str);

              const options_manager & options_mgr = options_manager::get_instance();
              if (options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_BOX)) {
                const double x  = a_gg_hit.get_x();
                const double y  = a_gg_hit.get_y();
                const double z  = a_gg_hit.get_z();
                const double dz = a_gg_hit.get_sigma_z();
                const double r = 22.0 / CLHEP::mm;

                TMarker3DBox * hit_3d = new TMarker3DBox;
                _objects->Add(hit_3d);
                hit_3d->SetPosition(x, y, z);
                hit_3d->SetSize(r, r, dz);
                hit_3d->SetLineColor(cluster_color);
                // Retrieve line width from properties if 'hit' is highlighted:
                size_t line_width = 1;
                if (gg_properties.has_flag(browser_tracks::HIGHLIGHT_FLAG)) line_width = 3;
                //gg_properties.update(browser_tracks::HIGHLIGHT_FLAG, false);
                hit_3d->SetLineWidth(line_width);
              } else if (options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_CIRCLE)) {
                make_calibrated_geiger_hit(a_gg_hit, _objects, true);
              }
              // // Make gradient color
              // const float H = 300 * (1 - distance(cluster_solutions.begin(), isolution)
              //                        / (double)cluster_solutions.size());
              // const float S = 1.0 * (1 - distance(clusters.begin(), icluster)
              //                        / (double)clusters.size());
              // const float V = 0.5;
              // float R, G, B;
              // TColor::HSV2RGB (H, S, V, R, G, B);
              // clog << "(H,S,V)= " << H << "," << S << "," << V << endl;
              // clog << "(R,G,B)= " << R << "," << G << "," << B << endl;
              // unsigned int cluster_color = TColor::GetColor (R, G, B);

            } // end of gg hits
          } // end of cluster loop
        } // end of solution loop

        return;
      }

      void tracker_hit_renderer::push_fitted_tracks()
      {
        const io::event_record & event = _server->get_event();
        const snemo::datamodel::tracker_trajectory_data & tracker_trajectory_data
          = event.get<snemo::datamodel::tracker_trajectory_data>(io::TTD_LABEL);

        const snemo::datamodel::tracker_trajectory_data::solution_col_type & trajectory_solutions
          = tracker_trajectory_data.get_solutions();
        for (snemo::datamodel::tracker_trajectory_data::solution_col_type::const_iterator
               isolution = trajectory_solutions.begin();
             isolution != trajectory_solutions.end(); ++isolution) {
          // Get current tracker trajectory solution:
          const snemo::datamodel::tracker_trajectory_solution & a_solution = isolution->get();

          // Check solution properties:
          if (a_solution.get_auxiliaries().has_key(browser_tracks::CHECKED_FLAG) &&
              !a_solution.get_auxiliaries().has_flag(browser_tracks::CHECKED_FLAG)) continue;

          // Get trajectories stored in the current tracker trajectory solution:
          const snemo::datamodel::tracker_trajectory_solution::trajectory_col_type & trajectories
            = a_solution.get_trajectories();

          for (snemo::datamodel::tracker_trajectory_solution::trajectory_col_type::const_iterator
                 itrajectory = trajectories.begin();
               itrajectory != trajectories.end(); ++itrajectory) {
            // Get current tracker trajectory:
            const snemo::datamodel::tracker_trajectory & a_trajectory = itrajectory->get();

            // Get trajectory properties:
            const datatools::properties & traj_properties = a_trajectory.get_auxiliaries();

            // Check if trajectory has to be shown or not:
            if (traj_properties.has_key(browser_tracks::CHECKED_FLAG)) {
              if (!traj_properties.has_flag(browser_tracks::CHECKED_FLAG)) continue;
            } else {
              if (!traj_properties.has_flag("default")) continue;
            }

            // Retrieve trajectory pattern:
            const snemo::datamodel::base_trajectory_pattern & a_pattern = a_trajectory.get_pattern();

            // Prepare ROOT polyline:
            TPolyLine3D * track = 0;
            if (a_pattern.get_pattern_id() == snemo::datamodel::helix_trajectory_pattern::pattern_id()) {
              const snemo::datamodel::helix_trajectory_pattern * a_helix_pattern
                = dynamic_cast<const snemo::datamodel::helix_trajectory_pattern *>(&a_pattern);
              const geomtools::helix_3d & a_helix = a_helix_pattern->get_helix();
              track = base_renderer::make_helix_track(a_helix);
            } else if (a_pattern.get_pattern_id() == snemo::datamodel::line_trajectory_pattern::pattern_id()) {
              const snemo::datamodel::line_trajectory_pattern * a_line_pattern
                = dynamic_cast<const snemo::datamodel::line_trajectory_pattern *>(&a_pattern);
              const geomtools::line_3d & a_line = a_line_pattern->get_segment();
              track = base_renderer::make_line_track(a_line);
            } else {
              DT_LOG_WARNING(options_manager::get_instance().get_logging_priority(),
                             "The pattern of the trajectory can not be determined!");
              continue;
            }
            _objects->Add(track);

            // Determine trajectory color by getting cluster color:
            int trajectory_color = 0;
            if (a_trajectory.has_cluster()) {
              const snemo::datamodel::tracker_cluster & a_cluster = a_trajectory.get_cluster();
              const datatools::properties & prop = a_cluster.get_auxiliaries();
              if (prop.has_key(browser_tracks::COLOR_FLAG)) {
                std::string hex_str;
                prop.fetch(browser_tracks::COLOR_FLAG, hex_str);
                trajectory_color = TColor::GetColor(hex_str.c_str());
              }
            }
            track->SetLineColor(trajectory_color);

            // Retrieve a mutable reference to tracker_trajectory:
            snemo::datamodel::tracker_trajectory * mutable_trajectory
              = const_cast<snemo::datamodel::tracker_trajectory*>(&(a_trajectory));
            datatools::properties & prop = mutable_trajectory->grab_auxiliaries();

            // Retrieve line width from properties if 'track' is highlighted:
            size_t line_width = 1;
            if (prop.has_flag(browser_tracks::HIGHLIGHT_FLAG)) line_width = 3;
            //prop.update(browser_tracks::HIGHLIGHT_FLAG, false);
            track->SetLineWidth(line_width);

          } // end of trajectory loop
        } // end of solution loop

        return;
      }

      void tracker_hit_renderer::make_calibrated_geiger_hit(const snemo::datamodel::calibrated_tracker_hit & hit_,
                                                            TObjArray * objects_,
                                                            const bool show_cluster)
      {
        // Get (x, y) position of triggered cell
        const double x_cell = hit_.get_x();
        const double y_cell = hit_.get_y();

        // Add error in z coordinate
        const double z = hit_.get_z();
        const double sigma_z = hit_.get_sigma_z();

        const options_manager & options_mgr = options_manager::get_instance();

        const datatools::properties & aux = hit_.get_auxiliaries();

        // Retrieve line width from properties if 'hit' is highlighted:
        size_t line_width = style_manager::get_instance().get_mc_line_width();
        if (aux.has_flag(browser_tracks::HIGHLIGHT_FLAG)) line_width = 3;

        TPolyLine3D * gg_dz = new TPolyLine3D;
        objects_->Add(gg_dz);

        int color = style_manager::get_instance().get_calibrated_data_color();
        if (show_cluster && aux.has_key(browser_tracks::COLOR_FLAG)) {
          std::string hex_str;
          aux.fetch(browser_tracks::COLOR_FLAG, hex_str);
          if (options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_HITS) &&
              options_mgr.get_option_flag(SHOW_TRACKER_CLUSTERED_CIRCLE))
            color = TColor::GetColor(hex_str.c_str());
        }
        gg_dz->SetLineColor(color);
        gg_dz->SetLineWidth(line_width);

        int cell_axis = 'z';

        const detector::detector_manager & detector_mgr = detector::detector_manager::get_instance();
        const std::string & setup_label = detector_mgr.get_setup_label_name();

        if (setup_label == "snemo::tracker_commissioning") {
          cell_axis = 'x';
        }

        DT_THROW_IF(cell_axis != 'z' && cell_axis != 'x', std::logic_error,
                    "Unsupported cell axis !");

        if (cell_axis == 'z') {
          gg_dz->SetPoint(0, x_cell, y_cell, z - sigma_z);
          gg_dz->SetPoint(1, x_cell, y_cell, z + sigma_z);
        } else if (cell_axis == 'x') {
          // gg_dz->SetPoint(0, x_cell, y_cell, z - sigma_z);
          // gg_dz->SetPoint(1, x_cell, y_cell, z + sigma_z);
        }

        if (hit_.is_delayed()) {
          const double r = 22.0 / CLHEP::mm;//hit_.get_r();
          std::vector<geomtools::vector_3d> points;
          points.push_back(geomtools::vector_3d(x_cell+r, y_cell+r, z));
          points.push_back(geomtools::vector_3d(x_cell+r, y_cell-r, z));
          points.push_back(geomtools::vector_3d(x_cell-r, y_cell-r, z));
          points.push_back(geomtools::vector_3d(x_cell-r, y_cell+r, z));
          points.push_back(geomtools::vector_3d(x_cell+r, y_cell+r, z));

          TPolyLine3D * gg_drift_square = base_renderer::make_polyline(points);
          objects_->Add(gg_drift_square);
          gg_drift_square->SetLineColor(color);
          gg_drift_square->SetLineWidth(line_width);

        } else {
          // add calibrated drift value:  r-dr; r+dr
          const size_t n_point = 100;
          TRotation dr;
          if (cell_axis == 'z') {
            dr.RotateZ(2*TMath::Pi()/(double)n_point);
          } else if (cell_axis == 'x') {
            dr.RotateX(2*TMath::Pi()/(double)n_point);
          }

          // get calibrated info
          const double r = hit_.get_r();
          const double sigma_r = hit_.get_sigma_r();

          std::vector<geomtools::vector_3d> rmins;
          std::vector<geomtools::vector_3d> rmaxs;
          rmins.reserve(n_point);
          rmaxs.reserve(n_point);
          if (cell_axis == 'z') {
            TVector3 r_min(r - sigma_r, 0, z);
            TVector3 r_max(r + sigma_r, 0, z);

            for (size_t i_point = 0; i_point <= n_point; ++i_point) {
              r_min *= dr;
              r_max *= dr;
              rmins.push_back(geomtools::vector_3d(r_min.x() + x_cell,
                                                   r_min.y() + y_cell,
                                                   r_min.z()));
              rmaxs.push_back(geomtools::vector_3d(r_max.x() + x_cell,
                                                   r_max.y() + y_cell,
                                                   r_max.z()));
            }
          } else if(cell_axis == 'x') {
            // XXX
          }
          TPolyLine3D * gg_drift_min = base_renderer::make_polyline(rmins);
          objects_->Add(gg_drift_min);
          gg_drift_min->SetLineColor(color);
          gg_drift_min->SetLineWidth(line_width);

          TPolyLine3D * gg_drift_max = base_renderer::make_polyline(rmaxs);
          objects_->Add(gg_drift_max);
          gg_drift_max->SetLineColor(color);
          gg_drift_max->SetLineWidth(line_width);
        }
        return;
      }

    } // end of namespace view

  } // end of namespace visualization

} // end of namespace snemo

// end of tracker_hit_renderer.cc
/*
** Local Variables: --
** mode: c++ --
** c-file-style: "gnu" --
** tab-width: 2 --
** End: --
*/