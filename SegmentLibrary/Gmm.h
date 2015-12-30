// IIS_Gmm.h : interface of the Gaussian Mixture Model

#include "Torch\EMTrainer.h"
#include "Torch\MeanVarNorm.h"
#include "Torch\DiagonalGMM.h"
#include "Torch\KFold.h"
#include "Torch\KMeans.h"
#include "Torch\DiskMatDataSet.h"
#include "Torch\MatDataSet.h"
#include "Torch\DataSet.h"
#include "Torch\CmdLine.h"
#include "Torch\NLLMeasurer.h"
#include "Torch\Random.h"
#include "Torch\FileListCmdOption.h"

#pragma once
using namespace Torch;

void initializeThreshold(DataSet* data, real* thresh, real threshold);
void formDataSet(MemoryDataSet* data, real **inputdata, int dataDim, int dataNum);
int GMM(real** inputdata, int dataDim, int dataNum, float** means, float** var, float* logw, int gaussNum);