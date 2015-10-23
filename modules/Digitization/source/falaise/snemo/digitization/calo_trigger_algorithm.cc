// snemo/digitization/calo_trigger_algorithm.cc
// Author(s): Yves LEMIERE <lemiere@lpccaen.in2p3.fr>
// Author(s): Guillaume OLIVIERO <goliviero@lpccaen.in2p3.fr>

// Ourselves:
#include <snemo/digitization/calo_trigger_algorithm.h>
#include <snemo/digitization/calo_ctw.h>

namespace snemo {
  
  namespace digitization {
     
    const int32_t calo_trigger_algorithm::LEVEL_ONE_MULT_BITSET_SIZE;
    const int32_t calo_trigger_algorithm::ZONING_PER_SIDE_BITSET_SIZE;
    const int32_t calo_trigger_algorithm::ZONING_GVETO_BITSET_SIZE;
    const int32_t calo_trigger_algorithm::XT_INFO_BITSET_SIZE;

    
    calo_trigger_algorithm::trigger_record::trigger_record()
    {
      clocktick_25ns = -1;
      LTO_side_0 = false;
      LTO_side_1 = false;
      LTO_gveto  = false;
      return;
    }
        
    void calo_trigger_algorithm::trigger_record::reset()
    {
      clocktick_25ns = -1;
      for (int iside = 0; iside < mapping::NUMBER_OF_SIDES; iside++)
	{
	  zoning_word[iside].reset();
	  zoning_word[iside].reset();
	}
      total_multiplicity_side_0.reset();
      total_multiplicity_side_1.reset();
      LTO_side_0 = false;
      LTO_side_1 = false;
      total_multiplicity_gveto.reset();
      LTO_gveto = false;
      xt_info_bitset.reset();
      return;
    }

    void calo_trigger_algorithm::trigger_record::display()
    {      
      std::clog << "Trigger info record : " << std::endl; 
      std::clog << "CLOCKTICK |XTS|L|HG|L|L|H1|H0| ZONING S1| ZONING S0 " << std::endl; 
      std::clog << clocktick_25ns << ' ';
      std::clog << xt_info_bitset << ' ';
      std::clog << LTO_gveto << ' ';
      std::clog << total_multiplicity_gveto << ' ';
      std::clog << LTO_side_1 << ' ';
      std::clog << LTO_side_0 << ' ';
      std::clog << total_multiplicity_side_1 << ' ';
      std::clog << total_multiplicity_side_0 << ' ';

      for (int iside = mapping::NUMBER_OF_SIDES-1; iside >= 0; iside--)
      	{
      	  for (int izone = mapping::NUMBER_OF_CALO_TRIGGER_ZONES-1; izone >= 0 ; izone--)
      	    {
      	      std::clog << zoning_word[iside][izone];
      	    }
      	  std::clog << ' ';
      	}
      std::clog << std::endl << std::endl;;
      return;
    }
    
    calo_trigger_algorithm::trigger_summary_record::trigger_summary_record()
    {
      single_side_coinc = false;
      threshold_total_multiplicity = false;
      trigger_finale_decision = false;
      return; 
    }

   void calo_trigger_algorithm::trigger_summary_record::reset()
   {
     trigger_record::reset();
     single_side_coinc = false;
     threshold_total_multiplicity = false;
     trigger_finale_decision = false;
     return;
   }
    
    void calo_trigger_algorithm::trigger_summary_record::reset_summary_boolean_only()
    {
      single_side_coinc = false;
      threshold_total_multiplicity = false;
      trigger_finale_decision = false;
      return;
    }

    void calo_trigger_algorithm::trigger_summary_record::display()
    {
      trigger_record::display();
      std::clog << "Single Side coinc : "      << single_side_coinc << std::endl;
      std::clog << "Threshold total mult : "   << threshold_total_multiplicity << std::endl;
      std::clog << "Trigger final decision : " << trigger_finale_decision  << std::endl;
      std::clog << std::endl;
      return;
    }

    calo_trigger_algorithm::calo_trigger_algorithm()
    {
      _initialized_ = false;
      _electronic_mapping_ = 0;
      _circular_buffer_depth_ = -1;
      _activated_threshold_ = false;
      _inhibit_both_side_coinc_ = false;
      _inhibit_single_side_coinc_ = false;
      return;
    }

    calo_trigger_algorithm::~calo_trigger_algorithm()
    {   
      if (is_initialized())
	{
	  reset();
	}
      return;
    }

