// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#include "model.hpp"

template<class T>
static T sample_keys(const std::vector<Model::AnimationKey<T>>& keys, double time)
{
    assert(!keys.empty());

    if (keys.size() == 1 || time <= keys.front().time)
        return keys.front().value;
    if (time >= keys.back().time)
        return keys.back().value;

    const auto next = std::upper_bound(keys.begin(), keys.end(), time,
        [](double t, const Model::AnimationKey<T>& key) { return t < key.time; });
    const auto prev = next - 1;

    const auto f = static_cast<float>((time - prev->time) / (next->time - prev->time));
    return interpolate(prev->value, next->value, f);
}

Model::~Model()
{
    if (!m_textures.empty())
        glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
}

void Model::update(double dt)
{
    Mat4 local[MAX_JOINT_COUNT];
    for (size_t i = 0; i < m_joints.size(); i++)
        local[i] = m_joints[i].bind_transform;

    if (m_current_animation < m_animations.size())
    {
        const auto& anim = m_animations[m_current_animation];

        if (anim.duration > 0.0)
            m_current_time = std::fmod(m_current_time + dt * anim.ticks_per_second, anim.duration);

        for (const auto& channel : anim.channels)
            local[channel.joint_index] = Mat4(
                sample_keys(channel.scaling_keys, m_current_time),
                sample_keys(channel.rotation_keys, m_current_time),
                sample_keys(channel.position_keys, m_current_time));
    }

    Mat4 global[MAX_JOINT_COUNT];
    for (size_t i = 0; i < m_joints.size(); i++)
    {
        const auto& joint = m_joints[i];
        global[i] = joint.parent_index < 0 ? local[i] : global[joint.parent_index] * local[i];

        m_joint_transforms[i] = m_global_inverse_transform * global[i] * joint.offset_matrix;
    }
}

void Model::draw()
{
    for(auto& m: m_meshes)
        m->draw();
}

Result<> Model::set_animation(size_t index)
{
    if (index >= m_animations.size())
        return "invalid animation index " + std::to_string(index);

    m_current_animation = index;
    m_current_time = 0.0;

    return Ok();
}

Result<> Model::set_animation(const std::string& name)
{
    for (size_t i = 0; i < m_animations.size(); i++)
        if (m_animations[i].name == name)
            return set_animation(i);

    return "no such animation \"" + name + "\"";
}
