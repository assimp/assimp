/*
Open Asset Import Library (assimp)
----------------------------------------------------------------------
Typed BLEND animation emit (Object TRS, pose, morph, mesh-cache MDD).
----------------------------------------------------------------------
*/

#ifndef ASSIMP_BUILD_NO_BLEND_IMPORTER

#include "BlenderAnimEmit.h"
#include <assimp/CreateAnimMesh.h>
#include <assimp/IOSystem.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <cstdio>

using namespace Assimp;
using namespace Assimp::Blender;

namespace {

float BlendEvalFCurve(const FCurve &fc, float time) {
    if (fc.bezt.empty()) {
        return 0.f;
    }
    if (time <= fc.bezt.front().vec[1][0]) {
        return fc.bezt.front().vec[1][1];
    }
    if (time >= fc.bezt.back().vec[1][0]) {
        return fc.bezt.back().vec[1][1];
    }
    for (size_t i = 1; i < fc.bezt.size(); ++i) {
        const float t0 = fc.bezt[i - 1].vec[1][0];
        const float t1 = fc.bezt[i].vec[1][0];
        if (time <= t1) {
            const float v0 = fc.bezt[i - 1].vec[1][1];
            const float v1 = fc.bezt[i].vec[1][1];
            const float u = (t1 > t0) ? ((time - t0) / (t1 - t0)) : 0.f;
            return v0 + (v1 - v0) * u;
        }
    }
    return fc.bezt.back().vec[1][1];
}

const FCurve *BlendCastFCurve(const ElemBase *e) {
    if (e == nullptr) {
        return nullptr;
    }
    if (e->dna_type != nullptr && std::strcmp(e->dna_type, "FCurve") != 0) {
        return nullptr;
    }
    return static_cast<const FCurve *>(e);
}

void BlendCollectActionFCurves(const bAction &act, std::vector<const FCurve *> &out) {
    for (std::shared_ptr<ElemBase> cur = act.curves.first; cur;) {
        const FCurve *fc = BlendCastFCurve(cur.get());
        if (fc == nullptr) {
            break;
        }
        out.push_back(fc);
        cur = fc->next;
    }
    for (const std::shared_ptr<ActionStripKeyframeData> &strip : act.strip_keyframe_data_array) {
        if (!strip) {
            continue;
        }
        for (const std::shared_ptr<ActionChannelBag> &bag : strip->channelbag_array) {
            if (!bag) {
                continue;
            }
            for (const std::shared_ptr<FCurve> &fc : bag->fcurve_array) {
                if (fc) {
                    out.push_back(fc.get());
                }
            }
        }
    }
}

aiAnimBehaviour BlendExtendToBehaviour(short extend) {
    if (extend == FCurve::Extend_Linear) {
        return aiAnimBehaviour_LINEAR;
    }
    if (extend == FCurve::Extend_Constant) {
        return aiAnimBehaviour_CONSTANT;
    }
    return aiAnimBehaviour_DEFAULT;
}

aiAnimBehaviour BlendMergeExtend(aiAnimBehaviour a, short extend) {
    const aiAnimBehaviour b = BlendExtendToBehaviour(extend);
    if (a == aiAnimBehaviour_DEFAULT) {
        return b;
    }
    if (b == aiAnimBehaviour_DEFAULT) {
        return a;
    }
    return (a == b) ? a : aiAnimBehaviour_DEFAULT;
}

void BlendIdentityM4(float m[4][4]) {
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            m[i][j] = (i == j) ? 1.f : 0.f;
        }
    }
}

void BlendQuatToMat3(float m[3][3], const float q[4]) {
    const float w = q[0], x = q[1], y = q[2], z = q[3];
    const float xx = x * x, yy = y * y, zz = z * z;
    const float xy = x * y, xz = x * z, yz = y * z;
    const float wx = w * x, wy = w * y, wz = w * z;
    m[0][0] = 1.f - 2.f * (yy + zz);
    m[0][1] = 2.f * (xy - wz);
    m[0][2] = 2.f * (xz + wy);
    m[1][0] = 2.f * (xy + wz);
    m[1][1] = 1.f - 2.f * (xx + zz);
    m[1][2] = 2.f * (yz - wx);
    m[2][0] = 2.f * (xz - wy);
    m[2][1] = 2.f * (yz + wx);
    m[2][2] = 1.f - 2.f * (xx + yy);
}

void BlendEulXYZToMat3(float m[3][3], const float eul[3]) {
    const float cx = std::cos(eul[0]), sx = std::sin(eul[0]);
    const float cy = std::cos(eul[1]), sy = std::sin(eul[1]);
    const float cz = std::cos(eul[2]), sz = std::sin(eul[2]);
    m[0][0] = cy * cz;
    m[0][1] = -cy * sz;
    m[0][2] = sy;
    m[1][0] = cz * sx * sy + cx * sz;
    m[1][1] = cx * cz - sx * sy * sz;
    m[1][2] = -cy * sx;
    m[2][0] = -cx * cz * sy + sx * sz;
    m[2][1] = cz * sx + cx * sy * sz;
    m[2][2] = cx * cy;
}

void BlendSizeToMat3(float m[3][3], const float size[3]) {
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) {
            m[i][j] = (i == j) ? size[i] : 0.f;
        }
    }
}

void BlendMulM3(float r[3][3], const float a[3][3], const float b[3][3]) {
    float t[3][3];
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) {
            t[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
        }
    }
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) {
            r[i][j] = t[i][j];
        }
    }
}

void BlendMulM4(float r[4][4], const float a[4][4], const float b[4][4]) {
    float t[4][4];
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            t[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
        }
    }
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            r[i][j] = t[i][j];
        }
    }
}

void BlendCopyMat4ToAi(aiMatrix4x4 &out, const float m[4][4]) {
    for (unsigned x = 0; x < 4; ++x) {
        for (unsigned y = 0; y < 4; ++y) {
            out[y][x] = m[x][y];
        }
    }
}

struct ObjTrsCurves {
    const FCurve *loc[3];
    const FCurve *dloc[3];
    const FCurve *eul[3];
    const FCurve *deul[3];
    const FCurve *quat[4];
    const FCurve *dquat[4];
    const FCurve *scl[3];
    const FCurve *dscl[3];
    aiAnimBehaviour pre, post;