    void calo_trigger_algorithm::set_electronic_mapping(const electronic_mapping & my_electronic_mapping_)
    {
      DT_THROW_IF(is_initialized(), std::logic_error, "Calo trigger algorithm is already initialized, electronic mapping can't be set ! ");
      _electronic_mapping_ = & my_electronic_mapping_;
      return;
    }
    
    void calo_trigger_algorithm::set_circular_buffer_depth(unsigned int & circular_buffer_depth_)
    {
      DT_THROW_IF(is_initialized(), std::logic_error, "Calo trigger algorithm is already initialized, calo circular buffer depth can't be set ! ");
      _circular_buffer_depth_ = circular_buffer_depth_;
      return;
    }
    
    void calo_trigger_algorithm::inhibit_both_side_coinc()
    {
      DT_THROW_IF(is_initialized(), std::logic_error, "Calo trigger algorithm is already initialized, boolean back to back coinc can't be inhibited ! ");      
      _inhibit_both_side_coinc_ = true;
      return;
    }    

    bool calo_trigger_algorithm::is_inhibited_both_side_coinc() const
    {
      return _inhibit_both_side_coinc_;
    }

    void calo_trigger_algorithm::inhibit_single_side_coinc()
    {
      DT_THROW_IF(is_initialized(), std::logic_error, "Calo trigger algorithm is already initialized, boolean single side coinc can't be inhibited ! ");
      _inhibit_single_side_coinc_ = true;
      return;
    }     

    bool calo_trigger_algorithm::is_inhibited_single_side_coinc() const
    {
      return _inhibit_single_side_coinc_;
    }

    void calo_trigger_algorithm::set_threshold_total_multiplicity(unsigned int & threshold_)
    {
      DT_THROW_IF(is_initialized(), std::logic_error, "Calo trigger algorithm is already initialized, calo threshold can't be set ! ");
      _threshold_total_multiplicity_ = threshold_;
      _activated_threshold_ = true;
      return;
    }

    bool calo_trigger_algorithm::is_activated_threshold_total_multiplicity() const
    {
      return _activated_threshold_;
    }

    const std::bitset<calo::ctw::HTM_BITSET_SIZE> calo_trigger_algorithm::get_threshold_total_multiplicity_coinc() const
    {
      return _threshold_total_multiplicity_;
    }

    void calo_trigger_algorithm::initialize_simple()
    {
      datatools::properties dummy_config;
      initialize(dummy_config);
      return;
    }

    void calo_trigger_algorithm::initialize(const datatools::properties & config_)
    {
      DT_THROW_IF(is_initialized(), std::logic_error, "Calo trigger algorithm is already initialized ! ");
      DT_THROW_IF(_electronic_mapping_ == 0, std::logic_error, "Missing electronic mapping ! " );
      DT_THROW_IF(_circular_buffer_depth_ <= 0, std::logic_error, "Calo circular buffer depth value [" << _circular_buffer_depth_ << "] is missing ! ");
      DT_THROW_IF(!_activated_threshold_, std::logic_error, " Threshold total multiplicity is not set ! ");
      _initialized_ = true;
      return;
    }
    
    bool calo_trigger_algorithm::is_initialized() const
    {
      return _initialized_;
    }

    void calo_trigger_algorithm::reset()
    {
      DT_THROW_IF(!is_initialized(), std::logic_error, "Calo trigger algorithm is not initialized, it can't be reset ! ");
      _initialized_ = false;
      _electronic_mapping_ = 0;
      _activated_threshold_ = false;
      _inhibit_both_side_coinc_ = false;
      _inhibit_single_side_coinc_ = false;
      _circular_buffer_depth_ = -1;
      reset_trigger_info();
      _gate_circular_buffer_.reset();
      return;
    }
    
    void calo_trigger_algorithm::reset_trigger_info()
    {
      _trigger_record_per_clocktick_.reset();
      _calo_level_1_finale_decision_.reset();
      return;
    }

    void calo_trigger_algorithm::reset_trigger_record_per_clocktick()
    {
      _trigger_record_per_clocktick_.reset();
      return;
    }    

    const bool calo_trigger_algorithm::get_calo_level_1_finale_decision() const
    {
      return _calo_level_1_finale_decision_.trigger_finale_decision;
    }
    
    const calo_trigger_algorithm::trigger_summary_record calo_trigger_algorithm::get_calo_level_1_finale_decision_structure() const
    {
      return _calo_level_1_finale_decision_;
    }

