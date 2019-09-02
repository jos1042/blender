#include "offset_handlers.hpp"

namespace BParticles {

void CreateTrailHandler::execute(OffsetHandlerInterface &interface)
{
  auto positions = interface.attributes().get<float3>("Position");
  auto position_offsets = interface.attribute_offsets().get<float3>("Position");

  auto inputs = m_compute_inputs->compute(interface);

  Vector<float3> new_positions;
  Vector<float> new_birth_times;
  for (uint pindex : interface.pindices()) {
    float rate = inputs->get<float>("Rate", 0, pindex);
    if (rate <= 0.0f) {
      continue;
    }

    float frequency = 1.0f / rate;
    float time_factor = interface.time_factors()[pindex];
    TimeSpan time_span = interface.time_span(pindex);
    float current_time = frequency * (std::floor(time_span.start() / frequency) + 1.0f);

    float3 total_offset = position_offsets[pindex] * time_factor;
    while (current_time < time_span.end()) {
      float factor = time_span.get_factor_safe(current_time);
      new_positions.append(positions[pindex] + total_offset * factor);
      new_birth_times.append(current_time);
      current_time += frequency;
    }
  }

  for (std::string &type_name : m_types_to_emit) {
    auto new_particles = interface.particle_allocator().request(type_name, new_positions.size());
    new_particles.set<float3>("Position", new_positions);
    new_particles.set<float>("Birth Time", new_birth_times);

    m_on_birth_action->execute_for_new_particles(new_particles, interface);
  }
}

}  // namespace BParticles