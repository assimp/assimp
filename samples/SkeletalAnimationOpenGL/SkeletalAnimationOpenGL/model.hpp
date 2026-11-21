// ----------------------------------------------------------------------------
// Skeletal Animation Sample using Assimp, freeglut and OpenGL 3.3 core.
//
// License: Released into the public domain.
//
// Written by Aravinda Samarathunga (I-A-S)
// ----------------------------------------------------------------------------

#pragma once

#include "math.hpp"
#include "mesh.hpp"

class Model
{
public:
    static constexpr uint32_t MAX_JOINT_COUNT = 64;
    static constexpr double DEFAULT_TICKS_PER_SECOND = 25;

    struct Joint
    {
        int32_t parent_index{-1};

        Mat4 offset_matrix{};
        Mat4 bind_transform{};
    };

    template<class T> struct AnimationKey { double time; T value; };

    struct AnimationChannel
    {
        int32_t joint_index{-1};

        std::vector<AnimationKey<Vec3>> position_keys{};
        std::vector<AnimationKey<Quat>> rotation_keys{};
        std::vector<AnimationKey<Vec3>> scaling_keys{};
    };

    struct Animation
    {
        std::string name{};
        double duration{};
        double ticks_per_second{};
        std::vector<AnimationChannel> channels{};
    };

public:
    ~Model();

    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;

    void draw();

    const Mat4* get_joint_transforms() const { return m_joint_transforms; }

private:
    Model() = default;

private:
    std::vector<Box<Mesh>> m_meshes;
    std::vector<GLuint> m_textures;
    std::vector<Joint> m_joints;
    std::vector<Animation> m_animations;
    Mat4 m_global_inverse_transform{};
    Mat4 m_joint_transforms[MAX_JOINT_COUNT];

    friend class ModelLoader;
};