    void calo_trigger_algorithm::_display_trigger_info_for_a_clocktick()
    {
      _trigger_record_per_clocktick_.display();

      for (int iside = 0; iside < mapping::NUMBER_OF_SIDES; iside++)
      	{
      	  if (iside == 1)
      	    {
      	      std::clog << " |                                                                                                                 |" << std::endl;
      	      if (_trigger_record_per_clocktick_.zoning_word[iside][0] == true) std::clog << "   |";
      	      else std::clog << " |";
      	      std::clog << "                                                                                                                 ";
      	      if (_trigger_record_per_clocktick_.zoning_word[iside][9] == true) std::clog << "| Side 1" << std::endl;
      	      else std::clog << "| Side 1" << std::endl;
      	    }
	  if (iside == 0) std::clog << "    Zone0                                                                                                   Zone9 " << std::endl;
      	  std::clog << " |";
      	  for (int izone = 0; izone < mapping::NUMBER_OF_CALO_TRIGGER_ZONES; izone++)
      	    {
      	      if (izone == 0 || izone == 9) 
      		{
      		  if (_trigger_record_per_clocktick_.zoning_word[iside][izone] == true) std::clog << "[*******]";
      		  else std::clog  << "[       ]";
      		}
      	      else if (izone == 5) 
      		{
      		  if (_trigger_record_per_clocktick_.zoning_word[iside][izone] == true) std::clog  << "[*********]";
      		  else std::clog  << "[         ]";
      		}
      	      else 
      		{
      		  if (_trigger_record_per_clocktick_.zoning_word[iside][izone] == true) std::clog  << "[**********]";
      		  else std::clog << "[          ]";
      		}
      	    } // end of izone
      	  std::clog << "|" << std::endl;
	  if (iside == 1) std::clog << "    Zone0                                                                                                   Zone9 " << std::endl;
      	  if (iside == 0)
      	    {
      	      if (_trigger_record_per_clocktick_.zoning_word[iside][0] == true) std::clog << " |";
      	      else std::clog << " |";
      	      std::clog << "                                                                                                                 ";
      	      if (_trigger_record_per_clocktick_.zoning_word[iside][9] == true) std::clog << "| Side 0" << std::endl;
      	      else std::clog << "| Side 0" << std::endl;
      	      std::clog << " |                                                                                                                 |" << std::endl;
      	      std::clog << " |-----------------------------------------------------------------------------------------------------------------| Source foil" << std::endl;
      	    }	  
      	} // end of iside
      std::clog << std::endl;
      return;
    }

    void calo_trigger_algorithm::_display_trigger_summary(trigger_summary_record & my_trigger_summary_record_)
    {
      my_trigger_summary_record_.display(); 

      for (int iside = 0; iside < mapping::NUMBER_OF_SIDES; iside++)
      	{
      	  if (iside == 1)
      	    {
      	      std::clog << " |                                                                                                                 |" << std::endl;
      	      if (my_trigger_summary_record_.zoning_word[iside][0] == true) std::clog << " |";
      	      else std::clog << " |";
      	      std::clog << "                                                                                                                 ";
      	      if (my_trigger_summary_record_.zoning_word[iside][9] == true) std::clog << "| Side 1" << std::endl;
      	      else std::clog << "| Side 1" << std::endl;
      	    }
	  if (iside == 0) std::clog << "    Zone0                                                                                                   Zone9 " << std::endl;
      	  std::clog << " |";
      	  for (int izone = 0; izone < mapping::NUMBER_OF_CALO_TRIGGER_ZONES; izone++)
      	    {
      	      if (izone == 0 || izone == 9) 
      		{
      		  if (my_trigger_summary_record_.zoning_word[iside][izone] == true) std::clog << "[*******]";
      		  else std::clog  << "[       ]";
      		}
      	      else if (izone == 5) 
      		{
      		  if (my_trigger_summary_record_.zoning_word[iside][izone] == true) std::clog  << "[*********]";
      		  else std::clog  << "[         ]";
      		}
      	      else 
      		{
      		  if (my_trigger_summary_record_.zoning_word[iside][izone] == true) std::clog  << "[**********]";
      		  else std::clog << "[          ]";
      		}
      	    } // end of izone
      	  std::clog << "|" << std::endl;
	  if (iside == 1) std::clog << "    Zone0                                                                                                   Zone9 " << std::endl;
      	  if (iside == 0)
      	    {
      	      if (my_trigger_summary_record_.zoning_word[iside][0] == true) std::clog << " |";
      	      else std::clog << " |";
      	      std::clog << "                                                                                                                 ";
      	      if (my_trigger_summary_record_.zoning_word[iside][9] == true) std::clog << "| Side 0" << std::endl;
      	      else std::clog << "| Side 0" << std::endl;
      	      std::clog << " |                                                                                                                 |" << std::endl;
      	      std::clog << " |-----------------------------------------------------------------------------------------------------------------| Source foil" << std::endl;
      	    }	  
      	} // end of iside
      std::clog << std::endl;

      return;
    }
    
