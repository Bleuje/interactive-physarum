#version 440

// warning: must be compatible with the c++ code
#define MAX_NUMBER_OF_WAVES 5
#define MAX_NUMBER_OF_RANDOM_SPAWN 7

#define PI 3.141592

uniform int width;
uniform int height;

uniform float time;

uniform float actionAreaSizeSigma;

uniform float actionX;
uniform float actionY;

uniform float moveBiasActionX;
uniform float moveBiasActionY;

uniform float waveXarray[MAX_NUMBER_OF_WAVES];
uniform float waveYarray[MAX_NUMBER_OF_WAVES];
uniform float waveTriggerTimes[MAX_NUMBER_OF_WAVES];
uniform float waveSavedSigmas[MAX_NUMBER_OF_WAVES];

uniform float mouseXchange;
uniform float L2Action;

uniform int spawnParticles;
uniform float spawnFraction;
uniform int randomSpawnNumber;
uniform float randomSpawnXarray[MAX_NUMBER_OF_WAVES];
uniform float randomSpawnYarray[MAX_NUMBER_OF_WAVES];

struct Particle{
	vec4 data;
	vec4 data2;
};

struct PointSettings{
	int typeIndex;

	float defaultScalingFactor;

	float SensorDistance0;
	float SD_exponent;
	float SD_amplitude;

	float SensorAngle0;
	float SA_exponent;
	float SA_amplitude;

	float RotationAngle0;
	float RA_exponent;
	float RA_amplitude;

	float MoveDistance0;
	float MD_exponent;
	float MD_amplitude;

	float SensorBias1;
	float SensorBias2;
};

