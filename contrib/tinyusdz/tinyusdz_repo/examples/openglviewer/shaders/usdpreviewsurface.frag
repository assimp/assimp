// Simple UsdPreviewSurface fragment shader.

// TODO
// - [ ] normal mapping
// - [ ] displacement mapping(mesh shader?)

in vec2 interpolated_uv0;

uniform vec3 baseColor;
uniform sampler2D baseColorTex;
uniform mat3 baseColorTexTransform;

uniform vec3      emissiveColor;
uniform sampler2D emissiveColorTex;
uniform mat3      emissiveColorTexTransform;

uniform vec3      specularColor;
uniform sampler2D specularColorTex;
uniform mat3      specularColorTexTransform;

uniform vec3      metallic;
uniform sampler2D metallicTex;
uniform mat3      metallicTexTransform;

uniform vec3      roughness;
uniform sampler2D roughnessTex;
uniform mat3      roughnessTexTransform;

uniform vec3      clearcoat;
uniform sampler2D clearcoatTex;
uniform mat3      clearcoatTexTransform;

uniform vec3      clearcoatRoughness;
uniform sampler2D clearcoatRoughnessTex;
uniform mat3      clearcoatRoughnessTexTransform;

uniform vec3      ior;
uniform sampler2D iorTex;
uniform mat3      iorTexTransform;

uniform float     occlusion;
uniform sampler2D occlusionTex;
uniform mat3      occlusionTexTransform;

uniform float     opacity;
uniform sampler2D opacityTex;
uniform mat3      opacityTexTransform;

uniform float     opacityThreshold;
uniform sampler2D opacityThresholdTex;
uniform mat3      opacityThresholdTexTransform;

uniform int useSpecularWorkflow;

out vec4 output_color;

void main()
{
  vec3 uv0 = vec3(interpolated_uv0, 1.0);
  vec4 baseColorResult = texture(baseColorTex, (baseColorTexTransform * uv0).xy) + vec4(baseColor, 1.0);

  output_color = baseColorResult;
}


