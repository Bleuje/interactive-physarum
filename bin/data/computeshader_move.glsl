#version 440

// warning: must match c++ code
#define MAX_NUMBER_OF_WAVES 5

uniform int width;
uniform int height;

uniform float time;

uniform float actionAreaSizeSigma;
uniform float waveActionAreaSizeSigma;

uniform float actionX;
uniform float actionY;

uniform float moveBiasActionX;
uniform float moveBiasActionY;

uniform float waveXarray[MAX_NUMBER_OF_WAVES];
uniform float waveYarray[MAX_NUMBER_OF_WAVES];
uniform float waveTriggerTimes[MAX_NUMBER_OF_WAVES];

struct Particle{
	vec4 data;
	vec4 data2;
};

struct ParametersSet{
	int typeIndex;

	float defaultScalingFactor;
	int scalingFactorCount;

	float SensorDistance0;
	float SD_exponent;
	float SD_amplitude;

	float SensorAngle0;
	float SA_exponent;
	float SA_amplitude;

	float RotationAngle0;
	float RA_exponent;
	float RA_amplitude;

	float JumpDistance0;
	float JD_exponent;
	float JD_amplitude;

	float SensorBias1;
	float SensorBias2;
};

layout(std430,binding=5) buffer parameters
{
	ParametersSet params[];
};

layout(rgba32f,binding=0) uniform readonly image2D src;
layout(rgba32f,binding=1) uniform writeonly image2D dst;
layout(std430,binding=3) buffer mutex
{
	uint particlesCounter[];
};

layout(std140, binding=2) buffer particle{
    Particle particlesArray[];
};

float getGridValue(vec2 pos)
{
	return imageLoad(src,ivec2(mod(pos.x + 0.5 + float(width),float(width)),mod(pos.y + 0.5 + float(height),float(height)))).x;
}

float senseFromAngle(float angle,vec2 pos,float heading,float so)
{
    return getGridValue(vec2(pos.x + so*cos(heading + angle),pos.y + so*sin(heading + angle)));
}

float random(vec2 st) {
    return fract(0.5+0.5*sin(dot(st.xy,vec2(12.9898,78.233)))*43758.5453123);
}

float random2 (vec3 st) {
    return fract(sin(dot(st.xyz,
                         vec3(12.9898,78.233, 151.7182)))
                 * 43758.5453123);
}

float noise (vec3 st) {
    vec3 i = floor(st);
    vec3 f = fract(st);

    // Calculate the eight corners of the cube
    float a = random2(i);
    float b = random2(i + vec3(1.0, 0.0, 0.0));
    float c = random2(i + vec3(0.0, 1.0, 0.0));
    float d = random2(i + vec3(1.0, 1.0, 0.0));
    float e = random2(i + vec3(0.0, 0.0, 1.0));
    float f_ = random2(i + vec3(1.0, 0.0, 1.0));
    float g = random2(i + vec3(0.0, 1.0, 1.0));
    float h = random2(i + vec3(1.0, 1.0, 1.0));

    // Smoothly interpolate the noise value
    vec3 u = f * f * (3.0 - 2.0 * f);

    return mix(mix(mix( a, b, u.x),
                   mix( c, d, u.x), u.y),
               mix(mix( e, f_, u.x),
                   mix( g, h, u.x), u.y), u.z);
}

float gn(in vec2 coordinate, in float seed)
{
	return abs(fract(123.654*sin(distance(coordinate*(seed+0.118446744073709551614), vec2(0.118446744073709551614, 0.314159265358979323846264)))*0.141421356237309504880169));
}