    ObjTrsCurves() :
            pre(aiAnimBehaviour_DEFAULT),
            post(aiAnimBehaviour_DEFAULT) {
        for (int i = 0; i < 3; ++i) {
            loc[i] = dloc[i] = eul[i] = deul[i] = scl[i] = dscl[i] = nullptr;
        }
        for (int i = 0; i < 4; ++i) {
            quat[i] = dquat[i] = nullptr;
        }
    }

    bool HasAny() const {
        for (int i = 0; i < 3; ++i) {
            if (loc[i] || dloc[i] || eul[i] || deul[i] || scl[i] || dscl[i]) {
                return true;
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (quat[i] || dquat[i]) {
                return true;
            }
        }
        return false;
    }

    bool PreferQuat() const {
        for (int i = 0; i < 4; ++i) {
            if (quat[i] || dquat[i]) {
                return true;
            }
        }
        return false;
    }
};

void BlendBagObjTrs(ObjTrsCurves &c, const FCurve *fc) {
    if (fc == nullptr || fc->rna_path.empty()) {
        return;
    }
    const std::string &p = fc->rna_path;
    auto take3 = [&](const FCurve *(&arr)[3]) {
        if (fc->array_index >= 0 && fc->array_index < 3) {
            arr[fc->array_index] = fc;
            c.pre = BlendMergeExtend(c.pre, fc->extend);
            c.post = BlendMergeExtend(c.post, fc->extend);
        }
    };
    auto take4 = [&](const FCurve *(&arr)[4]) {
        if (fc->array_index >= 0 && fc->array_index < 4) {
            arr[fc->array_index] = fc;
            c.pre = BlendMergeExtend(c.pre, fc->extend);
            c.post = BlendMergeExtend(c.post, fc->extend);
        }
    };
    if (p == "location") {
        take3(c.loc);
    } else if (p == "delta_location") {
        take3(c.dloc);
    } else if (p == "rotation_euler") {
        take3(c.eul);
    } else if (p == "delta_rotation_euler") {
        take3(c.deul);
    } else if (p == "rotation_quaternion") {
        take4(c.quat);
    } else if (p == "delta_rotation_quaternion") {
        take4(c.dquat);
    } else if (p == "scale") {
        take3(c.scl);
    } else if (p == "delta_scale") {
        take3(c.dscl);
    }
}

float BlendMapStripTime(float t, const NlaStrip *strip) {
    if (strip == nullptr) {
        return t;
    }
    // DNA defaults: scale=1, actstart=0. Map action time into strip window.
    const float sc = (strip->scale != 0.f) ? strip->scale : 1.f;
    return strip->start + (t - strip->actstart) * sc;
}

aiNodeAnim *BlendBuildObjectNodeAnim(const Object &obj, const bAction &act, const NlaStrip *strip) {
    std::vector<const FCurve *> fcurves;
    BlendCollectActionFCurves(act, fcurves);
    ObjTrsCurves c;
    for (const FCurve *fc : fcurves) {
        BlendBagObjTrs(c, fc);
    }
    if (!c.HasAny()) {
        return nullptr;
    }

    std::set<float> times;
    auto collect = [&](const FCurve *fc) {
        if (!fc) {
            return;
        }
        for (const BezTriple &b : fc->bezt) {
            times.insert(BlendMapStripTime(b.vec[1][0], strip));
        }
    };
    for (int i = 0; i < 3; ++i) {
        collect(c.loc[i]);
        collect(c.dloc[i]);
        collect(c.eul[i]);
        collect(c.deul[i]);
        collect(c.scl[i]);
        collect(c.dscl[i]);
    }
    for (int i = 0; i < 4; ++i) {
        collect(c.quat[i]);
        collect(c.dquat[i]);
    }
    if (times.empty()) {
        return nullptr;
    }
    if (strip != nullptr && strip->end > strip->start) {
        // Presence: clamp to strip window without inventing blend.
        std::set<float> clipped;
        for (float t : times) {
            if (t >= strip->start && t <= strip->end) {
                clipped.insert(t);
            }
        }
        if (!clipped.empty()) {
            times.swap(clipped);
        }
    }

    std::unique_ptr<aiNodeAnim> nd(new aiNodeAnim());
    nd->mNodeName.Set(obj.id.name + 2);
    nd->mNumPositionKeys = nd->mNumRotationKeys = nd->mNumScalingKeys = static_cast<unsigned int>(times.size());
    nd->mPositionKeys = new aiVectorKey[nd->mNumPositionKeys];
    nd->mRotationKeys = new aiQuatKey[nd->mNumRotationKeys];
    nd->mScalingKeys = new aiVectorKey[nd->mNumScalingKeys];
    nd->mPreState = c.pre;
    nd->mPostState = c.post;

    unsigned int i = 0;
    for (float tOut : times) {
        // Inverse of strip map for FCurve eval (Presence: identity if no strip).
        float tEval = tOut;
        if (strip != nullptr) {
            const float sc = (strip->scale != 0.f) ? strip->scale : 1.f;
            tEval = strip->actstart + (tOut - strip->start) / sc;
        }

        float loc[3] = { obj.loc[0], obj.loc[1], obj.loc[2] };
        float dloc[3] = { obj.dloc[0], obj.dloc[1], obj.dloc[2] };
        float eul[3] = { obj.rot[0], obj.rot[1], obj.rot[2] };
        float deul[3] = { obj.drot[0], obj.drot[1], obj.drot[2] };
        float quat[4] = { obj.quat[0], obj.quat[1], obj.quat[2], obj.quat[3] };
        float dquat[4] = { obj.dquat[0], obj.dquat[1], obj.dquat[2], obj.dquat[3] };
        float scl[3] = { obj.scale[0], obj.scale[1], obj.scale[2] };
        float dscl[3] = { obj.dscale[0], obj.dscale[1], obj.dscale[2] };

        for (int a = 0; a < 3; ++a) {
            if (c.loc[a]) {
                loc[a] = BlendEvalFCurve(*c.loc[a], tEval);
            }
            if (c.dloc[a]) {
                dloc[a] = BlendEvalFCurve(*c.dloc[a], tEval);
            }
            if (c.eul[a]) {
                eul[a] = BlendEvalFCurve(*c.eul[a], tEval);
            }
            if (c.deul[a]) {
                deul[a] = BlendEvalFCurve(*c.deul[a], tEval);
            }
            if (c.scl[a]) {
                scl[a] = BlendEvalFCurve(*c.scl[a], tEval);
            }
            if (c.dscl[a]) {
                dscl[a] = BlendEvalFCurve(*c.dscl[a], tEval);
            }
        }
        for (int a = 0; a < 4; ++a) {
            if (c.quat[a]) {
                quat[a] = BlendEvalFCurve(*c.quat[a], tEval);
            }
            if (c.dquat[a]) {
                dquat[a] = BlendEvalFCurve(*c.dquat[a], tEval);
            }
        }

        const aiVector3D p(loc[0] + dloc[0], loc[1] + dloc[1], loc[2] + dloc[2]);
        const aiVector3D s(scl[0] * dscl[0], scl[1] * dscl[1], scl[2] * dscl[2]);

        aiQuaternion q;
        if (c.PreferQuat()) {
            // Blender quat is (w,x,y,z); multiply base * delta when delta curves present.
            aiQuaternion q0(quat[0], quat[1], quat[2], quat[3]);
            if (c.dquat[0] || c.dquat[1] || c.dquat[2] || c.dquat[3]) {
                aiQuaternion qd(dquat[0], dquat[1], dquat[2], dquat[3]);
                q = q0 * qd;
            } else {
                q = q0;
            }
        } else {
            float er[3] = { eul[0] + deul[0], eul[1] + deul[1], eul[2] + deul[2] };
            float rmat[3][3];
            BlendEulXYZToMat3(rmat, er);
            aiMatrix3x3 mat;
            mat.a1 = rmat[0][0];
            mat.a2 = rmat[0][1];
            mat.a3 = rmat[0][2];
            mat.b1 = rmat[1][0];
            mat.b2 = rmat[1][1];
            mat.b3 = rmat[1][2];
            mat.c1 = rmat[2][0];
            mat.c2 = rmat[2][1];
            mat.c3 = rmat[2][2];
            q = aiQuaternion(mat);
        }

        nd->mPositionKeys[i].mTime = tOut;
        nd->mPositionKeys[i].mValue = p;
        nd->mRotationKeys[i].mTime = tOut;
        nd->mRotationKeys[i].mValue = q;
        nd->mScalingKeys[i].mTime = tOut;
        nd->mScalingKeys[i].mValue = s;
        ++i;
    }
    return nd.release();
}

void BlendPushObjectActionAnim(std::vector<aiAnimation *> &anims, const Object &obj, const bAction &act,
        const NlaStrip *strip) {
    aiNodeAnim *channel = BlendBuildObjectNodeAnim(obj, act, strip);
    if (channel == nullptr) {
        return;
    }
    aiAnimation *anim = new aiAnimation();
    anim->mName.Set(act.id.name[0] ? act.id.name + 2 : (obj.id.name + 2));
    anim->mTicksPerSecond = 24.0;
    anim->mNumChannels = 1;
    anim->mChannels = new aiNodeAnim *[1];
    anim->mChannels[0] = channel;
    double tmax = 0.0;
    for (unsigned int k = 0; k < channel->mNumPositionKeys; ++k) {
        tmax = std::max(tmax, channel->mPositionKeys[k].mTime);
    }
    if (strip != nullptr && strip->end > strip->start) {
        tmax = std::max(tmax, static_cast<double>(strip->end));
    }
    anim->mDuration = tmax;
    anims.push_back(anim);
}

bool BlendParsePoseBoneRnaPath(const std::string &path, std::string &boneName, std::string &prop) {
    static const char kPrefix[] = "pose.bones[\"";
    constexpr size_t kPrefixLen = sizeof(kPrefix) - 1;
    if (path.size() <= kPrefixLen || path.compare(0, kPrefixLen, kPrefix) != 0) {
        return false;
    }
    const size_t endName = path.find("\"].", kPrefixLen);
    if (endName == std::string::npos) {
        return false;
    }
    boneName.assign(path, kPrefixLen, endName - kPrefixLen);
    prop.assign(path, endName + 3, std::string::npos);
    return !boneName.empty() && !prop.empty();
}

struct BlendPoseBoneCurves {
    const FCurve *loc[3];
    const FCurve *eul[3];
    const FCurve *quat[4];
    const FCurve *scl[3];
    aiAnimBehaviour pre, post;
    BlendPoseBoneCurves() :
            pre(aiAnimBehaviour_DEFAULT),
            post(aiAnimBehaviour_DEFAULT) {
        for (int i = 0; i < 3; ++i) {
            loc[i] = eul[i] = scl[i] = nullptr;
        }
        for (int i = 0; i < 4; ++i) {
            quat[i] = nullptr;
        }
    }
    bool HasAny() const {
        for (int i = 0; i < 3; ++i) {
            if (loc[i] || eul[i] || scl[i]) {
                return true;
            }
        }
        for (int i = 0; i < 4; ++i) {
            if (quat[i]) {
                return true;
            }
        }
        return false;
    }
    bool PreferQuat() const {
        for (int i = 0; i < 4; ++i) {
            if (quat[i]) {
                return true;
            }
        }
        return false;
    }
};

void BlendPoseBasisToMat4(const BlendPoseBoneCurves &c, float t, float r_mat[4][4]) {
    float loc[3] = { 0.f, 0.f, 0.f };
    float eul[3] = { 0.f, 0.f, 0.f };
    float quat[4] = { 1.f, 0.f, 0.f, 0.f };
    float scl[3] = { 1.f, 1.f, 1.f };
    for (int a = 0; a < 3; ++a) {
        if (c.loc[a]) {
            loc[a] = BlendEvalFCurve(*c.loc[a], t);
        }
        if (c.eul[a]) {
            eul[a] = BlendEvalFCurve(*c.eul[a], t);
        }
        if (c.scl[a]) {
            scl[a] = BlendEvalFCurve(*c.scl[a], t);
        }
    }
    for (int a = 0; a < 4; ++a) {
        if (c.quat[a]) {
            quat[a] = BlendEvalFCurve(*c.quat[a], t);
        }
    }
    float smat[3][3], rmat[3][3], tmat[3][3];
    BlendSizeToMat3(smat, scl);
    if (c.PreferQuat()) {
        BlendQuatToMat3(rmat, quat);
    } else {
        BlendEulXYZToMat3(rmat, eul);
    }
    BlendMulM3(tmat, rmat, smat);
    BlendIdentityM4(r_mat);
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < 3; ++j) {
            r_mat[i][j] = tmat[i][j];
        }
        r_mat[i][3] = loc[i];
    }
}

