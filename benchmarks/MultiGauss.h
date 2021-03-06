
#define TEST_PROBLEM "MultiGauss"
#define N 2

#define PI 3.14159265359
#define SOL -1.2970


void bounds(double lb[], double ub[])
/*Provide lower and upper bounds for each of N variables.
 Number of bounds is equal to N.*/
{
  lb[0] = -2;
  ub[0] = 2;
  lb[1] = -2;
  ub[1] = 2;
   
}

/*Test feasibility of x.  Return 1 if feasible, 0 if not.*/
int feasible(double x[])

{
	return 1;
}

/*Calculate objective function value of x[].*/
double objfn(double x[])
{
	int i;
	double sum=0.;
	static double a[5] = {.5, 1.2, 1., 1., 1.2};
	static double b[5] = {0., 1., 0., -.5, 0.};
	static double c[5] = {0., 0., -.5, 0., 1.};
	static double d[5] = {.1, .5, .5, .5, .5};



	for (i=0; i<5; i++)
	{
		sum = sum - a[i]*exp(-(pow(x[0]-b[i],2)+pow((x[1]-c[i]),2))/pow(d[i],2));
	}

	return (sum);

}

double nelder_objfn(const gsl_vector *x, void *data){
	
	return objfn(x->data);
}

int levermed_objfn(const gsl_vector *x, void *data, gsl_vector *f){
	return 0;
}
