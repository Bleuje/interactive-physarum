#version 440

uniform int width;
uniform int height;
uniform float depositFactor;
uniform int colorModeType;
uniform int numberOfColorModes;

layout(std430,binding=3) buffer mutex
{
	uint particlesCounter[];
};

layout(rgba16f,binding=0) uniform readonly image2D trailRead;
layout(rgba16f,binding=1) uniform writeonly image2D trailWrite;
layout(rgba32f,binding=4) uniform writeonly image2D displayWrite;

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 changeHue(vec3 hsv, float hueChange) {
    hsv.x += hueChange; // Add the hue change
    hsv.x = mod(hsv.x, 1.0); // Wrap hue to stay within [0, 1]
    return hsv;
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// This shader is looking at a single pixel.
// It adds deposit to the trail map from the number of particles on this pixel.
// It also sets the color of the pixel in the displayed image.

layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;
void main(){
	vec4 prevColor = imageLoad(trailRead,ivec2(gl_GlobalInvocationID.xy)); // Getting the trail map color on current pixel

	float count = float(particlesCounter[ gl_GlobalInvocationID.x * height + gl_GlobalInvocationID.y ]); // number of particles on the pixel

	// The following 3 lines of code are my own innovation (looks like the license requires attribution if you use this :) ),
	// a way to define an amount of added trail in function of the number of particles on the pixel
	uint limit = 100;
	float limitedCount = count<float(limit)?count:float(limit);
	float addedDeposit = pow(limitedCount,0.5)*depositFactor;

	// Trail map update
	float val = prevColor.x + addedDeposit;
	imageStore(trailWrite,ivec2(gl_GlobalInvocationID.xy),vec4(val,val,prevColor.z,1.0)); // using the 3rd color component to keep track of the previous color, it's not easy to explain why I do this


	// Mapping the count on pixel to color intensity, looks like one day I tried hard to get something satisfying with a complicated formula
	float countColorValue = pow(tanh(7.5*pow(max(0.,(count-1)/1000.0),0.3)),8.5)*1.05;
	countColorValue = min(1.0,countColorValue);
	//float colorValue2 = pow(tanh(7.5*pow(max(0.,(250*val-1)/1000.0),0.3)),8.5)*1.05;

	vec4 outputColor = vec4(0.,1.,0.,1.); // (dummy overwritten value)
	if(colorModeType == 0) // white on black from particle counts
	{
		outputColor = vec4(countColorValue,countColorValue,countColorValue,1.0);
	}
	else
	{
		// Mapping the trail map intensity to some color intensity, using trail map color with a delay
		float trailColorValue = pow(tanh(9.0*pow(max(0.,(250*prevColor.z-1)/1100.0),0.3)),8.5)*1.05;
		trailColorValue = min(1.0,trailColorValue);

		// Color defined with combination of particle count (red) and trail map (green, blue)
		vec3 rgb0 = vec3(countColorValue, trailColorValue, trailColorValue);
		vec3 hsv0 = rgb2hsv(rgb0);

		float hueChange = float(colorModeType-1)/float(numberOfColorModes-1); // hue shift with different color modes
		vec3 hsv = changeHue(hsv0,hueChange);

		vec3 rgb = hsv2rgb(hsv);
		outputColor = vec4(rgb,1.0);
	}
	imageStore(displayWrite,ivec2(gl_GlobalInvocationID.xy),outputColor);
}