const Bone *BlendCastBone(const ElemBase *e) {
    if (e == nullptr) {
        return nullptr;
    }
    if (e->dna_type != nullptr && std::strcmp(e->dna_type, "Bone") != 0) {
        return nullptr;
    }
    return static_cast<const Bone *>(e);
}

bool BlendArmatureBoneRestLocal(const bArmature &arm, const std::string &boneName, float out[4][4]) {
    const Bone *found = nullptr;
    const Bone *parent = nullptr;
    std::function<bool(const Bone *, const Bone *)> walk = [&](const Bone *b, const Bone *p) -> bool {
        if (b == nullptr) {
            return false;
        }
        if (boneName == (b->name[0] ? b->name : "")) {
            found = b;
            parent = p;
            return true;
        }
        for (std::shared_ptr<ElemBase> cur = b->childbase.first; cur;) {
            const Bone *ch = BlendCastBone(cur.get());
            if (ch == nullptr) {
                break;
            }
            if (walk(ch, b)) {
                return true;
            }
            cur = ch->next;
        }
        return false;
    };
    for (std::shared_ptr<ElemBase> cur = arm.bonebase.first; cur;) {
        const Bone *b = BlendCastBone(cur.get());
        if (b == nullptr) {
            break;
        }
        if (walk(b, nullptr)) {
            break;
        }
        cur = b->next;
    }
    if (found == nullptr) {
        return false;
    }
    if (parent == nullptr) {
        for (unsigned i = 0; i < 4; ++i) {
            for (unsigned j = 0; j < 4; ++j) {
                out[i][j] = found->arm_mat[i][j];
            }
        }
        return true;
    }
    aiMatrix4x4 parentAi, boneAi;
    BlendCopyMat4ToAi(parentAi, parent->arm_mat);
    BlendCopyMat4ToAi(boneAi, found->arm_mat);
    const aiMatrix4x4 localAi = parentAi.Inverse() * boneAi;
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 4; ++j) {
            out[i][j] = localAi[j][i];
        }
    }
    return true;
}

