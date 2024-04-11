#version 440

uniform int width;
uniform int height;
uniform float depositFactor;

layout(std430,binding=3) buffer mutex
{
	uint particlesCounter[];
};

layout(rgba32f,binding=0) uniform readonly image2D trailRead;
layout(rgba32f,binding=1) uniform writeonly image2D trailWrite;
layout(rgba32f,binding=4) uniform writeonly image2D displayWrite;

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main(){
	vec4 prevColor = imageLoad(trailRead,ivec2(gl_GlobalInvocationID.xy));
	uint limit = 100;
	float cnt = float(particlesCounter[ gl_GlobalInvocationID.x * height + gl_GlobalInvocationID.y ]);
	float deposit = pow((cnt<float(limit)?cnt:float(limit)),0.5)*depositFactor;
	float val = prevColor.x + deposit;
	imageStore(trailWrite,ivec2(gl_GlobalInvocationID.xy),vec4(val,val,prevColor.z,1.0));

	float colorValue = pow(tanh(7.5*pow(max(0.,(cnt-1)/1000.0),0.3)),8.5)*1.05;
	//float colorValue2 = pow(tanh(7.5*pow(max(0.,(250*val-1)/1000.0),0.3)),8.5)*1.05;
	//float colorValue3 = pow(tanh(7.5*pow(max(0.,(250*prevColor.z-1)/1000.0),0.3)),8.5)*1.05;
	vec4 outputColor = vec4(colorValue,colorValue,colorValue,1.0);
	imageStore(displayWrite,ivec2(gl_GlobalInvocationID.xy),outputColor);
}
