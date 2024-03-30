in vec2 interpolated_uv;
in vec3 interpolated_normal;
in vec3 fragment_world_position;

uniform sampler2D normal_texture;
uniform vec3 camera_position;

out vec4 output_color;

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
	vec3 sampled_normal_map = texture(normal_texture, interpolated_uv).xyz * 255.f/127.f - 128.f/127.f;
	sampled_normal_map.y = - sampled_normal_map.y;
	mat3 TBN = cotangent_frame(N, -V, interpolated_uv);
	return normalize(TBN * sampled_normal_map);
}

void main()
{
	vec3 v = normalize(camera_position - fragment_world_position);
	vec3 n = perturb_normal(normalize(interpolated_normal), v);
	output_color = vec4(n, 1.0f);
}