float propagatedWaveFunction(float x)
{
	float sigmaWave = 0.5 * waveActionAreaSizeSigma;
	return exp(-x*x/sigmaWave/sigmaWave);
}

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main(){

	ParametersSet currentParams_1 = params[1];
	ParametersSet currentParams_2 = params[0];

	float tunedSensorScaler_1 = currentParams_1.defaultScalingFactor * pow(1.05, currentParams_1.scalingFactorCount);
	float tunedSensorScaler_2 = currentParams_2.defaultScalingFactor * pow(1.05, currentParams_2.scalingFactorCount);

	vec4 pInput = particlesArray[gl_GlobalInvocationID.x].data;
	vec4 pInput2 = particlesArray[gl_GlobalInvocationID.x].data2;

	vec2 particlePos = vec2(pInput.x,pInput.y);
	float heading = particlesArray[gl_GlobalInvocationID.x].data.w;
	vec2 direction = vec2(cos(heading), sin(heading));

	vec2 relPos = vec2(particlePos.x/width,particlePos.y/height);
	vec2 relActionPos = vec2(actionX/width,actionY/height);

	// float lerper = particlePos.x/width;
	vec2 relDiff = relPos - relActionPos;
	relDiff.x *= float(width)/height;
	float diffDist = distance(relDiff,vec2(0));
	float lerper = exp(-diffDist*diffDist/actionAreaSizeSigma/actionAreaSizeSigma);

	vec2 relPos2 = relPos;
	relPos2.x *= float(width)/height;
	float noiseScale = 20.0;
	relPos2 *= noiseScale;

	float waveIntensity = 1.0;

	float waveSum = 0.;
	
	for(int i=0;i<MAX_NUMBER_OF_WAVES;i++)
	{
		int maxWaveTime = 4; // in seconds
		if((time - waveTriggerTimes[i]) <= maxWaveTime)
		{
			vec2 relWaveCenterPos = vec2(waveXarray[i]/width,waveYarray[i]/height);
			vec2 relDiffWave = relPos - relWaveCenterPos;
			relDiffWave.x *= float(width)/height;
			float diffDistWave = distance(relDiffWave,vec2(0));
			float noiseVariationFactor = (0.8 + 0.4*noise(vec3(relPos2.x,relPos2.t,0.3*time)));

			float varWave = diffDistWave/0.35 * noiseVariationFactor  - (time - waveTriggerTimes[i]);
			waveSum += 0.6*propagatedWaveFunction(varWave) * max(0.,1. - 0.3*diffDistWave/waveActionAreaSizeSigma*noiseVariationFactor);
		}
	}

	waveIntensity += waveSum;
	

	float tunedSensorScaler_mix = mix(tunedSensorScaler_1, tunedSensorScaler_2, lerper);
	tunedSensorScaler_mix *= waveIntensity;

	float SensorDistance0_mix = mix(currentParams_1.SensorDistance0, currentParams_2.SensorDistance0, lerper);
	float SD_amplitude_mix = mix(currentParams_1.SD_amplitude, currentParams_2.SD_amplitude, lerper);
	float SD_exponent_mix = mix(currentParams_1.SD_exponent, currentParams_2.SD_exponent, lerper);

	float JumpDistance0_mix = mix(currentParams_1.JumpDistance0, currentParams_2.JumpDistance0, lerper);
	float JD_amplitude_mix = mix(currentParams_1.JD_amplitude, currentParams_2.JD_amplitude, lerper);
	float JD_exponent_mix = mix(currentParams_1.JD_exponent, currentParams_2.JD_exponent, lerper);

	float SensorAngle0_mix = mix(currentParams_1.SensorAngle0, currentParams_2.SensorAngle0, lerper);
	float SA_amplitude_mix = mix(currentParams_1.SA_amplitude, currentParams_2.SA_amplitude, lerper);
	float SA_exponent_mix = mix(currentParams_1.SA_exponent, currentParams_2.SA_exponent, lerper);

	float RotationAngle0_mix = mix(currentParams_1.RotationAngle0, currentParams_2.RotationAngle0, lerper);
	float RA_amplitude_mix = mix(currentParams_1.RA_amplitude, currentParams_2.RA_amplitude, lerper);
	float RA_exponent_mix = mix(currentParams_1.RA_exponent, currentParams_2.RA_exponent, lerper);

	float SensorBias1_mix = mix(currentParams_1.SensorBias1, currentParams_2.SensorBias1, lerper);
	float SensorBias2_mix = mix(currentParams_1.SensorBias2, currentParams_2.SensorBias2, lerper);

	float currentSensedValue = getGridValue(particlePos + SensorBias2_mix * direction + vec2(0.,SensorBias1_mix)) * tunedSensorScaler_mix;
	currentSensedValue = min(1.0,max(currentSensedValue, 0.000000001));

	float sensorDistance = SensorDistance0_mix + SD_amplitude_mix * pow(currentSensedValue, SD_exponent_mix) * 250.0;
	float jumpDistance = JumpDistance0_mix + JD_amplitude_mix * pow(currentSensedValue, JD_exponent_mix) * 250.0;
	float sensorAngle = SensorAngle0_mix + SA_amplitude_mix * pow(currentSensedValue, SA_exponent_mix);
	float rotationAngle = RotationAngle0_mix + RA_amplitude_mix * pow(currentSensedValue, RA_exponent_mix);
	
	float sensedLeft = senseFromAngle(-sensorAngle, particlePos, heading, sensorDistance);
	float sensedMiddle = senseFromAngle(0, particlePos, heading, sensorDistance);
	float sensedRight = senseFromAngle(sensorAngle, particlePos, heading, sensorDistance);

	float newHeading = heading;

	if(sensedMiddle > sensedLeft&&sensedMiddle>sensedRight)
	{
		;
	} else if(sensedMiddle < sensedLeft&&sensedMiddle<sensedRight)
	{
		newHeading = (random(particlePos)< 0.5 ? heading - rotationAngle : heading + rotationAngle);
	} else if(sensedRight < sensedLeft)
	{
		newHeading = heading - rotationAngle;
	} else if(sensedLeft < sensedRight)
	{
		newHeading = heading + rotationAngle;
	}

	float noiseValue = noise(vec3(relPos2.x,relPos2.t,0.3*time));

	float moveBiasFactor = 5 * lerper * noiseValue;
	vec2 moveBias = moveBiasFactor * vec2(moveBiasActionX,moveBiasActionY);

	float px = particlePos.x + jumpDistance*cos(newHeading) + moveBias.x;
	float py = particlePos.y + jumpDistance*sin(newHeading) + moveBias.y;
	vec2 nextPos = vec2(mod(px + float(width),float(width)),mod(py + float(height),float(height)));
	
	uint depositAmount = uint(1);
	atomicAdd(particlesCounter[ int(round(nextPos.x))*height + int(round(nextPos.y))],depositAmount);

	const float reinitSegment=0.0010;

	float curA = pInput.z;
	if (curA<reinitSegment)
	{
		nextPos = vec2(width*gn(particlePos*13.436515/width,14.365475),height*gn(particlePos*12.765177/width+vec2(353.647,958.6515),35.6198849));
  }
	float nextA = fract(curA+reinitSegment);

	particlesArray[gl_GlobalInvocationID.x].data = vec4(nextPos.x,nextPos.y,nextA,newHeading);
}
