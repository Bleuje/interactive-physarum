#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <string>

#include "pointsBaseMatrix.h"

#define NUMBER_OF_USED_POINTS 2

struct PointSettings{
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

    float MoveDistance0;
    float MD_exponent;
    float MD_amplitude;

    float SensorBias1;
    float SensorBias2;
};

template<typename T>
T lerp(T a, T b, T t) {
    return a + t * (b - a);
}


struct PointsDataManager
{
  using PointData = std::array<float,PARAMS_DIMENSION>;

  PointsDataManager()
  {
    for(int i=0;i<NumberOfBasePoints;i++)
    {
      PointData dataToAdd;
      for(int j=0;j<PARAMS_DIMENSION;j++)
      {
        dataToAdd[j] = ParametersMatrix[i][j];
      }
      currentPointsData.push_back(dataToAdd);
    }

    for(int k=0;k<NUMBER_OF_USED_POINTS;k++)
    {
      usedPointsTargets[k] = currentPointsData[0];
      currentPointValues[k] = currentPointsData[0];
      selectedIndices[k] = 0;
    }
  }

  PointSettings getPointsParamsFromArray(PointData pointData)
  {
    PointSettings ret;

    ret.defaultScalingFactor = pointData[PARAMS_DIMENSION-1];
    ret.SensorDistance0 = pointData[0];
    ret.SD_exponent = pointData[1];
    ret.SD_amplitude = pointData[2];
    ret.SensorAngle0 = pointData[3];
    ret.SA_exponent = pointData[4];
    ret.SA_amplitude = pointData[5];
    ret.RotationAngle0 = pointData[6];
    ret.RA_exponent = pointData[7];
    ret.RA_amplitude = pointData[8];
    ret.MoveDistance0 = pointData[9];
    ret.MD_exponent = pointData[10];
    ret.MD_amplitude = pointData[11];
    ret.SensorBias1 = pointData[12];
    ret.SensorBias2 = pointData[13];

    ret.scalingFactorCount = 0;

    return ret;
  }

  PointData pointDataFromPointSettings(PointSettings params)
  {
    PointData ret;

    ret[PARAMS_DIMENSION-1] = params.defaultScalingFactor;
    ret[0] = params.SensorDistance0;
    ret[1] = params.SD_exponent;
    ret[2] = params.SD_amplitude;
    ret[3] = params.SensorAngle0;
    ret[4] = params.SA_exponent;
    ret[5] = params.SA_amplitude;
    ret[6] = params.RotationAngle0;
    ret[7] = params.RA_exponent;
    ret[8] = params.RA_amplitude;
    ret[9] = params.MoveDistance0;
    ret[10] = params.MD_exponent;
    ret[11] = params.MD_amplitude;
    ret[12] = params.SensorBias1;
    ret[13] = params.SensorBias2;

    return ret;
  }

  void updateCurrentValuesFromTransitionProgress(float transitionProgress)
  {
      float lerper = pow(transitionProgress,1.5);
      for(int k=0;k<NUMBER_OF_USED_POINTS;k++)
      {
        for(int j=0;j<PARAMS_DIMENSION;j++)
        {
          currentPointValues[k][j] = lerp(currentPointValues[k][j],usedPointsTargets[k][j],lerper);
        }
      }
  }

  void swapUsedPoints()
  {
    std::swap(usedPointsTargets[0],usedPointsTargets[1]);
    std::swap(selectedIndices[0],selectedIndices[1]);
  }

  void useRandomIndices()
  {
    int sz = selectedPoints.size();

    selectedIndices[0] = rand() % sz;
    selectedIndices[1] = rand() % sz;

    usedPointsTargets[0] = currentPointsData[selectedPoints[selectedIndices[0]]];
    usedPointsTargets[1] = currentPointsData[selectedPoints[selectedIndices[1]]];
  }

  void changeSelectionIndex(int dir)
  {
    currentSelectionIndex = (currentSelectionIndex + dir + NUMBER_OF_USED_POINTS) % NUMBER_OF_USED_POINTS;
  }

  int getSelectionIndex()
  {
    return currentSelectionIndex;
  }

  void changeParamIndex(int dir)
  {
    int sz = selectedPoints.size();

    selectedIndices[currentSelectionIndex] = (selectedIndices[currentSelectionIndex] + dir + sz) % sz;

    usedPointsTargets[currentSelectionIndex] = currentPointsData[selectedPoints[selectedIndices[currentSelectionIndex]]];
  }

  std::string getPointName(int selectionIndex)
  {
      std::string ret = "params ";
      ret.push_back(char('A' + selectedIndices[selectionIndex]));
      return ret;
  }

  std::vector<int> selectedPoints = {0,1,2,4,5,6,7,11,13,14,15,19,21,27,30,34,37,40,32,36};
  int currentSelectionIndex = 0;

  PointData usedPointsTargets[NUMBER_OF_USED_POINTS];
  int selectedIndices[NUMBER_OF_USED_POINTS];
  PointData currentPointValues[NUMBER_OF_USED_POINTS];

  std::vector<PointData> currentPointsData;
};