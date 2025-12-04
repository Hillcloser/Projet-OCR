// Neural network structure
struct NeuralNetwork
{
	int n; // Number of hidden units

	double **input_w; // Input weights [784 x n]
	double **output_w; // Output weights [n x 26]

	double *hidden_b; // Hidden biases [n]
	double *hidden_z; // Hidden layer pre-activation [n]
	double *hidden_res; // Hidden layer post-activation [n]

	double *end_b;	 // Output biases [26]
	double *end_z;	 // Output layer pre-activation [26]
	double *end_res;	// Output layer post-activation [26]

	double learning_rate; // Learning rate for gradient descent
};

// Function declarations for Neural Network operations
struct NeuralNetwork init_NN(int n, double learning_rate);
void free_NN(struct NeuralNetwork *nn);
char forward(struct NeuralNetwork *nn, int *pixel); // Forward pass
void backward(struct NeuralNetwork *nn, int *pixel, int *e); // Backward pass (backpropagation)
int nn_main();