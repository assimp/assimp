#define ALPHA_OPAQUE 0
#define ALPHA_MASK 1
#define ALPHA_BLEND 2

out vec4 output_color;
in float selected;

in vec3 interpolated_normal;
in vec3 fragment_world_position;
in vec2 interpolated_uv;
in vec4 interpolated_weights;
in vec4 interpolated_colors;

//texture maps
uniform sampler2D normal_texture;
uniform sampler2D occlusion_texture;
uniform sampler2D emissive_texture;
uniform sampler2D base_color_texture;
uniform sampler2D metallic_roughness_texture;

//TODO BRDF lookup table here
uniform sampler2D brdf_lut;

uniform vec4 base_color_factor;
uniform float metallic_factor;
uniform float roughness_factor;
uniform vec3 emissive_factor;
uniform int alpha_mode;
uniform float alpha_cutoff;

uniform vec3 camera_position;
uniform vec3 light_direction;
uniform vec3 light_color;

uniform vec4 highlight_color;


//To hold the data during computation
struct pbr_info
{
	float n_dot_l;
	float n_dot_v;
	float n_dot_h;
	float l_dot_h;
	float v_dot_h;
	float preceptual_roughness;
	float metalnes;
	vec3 reflect_0;
	vec3 reflect_90;
	float alpha_roughness;
	vec3 diffuse_color;
	vec3 specular_color;
};

const float PI =  3.141592653589793;
const float min_roughness = 0.04;

mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{

 vec3 dp1 = dFdx( p );
 vec3 dp2 = dFdy( p );
 vec2 duv1 = dFdx( uv );
 vec2 duv2 = dFdy( uv );

 vec3 dp2perp = cross( dp2, N );
 vec3 dp1perp = cross( N, dp1 );


 vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
 vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

 float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
 return mat3( T * invmax, B * invmax, N );

}

// Perturb normal, see http://www.thetenthplanet.de/archives/1180
vec3 perturb_normal(vec3 N, vec3 V)
{
	vec3 sampled_normal_map = texture(normal_texture, interpolated_uv).rgb * 255.f/127.f - 128.f/127.f;
	sampled_normal_map.y = - sampled_normal_map.y;

	mat3 TBN = cotangent_frame(N, -V, interpolated_uv);
	return normalize(TBN * sampled_normal_map);
}

//TODO divide or don't divide by pi?
vec3 diffuse(pbr_info pbr_inputs)
{
	//return pbr_inputs.diffuse_color.rgb / PI;
	return pbr_inputs.diffuse_color.rgb;
}

vec3 specular_reflection(pbr_info pbr_inputs)
{
 return pbr_inputs.reflect_0 + (pbr_inputs.reflect_90 - pbr_inputs.reflect_0) 
 * pow(clamp( 1.0 - pbr_inputs.v_dot_h, 0.0, 1.0), 5.0 );
}

float geometric_occlusion(pbr_info pbr_inputs)
{
	//copy input parameters;
	float n_dot_l = pbr_inputs.n_dot_l;
	float n_dot_v = pbr_inputs.n_dot_v;
	float r = pbr_inputs.alpha_roughness;

	//calculate attenuation
	float att_l = 2.0 * n_dot_l / (n_dot_l + sqrt(r*r * (1.0 - r*r) * (n_dot_l * n_dot_l)));
	float att_v = 2.0 * n_dot_v / (n_dot_v + sqrt(r*r * (1.0 - r*r) * (n_dot_v * n_dot_v)));
	return att_l * att_v;
}

float microfaced_distribution(pbr_info pbr_inputs)
{
	float r_sq = pbr_inputs.alpha_roughness * pbr_inputs.alpha_roughness;
	float f = (pbr_inputs.n_dot_h * r_sq - pbr_inputs.n_dot_h) * pbr_inputs.n_dot_h +1.0f;
	return r_sq / (PI * f * f);
}


//IBL / ENV LIGHTING 
uniform bool use_ibl;
uniform float gamma;
uniform float exposure;

vec4 SRGB_to_LINEAR(vec4 srgb)
{
	return vec4(pow(srgb.rgb, vec3(gamma)), srgb.a);
}

