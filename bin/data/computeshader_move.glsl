#version 440

uniform int width;
uniform int height;

uniform float time;

uniform float actionAreaSizeSigma;

uniform float actionX;
uniform float actionY;

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
	uint m[];
};

layout(std140, binding=2) buffer particle{
    Particle p[];
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

float gn(in vec2 coordinate, in float seed)
{
	return abs(fract(123.654*sin(distance(coordinate*(seed+0.118446744073709551614), vec2(0.118446744073709551614, 0.314159265358979323846264)))*0.141421356237309504880169));
}

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main(){

	ParametersSet currentParams_1 = params[0];
	ParametersSet currentParams_2 = params[1];

	float tunedSensorScaler_1 = currentParams_1.defaultScalingFactor * pow(1.05, currentParams_1.scalingFactorCount);
	float tunedSensorScaler_2 = currentParams_2.defaultScalingFactor * pow(1.05, currentParams_2.scalingFactorCount);

	vec4 pInput = p[gl_GlobalInvocationID.x].data;
	vec2 particlePos = vec2(pInput.x,pInput.y);
	float heading = p[gl_GlobalInvocationID.x].data.w;
	vec2 direction = vec2(cos(heading), sin(heading));

	vec2 relPos = vec2(particlePos.x/width,particlePos.y/height);
	vec2 relActionPos = vec2(actionX/width,actionY/height);

	// float lerper = particlePos.x/width;
	vec2 relDiff = relPos - relActionPos;
	relDiff.x *= float(width)/height;
	float diffDist = distance(relDiff,vec2(0));
	float lerper = exp(-diffDist*diffDist/actionAreaSizeSigma/actionAreaSizeSigma);

	float tunedSensorScaler_mix = mix(tunedSensorScaler_1, tunedSensorScaler_2, lerper);

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

	float currentSensedValue = getGridValue(particlePos +SensorBias2_mix * direction+vec2(0.,SensorBias1_mix)) * tunedSensorScaler_mix;
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

	float px = particlePos.x + jumpDistance*cos(newHeading);
	float py = particlePos.y + jumpDistance*sin(newHeading);
	vec2 nextPos = vec2(mod(px + float(width),float(width)),mod(py + float(height),float(height)));
	
	uint depositAmount = uint(1);
	atomicAdd(m[ int(round(nextPos.x))*height + int(round(nextPos.y))],depositAmount);

	const float reinitSegment=0.0010;

	float curA = pInput.z;
	if (curA<reinitSegment)
	{
		nextPos = vec2(width*gn(particlePos*13.436515/width,14.365475),height*gn(particlePos*12.765177/width+vec2(353.647,958.6515),35.6198849));
    }
	float nextA = fract(curA+reinitSegment);

	p[gl_GlobalInvocationID.x].data = vec4(nextPos.x,nextPos.y,nextA,newHeading);
}
