#ifdef EYE_CANDY

// I N C L U D E S ////////////////////////////////////////////////////////////

#include "eye_candy.h"
#include "math_cache.h"

#include "effect_cloud.h"

namespace ec
{

// C L A S S   F U N C T I O N S //////////////////////////////////////////////

CloudParticle::CloudParticle(Effect* _effect, ParticleMover* _mover, const Vec3 _pos, const Vec3 _velocity, const coord_t _min_height, const coord_t _max_height, const coord_t _size, const alpha_t _alpha) : Particle(_effect, _mover, _pos, _velocity)
{
  color[0] = 1.0;
  color[1] = 1.0;
  color[2] = 1.0;
  size = _size;
  alpha = _alpha;
  flare_max = 2.0;
  flare_exp = 0.7;
  flare_frequency = 30.0;
  min_height = _min_height;
  max_height = _max_height;
  normal = Vec3(0.0, 1.0, 0.0);
}

bool CloudParticle::idle(const Uint64 delta_t)
{
  if (effect->recall)
    return false;
    
  Vec3 velocity_shift;
  velocity_shift.randomize();
  velocity_shift.y /= 3;
  velocity_shift.normalize(0.00002 * fastsqrt(delta_t));
  velocity += velocity_shift;
  const coord_t magnitude = velocity.magnitude();
  if (magnitude > 0.15)
    velocity /= (magnitude / 0.15);
    
  if (fabs(velocity.y) > 0.1)
    velocity.y *= math_cache.powf_05_close(delta_t / 300000.0);

  if (pos.y - size / 40 < min_height)
    velocity.y += delta_t / 500000.0;

  if (pos.y + size / 40 > max_height)
    velocity.y -= delta_t / 2500000.0;

  // Adjust our neighbors -- try a few points to see if they're closer.
  // First, create the map
  CloudEffect* eff = (CloudEffect*)effect;
  std::map<coord_t, CloudParticle*> neighbors_map;
  for (int i = 0; i < (int)neighbors.size(); i++)
  {
    const coord_t distsquared = (neighbors[i]->pos - pos).magnitude_squared();
    neighbors_map[distsquared] = neighbors[i];
  }
  
  // Now, try to replace elements.
  coord_t maxdist = neighbors_map.rbegin()->first;
//  std::cout << this << ": " << std::endl;
  for (int i = 0; (i < 1) || (neighbors_map.size() < 20); i++)
  {
    std::map<Particle*, bool>::iterator iter = eff->particles.begin();
    const int offset = randint(eff->particles.size());
    for (int j = 0; j < offset; j++)
      iter++;
    CloudParticle* neighbor = (CloudParticle*)iter->first;
    if (neighbor == this)
      continue;
    const coord_t distsquared = (neighbor->pos - pos).magnitude_squared();
    if (neighbors_map.size() >= 20)
    {
      if (distsquared > maxdist)
        continue;
      if (neighbors_map.count(distsquared))
        continue;
    }
//    std::cout << "  Subst (" << distsquared << ", " << maxdist << ", " << neighbors_map.size() << "): ";
    if (neighbors_map.size() >= 20)
    {
      std::map<coord_t, CloudParticle*>::iterator iter = neighbors_map.begin();
      for (int j = 0; j < (int)neighbors_map.size() - 1; j++)
        iter++;
//      std::cout << iter->second << ", " << iter->first;
      neighbors_map.erase(iter);
    }
//    std::cout << " (" << neighbors_map.size() << ")";
    neighbors_map[distsquared] = neighbor;
    maxdist = neighbors_map.rbegin()->first;
//    std::cout << " with " << neighbor << ", " << distsquared << " (" << neighbors_map.size() << "); " << maxdist << std::endl;
  }
//  for (std::map<coord_t, CloudParticle*>::iterator iter = neighbors_map.begin(); iter != neighbors_map.end(); iter++)
//    std::cout << "  " << iter->first << ": " << iter->second << std::endl;

  // Set our color based on how deep into the cloud we are, based on our neighbors.  Also rebuild the neighbors vector.
  coord_t distsquaredsum = 0;
  Vec3 centerpoint(0.0, 0.0, 0.0);
  neighbors.clear();
  for (std::map<coord_t, CloudParticle*>::iterator iter = neighbors_map.begin(); iter != neighbors_map.end(); iter++)
  {
    distsquaredsum += iter->first;
    centerpoint += iter->second->pos;	//Should really be (pos - iter->second->pos); will correct this below for speed.
    neighbors.push_back(iter->second);
  }
  centerpoint = (pos * 20) - centerpoint;
  Vec3 new_normal = centerpoint;
  const coord_t magnitude_squared = centerpoint.magnitude_squared();
  const coord_t scale = fastsqrt(magnitude_squared);
  new_normal /= scale;	// Normalize
//  light_t new_brightness = 1.0 - (25.0 / (scale + 50.0));
//  new_normal.x = (new_normal.x < 0 ? -1 : 1) * math_cache.powf_0_1_rough_close(fabs(new_normal.x), new_brightness * 2.0 - 1.0);
//  new_normal.y = (new_normal.y < 0 ? -1 : 1) * math_cache.powf_0_1_rough_close(fabs(new_normal.y), new_brightness * 2.0 - 1.0);
//  new_normal.z = (new_normal.z < 0 ? -1 : 1) * math_cache.powf_0_1_rough_close(fabs(new_normal.z), new_brightness * 2.0 - 1.0);
  const percent_t change_rate = math_cache.powf_05_close(delta_t / 2000000.0);
  normal = normal * change_rate + new_normal * (1.0 - change_rate);
  normal.normalize();
//  color[0] = color[0] * change_rate + new_brightness * (1.0 - change_rate);
//  color[1] = color[0];
//  color[2] = color[0];
//  std::cout << "  " << centerpoint << std::endl;
//  std::cout << "  " << normal << std::endl;
//  std::cout << "  " << brightness << std::endl;
  
  return true;
}

GLuint CloudParticle::get_texture(const Uint16 res_index)
{
  return base->TexSimple.get_texture(res_index);
}

void CloudParticle::draw(const Uint64 usec)
{
  glEnable(GL_LIGHTING);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//  Vec3 shifted_pos = *pos - ((CloudEffect*)effect)->pos;

  glNormal3f(normal.x, normal.y, normal.z);
  Particle::draw(usec);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glDisable(GL_LIGHTING);
}

CloudEffect::CloudEffect(EyeCandy* _base, bool* _dead, Vec3* _pos, const float _density, const std::vector<PolarCoordElement> bounding_range, const Uint16 _LOD)
{
  if (EC_DEBUG)
    std::cout << "CloudEffect (" << this << ") created." << std::endl;
  base = _base;
  dead = _dead;
  pos = _pos;
  center = *pos;
  LOD = _LOD;
  desired_LOD = _LOD;
  mover = new PolarCoordsBoundingMover(this, center, bounding_range, 1.0);
  spawner = new FilledPolarCoordsSpawner(bounding_range);
  int count = (int)(spawner->get_area() * 0.03 * (LOD  + 1));
  if (count < 21)
    count = 21;
  const alpha_t alpha = 0.1725 / (1.0 / _density + 0.15);

  const coord_t size_scalar = 110.0 * invsqrt(LOD + 1);
  for (int i = 0; i < count; i++)
  {
    const Vec3 coords = spawner->get_new_coords() + center + Vec3(0.0, randcoord(5.0), 0.0);
    Vec3 velocity;
    velocity.randomize(0.15);
    velocity.y /= 3;
    const coord_t size = size_scalar + randcoord(size_scalar);
    Particle* p = new CloudParticle(this, mover, coords, velocity, center.y, center.y + 20.0, size, alpha);
    if (!base->push_back_particle(p))
      break;
  }
  
  // Load one neighbor for each one.  It'll get more on its own.
  for (std::map<Particle*, bool>::iterator iter = particles.begin(); iter != particles.end(); iter++)
  {
    CloudParticle* p = (CloudParticle*)iter->first;
    CloudParticle* next;
    std::map<Particle*, bool>::iterator iter2 = iter;
    iter2++;
    if (iter2 != particles.end())
      next = (CloudParticle*)iter2->first;
    else
      next = (CloudParticle*)particles.begin()->first;
    p->neighbors.push_back(next);
  }
}

CloudEffect::~CloudEffect()
{
  delete mover;
  delete spawner;
  if (EC_DEBUG)
    std::cout << "CloudEffect (" << this << ") destroyed." << std::endl;
}

bool CloudEffect::idle(const Uint64 usec)
{
  if (particles.size() == 0)
    return false;
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////

};

#endif	// #ifdef EYE_CANDY