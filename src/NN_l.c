#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>



/*-----------------------------------------------------------------------------
w = weight,  
b = bias, 





 784=28×28
 neurons



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

double sigmoid_prime(double z) //Backpropagation of sigmoid
{return sigmoid(z)*(1-sigmoid(z));}

void softmax(double *z, double *res)
{
	int z_exp_sum = 0;
	for (int i = 0; i < 26; i++) z_exp_sum += exp(z[i]);
	for (int i = 0; i < 26; i++) res[i] = exp(z[i])/z_exp_sum;
}

double softmax_B(double *res, double *y) //Backpropagation of softmax
{return res[i] - y[i];}

struct NeuralNetwork //ok
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

struct NeuralNetwork init_NN(int n, double learning_rate) //ok
{
	struct NeuralNetwork res;
	
	res.n = n;
	
	res.input_w = calloc(784, sizeof(double *));
	for (int i = 0 ; i < 784; i++) res.input_w[i] = calloc(res.n, sizeof(double));
	
	res.output_w = calloc(26, sizeof(double *))
	for (int i = 0 ; i < 26; i++) res.output_w[i] = calloc(res.n, sizeof(double));
	
	res.hidden_b = calloc(res.n, sizeof(double));
	res.hidden_z = calloc(res.n, sizeof(double));;
	res.hidden_res = calloc(res.n, sizeof(double));
	
	res.end_b = calloc(26, sizeof(double));
	res.end_z = calloc(26, sizeof(double));;
	
	res.learning_rate = learning_rate;
	
	for (int i = 0 ; i < res.n; i++)
	{
		for (int j = 0; j < 784 ; j++) res.input_w[j][i] = random_d();
		for (int j = 0; j < 26 ; j++) res.input_w[i][j] = random_d();
		res.hidden_b[i] = random_d(); 
		res.end_b[i] = random_d(); 
	}
	return res;
}

void free_NN(struct NeuralNetwork *nn) //ok
{
	for (int i = 0 ; i < 784; i++) free(nn -> input_w[i]);
	free(nn -> input_w);
	
	for (int i = 0 ; i < 26; i++) free(nn -> output_w[i]);
	free(nn -> output_w);
	
	free(nn -> hidden_b);
	free(nn -> hidden_z);
	free(nn -> hidden_res);
	
	free(nn -> end_b);
	free(nn -> end_z);
	free(nn -> end_res);
}

double forward(struct NeuralNetwork *nn, int *pixel) //ok
{
	for (int j = 0 ; j < res.n; j++)
	{
		nn -> hidden_z[j] = hidden_b[j];
		for (int i = 0 ; i < 784; i++) nn -> hidden_z[j] += nn -> input_w[i][j] * pixel[i]; 
		nn -> hidden_res[j] = sigmoid(nn -> hidden_z[j]);
	}
	for (int j = 0 ; j < 26; j++)
	{
		nn -> end_z[j] = end_b[j];
		for (int i = 0 ; i < res.n; i++) nn -> end_z[j] += nn -> output_w[i][j] * hidden_res[i]; 
	}
	end_res = softmax(nn -> end_z);
	return 
}

void backward(struct NeuralNetwork *nn, int A, int B, double f, int e) //
{
	double *end_delta = calloc(26, sizeof(double));
	for (int i = 0 ; i < 26; i++)
	{
		end_delta[i] = ((double)e - f) 
				* softmax_B(nn -> end_res, nn -> //y));
	}
	/////////////////////////////////////////////////////////////::
	////// new try
	double *end_delta = calloc(26, sizeof(double));
	double **hidden_delta = calloc(nn -> n, sizeof(double *));
	for (int j = 0 ; j < nn -> n; j++) 
		hidden_delta[j] = calloc(26, sizeof(double));
	
	for (int i = 0 ; i < 26; i++)
	{
		end_delta[i] = ((double)e - f) 
				* softmax_B(nn -> end_res, nn -> //y));
		
		for (int j = 0 ; j < nn -> n; j++)
		{
			hidden_delta[j][i] = end_delta[i]
					* nn -> output_w[j][i]
					* sigmoid_prime(nn -> hidden_z[j]);
					
			nn -> output_w[j][i] += nn -> learning_rate 
				* end_delta
				* nn -> hidden_res[i];
		}
	}
	//////
	double **hidden_delta = calloc(nn -> n, sizeof(double *));
	for (int i = 0 ; i < nn -> n; i++)
	{
		hidden_delta[i] = end_delta 
				* nn -> output_w[i]
				* sigmoid_prime(nn -> hidden_z[i]);


		nn -> output_w[i] += nn -> learning_rate 
				* end_delta
				* nn -> hidden_res[i];
		


		nn -> input_w[0][i] += nn -> learning_rate
			* hidden_delta[i] * (double)A;
		nn -> input_w[1][i] += nn -> learning_rate
			* hidden_delta[i] * (double)B;
		nn -> hidden_b[i] += nn -> learning_rate
			* hidden_delta[i];
	}
	
	nn -> end_b += nn -> learning_rate * end_delta;
	
	free(hidden_delta);
}

void train(struct NeuralNetwork *nn, int loop) //
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

int main(int argc, char *argv[]) //
{
	srand(time(NULL));
	struct NeuralNetwork NN = init_NN(3, 0.5);
	if (argc == 1) 
	{
		train(&NN, 100000);
		double res = forward(&NN, 0, 0);
		printf("forward((A=%i), (B=%i)) = %f ≈ %i\n",
			0,
			0,
			res,
			res > 0.5
			);
		res = forward(&NN, 0, 1);
		printf("forward((A=%i), (B=%i)) = %f ≈ %i\n",
			0,
			1,
			res,
			res > 0.5
			);
		res = forward(&NN, 1, 0);
		printf("forward((A=%i), (B=%i)) = %f ≈ %i\n",
			1,
			0,
			res,
			res > 0.5
			);
		res = forward(&NN, 1, 1);
		printf("forward((A=%i), (B=%i)) = %f ≈ %i\n",
			1,
			1,
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

/*
int main(int argc, char *argv[])
{
	srand(time(NULL));
	struct NeuralNetwork NN = init_NN(3, 0.1);
	double res;
	int rr = 1;
	train(&NN, 100000);
	for (int i = 0 ; i < 100000000000; i++)
	{
	    res = forward(&NN, 0, 0);
	    rr = rr && (1 == (res > 0.5));
	    printf("%i",(res > 0.5));
	    res = forward(&NN, 0, 1);
		rr = rr && (0 == (res > 0.5));
		printf("%i",(res > 0.5));
		res = forward(&NN, 1, 0);
		rr = rr && (0 == (res > 0.5));
		printf("%i",(res > 0.5));
		res = forward(&NN, 1, 1);
		rr = rr && (1 == (res > 0.5));
		printf("%i\n",(res > 0.5));
	}
	printf("\n%i",rr);
	free_NN(&NN);
}
*/