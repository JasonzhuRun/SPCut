//IIS_Gmm.cpp : Implementation of Gaussain Mixture Model

#include "Gmm.h"

using namespace Torch;

/******************************************
函数名称：GMM
函数功能：根据输入数据进行EM计算
输入参数：
	inputdata：输入数据
	dataDim：输入数据维数
	dataNum：输入数据个数
返回参数：
	建模过程是否正常
*******************************************/
int GMM(real **inputdata, int dataDim, int dataNum, float** means, float** var, float* logw, int gaussNum)
{
	real accuracy = 0.001; //end accuracy
	real threshold = 0.001; //variance threshold
	int max_iter_kmeans = 20; //max number of iterations of KMeans
	int max_iter_gmm = 5; //max number of iterations of GMM
	int n_gaussians = gaussNum; //number of Gaussians
	real prior = 0.001; //prior on the weights

	int max_load = -1; //max number of examples to load for train
	int the_seed = -1; //the random seed

	int k_fold = -1; //number of folds, if you want to do cross-validation
	bool binary_mode = true; //binary mode for files

	Allocator *allocator = new Allocator;

	//==================================================================== 
	//=================== Create the DataSet ... =========================
	//==================================================================== 

	MemoryDataSet data;
	formDataSet(&data,inputdata,dataDim,dataNum);

	//==================================================================== 
	//=================== Training Mode  =================================
	//==================================================================== 

	if (the_seed == -1)
	{
		Random::seed();
	}
	else
	{
		Random::manualSeed((long)the_seed);
	}

	//=================== Create the GMM... =========================

	// create a KMeans object to initialize the GMM
	KMeans kmeans(data.n_inputs, n_gaussians);
	kmeans.setROption("prior weights",prior);

	// the kmeans trainer
	EMTrainer kmeans_trainer(&kmeans);
	kmeans_trainer.setROption("end accuracy", accuracy);
	kmeans_trainer.setIOption("max iter", max_iter_kmeans);

	// create the GMM
	DiagonalGMM gmm(data.n_inputs,n_gaussians,&kmeans_trainer);

	// set the training options
	real* thresh = (real*)allocator->alloc(data.n_inputs*sizeof(real));
	initializeThreshold(&data,thresh,threshold);	
	gmm.setVarThreshold(thresh);
	gmm.setROption("prior weights",prior);

	//=================== Measurers and Trainer  ===============================

	// The Gradient Machine Trainer
	EMTrainer trainer(&gmm);
	trainer.setIOption("max iter", max_iter_gmm);
	trainer.setROption("end accuracy", accuracy);

	//=================== Let's go... ===============================
	{
		trainer.train(&data, NULL); //&measurers);

		for (int i = 0; i < gaussNum; i++) 
		{
			logw[i] = gmm.log_weights[i];
			for (int j = 0; j < dataDim; j++) 
			{
				means[i][j] = gmm.means[i][j];
				var[i][j] = gmm.var[i][j];
			}
		}
	}

	delete allocator;

	return(0);
}

//==================================================================================================== 
//==================================== Functions ===================================================== 
//==================================================================================================== 

void initializeThreshold(DataSet* data, real* thresh, real threshold)
{
	MeanVarNorm norm(data);
	real*	ptr = norm.inputs_stdv;
	real* p_var = thresh;
	for (int i = 0; i < data->n_inputs; i++)
	{
		*p_var++ = *ptr * *ptr++ * threshold;
	}
}

//根据输入数据，形成EM算法所需要的数据形式
void formDataSet(MemoryDataSet* data, real **inputdata, int dataDim, int dataNum)
{
	int n_examples_ = 0;

	data->n_inputs = dataDim;

	data->n_examples = 1;

	data->DataSet::init(1, data->n_inputs, 0);

	data->inputs_array = (Sequence **)data->allocator->alloc(sizeof(Sequence*)*data->n_examples);
	Sequence *sequences_buffer = (Sequence *)data->allocator->alloc(sizeof(Sequence*)*data->n_examples);

	data->inputs_array[0] = new Sequence(inputdata,dataNum,dataDim);

	data->setRealExample(0,true,false);
}

