#version 440

uniform int width;
uniform int height;
uniform uint value;

layout(std430,binding=3) buffer mutex
{
	uint particlesCounter[];
};

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main(){
	particlesCounter[ uint(gl_GlobalInvocationID.x * height + gl_GlobalInvocationID.y) ] = value;
}