    void calo_trigger_algorithm::_build_trigger_record_per_clocktick(const calo_ctw & my_calo_ctw_)
    {  
      uint32_t crate_index = my_calo_ctw_.get_geom_id().get(mapping::CRATE_INDEX);  
      DT_THROW_IF(crate_index < mapping::MAIN_CALO_SIDE_0_CRATE || crate_index > mapping::XWALL_GVETO_CALO_CRATE, std::logic_error, "Crate index '"<< crate_index << "' is not defined, check your value ! ");
      
      _trigger_record_per_clocktick_.clocktick_25ns = my_calo_ctw_.get_clocktick_25ns();      

      // Fill structure if crate is number 2
      if (!my_calo_ctw_.is_main_wall())
	{
	  unsigned int multiplicity_side_0 = my_calo_ctw_.get_htm_xwall_side_0_info();
	  unsigned int multiplicity_side_1 = my_calo_ctw_.get_htm_xwall_side_1_info();
 
	  // Fill total HTM :
	  
	  // -- Fill xwall side 0 multipliciy
	  if (multiplicity_side_0 != 0)
	    {
	      _trigger_record_per_clocktick_.total_multiplicity_side_0 = _trigger_record_per_clocktick_.total_multiplicity_side_0.to_ulong() + multiplicity_side_0;
	    }

	  // -- Fill xwall side 1 multiplicity
	  if (multiplicity_side_1 != 0)
	    {
	      _trigger_record_per_clocktick_.total_multiplicity_side_1 = _trigger_record_per_clocktick_.total_multiplicity_side_1.to_ulong() + multiplicity_side_1;
	    }
	  
	  // -- Fill gamma veto multiplicity
	  if (my_calo_ctw_.get_htm_gveto_info() != 0)
	    {
	      _trigger_record_per_clocktick_.total_multiplicity_gveto = my_calo_ctw_.get_htm_gveto_info();
	    }
	  else _trigger_record_per_clocktick_.total_multiplicity_gveto = 0;
	  
	  // Fill xwall zone corresponding of xwall zoning word :     

	  std::bitset<calo::ctw::XWALL_ZONING_BITSET_SIZE> xwall_zoning_bitset;
	  my_calo_ctw_.get_xwall_zoning_word(xwall_zoning_bitset);

	  for (int izone = calo::ctw::ZONING_XWALL_BIT0; izone < (calo::ctw::ZONING_XWALL_BIT0 + mapping::NUMBER_OF_XWALL_CALO_TRIGGER_ZONES); izone++)
	    {
	      switch (izone)
		{
		case calo::ctw::ZONING_XWALL_BIT0 :
		  if (xwall_zoning_bitset.test(izone - calo::ctw::ZONING_XWALL_BIT0) == true)
		    {		      
		      _trigger_record_per_clocktick_.zoning_word[SIDE_0_INDEX].set(ZONE_0_INDEX, true);
		    }
		  break;
		case calo::ctw::ZONING_XWALL_BIT1 :
		  if (xwall_zoning_bitset.test(izone - calo::ctw::ZONING_XWALL_BIT0) == true)
		    {
		      _trigger_record_per_clocktick_.zoning_word[SIDE_0_INDEX].set(ZONE_9_INDEX, true);
		    }
		  break;
		case calo::ctw::ZONING_XWALL_BIT2 :
		  if (xwall_zoning_bitset.test(izone - calo::ctw::ZONING_XWALL_BIT0) == true)
		    {
		      _trigger_record_per_clocktick_.zoning_word[SIDE_1_INDEX].set(ZONE_0_INDEX, true);
		    }
		  break;
		case calo::ctw::ZONING_XWALL_BIT3 :
		  if (xwall_zoning_bitset.test(izone - calo::ctw::ZONING_XWALL_BIT0) == true)
		    {	
		      _trigger_record_per_clocktick_.zoning_word[SIDE_1_INDEX].set(ZONE_9_INDEX, true);
		    }
		  break;
		default :
		  break;
		}
	    }

	  // Fill LTO boolean for each side of xwall and gamma veto

	  if (my_calo_ctw_.is_lto_xwall_side_0()) _trigger_record_per_clocktick_.LTO_side_0 = true;
	  if (my_calo_ctw_.is_lto_xwall_side_1()) _trigger_record_per_clocktick_.LTO_side_1 = true;
	  if (my_calo_ctw_.is_lto_gveto()) _trigger_record_per_clocktick_.LTO_gveto = true;
	}

      // Fill structure if crate is number 0 or 1
      else
	{	  
	  // Fill total HTM :
	  unsigned int main_multiplicity = my_calo_ctw_.get_htm_main_wall_info();

	  if (crate_index == mapping::MAIN_CALO_SIDE_0_CRATE && main_multiplicity != 0)
	    {
	      _trigger_record_per_clocktick_.total_multiplicity_side_0 = _trigger_record_per_clocktick_.total_multiplicity_side_0.to_ulong() + main_multiplicity;
	    }

	  if (crate_index == mapping::MAIN_CALO_SIDE_1_CRATE && main_multiplicity != 0)
	    {
	      _trigger_record_per_clocktick_.total_multiplicity_side_1 = _trigger_record_per_clocktick_.total_multiplicity_side_1.to_ulong() + main_multiplicity;
	    }

	  // Fill zone main wall bitset :

	  std::bitset<calo::ctw::MAIN_ZONING_BITSET_SIZE> main_zoning_bitset;
	  my_calo_ctw_.get_main_zoning_word(main_zoning_bitset);

	  for (int izone = 0; izone < mapping::NUMBER_OF_CALO_TRIGGER_ZONES; izone++)
	    {
	      if (main_zoning_bitset.test(izone) == true) _trigger_record_per_clocktick_.zoning_word[crate_index].set(izone,true);
	    }
	  
	  // Fill LTO boolean for each side
	  if (crate_index == mapping::MAIN_CALO_SIDE_0_CRATE && my_calo_ctw_.is_lto_main_wall()) _trigger_record_per_clocktick_.LTO_side_0 = true;
	  if (crate_index == mapping::MAIN_CALO_SIDE_1_CRATE && my_calo_ctw_.is_lto_main_wall()) _trigger_record_per_clocktick_.LTO_side_1 = true;
	}

      return;
    }    
    
