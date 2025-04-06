#pragma once

#include <cmath>
#include <vector>
#include <array>
#include <string>

#include "points_basematrix.h"

#define NUMBER_OF_USED_POINTS 2

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

template<typename T>
T lerp(T a, T b, T t) {
    return a + t * (b - a);
}


struct PointsDataManager
{
  using PointData = std::array<float,PARAMS_DIMENSION>;

  std::vector<int> selectedPoints = {0,5,2,15,3,4,6,1,7,8,9,10,11,12,14,16,13,17,18,19,20,21};

  int currentSelectionIndex = 0;
  PointData usedPointsTargets[NUMBER_OF_USED_POINTS];
  int selectedIndices[NUMBER_OF_USED_POINTS];
  PointData currentPointValues[NUMBER_OF_USED_POINTS];
  std::vector<PointData> currentPointsData;

  PointData mean;

  PointsDataManager()
  {
    mean = {};
    for(int i=0;i<NumberOfBasePoints;i++)
    {
      PointData dataToAdd;
      for(int j=0;j<PARAMS_DIMENSION;j++)
      {
        dataToAdd[j] = ParametersMatrix[i][j];
        mean[j] += ParametersMatrix[i][j];
      }
      currentPointsData.push_back(dataToAdd);
    }
    for(int j=0;j<PARAMS_DIMENSION;j++)
    {
      mean[j] /= NumberOfBasePoints;
    }

    for(int k=0;k<NUMBER_OF_USED_POINTS;k++)
    {
      usedPointsTargets[k] = currentPointsData[0];
      currentPointValues[k] = currentPointsData[0];
      selectedIndices[k] = 0;
    }
  }

  void reloadUsedPointsTargets()
  {
    for(int i=0;i<NUMBER_OF_USED_POINTS;i++)
    {
      usedPointsTargets[i] = currentPointsData[selectedPoints[selectedIndices[i]]];
    }
  }

  void resetCurrentPoint()
  {
    int chosenPoint = selectedIndices[currentSelectionIndex];
    for(int j=0;j<PARAMS_DIMENSION;j++)
    {
      currentPointsData[selectedPoints[chosenPoint]][j] = ParametersMatrix[selectedPoints[chosenPoint]][j];
    }
    reloadUsedPointsTargets();
  }

  void resetAllPoints()
  {
    for(int i=0;i<int(selectedPoints.size());i++)
    {
      for(int j=0;j<PARAMS_DIMENSION;j++)
      {
        currentPointsData[selectedPoints[i]][j] = ParametersMatrix[selectedPoints[i]][j];
      }
    }
    reloadUsedPointsTargets();
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

    reloadUsedPointsTargets();
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
      std::string ret = "Point ";
      ret.push_back(char('A' + selectedIndices[selectionIndex]));
      return ret;
  }

  int getNumberOfPoints()
  {
    return selectedPoints.size();
  }

  float getValue(int settingIndex)
  {
    int lineIndex = selectedPoints[selectedIndices[currentSelectionIndex]];
    PointData point = currentPointsData[lineIndex];

    int matrixColumnIndex = (settingIndex==0 ? PARAMS_DIMENSION-1 : settingIndex-1);

    return point[matrixColumnIndex];
  }

  void changeValue(int settingIndex,int dir)
  {
    int matrixColumnIndex = (settingIndex==0 ? PARAMS_DIMENSION-1 : settingIndex-1);
    int lineIndex = selectedPoints[selectedIndices[currentSelectionIndex]];
    PointData& point = currentPointsData[lineIndex];

    float meanStep = 0.025 * mean[matrixColumnIndex];
    float defaultValue = ParametersMatrix[lineIndex][matrixColumnIndex];
    float defaultValueStep = (defaultValue < 0.00001 ? meanStep/2 : 0.05 * defaultValue);
    float step = std::min(meanStep,defaultValueStep);
    point[matrixColumnIndex] += step * dir;
    point[matrixColumnIndex] = std::max(0.f,point[matrixColumnIndex]);

    // float fStep = 1.04;
    // point[index] *= pow(fStep, dir);

    for(int i=0;i<NUMBER_OF_USED_POINTS;i++)
    {
      if(lineIndex == selectedPoints[selectedIndices[i]]) usedPointsTargets[i] = point;
    }
  }

  void createRandomParameters()
  {
    resetAllPoints();

    int numberOfSelectedPoints = selectedPoints.size();

    int pointChoice0 = rand() % numberOfSelectedPoints;

    for (int j = 0; j < PARAMS_DIMENSION; j++)
    {
      currentPointsData[selectedPoints[selectedIndices[currentSelectionIndex]]][j] = currentPointsData[selectedPoints[pointChoice0]][j];;
    }

    for (int j = 0; j < PARAMS_DIMENSION; j++)
    {
      if(rand()%2 == 0) continue;

      int pointChoice = -1;
      bool ok = false;
      while(!ok)
      {
        pointChoice = rand() % numberOfSelectedPoints;
        ok = pointChoice != pointChoice0;

        // if parameter is 0 and an exponent, it's not ok
        ok = ok && (!(j==1 || j==4 || j==7 || j==10) || currentPointsData[selectedPoints[pointChoice]][j]>=0.001);
      }

      double value = currentPointsData[selectedPoints[pointChoice]][j];

      if (rand() % 2 == 0)
      {
        int pointChoice2 = rand() % numberOfSelectedPoints;
        while (pointChoice2 == pointChoice0)
        {
          pointChoice2 = rand() % numberOfSelectedPoints;
        }
        double value2 = currentPointsData[selectedPoints[pointChoice2]][j];
        double lerper = 0.01 * (rand() % 100);
        value = (1 - lerper) * value + lerper * value2;
      }
      currentPointsData[selectedPoints[selectedIndices[currentSelectionIndex]]][j] = value;
    }
    reloadUsedPointsTargets();
  }

  std::string getSettingName(int settingIndex)
  {
    switch (settingIndex) {
        case 0:
            return "Sensing factor";
        case 1:
            return "Sensor Distance 0";
        case 2:
            return "SD exponent";
        case 3:
            return "SD amplitude";
        case 4:
            return "Sensor Angle 0";
        case 5:
            return "SA exponent";
        case 6:
            return "SA amplitude";
        case 7:
            return "Rotation Angle 0";
        case 8:
            return "RA exponent";
        case 9:
            return "RA amplitude";
        case 10:
            return "Move Distance 0";
        case 11:
            return "MD exponent";
        case 12:
            return "MD amplitude";
        case 13:
            return "SensorBias1";
        case 14:
            return "SensorBias2";
        default:
            return "Unknown";
    }
  }

  void writeParamsToFile() {
      // Create timestamp
      auto now = std::chrono::system_clock::now();
      auto t = std::chrono::system_clock::to_time_t(now);
      std::tm tm = *std::localtime(&t);

      std::ostringstream filenameStream;
      filenameStream << "parameters/params_"
                    << std::put_time(&tm, "%Y%m%d_%H%M%S")
                    << ".txt";

      std::string filename = filenameStream.str();

      std::ofstream file(ofToDataPath(filename));
      if (!file.is_open()) {
          ofLogError() << "Could not open file: " << filename;
          return;
      }

      file << "{";
      for (size_t i = 0; i < PARAMS_DIMENSION; ++i) {
          file << std::fixed << std::setprecision(3) << currentPointsData[selectedPoints[selectedIndices[currentSelectionIndex]]][i];
          if (i != PARAMS_DIMENSION - 1) {
              file << ", ";
          }
      }
      file << "}" << std::endl;
      file.close();

      ofLogNotice() << "Wrote parameters to " << filename;
  }
};