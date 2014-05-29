#include "ess.h"

/**
 * Request heap memory for all the variables needed based on their size.
 * So, the init_sampleParams should be called before.
 */
void init_essParams(eSSType *eSSParams){


	eSSParams->stats->n_successful_goBeyond      = 0;   
	eSSParams->stats->n_successful_localSearch   = 0;
	eSSParams->stats->n_Stuck                    = 0;     
	eSSParams->stats->n_successful_recombination = 0;            



	eSSParams->refSet = (Set*)malloc(sizeof(Set));
	eSSParams->refSet->size = eSSParams->n_refSet;
	allocate_Set(eSSParams, eSSParams->refSet);

	eSSParams->scatterSet = (Set*)malloc(sizeof(Set));
	eSSParams->scatterSet->size = eSSParams->n_scatterSet;
	allocate_Set(eSSParams, eSSParams->scatterSet);

	eSSParams->childsSet = (Set*)malloc(sizeof(Set));
	eSSParams->childsSet->size = eSSParams->n_childsSet;
	allocate_Set(eSSParams, eSSParams->childsSet);

	eSSParams->candidateSet = (Set*)malloc(sizeof(Set));
	eSSParams->candidateSet->size = eSSParams->n_candidateSet;
	allocate_Set(eSSParams, eSSParams->candidateSet);

	eSSParams->best = (individual*)malloc(sizeof(individual));

	/**
	 * Initializing stats matrices and boundary matrices
	 */
	eSSParams->stats = (Stats *)malloc(sizeof(Stats));

	eSSParams->stats->freqs_matrix = (int **)malloc(eSSParams->n_Params * sizeof(int *));
	eSSParams->stats->probs_matrix = (double **)malloc(eSSParams->n_Params * sizeof(double *));
	
	// Generating the sub regions matrixes
	eSSParams->min_boundary_matrix = (double **)malloc( eSSParams->n_Params * sizeof(double *));
	eSSParams->max_boundary_matrix = (double **)malloc( eSSParams->n_Params * sizeof(double *));

	for (int i = 0; i < eSSParams->n_Params; ++i)
	{
		eSSParams->min_boundary_matrix[i] = (double *)malloc((eSSParams->n_subRegions) * sizeof(double));
		eSSParams->max_boundary_matrix[i] = (double *)malloc((eSSParams->n_subRegions) * sizeof(double));
		
		eSSParams->stats->freqs_matrix[i] = (int *)malloc(eSSParams->n_subRegions * sizeof(int));
		eSSParams->stats->probs_matrix[i] = (double *)malloc(eSSParams->n_subRegions * sizeof(double));

		for (int j = 1; j <= eSSParams->n_subRegions; ++j)
		{
			/* Building the sub region matrixes */	// One matrix would be enough but for clarity I use two.
			eSSParams->min_boundary_matrix[i][j - 1] = eSSParams->min_real_var[i] + ((eSSParams->max_real_var[i] - eSSParams->min_real_var[i]) / eSSParams->n_subRegions) * (j - 1);
			eSSParams->max_boundary_matrix[i][j - 1] = eSSParams->min_real_var[i] + ((eSSParams->max_real_var[i] - eSSParams->min_real_var[i]) / eSSParams->n_subRegions) * j; 
			
			/* Building the freqs_matrix: frequently_matrix */
			eSSParams->stats->freqs_matrix[i][j - 1] = 1;
			eSSParams->stats->probs_matrix[i][j - 1] = 1./(eSSParams->n_subRegions);		// Since all values are 1.
		}
	}


}

void init_report_files(eSSType *eSSParams){
	char mode = 'w';
	if (eSSParams->is_warmStart)
		mode = 'a';
	refSet_history_file   = fopen("refSet_history_file.out", &mode);
	best_sols_history_file = fopen("best_sols_history_file.out", &mode);
	freqs_matrix_file      = fopen("freqs_matrix_history.out", &mode);
	stats_file			   = fopen("stats_file.csv", &mode);	
	// TODO: Check the mode
}


void init_stats(eSSType *eSSParams){

}

/**
 * Generate scatterSet by first generating one solution in each subRegion and then expanding
 * the set by preserving the diversity.
 * @param eSSParams 
 * @param inp       
 * @param out       
 */