aiNodeAnim *BlendBuildPoseBoneNodeAnim(const std::string &nodeName, const float restLocal[4][4],
        const BlendPoseBoneCurves &c, const NlaStrip *strip) {
    std::set<float> times;
    auto collect = [&](const FCurve *fc) {
        if (!fc) {
            return;
        }
        for (const BezTriple &b : fc->bezt) {
            times.insert(BlendMapStripTime(b.vec[1][0], strip));
        }
    };
    for (int i = 0; i < 3; ++i) {
        collect(c.loc[i]);
        collect(c.eul[i]);
        collect(c.scl[i]);
    }
    for (int i = 0; i < 4; ++i) {
        collect(c.quat[i]);
    }
    if (times.empty()) {
        return nullptr;
    }
    if (strip != nullptr && strip->end > strip->start) {
        std::set<float> clipped;
        for (float t : times) {
            if (t >= strip->start && t <= strip->end) {
                clipped.insert(t);
            }
        }
        if (!clipped.empty()) {
            times.swap(clipped);
        }
    }

    std::unique_ptr<aiNodeAnim> nd(new aiNodeAnim());
    nd->mNodeName.Set(nodeName);
    nd->mNumPositionKeys = nd->mNumRotationKeys = nd->mNumScalingKeys = static_cast<unsigned int>(times.size());
    nd->mPositionKeys = new aiVectorKey[nd->mNumPositionKeys];
    nd->mRotationKeys = new aiQuatKey[nd->mNumRotationKeys];
    nd->mScalingKeys = new aiVectorKey[nd->mNumScalingKeys];
    nd->mPreState = c.pre;
    nd->mPostState = c.post;

    unsigned idx = 0;
    for (float tOut : times) {
        float tEval = tOut;
        if (strip != nullptr) {
            const float sc = (strip->scale != 0.f) ? strip->scale : 1.f;
            tEval = strip->actstart + (tOut - strip->start) / sc;
        }
        float pose[4][4], composed[4][4];
        BlendPoseBasisToMat4(c, tEval, pose);
        BlendMulM4(composed, restLocal, pose);
        aiMatrix4x4 ai;
        BlendCopyMat4ToAi(ai, composed);
        aiVector3D p, s;
        aiQuaternion q;
        ai.Decompose(s, q, p);
        nd->mPositionKeys[idx].mTime = tOut;
        nd->mPositionKeys[idx].mValue = p;
        nd->mRotationKeys[idx].mTime = tOut;
        nd->mRotationKeys[idx].mValue = q;
        nd->mScalingKeys[idx].mTime = tOut;
        nd->mScalingKeys[idx].mValue = s;
        ++idx;
    }
    return nd.release();
}

