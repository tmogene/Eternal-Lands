#ifdef EYE_CANDY

// I N C L U D E S ////////////////////////////////////////////////////////////

#include "eye_candy.h"
#include "math_cache.h"

#include "effect_candle.h"

namespace ec
{

// C L A S S   F U N C T I O N S //////////////////////////////////////////////

CandleParticle::CandleParticle(Effect* _effect, ParticleMover* _mover, const Vec3 _pos, const Vec3 _velocity, const float _scale, const Uint16 _LOD) : Particle(_effect, _mover, _pos, _velocity)
{
  LOD = _LOD;
  color[0] = 0.9;
  color[1] = 0.3 + randfloat(0.25);
  color[2] = 0.2;
  size = 6.0 * (2.0 + randcoord()) / (LOD + 2);
  alpha = 0.4 * 5 / size / (LOD + 2);
  if (alpha > 1.0)
    alpha = 1.0;
  size *= _scale;
  flare_max = 1.0;
  flare_exp = 0.0;
  flare_frequency = 2.0;
  state = ((rand() % 3) == 0);
}

bool CandleParticle::idle(const Uint64 delta_t)
{
  if (effect->recall)
    return false;

  const float scalar = 1.0 - math_cache.powf_05_close((interval_t)delta_t * LOD / 42000000.0);
  alpha -= scalar;

  if (alpha < 0.02)
    return false;
  
  return true;
}

GLuint CandleParticle::get_texture(const Uint16 res_index)
{
  return base->TexFlare.get_texture(res_index);
}

void CandleParticle::draw(const Uint64 usec)
{
  if (state == 0)
  {
    Particle::draw(usec);
  }
  else
  {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Particle::draw(usec);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  }
}

CandleEffect::CandleEffect(EyeCandy* _base, bool* _dead, Vec3* _pos, const float _scale, const Uint16 _LOD)
{
  if (EC_DEBUG)
    std::cout << "CandleEffect (" << this << ") created." << std::endl;
  base = _base;
  dead = _dead,
  pos = _pos;
  scale = _scale;
  sqrt_scale = fastsqrt(scale);
  LOD = base->last_forced_LOD;
  desired_LOD = _LOD;
  mover = new SmokeMover(this, sqrt_scale);
  spawner = new FilledSphereSpawner(0.015 * sqrt_scale);
}

CandleEffect::~CandleEffect()
{
  delete mover;
  delete spawner;
  if (EC_DEBUG)
    std::cout << "CandleEffect (" << this << ") destroyed." << std::endl;
}

bool CandleEffect::idle(const Uint64 usec)
{
  if ((recall) && (particles.size() == 0))
    return false;
    
  if (recall)
    return true;
    
  while (((int)particles.size() < LOD * 20) && ((math_cache.powf_0_1_rough_close(randfloat(), (LOD * 20 - particles.size()) * (interval_t)usec / 80 / square(LOD)) < 0.5) || ((int)particles.size() < LOD * 10)))
  {
    Vec3 coords = spawner->get_new_coords();
    coords.y += 0.1 * sqrt_scale;
    coords += *pos;
    Vec3 velocity;
    velocity.randomize(0.02 * sqrt_scale);
    velocity.y *= 5.0;
    velocity.y += 0.04 * sqrt_scale;
    Particle* p = new CandleParticle(this, mover, coords, velocity, scale, LOD);
    if (!base->push_back_particle(p))
      break;
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

};

#endif	// #ifdef EYE_CANDY