void init_scatterSet(eSSType *eSSParams, void* inp, void *out){

	// Generating Scatter Set with the size of p
	// The first `p` members are uniformy selected from all subRegions
	for (int k = 0; k < eSSParams->n_subRegions; ++k)
		for (int i = 0; i < eSSParams->n_Params; ++i)
			eSSParams->scatterSet->members[k].params[i] = rndreal(eSSParams->min_boundary_matrix[i][k], eSSParams->max_boundary_matrix[i][k]);

	// `a` indicates the index of selected subRegions that the new value should be generated from.
	int a            = 0;
	double probs_sum = 0;
	double rnd       = 0;

	/* Generate new item to add to scatterSet
	   eSSParams->scatterSet->members[eSSParams->p + k] is going to be generated */
	for (int k = 0; k < eSSParams->scatterSet->size - eSSParams->n_subRegions; ++k)
	{		
		for (int i = 0; i < eSSParams->n_Params; ++i)
		{	
			rnd = rndreal(0, 1);
			for (int j = 0; j < eSSParams->n_subRegions; ++j)
			{	
				/* Compute `probs` and find the index */
				probs_sum += eSSParams->stats->probs_matrix[i][j];

				if ( rnd <= probs_sum ){
					a = j;
					
					// Updating the f matrix, and prob_matrix
					eSSParams->stats->freqs_matrix[i][a]++;
					
					// Updating the eSSParams->stats->probs_matrix
					// IMPROVE: This could be rewritten much nicer
					double f_col_prob = 0;
					for (int t = 0; t < eSSParams->n_subRegions; ++t){
						f_col_prob += (1. / eSSParams->stats->freqs_matrix[i][t]);
					}
					eSSParams->stats->probs_matrix[i][a] = (1. / eSSParams->stats->freqs_matrix[i][a] ) / (f_col_prob);

					probs_sum = 0;
					break;
				}
			}

			// subRegoins index is selected now: `a`
			eSSParams->scatterSet->members[eSSParams->n_subRegions + k].params[i] = rndreal(eSSParams->min_boundary_matrix[i][a], eSSParams->max_boundary_matrix[i][a] );
		}

	}	// Scatter Set is now extented...

	// #ifdef STATS
	// 	write_int_matrix(eSSParams, eSSParams->freqs_matrix, eSSParams->n_Params, eSSParams->p, freqs_matrix_file, 0, 'w');
	// #endif

}

void init_refSet(eSSType *eSSParams, void* inp, void *out){

	printf("Forming the refSet...\n");

	int m = eSSParams->scatterSet->size;
	int b = eSSParams->refSet->size;
	int h = b / 2;	// max_elite

	// Sorting the scatterSet
	quickSort_Set(eSSParams, eSSParams->scatterSet, 0, eSSParams->scatterSet->size - 1, 'c');

	for (int i = 0; i < h; ++i){
		// Copying first `h` best items of the scatterSet with respect to the cost 
		copy_Ind(eSSParams, &(eSSParams->refSet->members[i]), &(eSSParams->scatterSet->members[i]));
	}

	/************************************************************/
	/* Expanding the refSet by applying the diversity distance */
	/************************************************************/
	
	// dist_matrix will store the distances between the rest of scatterSet and the refSet
	double **dist_matrix = (double **)malloc( (m - h) * sizeof(double *));;
	for (int i = 0; i < (m - h); ++i)
		dist_matrix[i] = (double *)malloc( (b) * sizeof(double));

	double *min_dists = (double *)malloc((m - h) * sizeof(double));

	int min_index = 0;
	int max_index = 0;

	int i = 0;
	for (int k = h; k < b; ++k, --m)		// Iterate through the refSet for expanding
	{										// --m is reduce the depth that the second `for` should go through it
											// since the array is shifted after each iteration

		for (i = h; i < m; ++i)		// Iterate through the rest of scatterSet
		{							 

			for (int j = 0; j < k; ++j)		// Iterate trhough the selected members of refSet
			{								
				/* Computing the distance */
				dist_matrix[i - (b / 2)][j] = euclidean_distance(eSSParams, &(eSSParams->scatterSet->members[i]), &(eSSParams->refSet->members[j]));
			}

			// Find the minimum of each row into a new variable, then finding it's max
			min_dists[i - (b / 2)] = min(dist_matrix[i - (b / 2)], k, &min_index);
		}
		
		// Now, the min_dists of vectors to all the members of R is computed,
		// we could compute the maximum of them and also it's index
		
		// Finding the place of the max/min distance
		max(min_dists, i - (b / 2), &max_index); 

		// Adding the vector in max_index to the refSet
		copy_Ind(eSSParams, &(eSSParams->refSet->members[k]), &(eSSParams->scatterSet->members[h + max_index]));

		// After assigment, the rest of should be shift into the left form the position that the 
		// vector is selected
		
		// It could be done more complicated!
		delete_and_shift(eSSParams, eSSParams->scatterSet, eSSParams->n_scatterSet, (b / 2) + max_index);

	}

	// #ifdef STATS
	// 	write_int_matrix(eSSParams, eSSParams->stats->freqs_matrix, eSSParams->n_Params, eSSParams->n_subRegions, freqs_matrix_file, 0, 'w');
	// #endif

	free(min_dists);
	for (int i = 0; i < (eSSParams->n_scatterSet - b/2); ++i){
		free(dist_matrix[i]);
	}
	free(dist_matrix);

}