void BlendPushArmatureActionAnim(std::vector<aiAnimation *> &anims, const Object &obj, const bAction &act,
        ConversionData &conv_data, const NlaStrip *strip) {
    if (!obj.data || (obj.data->dna_type != nullptr && std::strcmp(obj.data->dna_type, "bArmature") != 0)) {
        return;
    }
    const bArmature *arm = static_cast<const bArmature *>(obj.data.get());
    const std::string armName = obj.id.name + 2;
    const auto nameIt = conv_data.armatureBoneNodeNames.find(armName);
    if (nameIt == conv_data.armatureBoneNodeNames.end() || nameIt->second.empty()) {
        return;
    }

    std::vector<const FCurve *> fcurves;
    BlendCollectActionFCurves(act, fcurves);
    if (fcurves.empty()) {
        return;
    }

    std::unordered_map<std::string, BlendPoseBoneCurves> byBone;
    for (const FCurve *fc : fcurves) {
        if (fc == nullptr) {
            continue;
        }
        std::string boneName, prop;
        if (!BlendParsePoseBoneRnaPath(fc->rna_path, boneName, prop)) {
            continue;
        }
        if (nameIt->second.find(boneName) == nameIt->second.end()) {
            continue;
        }
        BlendPoseBoneCurves &bag = byBone[boneName];
        if (prop == "location" && fc->array_index >= 0 && fc->array_index < 3) {
            bag.loc[fc->array_index] = fc;
            bag.pre = BlendMergeExtend(bag.pre, fc->extend);
            bag.post = BlendMergeExtend(bag.post, fc->extend);
        } else if (prop == "rotation_euler" && fc->array_index >= 0 && fc->array_index < 3) {
            bag.eul[fc->array_index] = fc;
            bag.pre = BlendMergeExtend(bag.pre, fc->extend);
            bag.post = BlendMergeExtend(bag.post, fc->extend);
        } else if (prop == "rotation_quaternion" && fc->array_index >= 0 && fc->array_index < 4) {
            bag.quat[fc->array_index] = fc;
            bag.pre = BlendMergeExtend(bag.pre, fc->extend);
            bag.post = BlendMergeExtend(bag.post, fc->extend);
        } else if (prop == "scale" && fc->array_index >= 0 && fc->array_index < 3) {
            bag.scl[fc->array_index] = fc;
            bag.pre = BlendMergeExtend(bag.pre, fc->extend);
            bag.post = BlendMergeExtend(bag.post, fc->extend);
        }
    }
    if (byBone.empty()) {
        return;
    }

    std::vector<aiNodeAnim *> channels;
    double tmax = 0.0;
    for (const auto &kv : byBone) {
        float restLocal[4][4];
        if (!BlendArmatureBoneRestLocal(*arm, kv.first, restLocal)) {
            continue;
        }
        const auto nodeNameIt = nameIt->second.find(kv.first);
        if (nodeNameIt == nameIt->second.end()) {
            continue;
        }
        aiNodeAnim *ch = BlendBuildPoseBoneNodeAnim(nodeNameIt->second, restLocal, kv.second, strip);
        if (ch == nullptr) {
            continue;
        }
        for (unsigned int k = 0; k < ch->mNumPositionKeys; ++k) {
            tmax = std::max(tmax, ch->mPositionKeys[k].mTime);
        }
        channels.push_back(ch);
    }
    if (channels.empty()) {
        return;
    }
    if (strip != nullptr && strip->end > strip->start) {
        tmax = std::max(tmax, static_cast<double>(strip->end));
    }

    aiAnimation *anim = new aiAnimation();
    anim->mName.Set(act.id.name[0] ? act.id.name + 2 : armName.c_str());
    anim->mTicksPerSecond = 24.0;
    anim->mDuration = tmax;
    anim->mNumChannels = static_cast<unsigned int>(channels.size());
    anim->mChannels = new aiNodeAnim *[anim->mNumChannels];
    std::copy(channels.begin(), channels.end(), anim->mChannels);
    anims.push_back(anim);
}

void BlendForEachObjectAction(const Object &obj,
        const std::function<void(const std::shared_ptr<bAction> &, const NlaStrip *)> &fn) {
    if (!obj.adt) {
        return;
    }
    std::set<std::pair<const bAction *, const NlaStrip *>> seen;
    auto tryAct = [&](const std::shared_ptr<bAction> &act, const NlaStrip *strip) {
        if (!act || !seen.insert({ act.get(), strip }).second) {
            return;
        }
        fn(act, strip);
    };
    tryAct(obj.adt->action, nullptr);
    for (std::shared_ptr<ElemBase> tcur = obj.adt->nla_tracks.first; tcur;) {
        const NlaTrack *track = nullptr;
        if (tcur->dna_type == nullptr || std::strcmp(tcur->dna_type, "NlaTrack") == 0) {
            track = static_cast<const NlaTrack *>(tcur.get());
        }
        if (track == nullptr) {
            break;
        }
        for (std::shared_ptr<ElemBase> scur = track->strips.first; scur;) {
            const NlaStrip *strip = nullptr;
            if (scur->dna_type == nullptr || std::strcmp(scur->dna_type, "NlaStrip") == 0) {
                strip = static_cast<const NlaStrip *>(scur.get());
            }
            if (strip == nullptr) {
                break;
            }
            tryAct(strip->act, strip);
            scur = strip->next;
        }
        tcur = track->next;
    }
}

std::vector<std::string> BlendCollectVGroupNames(const Object &obj, const Mesh &mesh) {
    std::vector<std::string> names;
    auto pushList = [&](const ListBase &lb) {
        for (std::shared_ptr<ElemBase> cur = lb.first; cur;) {
            const bDeformGroup *dg = nullptr;
            if (cur->dna_type == nullptr || std::strcmp(cur->dna_type, "bDeformGroup") == 0) {
                dg = static_cast<const bDeformGroup *>(cur.get());
            }
            if (dg == nullptr) {
                break;
            }
            names.emplace_back(dg->name);
            cur = dg->next;
        }
    };
    if (mesh.vertex_group_names.first) {
        pushList(mesh.vertex_group_names);
    } else if (obj.defbase.first) {
        pushList(obj.defbase);
    }
    return names;
}

const ArmatureModifierData *BlendFindArmatureModifier(const Object &obj) {
    const SharedModifierData *cur = static_cast<const SharedModifierData *>(obj.modifiers.first.get());
    for (; cur; cur = static_cast<const SharedModifierData *>(cur->modifier.next.get())) {
        if (cur->dna_type != nullptr && std::strcmp(cur->dna_type, "ArmatureModifierData") == 0) {
            return static_cast<const ArmatureModifierData *>(cur);
        }
        if (cur->modifier.type == ModifierData::eModifierType_Armature &&
                cur->dna_type != nullptr && std::strcmp(cur->dna_type, "GenericModifierData") == 0) {
            // Header-only: cannot resolve Object* - skip (no invent).
            continue;
        }
    }
    return nullptr;
}

