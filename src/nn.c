#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "nn.h"

// -1 <= random_d() <= 1
double random_d()
{return ((double)rand() / RAND_MAX) * 2 - 1;} 

double sigmoid(double z)
{return 1.0/(1.0 + exp(-z));}

double sigmoid_prime(double z) //Backpropagation of sigmoid
{return sigmoid(z)*(1-sigmoid(z));}

void softmax(double *z, double *res)
{
	double z_exp_sum = 0;
	for (int e_i = 0; e_i < 26; e_i++) z_exp_sum += exp(z[e_i]);
	for (int e_i = 0; e_i < 26; e_i++) res[e_i] = exp(z[e_i])/z_exp_sum;
}

double softmax_B(double res, double y) //Backpropagation of softmax
{return res - y;}


struct NeuralNetwork
{
	int n; //hidden len
	
	double **input_w; // 26*n
	double **output_w;// n*784
	
	double *hidden_b; 
	double *hidden_z;
	double *hidden_res;
	
	double *end_b;
	double *end_z;
	double *end_res; 
	
	double learning_rate;
};

struct NeuralNetwork init_NN(int n, double learning_rate)
{
	struct NeuralNetwork res;
	
	res.n = n;
	///// init memory
	//[p_i][h_i]
	res.input_w = calloc(784, sizeof(double *));					
	for (int p_i = 0 ; p_i < 784; p_i++) res.input_w[p_i] = calloc(res.n, sizeof(double));
	
	//[h_i][e_i]
	res.output_w = calloc(res.n, sizeof(double *));
	for (int h_i = 0 ; h_i < res.n; h_i++) res.output_w[h_i] = calloc(26, sizeof(double));
	
	//[h_i]
	res.hidden_b = calloc(res.n, sizeof(double));
	res.hidden_z = calloc(res.n, sizeof(double));
	res.hidden_res = calloc(res.n, sizeof(double));
	
	//[e_i]
	res.end_b = calloc(26, sizeof(double));
	res.end_z = calloc(26, sizeof(double));
	res.end_res = calloc(26, sizeof(double));
	
	res.learning_rate = learning_rate;
	/////
	
	///// init val
	for (int h_i = 0 ; h_i < res.n; h_i++) res.hidden_b[h_i] = random_d(); 
	for (int e_i = 0; e_i < 26 ; e_i++) res.end_b[e_i] = random_d(); 
	
	for (int p_i = 0; p_i < 784 ; p_i++) for (int h_i = 0 ; h_i < res.n; h_i++) res.input_w[p_i][h_i] = random_d();
	for (int h_i = 0 ; h_i < res.n; h_i++) for (int e_i = 0; e_i < 26 ; e_i++) res.output_w[h_i][e_i] = random_d();
	/////
	
	return res;
}

void free_NN(struct NeuralNetwork *nn)
{
						
	for (int p_i = 0; p_i < 26 ; p_i++) free(nn->input_w[p_i]);
	free(nn->input_w);
	
	for (int h_i = 0 ; h_i < nn->n; h_i++) free(nn->output_w[h_i]);
	free(nn->output_w);
	
	free(nn->hidden_b);
	free(nn->hidden_z);
	free(nn->hidden_res);
	
	free(nn->end_b);
	free(nn->end_z);
	free(nn->end_res);
}

char forward(struct NeuralNetwork *nn, int *pixel) //return for backward in NN
{
	for (int h_i = 0; h_i < nn->n ; h_i++) 
		nn->hidden_z[h_i] = nn->hidden_b[h_i];
	
	for (int p_i = 0; p_i < 784 ; p_i++) 
		for (int h_i = 0; h_i < nn->n ; h_i++) 
			nn->hidden_z[h_i] += nn->input_w[p_i][h_i] * (double)pixel[p_i]; 
		
	for (int h_i = 0; h_i < nn->n ; h_i++) 
		nn->hidden_res[h_i] = sigmoid(nn->hidden_z[h_i]);
	
	for (int e_i = 0; e_i < 26 ; e_i++) 
		nn->end_z[e_i] = nn->end_b[e_i];
	
	for (int h_i = 0; h_i < nn->n ; h_i++) 
		for (int e_i = 0; e_i < 26 ; e_i++) 
			nn->end_z[e_i] += nn->output_w[h_i][e_i] * nn->hidden_res[h_i]; 
	
	softmax(nn->end_z, nn->end_res);
	
	int res = 0;
	for (int e_i = 1; e_i < 26 ; e_i++) if (nn->end_res[e_i] > nn->end_res[res]) res = e_i;
	
	return 'A'+res;
}

void backward(struct NeuralNetwork *nn, int *pixel, int *e) //
{
	double *end_delta = calloc(26, sizeof(double));
	double *hidden_delta = calloc(nn->n, sizeof(double));
	
	for (int e_i = 0 ; e_i < 26; e_i++)
		end_delta[e_i] = softmax_B(nn->end_res[e_i], e[e_i]);
	for (int e_i = 0 ; e_i < 26; e_i++) for (int h_i = 0 ; h_i < nn -> n; h_i++)
		hidden_delta[h_i] += end_delta[e_i] * nn -> output_w[h_i][e_i] * sigmoid_prime(nn -> hidden_z[h_i]);
	
	
	for (int e_i = 0 ; e_i < 26; e_i++)
		nn -> end_b[e_i] += nn -> learning_rate * end_delta[e_i];
	for (int h_i = 0 ; h_i < nn -> n; h_i++) for (int e_i = 0 ; e_i < 26; e_i++)  
		nn -> output_w[h_i][e_i] += nn -> learning_rate * end_delta[e_i] * nn -> hidden_res[h_i];
	

	for (int h_i = 0 ; h_i < nn -> n; h_i++) 
		nn -> hidden_b[h_i] += nn -> learning_rate * hidden_delta[h_i];
	for (int p_i = 0 ; p_i < 784; p_i++) for (int h_i = 0 ; h_i < nn -> n; h_i++) 
		nn -> input_w[p_i][h_i] += nn -> learning_rate * hidden_delta[h_i] * (double)pixel[p_i];
	
	free(end_delta);
	free(hidden_delta);
}
/*
void train(struct NeuralNetwork *nn, int loop) //
{
	int *abc = calloc(26, sizeof(int));
	for (int letter = 0 ; letter < 26; letter++)//A - Z
	{
		abc[letter] = 1;
		
		forward(
				nn, 
				image[letter]
				);
				
		backward(
				nn,
				image[letter], 
				abc
				);
				
		abc[Letter] = 0;
	}
	free(abc);
}

int main(int argc, char *argv[]) //
{
	srand(time(NULL));
	struct NeuralNetwork NN = init_NN(3, 0.5);
	if (argc == 1) 
	{
		train(&NN, 100000);
		double res = forward(&NN, 0, 0);
		printf("forward(Letter = ) = %f ≈ %i\n",
			0,
			0,
			res,
			res > 0.5
			);
	}
	else if (argc == 3)
	{
		train(&NN, 100000);
		double res = forward(&NN, test);
		printf("forward((A=%i), (B=%i)) = %f ≈ %i\n",
				atoi(argv[1]),
				atoi(argv[2]),
				res,
				res > 0.5
				);
	}
	else if (argc == 4)
	{
		train(&NN, atoi(argv[3]));
		double res = forward(&NN, test);
		printf("forward((A=%i), (B=%i)) = %f ≈ %i\n",
				atoi(argv[1]),
				atoi(argv[2]),
				res,
				res > 0.5
				);
	}
	else
	{
		printf("🚫");
	}
	free_NN(&NN);
}

*/
