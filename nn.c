#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>



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

double sigmoid(double z)
{return 1.0/(1.0 + exp(-z));}

double sigmoid_prime(double z)
{return sigmoid(z)*(1-sigmoid(z));}

struct NeuralNetwork 
{
	int n; //hidden len
	
	double **input_w;
	double *output_w;
	
	double *hidden_b;
	double *hidden_z;
	double *hidden_res;
	
	double end_b;
	double end_z;
	double end_res;
	
	double learning_rate;
};

struct NeuralNetwork init_NN(int n, double learning_rate)
{
	struct NeuralNetwork res;
	
	res.n = n;
	
	res.input_w = calloc(2, sizeof(double *));
	res.input_w[0] = calloc(res.n, sizeof(double));
	res.input_w[1] = calloc(res.n, sizeof(double));
	
	res.output_w = calloc(res.n, sizeof(double));
	
	res.hidden_b = calloc(res.n, sizeof(double));
	res.hidden_z = calloc(res.n, sizeof(double));;
	res.hidden_res = calloc(res.n, sizeof(double));
	
	res.end_b = random_d();
	
	res.learning_rate = learning_rate;
	
	for (int i = 0 ; i < res.n; i++)
	{
		for (int j = 0; j < 2 ; j++) res.input_w[j][i] = random_d();
		res.output_w[i] = random_d();
		res.hidden_b[i] = random_d(); 
	}
	return res;
}

void free_NN(struct NeuralNetwork *nn)
{
	free(nn -> input_w[0]);
	free(nn -> input_w[1]);
	free(nn -> input_w);
	free(nn -> output_w);
	free(nn -> hidden_b);
	free(nn -> hidden_z);
	free(nn -> hidden_res);
}

double forward(struct NeuralNetwork *nn, int A, int B)
{
	int i = 0;
	while (i < nn -> n)
	{
		nn -> hidden_z[i] = nn -> input_w[0][i] * (double)A
			+ nn -> input_w[1][i] * (double)B
			+ nn -> hidden_b[i];
		i++;
	}
	i = 0;
	nn -> end_z = nn -> end_b;
	while (i < nn -> n)
	{
		nn -> hidden_res[i] = sigmoid(nn -> hidden_z[i]);
		nn -> end_z += nn -> output_w[i] * nn -> hidden_res[i];
		i++;
	}
	nn -> end_res = sigmoid(nn -> end_z);
	
	return nn -> end_res;
}

void backward(struct NeuralNetwork *nn, int A, int B, double f, int e)
{
	double end_delta = ((double)e - f)
		* sigmoid_prime(nn -> end_z);
	double *hidden_delta = calloc(nn -> n, sizeof(double));
	for (int i = 0 ; i < nn -> n; i++)
	{
		hidden_delta[i] = end_delta 
				* nn -> output_w[i]
				* sigmoid_prime(nn -> hidden_z[i]);


		nn -> output_w[i] -= nn -> learning_rate 
				* end_delta
				* nn -> hidden_res[i];
		


		nn -> input_w[0][i] -= nn -> learning_rate
			* hidden_delta[i] * (double)A;
		nn -> input_w[1][i] -= nn -> learning_rate
			* hidden_delta[i] * (double)B;
		nn -> hidden_b[i] -= nn -> learning_rate
			* hidden_delta[i];
	}
	
	nn -> end_b -= nn -> learning_rate * end_delta;
	
	free(hidden_delta);
}

void train(struct NeuralNetwork *nn, int loop)
{
	double tmp;
	for (int i = 0 ; i < loop; i++)
	{
		tmp = forward(nn, 0, 0);
		backward(nn, 0, 0, tmp, 1);
		tmp = forward(nn, 0, 1);
		backward(nn, 0, 1, tmp, 0);
		tmp = forward(nn, 1, 0);
		backward(nn, 1, 0, tmp, 0);
		tmp = forward(nn, 1, 1);
		backward(nn, 1, 1, tmp, 1);
	}
}

int main(int argc, char *argv[])
{
	srand(time(NULL));
	struct NeuralNetwork NN = init_NN(2, 0.5);
	if (argc == 4) train(&NN, atoi(argv[3]));
	double res = forward(&NN, atoi(argv[1]), atoi(argv[2]));
	printf("forward((A=%i), (B=%i)) = %f ≈ %i",
			atoi(argv[1]),
			atoi(argv[2]),
			res,
			res > 0.5
			);
	free_NN(&NN);
}