const MeshCacheModifierData *BlendFindMeshCacheModifier(const Object &obj) {
    const SharedModifierData *cur = static_cast<const SharedModifierData *>(obj.modifiers.first.get());
    for (; cur; cur = static_cast<const SharedModifierData *>(cur->modifier.next.get())) {
        if (cur->dna_type != nullptr && std::strcmp(cur->dna_type, "MeshCacheModifierData") == 0) {
            return static_cast<const MeshCacheModifierData *>(cur);
        }
    }
    return nullptr;
}

std::string BlendResolveBlendRelativePath(const std::string &blendDir, const char *filepath) {
    if (filepath == nullptr || filepath[0] == '\0') {
        return {};
    }
    std::string p(filepath);
    if (p.size() >= 2 && p[0] == '/' && p[1] == '/') {
        p = p.substr(2);
        if (!blendDir.empty()) {
            return blendDir + "/" + p;
        }
        return p;
    }
    return p;
}

bool BlendReadMddFile(const std::string &path, unsigned expectVerts,
        std::vector<float> &times, std::vector<std::vector<aiVector3D>> &frames) {
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in) {
        return false;
    }
    auto rdBeU32 = [&](uint32_t &v) -> bool {
        unsigned char b[4];
        if (!in.read(reinterpret_cast<char *>(b), 4)) {
            return false;
        }
        v = (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) | (uint32_t(b[2]) << 8) | uint32_t(b[3]);
        return true;
    };
    auto rdBeF32 = [&](float &v) -> bool {
        uint32_t u = 0;
        if (!rdBeU32(u)) {
            return false;
        }
        std::memcpy(&v, &u, sizeof v);
        return true;
    };
    uint32_t nFrames = 0, nVerts = 0;
    if (!rdBeU32(nFrames) || !rdBeU32(nVerts)) {
        return false;
    }
    if (nVerts != expectVerts || nFrames == 0) {
        return false;
    }
    times.resize(nFrames);
    for (uint32_t i = 0; i < nFrames; ++i) {
        if (!rdBeF32(times[i])) {
            return false;
        }
    }
    frames.resize(nFrames);
    for (uint32_t f = 0; f < nFrames; ++f) {
        frames[f].resize(nVerts);
        for (uint32_t v = 0; v < nVerts; ++v) {
            float x, y, z;
            if (!rdBeF32(x) || !rdBeF32(y) || !rdBeF32(z)) {
                return false;
            }
            frames[f][v] = aiVector3D(x, y, z);
        }
    }
    return true;
}

bool BlendParseShapeKeyValuePath(const std::string &path, std::string &keyName) {
    // data.shape_keys.key_blocks["Name"].value  OR  key_blocks["Name"].value
    const char *markers[] = { "key_blocks[\"", "key_blocks['" };
    for (const char *m : markers) {
        const size_t pos = path.find(m);
        if (pos == std::string::npos) {
            continue;
        }
        const size_t start = pos + std::strlen(m);
        const char endCh = (m[std::strlen(m) - 1] == '"') ? '"' : '\'';
        const size_t end = path.find(endCh, start);
        if (end == std::string::npos) {
            continue;
        }
        if (path.find(".value", end) == std::string::npos) {
            continue;
        }
        keyName.assign(path, start, end - start);
        return !keyName.empty();
    }
    return false;
}

} // namespace

void Assimp::Blender::BlendConvertObjectAnimations(std::vector<aiAnimation *> &anims, ConversionData &conv_data) {
    if (conv_data.trs_anim_objects.empty()) {
        return;
    }
    for (const Object *obj : conv_data.trs_anim_objects) {
        if (obj == nullptr) {
            continue;
        }
        BlendForEachObjectAction(*obj, [&](const std::shared_ptr<bAction> &act, const NlaStrip *strip) {
            BlendPushObjectActionAnim(anims, *obj, *act, strip);
        });
    }
}

void Assimp::Blender::BlendConvertArmatureAnimations(std::vector<aiAnimation *> &anims, ConversionData &conv_data) {
    if (conv_data.armature_objects.empty()) {
        return;
    }
    for (const Object *obj : conv_data.armature_objects) {
        if (obj == nullptr) {
            continue;
        }
        BlendForEachObjectAction(*obj, [&](const std::shared_ptr<bAction> &act, const NlaStrip *strip) {
            BlendPushArmatureActionAnim(anims, *obj, *act, conv_data, strip);
        });
    }
}