    void calo_trigger_algorithm::_build_trigger_record_summary_structure(trigger_summary_record & my_trigger_summary_record_)
    {
      for (boost::circular_buffer<trigger_record>::iterator it =_gate_circular_buffer_->begin() ; it != _gate_circular_buffer_->end(); it++)
      	{
      	  const trigger_record & ctrec = *it; 

	  // Total mult side 0 :

      	  if (my_trigger_summary_record_.total_multiplicity_side_0 != 0)
      	    {
      	      if (my_trigger_summary_record_.total_multiplicity_side_0 == 3) my_trigger_summary_record_.total_multiplicity_side_0 = 3;
      	      else my_trigger_summary_record_.total_multiplicity_side_0 = my_trigger_summary_record_.total_multiplicity_side_0.to_ulong() + ctrec.total_multiplicity_side_0.to_ulong();
      	    }
      	  else
      	    {
      	      my_trigger_summary_record_.total_multiplicity_side_0 = ctrec.total_multiplicity_side_0;
      	    }

	  // Total mult side 1 :

      	  if (my_trigger_summary_record_.total_multiplicity_side_1 != 0)
      	    {
      	      if (my_trigger_summary_record_.total_multiplicity_side_1 == 3) my_trigger_summary_record_.total_multiplicity_side_1 = 3;
      	      else my_trigger_summary_record_.total_multiplicity_side_1 = my_trigger_summary_record_.total_multiplicity_side_1.to_ulong() + ctrec.total_multiplicity_side_1.to_ulong();
      	    }
      	  else
      	    {
      	      my_trigger_summary_record_.total_multiplicity_side_1 = ctrec.total_multiplicity_side_1;
      	    }
	  
	  // Total mult gveto :
	  if (my_trigger_summary_record_.total_multiplicity_gveto != 0)
	    {
	      my_trigger_summary_record_.total_multiplicity_gveto = my_trigger_summary_record_.total_multiplicity_gveto.to_ulong() + ctrec.total_multiplicity_gveto.to_ulong();
	    }
	  else
	    {
	      my_trigger_summary_record_.total_multiplicity_gveto = ctrec.total_multiplicity_gveto;
	    }
	  
	  // LTO bits :
	  
	  if (my_trigger_summary_record_.LTO_side_0 == false) my_trigger_summary_record_.LTO_side_0 = ctrec.LTO_side_0;
	  if (my_trigger_summary_record_.LTO_side_1 == false) my_trigger_summary_record_.LTO_side_1 = ctrec.LTO_side_1;
	  if (my_trigger_summary_record_.LTO_gveto == false) my_trigger_summary_record_.LTO_gveto = ctrec.LTO_gveto;
	  // Zoning word :

      	  for (int i = 0; i < mapping::NUMBER_OF_SIDES; i++)
      	    {
      	      for (int j = 0; j < ZONING_PER_SIDE_BITSET_SIZE; j++)
      		{
      		  if (ctrec.zoning_word[i].test(j) == true) my_trigger_summary_record_.zoning_word[i].set(j, true);
      		  if (j < XT_INFO_BITSET_SIZE && ctrec.xt_info_bitset.test(j) == true) my_trigger_summary_record_.xt_info_bitset.set(j, true);   
      		}
      	    }	  
      	}

      bool side_0_activated = my_trigger_summary_record_.zoning_word[SIDE_0_INDEX].any();
      bool side_1_activated = my_trigger_summary_record_.zoning_word[SIDE_1_INDEX].any();
      
      if ((side_0_activated && !side_1_activated) || (!side_0_activated && side_1_activated)) my_trigger_summary_record_.single_side_coinc = true;
      else my_trigger_summary_record_.single_side_coinc = false;

      if ((my_trigger_summary_record_.total_multiplicity_side_0.to_ulong() + my_trigger_summary_record_.total_multiplicity_side_1.to_ulong()) >= _threshold_total_multiplicity_.to_ulong())   my_trigger_summary_record_.threshold_total_multiplicity = true;
      else my_trigger_summary_record_.threshold_total_multiplicity = false;
    }

