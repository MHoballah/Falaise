// falaise/snemo/datamodels/particle_track.cc

// Ourselves:
#include <falaise/snemo/datamodels/particle_track.h>

namespace snemo {

namespace datamodel {

bool particle_has(const Particle& pt_, Particle::charge_type charge_) {
  return pt_.get_charge() == charge_;
}

bool particle_has_negative_charge(const Particle& pt_) {
  return particle_has(pt_, Particle::NEGATIVE);
}

bool particle_has_positive_charge(const Particle& pt_) {
  return particle_has(pt_, Particle::POSITIVE);
}

bool particle_has_undefined_charge(const Particle& pt_) {
  return particle_has(pt_, Particle::UNDEFINED);
}

bool particle_has_neutral_charge(const Particle& pt_) {
  return particle_has(pt_, Particle::NEUTRAL);
}

const std::string& particle_track::vertex_type_key() {
  static const std::string _flag("vertex.type");
  return _flag;
}

const std::string& particle_track::vertex_type_to_label(vertex_type vt_) {
  switch (vt_) {
    case VERTEX_ON_SOURCE_FOIL:
      return vertex_on_source_foil_label();
    case VERTEX_ON_MAIN_CALORIMETER:
      return vertex_on_main_calorimeter_label();
    case VERTEX_ON_X_CALORIMETER:
      return vertex_on_x_calorimeter_label();
    case VERTEX_ON_GAMMA_VETO:
      return vertex_on_gamma_veto_label();
    case VERTEX_ON_WIRE:
      return vertex_on_wire_label();
    default:
      return vertex_none_label();
  }
}

particle_track::vertex_type particle_track::label_to_vertex_type(const std::string& label_) {
  if (label_ == vertex_on_source_foil_label()) {
    return VERTEX_ON_SOURCE_FOIL;
  }
  if (label_ == vertex_on_main_calorimeter_label()) {
    return VERTEX_ON_MAIN_CALORIMETER;
  }
  if (label_ == vertex_on_x_calorimeter_label()) {
    return VERTEX_ON_X_CALORIMETER;
  }
  if (label_ == vertex_on_gamma_veto_label()) {
    return VERTEX_ON_GAMMA_VETO;
  }
  if (label_ == vertex_on_wire_label()) {
    return VERTEX_ON_WIRE;
  }
  return VERTEX_NONE;
}

const std::string& particle_track::vertex_none_label() {
  static const std::string _flag("!");
  return _flag;
}

const std::string& particle_track::vertex_on_wire_label() {
  static const std::string _flag("wire");
  return _flag;
}

const std::string& particle_track::vertex_on_source_foil_label() {
  static const std::string _flag("foil");
  return _flag;
}

const std::string& particle_track::vertex_on_main_calorimeter_label() {
  static const std::string _flag("calo");
  return _flag;
}

const std::string& particle_track::vertex_on_x_calorimeter_label() {
  static const std::string _flag("xcalo");
  return _flag;
}

const std::string& particle_track::vertex_on_gamma_veto_label() {
  static const std::string _flag("gveto");
  return _flag;
}

bool particle_track::vertex_is(const geomtools::blur_spot& vtx_, vertex_type vtype_) {
  if (vtx_.get_auxiliaries().has_key(vertex_type_key())) {
    std::string vtype_label = vtx_.get_auxiliaries().fetch_string(vertex_type_key());
    return vtype_label == vertex_type_to_label(vtype_);
  }
  return vtype_ == VERTEX_NONE;
}

bool particle_track::vertex_is_on_source_foil(const geomtools::blur_spot& vtx_) {
  return vertex_is(vtx_, VERTEX_ON_SOURCE_FOIL);
}

bool particle_track::vertex_is_on_main_calorimeter(const geomtools::blur_spot& vtx_) {
  return vertex_is(vtx_, VERTEX_ON_MAIN_CALORIMETER);
}

bool particle_track::vertex_is_on_x_calorimeter(const geomtools::blur_spot& vtx_) {
  return vertex_is(vtx_, VERTEX_ON_X_CALORIMETER);
}

bool particle_track::vertex_is_on_gamma_veto(const geomtools::blur_spot& vtx_) {
  return vertex_is(vtx_, VERTEX_ON_GAMMA_VETO);
}

bool particle_track::vertex_is_on_wire(const geomtools::blur_spot& vtx_) {
  return vertex_is(vtx_, VERTEX_ON_WIRE);
}

bool particle_track::has_track_id() const { return has_hit_id(); }

int particle_track::get_track_id() const { return get_hit_id(); }

void particle_track::set_track_id(int32_t track_id_) { set_hit_id(track_id_); }

void particle_track::set_charge(charge_type charge_) { _charge_from_source_ = charge_; }

particle_track::charge_type particle_track::get_charge() const { return _charge_from_source_; }

bool particle_track::has_trajectory() const { return _trajectory_.has_data(); }

void particle_track::detach_trajectory() { _trajectory_.reset(); }

TrackerTrajectoryHdl& particle_track::get_trajectory_handle() { return _trajectory_; }

const TrackerTrajectoryHdl& particle_track::get_trajectory_handle() const {
  return _trajectory_;
}

void particle_track::set_trajectory_handle(
    const TrackerTrajectoryHdl& trajectory_handle_) {
  _trajectory_ = trajectory_handle_;
}

tracker_trajectory& particle_track::get_trajectory() { return *_trajectory_; }

const tracker_trajectory& particle_track::get_trajectory() const { return *_trajectory_; }

bool particle_track::has_vertices() const { return !_vertices_.empty(); }

void particle_track::reset_vertices() { _vertices_.clear(); }

particle_track::vertex_collection_type& particle_track::get_vertices() { return _vertices_; }

const particle_track::vertex_collection_type& particle_track::get_vertices() const {
  return _vertices_;
}

size_t particle_track::fetch_vertices(vertex_collection_type& vertices_, const uint32_t flags_,
                                      const bool clear_) const {
  if (clear_) {
    vertices_.clear();
  }
  size_t ivtx = 0;
  for (const auto& a_vertex : get_vertices()) {
    const datatools::properties& aux = a_vertex->get_auxiliaries();
    std::string vtx_type;
    if (aux.has_key(vertex_type_key())) {
      vtx_type = aux.fetch_string(vertex_type_key());
    }
    const bool has_vertex_on_foil =
        ((flags_ & VERTEX_ON_SOURCE_FOIL) != 0u) && vertex_is_on_source_foil(*a_vertex);
    const bool has_vertex_on_calo =
        ((flags_ & VERTEX_ON_MAIN_CALORIMETER) != 0u) && vertex_is_on_main_calorimeter(*a_vertex);
    const bool has_vertex_on_xcalo =
        ((flags_ & VERTEX_ON_X_CALORIMETER) != 0u) && vertex_is_on_x_calorimeter(*a_vertex);
    const bool has_vertex_on_gveto =
        ((flags_ & VERTEX_ON_GAMMA_VETO) != 0u) && vertex_is_on_gamma_veto(*a_vertex);
    const bool has_vertex_on_wire =
        ((flags_ & VERTEX_ON_WIRE) != 0u) && vertex_is_on_wire(*a_vertex);

    if (has_vertex_on_foil || has_vertex_on_calo || has_vertex_on_xcalo || has_vertex_on_gveto ||
        has_vertex_on_wire) {
      vertices_.push_back(a_vertex);
      ivtx++;
    }
  }
  return ivtx;
}

bool particle_track::has_associated_calorimeter_hits() const {
  return !_associated_calorimeter_hits_.empty();
}

void particle_track::reset_associated_calorimeter_hits() { _associated_calorimeter_hits_.clear(); }

CalorimeterHitHdlCollection& particle_track::get_associated_calorimeter_hits() {
  return _associated_calorimeter_hits_;
}

const CalorimeterHitHdlCollection& particle_track::get_associated_calorimeter_hits()
    const {
  return _associated_calorimeter_hits_;
}

void particle_track::reset() { this->clear(); }

void particle_track::clear() {
  reset_vertices();
  reset_associated_calorimeter_hits();
  detach_trajectory();
  base_hit::clear();
  _charge_from_source_ = UNDEFINED;
}

void particle_track::tree_dump(std::ostream& out_, const std::string& title_,
                               const std::string& indent_, bool inherit_) const {
  base_hit::tree_dump(out_, title_, indent_, true);

  out_ << indent_ << datatools::i_tree_dumpable::tag << "Track ID : " << get_track_id()
       << std::endl;

  out_ << indent_ << datatools::i_tree_dumpable::tag << "Trajectory : ";
  if (has_trajectory()) {
    out_ << _trajectory_->get_id();
  } else {
    out_ << "<No>";
  }
  out_ << std::endl;

  out_ << indent_ << datatools::i_tree_dumpable::tag << "Particle charge : ";
  if (get_charge() == UNDEFINED) {
    out_ << "invalid";
  } else if (get_charge() == NEGATIVE) {
    out_ << "negative";
  } else if (get_charge() == POSITIVE) {
    out_ << "positive";
  } else if (get_charge() == NEUTRAL) {
    out_ << "neutral";
  }
  out_ << std::endl;

  out_ << indent_ << datatools::i_tree_dumpable::tag << "Vertices : ";
  if (has_vertices()) {
    out_ << _vertices_.size();
  } else {
    out_ << "<No>";
  }
  out_ << std::endl;
  for (auto i = _vertices_.begin(); i != _vertices_.end(); i++) {
    out_ << indent_ << datatools::i_tree_dumpable::skip_tag;
    auto j = i;
    j++;
    if (j == _vertices_.end()) {
      out_ << datatools::i_tree_dumpable::last_tag;
    } else {
      out_ << datatools::i_tree_dumpable::tag;
    }
    const geomtools::blur_spot& spot = i->get();
    out_ << "Vertex Id=" << spot.get_hit_id() << " @ "
         << spot.get_placement().get_translation() / CLHEP::mm << " mm"
         << " (" << spot.get_auxiliaries().fetch_string(vertex_type_key()) << ")";

    out_ << std::endl;
  }

  out_ << indent_ << datatools::i_tree_dumpable::inherit_tag(inherit_)
       << "Associated calorimeter hit(s) : ";
  if (has_associated_calorimeter_hits()) {
    out_ << _associated_calorimeter_hits_.size();
  } else {
    out_ << "<No>";
  }
  out_ << std::endl;
  for (auto i = _associated_calorimeter_hits_.begin(); i != _associated_calorimeter_hits_.end();
       i++) {
    out_ << indent_ << datatools::i_tree_dumpable::inherit_skip_tag(inherit_);
    auto j = i;
    j++;
    if (j == _associated_calorimeter_hits_.end()) {
      out_ << datatools::i_tree_dumpable::last_tag;
    } else {
      out_ << datatools::i_tree_dumpable::tag;
    }
    const calibrated_calorimeter_hit& calo_hit = i->get();
    out_ << "Hit Id=" << calo_hit.get_hit_id() << " @ " << calo_hit.get_geom_id();
    out_ << std::endl;
  }
}

// serial tag for datatools::serialization::i_serializable interface :
DATATOOLS_SERIALIZATION_SERIAL_TAG_IMPLEMENTATION(particle_track,
                                                  "snemo::datamodel::particle_track")

}  // end of namespace datamodel

}  // end of namespace snemo

// end of falaise/snemo/datamodels/particle_track.cc