void Assimp::Blender::BlendConvertShapeKeyAnimations(std::vector<aiAnimation *> &anims, ConversionData &conv_data) {
    for (const ConversionData::ShapeKeyMeshRef &ref : conv_data.shapeKeyMeshes) {
        if (ref.key == nullptr || ref.keyNames.empty()) {
            continue;
        }
        std::vector<const FCurve *> fcurves;
        auto collectFromAdt = [&](const std::shared_ptr<AnimData> &adt) {
            if (!adt) {
                return;
            }
            auto grab = [&](const std::shared_ptr<bAction> &act) {
                if (!act) {
                    return;
                }
                BlendCollectActionFCurves(*act, fcurves);
            };
            grab(adt->action);
            for (std::shared_ptr<ElemBase> tcur = adt->nla_tracks.first; tcur;) {
                const NlaTrack *track = (tcur->dna_type == nullptr || std::strcmp(tcur->dna_type, "NlaTrack") == 0) ?
                                                static_cast<const NlaTrack *>(tcur.get()) :
                                                nullptr;
                if (track == nullptr) {
                    break;
                }
                for (std::shared_ptr<ElemBase> scur = track->strips.first; scur;) {
                    const NlaStrip *strip = (scur->dna_type == nullptr || std::strcmp(scur->dna_type, "NlaStrip") == 0) ?
                                                   static_cast<const NlaStrip *>(scur.get()) :
                                                   nullptr;
                    if (strip == nullptr) {
                        break;
                    }
                    grab(strip->act);
                    scur = strip->next;
                }
                tcur = track->next;
            }
        };
        collectFromAdt(ref.key->adt);
        if (ref.obj) {
            collectFromAdt(ref.obj->adt);
        }

        std::unordered_map<std::string, const FCurve *> byKey;
        for (const FCurve *fc : fcurves) {
            if (fc == nullptr) {
                continue;
            }
            std::string kn;
            if (!BlendParseShapeKeyValuePath(fc->rna_path, kn)) {
                continue;
            }
            byKey[kn] = fc;
        }
        if (byKey.empty()) {
            continue;
        }

        std::set<float> times;
        for (const auto &kv : byKey) {
            for (const BezTriple &b : kv.second->bezt) {
                times.insert(b.vec[1][0]);
            }
        }
        if (times.empty()) {
            continue;
        }

        std::unique_ptr<aiMeshMorphAnim> morph(new aiMeshMorphAnim());
        morph->mName.Set(ref.meshName);
        morph->mNumKeys = static_cast<unsigned int>(times.size());
        morph->mKeys = new aiMeshMorphKey[morph->mNumKeys];
        unsigned ki = 0;
        double tmax = 0.0;
        for (float t : times) {
            aiMeshMorphKey &mk = morph->mKeys[ki++];
            mk.mTime = t;
            mk.mNumValuesAndWeights = static_cast<unsigned int>(ref.keyNames.size());
            mk.mValues = new unsigned int[mk.mNumValuesAndWeights];
            mk.mWeights = new double[mk.mNumValuesAndWeights];
            for (unsigned ti = 0; ti < mk.mNumValuesAndWeights; ++ti) {
                mk.mValues[ti] = ti;
                mk.mWeights[ti] = 0.0;
                const auto it = byKey.find(ref.keyNames[ti]);
                if (it != byKey.end()) {
                    mk.mWeights[ti] = BlendEvalFCurve(*it->second, t);
                }
            }
            tmax = std::max(tmax, static_cast<double>(t));
        }

        aiAnimation *anim = new aiAnimation();
        anim->mName.Set(ref.meshName + "_ShapeKeys");
        anim->mTicksPerSecond = 24.0;
        anim->mDuration = tmax;
        anim->mNumMorphMeshChannels = 1;
        anim->mMorphMeshChannels = new aiMeshMorphAnim *[1];
        anim->mMorphMeshChannels[0] = morph.release();
        anims.push_back(anim);
    }
}

void Assimp::Blender::BlendAttachMeshBones(aiMesh *out, const Object &obj, const Mesh &mesh,
        ConversionData &conv_data, const std::vector<int> &outToSrcVert) {
    if (out == nullptr || mesh.dvert.empty() || outToSrcVert.empty()) {
        return;
    }
    const ArmatureModifierData *amd = BlendFindArmatureModifier(obj);
    if (amd == nullptr) {
        return;
    }
    const std::shared_ptr<Object> armOb = amd->object.lock();
    const Object *armObj = armOb.get();
    if (armObj == nullptr || !armObj->data ||
            (armObj->data->dna_type != nullptr && std::strcmp(armObj->data->dna_type, "bArmature") != 0)) {
        return;
    }
    const bArmature *arm = static_cast<const bArmature *>(armObj->data.get());
    const std::vector<std::string> vgroups = BlendCollectVGroupNames(obj, mesh);
    if (vgroups.empty()) {
        return;
    }

    // Map bone name -> rest arm_mat
    std::unordered_map<std::string, const Bone *> bones;
    std::function<void(const Bone *)> walk = [&](const Bone *b) {
        if (b == nullptr) {
            return;
        }
        bones[b->name] = b;
        for (std::shared_ptr<ElemBase> cur = b->childbase.first; cur;) {
            const Bone *ch = BlendCastBone(cur.get());
            if (ch == nullptr) {
                break;
            }
            walk(ch);
            cur = ch->next;
        }
    };
    for (std::shared_ptr<ElemBase> cur = arm->bonebase.first; cur;) {
        const Bone *b = BlendCastBone(cur.get());
        if (b == nullptr) {
            break;
        }
        walk(b);
        cur = b->next;
    }

    std::vector<std::vector<aiVertexWeight>> weights(vgroups.size());
    for (unsigned outVi = 0; outVi < outToSrcVert.size() && outVi < out->mNumVertices; ++outVi) {
        const int src = outToSrcVert[outVi];
        if (src < 0 || static_cast<size_t>(src) >= mesh.dvert.size()) {
            continue;
        }
        const MDeformVert &dv = mesh.dvert[static_cast<size_t>(src)];
        for (const MDeformWeight &w : dv.dw) {
            if (w.def_nr < 0 || static_cast<size_t>(w.def_nr) >= vgroups.size()) {
                continue;
            }
            if (w.weight <= 0.f) {
                continue;
            }
            weights[static_cast<size_t>(w.def_nr)].push_back(aiVertexWeight(outVi, w.weight));
        }
    }

    std::vector<aiBone *> boneList;
    for (size_t gi = 0; gi < vgroups.size(); ++gi) {
        if (weights[gi].empty()) {
            continue;
        }
        const auto bit = bones.find(vgroups[gi]);
        if (bit == bones.end()) {
            continue; // VGroup without matching bone - skip (no invent)
        }
        aiBone *bone = new aiBone();
        // Prefer armature-qualified node name when present.
        const std::string armName = armObj->id.name + 2;
        const auto nameMap = conv_data.armatureBoneNodeNames.find(armName);
        if (nameMap != conv_data.armatureBoneNodeNames.end()) {
            const auto nn = nameMap->second.find(vgroups[gi]);
            if (nn != nameMap->second.end()) {
                bone->mName.Set(nn->second);
            } else {
                bone->mName.Set(vgroups[gi]);
            }
        } else {
            bone->mName.Set(vgroups[gi]);
        }
        bone->mNumWeights = static_cast<unsigned int>(weights[gi].size());
        bone->mWeights = new aiVertexWeight[bone->mNumWeights];
        std::copy(weights[gi].begin(), weights[gi].end(), bone->mWeights);
        // Inverse bind: inverse(bone.arm_mat) from DNA rest (armature space).
        aiMatrix4x4 armMat;
        BlendCopyMat4ToAi(armMat, bit->second->arm_mat);
        bone->mOffsetMatrix = armMat.Inverse();
        boneList.push_back(bone);
    }
    if (boneList.empty()) {
        return;
    }
    out->mNumBones = static_cast<unsigned int>(boneList.size());
    out->mBones = new aiBone *[out->mNumBones];
    std::copy(boneList.begin(), boneList.end(), out->mBones);
}

