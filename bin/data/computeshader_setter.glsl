#version 440

uniform int width;
uniform int height;
uniform uint value;

layout(std430,binding=3) buffer mutex
{
	uint m[];
};

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main(){
	m[ uint(gl_GlobalInvocationID.x * height + gl_GlobalInvocationID.y) ] = value;
	//float dc = 0.75;
	//m[ uint(gl_GlobalInvocationID.x * height + gl_GlobalInvocationID.y) ] = uint(floor(dc*float(m[ uint(gl_GlobalInvocationID.x * height + gl_GlobalInvocationID.y) ])));
}
