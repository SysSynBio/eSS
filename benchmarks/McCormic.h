
#define TEST_PROBLEM "McCormic"
#define N 2

#define MAXITER 100*N*N /*500*N */

#define PI 3.14159265359
#define SOL -1.9133


void bounds(double lb[], double ub[])
/*Provide lower and upper bounds for each of N variables.
 Number of bounds is equal to N.*/
{
  lb[0] = -1.5;
  ub[0] = 4;
  lb[1] = -3;
  ub[1] = 3;
}

/*Test feasibility of x.  Return 1 if feasible, 0 if not.*/
int feasible(double x[])

{
	return 1;
}

/*Calculate objective function value of x[].*/
double objfn(double x[])
{
	double sum=0.;

    sum = sin((x[0]+x[1])) + pow(x[0]-x[1],2) - 1.5*x[0] + 2.5*x[1] + 1.;  
	
	return (sum);

}

double nelder_objfn(const gsl_vector *x, void *data){
	
	return objfn(x->data);
}

int levermed_objfn(const gsl_vector *x, void *data, gsl_vector *f){
	return 0;
}
