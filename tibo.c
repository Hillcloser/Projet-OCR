#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "NN.h"



/*-----------------------------------------------------------------------------
w = weight, <= w <= 
b = bias, < b <



-------------------------------------------------------------------------------
input = 0.1 = start.hidden 
output = 1.2 = hidden.end
-----------------------------------------------------------------------------*/

// basic function :

// -1 <= random_d() <= 1
double random_d()
{return ((double)rand() / RAND_MAX) * 2 - 1;} 


double sigmoid(double z):
{return 1.0/(1.0 + exp(-z))}




#define n 3



struct NeuralNetwork 
{
    double **input_w;
	double *output_w;
	double *hidden_b;
	double end_b;
	double *hidden_z;
	double end_z;
	double *hidden_res;
	double end_res;
	double var_err;
};

struct NeuralNetwork init_NN()
{
	struct NeuralNetwork res;
	
	res.input_w = calloc(2, sizeof(double *));
	res.input_w[0] = calloc(n, sizeof(double));
	res.input_w[1] = calloc(n, sizeof(double));
	
	res.output_w = calloc(n, sizeof(double));
	
	res.hidden_b = calloc(n, sizeof(double));
	res.end_b = 0;
	
	res.hidden_z = calloc(n, sizeof(double));;
	res.hidden_res = calloc(n, sizeof(double));
	res.var_err = 0;
	srand(time(NULL));
	for (int i = 0 ; i < n; i++)
	{
		for (int j = 0; j < 2 ; j++) res.input_w[j][i] = random_d();
		res.output_w[i] = random_d();
		res.hidden_b[i] = random_d(); 
	}
	return res;
}



void free_NN(struct NeuralNetwork tmp)
{
	free(res.input_w[0]);
	free(res.input_w[1]);
	free(res.input_w);
	free(res.output_w);
	free(res.hidden_b);
	free(res.hidden_res);
}





int main()
{
	NN = init_NN();
	
	
	
}