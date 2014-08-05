#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_multifit_nlin.h>

/**
 * Colors code for printing
 */
#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define eul 2.71828182845905
#define pi 3.14159265358979

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

typedef struct individual{
	
	double *params;						/*!< Stores inidividual's parameters */
	double mean_cost;					/*!< Stores the mean of current individual's cost until its randomization */
	double var_cost;					/*!< Stores the variance of current individual's cost until its randomization */
	double cost;						/*!< Stores the cost of current individual */
	double dist;						/*!< Stores the distance of current individual to a set of other individuals */

	int n_notRandomized;				/*!< Stores the number of times that the specific individual updates but not randomize. It will be used to update the stats */
	int nStuck;							/*!< Stores the number of times that the specific individual hasn't updated during the optimization process */

} individual;

typedef struct Set{
	
	individual *members;				/*!< Array of individuals */
	double mean_cost;					/*!< mean cost of all the individual in the current set */
	double std_cost;					/*!< standard deviation of cost of all individuals in the current set */
	double *params_means;				/*!< Stores the mean values of each parameters in the set among all the individuals */
	int size;							/*!< Size of the set */

} Set;

typedef struct Stats{

	int n_successful_goBeyond;
	int n_local_search_performed;
	int n_successful_localSearch;
	int n_local_search_iterations;
	int n_Stuck;
	int n_successful_recombination;
	int n_refSet_randomized;
	int n_duplicate_found;
	int n_flatzone_detected;

	int **freqs_matrix;
	double **probs_matrix;
} Stats;


typedef struct eSSType{

	/**
	 * User Options
	 */
	int logBound;
	int maxeval;						/*!< Maxiumum number of function evaluation before stop */
	int maxtime;						/*!< Maximum CPU time before stop */
	int maxiter;						/*!< Maximum iteration before stop */
	int iterprint;						/*!< Frequency of printing the output including the stats, refSet, and bestSol. */
	bool plot;							/*!< Indicates if the result should be plotted or not. */
	double *weight;						/*!< Parameters importance weights to be used by Levenberg method. */
	double tolc;						
	double prob_bound;
	int strategy;
	int inter_save;
	int warmStart;
	int n_subRegions;
	int debug;
	int log;
	double sol;
	int collectStats;
	int saveOutput;

	int user_guesses;

	int goBeyond_Freqs;

	int iter;

	int perform_refSet_randomization;	/*!< Randomize the refSet if the standard deviation of the set's cost is below some threshold. NOTE: The compute_Set_Stats flag should be ON to compute the neccessary information for this operation. */
	int n_randomization_Freqs;			/*!< The frequency of randomzing stuck refSet memebers, this somehow increase the maxStuck value and give some solution some extra chance to escape the local minima. */
	double set_std_Tol;					/*!< Tolerance value for the standard deviation of a set to perform the randomization */
	/**
	 * Global Options
	 */
	int n_Params;
	int n_DataPoints;					/* Stores the number of datapoints for perfoming the efficient least-square optimizations. */
	double *min_real_var;
	double *max_real_var;
	double **min_boundary_matrix;
	double **max_boundary_matrix;
	
	int n_refSet;						/* Reference Set size, can be set or computed automacitally. */
	Set *refSet;
	
	int n_scatterSet;					/* Scatter Set size, computed automaticall. */
	Set *scatterSet;

	int n_childsSet;
	Set *childsSet;						/* Stores best members of recombinedSet for each refSet member, size: n_refSet.
	`label` variable indidcates indices that has a new values during the iterations. */

	int n_candidateSet;
	Set *candidateSet;					// Stores childs generated from each refSet in each generation, size: n_refSet - 1

	int n_localSearch_Candidate;
	Set *localSearchCandidateSet;

	int n_archiveSet;					/*<! Store the size of archiveSet */
	Set *archiveSet;					/*<! Use for storing the stuck solutions in the refSet. */

	individual *best;					/*<! Pointer to the first member of refSet which is always the best sol */

	int RefSet_Initialization_Method;		/*<! */
	int combination_Method;					/*<! */
	int regeneration_Method;				/*<! */
	int n_delete;					/*<! Specify the number of individual that should be deleted during the randomization of the refSet. */
	int intensification_Freqs;
	int diversification_Type;
	int perform_cost_tol_stopping;
	double cost_Tol;
	int perform_flatzone_check;
	double flatzone_Tol;

	int equality_type;				/*<! Specify how the equality of two individuals should be computed, either by the closness of its parameters (1) or by euclidean distance between two individuals (0) */
	double dist_Tol;
	double param_Tol;
	int maxStuck;

	int perform_refSet_convergence_stopping;
	double refSet_convergence_Tol;

	/**
	 * Local Search Options
	 */
	int perform_LocalSearch;
	char local_method;
	double local_min_criteria;
	int local_maxIter;				/*<! Maximum iterations of the local search algorithm */
	// int local_Freqs;
	int local_SolverMethod;			/*<! Local search method, `l`: Levenberg, 'n': Nelder */
	double local_Tol;				/*<! Local search convergence tolerance */
	// int local_IterPrint;
	int local_N1;					/*<! Starting the local search after local_N1 iterations */
	int local_N2;					/*<! Frequency of local search after first local_N1 iterations */
	int local_Balance;				/*<! Balance between the diversity and quality for choosing the initial point for local search, NOTE: It's not implemented yet! */
	int local_atEnd;				/*<! Indicates if the local_search should only be applied at the end of the search */
	int local_onBest_Only;			/*<! Indicates if the local search should be only applied on the bestSol during the search */
	int local_merit_Filter;
	int local_distance_Filter;
	double local_th_merit_Factor;
	double local_max_distance_Factor;
	int local_wait_maxDist_limit;
	int local_wait_th_limit;


	Stats *stats;					/*<! Storing different statistics durig the running, check `struct Stats` */
	int compute_Ind_Stats;			/*<! Flag that indicates if individuals should compute and store their statistics */ 
	int compute_Set_Stats;			/*<! Flag that indicates if a set should computes and stores its statistics */

} eSSType;



