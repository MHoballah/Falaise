// -*- mode: c++ ; -*-
/** \file falaise/snemo/processing/mock_calorimeter_s2c_module.h
 * Author(s) :    Francois Mauger <mauger@lpccaen.in2p3.fr>
 * Creation date: 2011-01-12
 * Last modified: 2014-01-30
 *
 * License:
 *
 * Description:
 *
 *   Mock simulation data processor for calorimeter MC hits
 *
 * History:
 *
 */

#ifndef FALAISE_SNEMO_PROCESSING_MOCK_CALORIMETER_S2C_MODULE_H
#define FALAISE_SNEMO_PROCESSING_MOCK_CALORIMETER_S2C_MODULE_H 1

// Standard library:
#include <string>
#include <map>
#include <vector>

// Third party:
// - Bayeux/mygsl:
#include <mygsl/rng.h>
// - Bayeux/dpp:
#include <dpp/base_module.h>

// This project :
#include <falaise/snemo/datamodels/calibrated_data.h>
#include <falaise/snemo/processing/calorimeter_regime.h>

namespace geomtools {
  class manager;
}

namespace mctools {
  class simulated_data;
}

namespace snemo {

  namespace processing {

    /// \brief A mock calibration for SuperNEMO calorimeter hits
    class mock_calorimeter_s2c_module : public dpp::base_module
    {
    public:

      typedef std::map<std::string, calorimeter_regime> calorimeter_regime_col_type;
      typedef std::vector<std::string>                  category_col_type;

      /// Setting geometry manager
      void set_geom_manager(const geomtools::manager & gmgr_);

      /// Getting geometry manager
      const geomtools::manager & get_geom_manager() const;

      /// Constructor
      mock_calorimeter_s2c_module(datatools::logger::priority = datatools::logger::PRIO_FATAL);

      /// Destructor
      virtual ~mock_calorimeter_s2c_module();

      /// Initialization
      virtual void initialize(const datatools::properties  & setup_,
                              datatools::service_manager   & service_manager_,
                              dpp::module_handle_dict_type & module_dict_);

      /// Reset
      virtual void reset();

      /// Data record processing
      virtual process_status process(datatools::things & data_);

    protected:

      /// Getting random number generator
      mygsl::rng & _get_random ();

      /// Digitize calorimeter hits
      void _process_calorimeter_digitization
      (const mctools::simulated_data & simulated_data_,
       snemo::datamodel::calibrated_data::calorimeter_hit_collection_type & calibrated_calorimeter_hits_);

      /// Calibrate calorimeter hits (energy/time resolution spread)
      void _process_calorimeter_calibration
      (snemo::datamodel::calibrated_data::calorimeter_hit_collection_type & calorimeter_hits_);

      /// Apply basic trigger effect
      void _process_calorimeter_trigger
      (snemo::datamodel::calibrated_data::calorimeter_hit_collection_type & calorimeter_hits_);

      /// Main process function
      void _process
      (const mctools::simulated_data & simulated_data_,
       snemo::datamodel::calibrated_data::calorimeter_hit_collection_type & calibrated_calorimeter_hits_);

    private:

      const geomtools::manager *  _geom_manager_;         //!< The geometry manager
      mygsl::rng                  _random_;              //!< PRN generator
      category_col_type           _hit_categories_;      //!< Calorimeter categories
      calorimeter_regime_col_type _calorimeter_regimes_; //!< Calorimeter regime tools
      std::string                 _SD_label_;            //!< The label of the simulated data bank
      std::string                 _CD_label_;            //!< The label of the calibrated data bank
      double                      _cluster_time_width_;  //!< Time width of a calo cluster
      bool                        _alpha_quenching_;     //!< Flag to (dis)activate the alpha quenching
      bool                        _store_mc_hit_id_;     //!< The flag to reference MC true hit


      // Macro to automate the registration of the module :
      DPP_MODULE_REGISTRATION_INTERFACE(mock_calorimeter_s2c_module);
    };

  } // end of namespace processing

} // end of namespace snemo

#endif // FALAISE_SNEMO_PROCESSING_MOCK_CALORIMETER_S2C_MODULE_H

// end of falaise/snemo/processing/mock_calorimeter_s2c_module.h