layout(std430,binding=5) buffer parameters
{
	PointSettings params[];
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

vec2 getRandomPos(vec2 particlePos)
{
	return vec2(width*gn(particlePos*13.436515/width,14.365475),height*gn(particlePos*12.765177/width+vec2(353.647,958.6515),35.6198849));
}

float propagatedWaveFunction(float x,float sigma)
{
	//float waveSigma = 0.5*(0.4 + 0.8*waveActionAreaSizeSigma);
	float waveSigma = 0.15 + 0.4*sigma;
	return float(x <= 0.)*exp(-x*x/waveSigma/waveSigma);
}

layout(local_size_x = 128, local_size_y = 1, local_size_z = 1) in;
void main(){

	PointSettings currentParams_1 = params[1];
	PointSettings currentParams_2 = params[0];

	float tunedSensorScaler_1 = currentParams_1.defaultScalingFactor;
	float tunedSensorScaler_2 = currentParams_2.defaultScalingFactor;

	vec4 pInput = particlesArray[gl_GlobalInvocationID.x].data;
	vec4 pInput2 = particlesArray[gl_GlobalInvocationID.x].data2;

	vec2 particlePos = vec2(pInput.x,pInput.y);
	float heading = particlesArray[gl_GlobalInvocationID.x].data.w;
	vec2 direction = vec2(cos(heading), sin(heading));
	vec2 vel = vec2(pInput2.x,pInput2.y);

	vec2 relPos = vec2(particlePos.x/width,particlePos.y/height);
	vec2 relActionPos = vec2(actionX/width,actionY/height);

	vec2 relPos2 = relPos;
	relPos2.x *= float(width)/height;
	vec2 relPos3 = relPos2;
	float noiseScale = 20.0;
	relPos2 *= noiseScale;
	float noiseScale2 = 6.0;
	relPos3 *= noiseScale2;

	// float lerper = particlePos.x/width;
	vec2 relDiff = relPos - relActionPos;
	relDiff.x *= float(width)/height;
	float distanceNoiseFactor = (0.9 + 0.2*noise(vec3(relPos3.x,relPos3.t,0.6*time)));
	float diffDist = distance(relDiff,vec2(0))*distanceNoiseFactor;
	float lerper = exp(-diffDist*diffDist/actionAreaSizeSigma/actionAreaSizeSigma);
	//lerper = diffDist<=actionAreaSizeSigma ? 1 : 0;



	float waveIntensity = 1.0;
	float waveSum = 0.;
	
	for(int i=0;i<MAX_NUMBER_OF_WAVES;i++)
	{
		int maxWaveTime = 5; // in seconds
		if((time - waveTriggerTimes[i]) <= maxWaveTime)
		{
			vec2 relWaveCenterPos = vec2(waveXarray[i]/width,waveYarray[i]/height);
			vec2 relDiffWave = relPos - relWaveCenterPos;
			relDiffWave.x *= float(width)/height;
			float diffDistWave = distance(relDiffWave,vec2(0));
			float noiseVariationFactor = (0.95 + 0.1*noise(vec3(relPos2.x,relPos2.y,0.3*time)));
			float angleToCenter = atan(relDiffWave.y,relDiffWave.x);
			float dir = (i%2==0) ? 1. : -1.;

			float delay = -0.1 + diffDistWave/0.3 * noiseVariationFactor + 0.4*pow(0.5 + 0.5*cos(18.*angleToCenter + 10.0*dir*diffDistWave),0.3);
			float varWave = delay  - (time - waveTriggerTimes[i]);
			float sigmaVariation = pow(waveSavedSigmas[i],0.75);
			waveSum += 0.6*propagatedWaveFunction(varWave,waveSavedSigmas[i]) * max(0.,1. - 0.3*diffDistWave/sigmaVariation*noiseVariationFactor);
		}
	}

	//float addedFromWave = pow(tanh(5.0*waveSum),5.0);
	waveSum = 1.7*tanh(waveSum/1.7) + 0.4*tanh(4.*waveSum);

	waveIntensity += 0.3*waveSum;
	//lerper = mix(lerper,0.,tanh(5.*waveSum));
	

	float tunedSensorScaler_mix = mix(tunedSensorScaler_1, tunedSensorScaler_2, lerper);
	tunedSensorScaler_mix *= waveIntensity;

	float SensorDistance0_mix = mix(currentParams_1.SensorDistance0, currentParams_2.SensorDistance0, lerper);
	float SD_amplitude_mix = mix(currentParams_1.SD_amplitude, currentParams_2.SD_amplitude, lerper);
	float SD_exponent_mix = mix(currentParams_1.SD_exponent, currentParams_2.SD_exponent, lerper);

	float MoveDistance0_mix = mix(currentParams_1.MoveDistance0, currentParams_2.MoveDistance0, lerper);
	float MD_amplitude_mix = mix(currentParams_1.MD_amplitude, currentParams_2.MD_amplitude, lerper);
	float MD_exponent_mix = mix(currentParams_1.MD_exponent, currentParams_2.MD_exponent, lerper);

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
	float moveDistance = MoveDistance0_mix + MD_amplitude_mix * pow(currentSensedValue, MD_exponent_mix) * 250.0;
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

	float noiseValue = noise(vec3(relPos2.x,relPos2.t,0.8*time));

	float moveBiasFactor = 5 * lerper * noiseValue;
	vec2 moveBias = moveBiasFactor * vec2(moveBiasActionX,moveBiasActionY);

	float velBias = 0.2*L2Action;

	float px1 = particlePos.x + moveDistance*cos(newHeading) + moveBias.x;
	float py1 = particlePos.y + moveDistance*sin(newHeading) + moveBias.y;
	
	vel *= 0.98;
	float vf = 1.0;
	float vx = vel.x + vf*cos(newHeading) + velBias*moveBias.x;
	float vy = vel.y + vf*sin(newHeading) + velBias*moveBias.y;

	//float dt = 0.05*moveDistance;
	float dt = 0.07*pow(moveDistance,1.4);

	float px2 = particlePos.x + dt*vx + moveBias.x;
	float py2 = particlePos.y + dt*vy + moveBias.y;

	float moveStyleLerper = 0.6*L2Action + 0.8*waveSum;

	float px = mix(px1,px2,moveStyleLerper);
	float py = mix(py1,py2,moveStyleLerper);

	if(spawnParticles >= 1)
	{
		float randForChoice = gn(particlePos*53.146515/width,13.955475);

		if(randForChoice < spawnFraction)
		{
			float randForRadius = gn(particlePos*22.698515/width,33.265475);

			if(spawnParticles==1)
			{
				float randForTheta = gn(particlePos*8.129515/width,17.622475);
				float theta = randForTheta * PI * 2.0;
				float r1 = actionAreaSizeSigma * 0.55 * (0.95 + 0.1*randForRadius);
				float sx = r1*cos(theta);
				float sy = r1*sin(theta);
				vec2 spos = vec2(sx,sy);
				spos *= height;
				px = actionX + spos.x;
				py = actionY + spos.y;
			}
			if(spawnParticles==2)
			{
				int randForSpawnIndex = int(floor(randomSpawnNumber * gn(particlePos*28.218515/width,35.435475)));
				float sx = randomSpawnXarray[randForSpawnIndex];
				float sy = randomSpawnYarray[randForSpawnIndex];
				vec2 spos = 0.65 * actionAreaSizeSigma * vec2(sx,sy) * (0.9 + 0.1*randForRadius);
				spos *= height;
				px = actionX + spos.x;
				py = actionY + spos.y;
			}
		}
	}

	vec2 nextPos = vec2(mod(px + float(width),float(width)),mod(py + float(height),float(height)));
	
	uint depositAmount = uint(1);
	atomicAdd(particlesCounter[ int(round(nextPos.x))*height + int(round(nextPos.y))],depositAmount);

	const float reinitSegment=0.0010;

	float curA = pInput.z;
	if (curA<reinitSegment)
	{
		nextPos = getRandomPos(particlePos);
	}
	float nextA = fract(curA+reinitSegment);

	particlesArray[gl_GlobalInvocationID.x].data = vec4(nextPos.x,nextPos.y,nextA,newHeading);
	particlesArray[gl_GlobalInvocationID.x].data2 = vec4(vx,vy,0,0);
}
