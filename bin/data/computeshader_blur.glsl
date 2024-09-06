#version 440

uniform int width;
uniform int height;
uniform float decayFactor;
uniform float PI;
uniform float time;

layout(rg16f,binding=0) uniform readonly image2D trailRead;
layout(rg16f,binding=1) uniform writeonly image2D trailWrite;

ivec2 LoopedPosition(ivec2 pos)
{
	return ivec2(mod(pos.x + width,width),mod(pos.y + height,height));
}

// Blur (trail map diffusion) shader

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main(){
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

	vec2 csum = vec2(0.);

	float fw = 1.0;

	for(float i=-fw;i<fw+0.5;i+=1.0)
	{
		for(float j=-fw;j<fw+0.5;j+=1.0)
		{
			csum += imageLoad(trailRead,LoopedPosition(pos-ivec2(i,j))).xy;
		}
	}

	vec2 c = csum/pow(2*fw+1.0,2.0);

	vec2 uv = vec2(pos)/float(height);
	
	float decayed = c.x*decayFactor;
	vec4 cOutput = vec4(decayed,0.8*decayed + 0.2*c.y, 0, 0);
	// (It's possible to control some amount of delay with this second color component, this is used for drawing of delayed trail map.)
	
	imageStore(trailWrite,ivec2(gl_GlobalInvocationID.xy),cOutput);
}