/**
 * Gloabl output files...
 */
extern FILE *refSet_history_file;
extern FILE *best_sols_history_file;
extern FILE *freqs_matrix_file;
extern FILE *freq_mat_final_file;
extern FILE *prob_mat_final_file;
extern FILE *refSet_final_file;
extern FILE *stats_file;
extern FILE *ref_set_stats_history_file;
extern FILE *user_initial_guesses_file;
extern FILE *archive_set_file;


/**
 * essInit.c
 */
void init_defaultSettings(eSSType *eSSParams);
void init_essParams(eSSType*);
void init_scatterSet(eSSType*, void*, void*);
void init_refSet(eSSType*, void*, void*);
void init_report_files(eSSType *);
void init_stats(eSSType *);
void init_warmStart(eSSType *);

/**
 * essStats.c
 */
void updateFrequencyMatrix(eSSType*);
void compute_Mean(eSSType*, individual*);
void compute_Std(eSSType*, individual*);
void compute_SetStats(eSSType*, Set*);
void update_SetStats(eSSType *eSSParams, Set *set);
void update_IndStats(eSSType *, individual *);

/**
 * essGoBeyond
 */
void goBeyond(eSSType*, int, void*, void*);

/**
 * essLocalSearch.c
 */
void localSearch(eSSType*, individual*, void*, void*, char method);
int levmer_localSearch(eSSType *eSSParams, individual *ind, void *inp, void *out);
int neldermead_localSearch(eSSType *eSSParams, individual *ind, void *inp, void *out);

/**
 * essRecombine.c
 */
int recombine(eSSType*, individual*, int, void*, void*);

/**
 * essSort.c
 */
void quickSort_Set(eSSType*, Set*, int, int, char);
void quickSort(eSSType*, Set*, double*, int, int);
void insertionSort(eSSType*, Set*, int, char);

/**
 * essAllocate.c
 */
void allocate_Ind(eSSType *, individual *);
void deallocate_Ind(eSSType *, individual *);
void allocate_Set(eSSType *, Set *);
void deallocate_Set(eSSType *, Set *);
void deallocate_eSSParams(eSSType *);


/**
 * essTools.c
 */
double euclidean_distance(eSSType*, individual*, individual*);
void isExist(eSSType*, individual*);
double min(double*, int, int*);
double max(double*, int, int*);
void copy_Ind(eSSType *, individual *, individual *);
void delete_and_shift(eSSType *eSSParams, Set *set, int set_size, int index_to_delete);
int closest_member(eSSType *, Set *, int , individual *, int );
int is_exist(eSSType *eSSParams, Set *set, individual *ind);
bool is_equal_dist(eSSType *eSSParams, individual *ind1, individual *ind2);
bool is_equal_pairwise(eSSType *eSSParams, individual *ind1, individual *ind2);
bool is_in_flatzone(eSSType *eSSParams, Set *set, individual *ind);

/**
 * essRand.c
 */
double rndreal(double, double);
void random_Set(eSSType*, Set*, double*, double* );
void random_Ind(eSSType*, individual*, double*, double* );

/**
 * essProblem.c
 */
double objectiveFunction(eSSType*, individual*, void*, void*);
void init_sampleParams(eSSType*);

double objfn(double []);
void bounds(double lb[], double ub[]);
int feasible(double x[]);
int levermed_objfn(const gsl_vector *x, void *data, gsl_vector *f);
double nelder_objfn(const gsl_vector *x, void *data);


/**
 * Benchmark functions prototype
 */
void bounds(double lb[], double ub[]);
int feasible(double x[]);
double objfn(double x[]);
double nelder_objfn(const gsl_vector *x, void *data);


/**
 * essEvaluate.c
 */
void evaluate_Individual(eSSType*, individual*, void*, void*);
void evaluate_Set(eSSType*, Set*, void*, void*);

/**
 * ess.c
 */
void init_eSS(eSSType*, void*, void*);
void run_eSS(eSSType*, void*, void*);


/**
 * essIO.c
 */
void read_cli_params(eSSType *, int, char **);
void print_eSSParams(eSSType*);
void print_Set(eSSType*, Set*);
void print_Ind(eSSType*, individual*);
void write_Set(eSSType*, Set*, FILE*, int);
void write_Ind(eSSType*, individual*, FILE*, int);
void print_Stats(eSSType *);
void write_Stats(eSSType *, FILE *);
void parse_double_row(eSSType *eSSParams, char *line, double *row);
void parse_int_row(eSSType *eSSParams, char *line, int *row);
void print_Inputs(eSSType *);

/**
 * essMain.c
 */