    void calo_trigger_algorithm::_compute_trigger_finale_decision(trigger_summary_record & my_trigger_summary_record_)
    {
      if ((_activated_threshold_ && my_trigger_summary_record_.threshold_total_multiplicity)
      	  && !(is_inhibited_single_side_coinc() && my_trigger_summary_record_.single_side_coinc)
      	  && !(is_inhibited_both_side_coinc() && !my_trigger_summary_record_.single_side_coinc))
      	{
      	  my_trigger_summary_record_.trigger_finale_decision = true;
      	  _calo_level_1_finale_decision_ = my_trigger_summary_record_;
      	}	 

      return;      
    }

    void calo_trigger_algorithm::process(const calo_ctw_data & calo_ctw_data_)
    {
      DT_THROW_IF(!is_initialized(), std::logic_error, "Calo trigger algorithm is not initialized, it can't process ! ");
      _process(calo_ctw_data_);
      return;
    }

    void calo_trigger_algorithm::_process(const calo_ctw_data & calo_ctw_data_)
    { 
      reset_trigger_info();
      _gate_circular_buffer_.reset(new buffer_type(_circular_buffer_depth_));

      for(int32_t iclocktick = calo_ctw_data_.get_clocktick_min(); iclocktick <= calo_ctw_data_.get_clocktick_max(); iclocktick++)
	{
	  std::vector<datatools::handle<calo_ctw> > ctw_list_per_clocktick;
	  calo_ctw_data_.get_list_of_calo_ctw_per_clocktick(iclocktick, ctw_list_per_clocktick);
	  
	  for (int isize = 0; isize < ctw_list_per_clocktick.size(); isize++)
	    {
	      _build_trigger_record_per_clocktick(ctw_list_per_clocktick[isize].get());
	    } // end of isize 
	  _gate_circular_buffer_->push_back(_trigger_record_per_clocktick_);
	  std::clog <<"*************************** Clocktick = " << iclocktick << "***************************" << std::endl << std::endl;
	  //_display_trigger_info_for_a_clocktick();
	  trigger_summary_record my_trigger_summary_record;
	  my_trigger_summary_record.clocktick_25ns = iclocktick;
	  _build_trigger_record_summary_structure(my_trigger_summary_record);
	  _compute_trigger_finale_decision(my_trigger_summary_record);
	  
	  if (get_calo_level_1_finale_decision()) _display_trigger_summary(_calo_level_1_finale_decision_);
	  
	  reset_trigger_record_per_clocktick();
	} // end of iclocktick
      return;
    }

  } // end of namespace digitization

} // end of namespace snemo