void Assimp::Blender::BlendAttachShapeKeyTargets(aiMesh *out, const Object *obj, const Mesh &mesh,
        const std::vector<int> &outToSrcVert, ConversionData &conv_data) {
    if (out == nullptr || !mesh.key || outToSrcVert.empty()) {
        return;
    }
    const Key &key = *mesh.key;
    std::vector<const KeyBlock *> blocks;
    for (std::shared_ptr<ElemBase> cur = key.block.first; cur;) {
        const KeyBlock *kb = nullptr;
        if (cur->dna_type == nullptr || std::strcmp(cur->dna_type, "KeyBlock") == 0) {
            kb = static_cast<const KeyBlock *>(cur.get());
        }
        if (kb == nullptr) {
            break;
        }
        blocks.push_back(kb);
        cur = kb->next;
    }
    if (blocks.size() < 2) {
        return; // need basis + at least one target
    }

    std::vector<aiAnimMesh *> targets;
    std::vector<std::string> names;
    // Skip basis (first block).
    for (size_t bi = 1; bi < blocks.size(); ++bi) {
        const KeyBlock *kb = blocks[bi];
        if (kb->co.size() < static_cast<size_t>(mesh.totvert) * 3u) {
            continue; // incomplete DNA - skip (no invent)
        }
        aiAnimMesh *am = aiCreateAnimMesh(out);
        if (am == nullptr) {
            continue;
        }
        am->mName.Set(kb->name);
        am->mWeight = kb->curval;
        for (unsigned vi = 0; vi < out->mNumVertices && vi < outToSrcVert.size(); ++vi) {
            const int src = outToSrcVert[vi];
            if (src < 0 || src >= mesh.totvert) {
                continue;
            }
            const size_t o = static_cast<size_t>(src) * 3u;
            am->mVertices[vi] = aiVector3D(kb->co[o], kb->co[o + 1], kb->co[o + 2]);
        }
        targets.push_back(am);
        names.emplace_back(kb->name);
    }
    if (targets.empty()) {
        return;
    }
    out->mNumAnimMeshes = static_cast<unsigned int>(targets.size());
    out->mAnimMeshes = new aiAnimMesh *[out->mNumAnimMeshes];
    std::copy(targets.begin(), targets.end(), out->mAnimMeshes);

    ConversionData::ShapeKeyMeshRef ref;
    ref.meshName = out->mName.C_Str();
    ref.keyNames = std::move(names);
    ref.key = mesh.key.get();
    ref.obj = obj;
    conv_data.shapeKeyMeshes.push_back(std::move(ref));
}

void Assimp::Blender::BlendAttachMeshCacheAnim(aiMesh *out, const Object &obj, const Mesh &mesh,
        ConversionData &conv_data, const std::vector<int> &outToSrcVert,
        std::vector<aiAnimation *> &anims) {
    if (out == nullptr || outToSrcVert.empty()) {
        return;
    }
    const MeshCacheModifierData *mc = BlendFindMeshCacheModifier(obj);
    if (mc == nullptr || mc->filepath[0] == '\0') {
        return;
    }
    // Only MDD is mapped to typed aiMeshAnim here; PC2 left unresolved (no invent).
    if (mc->type != MeshCacheModifierData::Type_MDD) {
        return;
    }
    const std::string path = BlendResolveBlendRelativePath(conv_data.blendDirectory, mc->filepath);
    if (path.empty()) {
        return;
    }
    std::vector<float> times;
    std::vector<std::vector<aiVector3D>> frames;
    if (!BlendReadMddFile(path, static_cast<unsigned>(mesh.totvert), times, frames)) {
        return;
    }
    if (times.size() != frames.size() || frames.empty()) {
        return;
    }

    // Do not destroy existing shape-key targets; only emit MDD when no anim meshes yet.
    if (out->mNumAnimMeshes > 0) {
        return;
    }

    out->mNumAnimMeshes = static_cast<unsigned int>(frames.size());
    out->mAnimMeshes = new aiAnimMesh *[out->mNumAnimMeshes];
    for (unsigned fi = 0; fi < out->mNumAnimMeshes; ++fi) {
        aiAnimMesh *am = aiCreateAnimMesh(out);
        for (unsigned vi = 0; vi < out->mNumVertices && vi < outToSrcVert.size(); ++vi) {
            const int src = outToSrcVert[vi];
            if (src < 0 || static_cast<size_t>(src) >= frames[fi].size()) {
                continue;
            }
            am->mVertices[vi] = frames[fi][static_cast<size_t>(src)];
        }
        char nm[64];
        std::snprintf(nm, sizeof nm, "MDD_%u", fi);
        am->mName.Set(nm);
        out->mAnimMeshes[fi] = am;
    }

    aiMeshAnim *ma = new aiMeshAnim();
    ma->mName = out->mName;
    ma->mNumKeys = out->mNumAnimMeshes;
    ma->mKeys = new aiMeshKey[ma->mNumKeys];
    double tmax = 0.0;
    for (unsigned i = 0; i < ma->mNumKeys; ++i) {
        // MDD times are seconds; convert to frames at 24 fps for tick consistency.
        const double t = static_cast<double>(times[i]) * 24.0;
        ma->mKeys[i].mTime = t;
        ma->mKeys[i].mValue = i;
        tmax = std::max(tmax, t);
    }
    aiAnimation *anim = new aiAnimation();
    anim->mName.Set(std::string(out->mName.C_Str()) + "_MeshCache");
    anim->mTicksPerSecond = 24.0;
    anim->mDuration = tmax;
    anim->mNumMeshChannels = 1;
    anim->mMeshChannels = new aiMeshAnim *[1];
    anim->mMeshChannels[0] = ma;
    anims.push_back(anim);
}

#endif // ASSIMP_BUILD_NO_BLEND_IMPORTER