vec3 uncharted2_tonemap(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec4 tonemap(vec4 color)
{
	vec3 outcol = uncharted2_tonemap(color.rgb * exposure);
	outcol = outcol * (1.0f / uncharted2_tonemap(vec3(11.2f)));	
	return vec4(pow(outcol, vec3(1.0f /gamma)), color.a);
}


uniform samplerCube env_irradiance;
uniform samplerCube env_prefiltered_specular;
uniform float prefiltered_cube_mip_levels;

// Calculation of the lighting contribution from an optional Image Based Light source.
// Precomputed Environment Maps are required uniform inputs and are computed as outlined in [1].
// See our README.md on Environment Maps [3] for additional discussion.
vec3 get_IBL_contribution(pbr_info pbr_inputs, vec3 n, vec3 reflection)
{
	float lod = (pbr_inputs.preceptual_roughness * prefiltered_cube_mip_levels);
	// retrieve a scale and bias to F0. See [1], Figure 3
	vec3 brdf = (texture(brdf_lut, vec2(pbr_inputs.n_dot_v, 1.0 - pbr_inputs.preceptual_roughness))).rgb;
	
	
	vec3 diffuseLight = SRGB_to_LINEAR(tonemap(texture(env_irradiance, n))).rgb;
	vec3 specularLight = SRGB_to_LINEAR(tonemap(textureLod(env_prefiltered_specular, reflection, lod))).rgb;

	vec3 diffuse = diffuseLight * pbr_inputs.diffuse_color;
	vec3 specular = specularLight * (pbr_inputs.specular_color * brdf.x + brdf.y);

	// For presentation, this allows us to disable IBL terms
	// For presentation, this allows us to disable IBL terms
	
	//diffuse *= uboParams.scaleIBLAmbient;
	//specular *= uboParams.scaleIBLAmbient;

	return diffuse + specular;
}

void main()
{
	
	//sample the base_color texture. //TODO SRGB color space.
	vec4 base_color = interpolated_colors * base_color_factor * texture(base_color_texture, interpolated_uv);

	if(alpha_mode == ALPHA_MASK)
	{
		if(base_color.a < alpha_cutoff)
			discard;
	}

	//Get material phycisal properties : how much metallic it is, and the surface
	//roughness
	float metallic;
	float perceptual_roughness;
	vec4 physics_sample = texture(metallic_roughness_texture, interpolated_uv);
	metallic = metallic_factor * physics_sample.b;
	perceptual_roughness = clamp(roughness_factor * physics_sample.g, min_roughness, 1.0);
	float alpha_roughness = perceptual_roughness * perceptual_roughness;

	vec3 f0 = vec3(0.04f); //frenel factor
	vec3 diffuse_color = base_color.rgb * (vec3(1.0f) - f0); 
	diffuse_color *= 1.0f - metallic;
	vec3 specular_color = mix(f0, base_color.rgb, metallic);


	//surface reflectance
	float reflectance = max(max(specular_color.r, specular_color.g), specular_color.b);

	float reflectance90 = clamp(reflectance * 25.0f, 0.0f, 1.f);
	vec3 specular_env_r0 = specular_color.rgb;
	vec3 specular_env_r90 =  vec3(1.0f, 1.0f, 1.0f) * reflectance90;

	//vec3 n = normalize(interpolated_normal);
	vec3 v = normalize(camera_position - fragment_world_position);
	vec3 n = perturb_normal(normalize(interpolated_normal), v); //TODO fix my tangent space for normal mapping!
	vec3 l = normalize(-light_direction);
	vec3 h = normalize(l+v);
	vec3 reflection = -normalize(reflect(v, n));
	reflection.y *= -1.0f;
	
	float n_dot_l = clamp(dot(n, l), 0.001, 1.0);
	float n_dot_v = clamp(abs(dot(n, v)), 0.001, 1.0);
	float n_dot_h = clamp(dot(n, h), 0.0, 1.0);
	float l_dot_h = clamp(dot(l, h), 0.0, 1.0);
	float v_dot_h = clamp(dot(v, h), 0.0, 1.0);

	pbr_info pbr_inputs = pbr_info
	(
		n_dot_l,
		n_dot_v,
		n_dot_h,
		l_dot_h,
		v_dot_h,
		perceptual_roughness,
		metallic,
		specular_env_r0,
		specular_env_r90,
		alpha_roughness,
		diffuse_color,
		specular_color
	);

	vec3  F = specular_reflection(pbr_inputs);
	float G = geometric_occlusion(pbr_inputs);
	float D = microfaced_distribution(pbr_inputs);

	vec3 diffuse_contribution = (1.0f - F) * diffuse(pbr_inputs);
	vec3 sepcular_contribution = F * G * D / (4.0f * n_dot_l * n_dot_v);
	vec3 color = n_dot_l * light_color * (diffuse_contribution + sepcular_contribution);

    /* 
	//TODO add uniform bool use_ibl;
	if(use_ibl == true)
	{
		color += get_IBL_contribution(pbr_inputs, n, reflection);
	}
	*/ 
	
	//TODO make these tweakable?
	//Occlusion remove light is small features of the geometry
	const float occlusion_strength = 1.0f;
	float ao = texture(occlusion_texture, interpolated_uv).r;
	color = mix(color, color * ao, occlusion_strength);
	
	//Emissive makes part of the object actually "emit" light
	const float emissive_strength = 1.0f;
	vec3 emissive = (vec4(emissive_factor, emissive_strength) 
		* texture(emissive_texture, interpolated_uv)).rgb;
	color += emissive;

	output_color = vec4(color, float(base_color.a));


	if(selected >= 0.999)
	{
		output_color = mix(output_color, highlight_color, 0.5);
	}

}
