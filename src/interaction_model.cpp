//
//  interaction_model.cpp
//  
//
//  Copyright (c) 2016 The Voth Group at The University of Chicago. All rights reserved.
//

#include "interaction_model.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sstream>

//---------------------------------------------------------------
// Prototypes for internal implementation
// Member Functions of InteractionClassSpec
//---------------------------------------------------------------

void report_tabulated_interaction_data_consistency_error(const int line);
void report_tabulated_interaction_format_error(const int line);
void check_nonbonded_interaction_range_cutoffs(PairNonbondedClassSpec *const ispec, double const cutoff);

// Get the name of a single defined interaction via its index among
// defined intereactions.

std::string InteractionClassSpec::get_interaction_name(char **type_names, const int intrxn_index_among_defined) const
{
    // Name first by the types involved in the interaction.
    // Name as type1_type2..._typeN.
    std::vector<int> types = get_interaction_types(intrxn_index_among_defined);
    std::string namestring = std::string(type_names[types[0] - 1]);
    for(unsigned i = 1; i < types.size(); i++) {
        namestring += std::string("_") + type_names[types[i] - 1];
    }
    // Append the class's short name with an underscore if one exists.
    if (!get_short_name().empty()) {
        namestring += "_" + get_short_name();
    }
    return namestring;
}

std::vector<int> InteractionClassSpec::get_interaction_types(const int index_among_defined_intrxns) const 
{
    std::vector<int> types(get_n_body(), 0);
    invert_interaction_hash(get_hash_from_index(index_among_defined_intrxns), n_cg_types, types);
    return types;
}

// Check that specified nonbonded interactions do not extend past the nonbonded cutoff
void check_nonbonded_interaction_range_cutoffs(PairNonbondedClassSpec *const ispec, double const cutoff)
{
	for (int i = 0; i < ispec->n_defined; i++) {
		if (ispec->defined_to_matched_intrxn_index_map[i] != 0) {
			if (ispec->upper_cutoffs[i] > (cutoff + ispec->output_binwidth + VERYSMALL) ) {
				printf("An upper cutoff (%lf) specified in the range file is larger than the pair nonbonded cutoff speicified in the control file (%lf).\n", ispec->upper_cutoffs[i], cutoff);
				printf("This can lead to unphysical looking interactions.\n");
				printf("Please adjust and run again.\n");
				fflush(stdout);
				exit(EXIT_FAILURE);
			}
		}
	}
}

// Functions for reading a range.in file and assigning the FM matrix column indices for each basis function.

void InteractionClassSpec::read_interaction_class_ranges(FILE *range_in)
{
    printf("Reading interaction ranges for %d %s interactions.\n", get_n_defined(), get_full_name().c_str());    

    int total_tabulated = 0;
    int total_to_fm = 0;
    char mode[100] = "";
	char junk[10];
	
    for (int i = 0; i < n_defined; i++) {
        // Skip over the type information.
        for (int j = 0; j < get_n_body(); j++) {
            fscanf(range_in, "%s", junk);
        }

        fscanf(range_in, "%lf%lf%s\n", &lower_cutoffs[i], &upper_cutoffs[i], mode);

        // If the mode is fm, the interaction is not actually in the model.
        // If the mode is fm or tab+fm, the interaction should be force matched.
        // If the mode is tab or tab+fm, the interaction should be tabulated.
        
        if (strcmp(mode,"none") != 0 && strcmp(mode,"fm") != 0 && strcmp(mode,"tab") != 0 && strcmp(mode,"fm+tab") != 0) {
            printf("Interaction mode %s is not recognized\n", mode);
            exit(EXIT_FAILURE);
        }

        if (strcmp(mode,"fm") == 0 || strcmp(mode,"tab+fm") == 0) {
			// This interaction is to be force matched.
            // Increment running total and set the new index.
            total_to_fm++;
            defined_to_matched_intrxn_index_map[i] = total_to_fm;
            // Adjust for a basis by rounding the cutoffs to even
            // numbers of bins.
			adjust_cutoffs_for_basis(i);
            // Adjust nonbonded interactions to match the global cutoff,
            // adjust dihedrals by 180 if the interactions are degree-based.
            adjust_cutoffs_for_type(i);
		}
        if (strcmp(mode,"tab") == 0 || strcmp(mode,"tab+fm") == 0) {
			// This interaction is tabulated.
			total_tabulated++;
			defined_to_tabulated_intrxn_index_map[i] = total_tabulated;
		}
    }
    n_to_force_match = total_to_fm;
    n_tabulated = total_tabulated;
    printf("Will force match %d %s interactions; %d are tabulated.\n", n_to_force_match, get_full_name().c_str(), n_tabulated);    
}

void InteractionClassSpec::adjust_cutoffs_for_basis(int i)
{
    if (basis_type == kLinearSpline) {
        lower_cutoffs[i] = floor(lower_cutoffs[i] / output_binwidth + 0.5) * output_binwidth;
        if (lower_cutoffs[i] < 0.0) lower_cutoffs[i] = 0.0;
        upper_cutoffs[i] = lower_cutoffs[i] + floor((upper_cutoffs[i] - lower_cutoffs[i]) / fm_binwidth + 0.5) * fm_binwidth;
    } else if (basis_type == kBSpline) {
        upper_cutoffs[i] = (((int)(upper_cutoffs[i] / output_binwidth)) + 1) * output_binwidth;
        lower_cutoffs[i] = upper_cutoffs[i] - ((int)((upper_cutoffs[i] - lower_cutoffs[i]) / fm_binwidth) + 1) * fm_binwidth;
    }
}

void InteractionClassSpec::adjust_cutoffs_for_type(int i)
{
    if (basis_type == kLinearSpline) {
        if (class_type == kPairNonbonded && fabs(upper_cutoffs[i] - cutoff - fm_binwidth) < VERYSMALL_F) upper_cutoffs[i] -= fm_binwidth;
    }
    if (class_type == kDihedralBonded && class_subtype == 0) {
        upper_cutoffs[i] -= 180.0;
        lower_cutoffs[i] -= 180.0;
    }
}

// Determine number of columns for each interaction to be force matched.

void InteractionClassSpec::setup_indices_in_fm_matrix(void)
{
	int counter = 0;
	int grid_i;
	interaction_column_indices = std::vector<unsigned>(n_to_force_match + 1, 0);

	for (int i = 0; i < n_defined; i++) {
		if (defined_to_matched_intrxn_index_map[i] != 0) {
			grid_i = floor((upper_cutoffs[i] - lower_cutoffs[i]) / fm_binwidth + 0.5) + 1;
			if (grid_i > 1000) {
				fprintf(stderr, "\nWarning: An individual interaction has more than 1000 bins associated with it!\n");
				fprintf(stderr, "Please check that this is intentional.\n");
				fprintf(stderr, "This may be a sign that the wrong angle_style and dihedral_style is selected.\n\n");
				fflush(stderr);
			}
			
			interaction_column_indices[counter + 1] = interaction_column_indices[counter] + grid_i;
			if (basis_type == kBSpline) interaction_column_indices[counter + 1] = interaction_column_indices[counter + 1] + bspline_k - 2;
			counter++;
		}
	}
}

// Allocate space for interactions that will be used.

void InteractionClassSpec::setup_for_defined_interactions(TopologyData* topo_data)
{
	n_cg_types = topo_data->n_cg_types;
    determine_defined_intrxns(topo_data);
	defined_to_matched_intrxn_index_map = std::vector<unsigned>(n_defined, 0);
	defined_to_tabulated_intrxn_index_map = std::vector<unsigned>(n_defined, 0);
	lower_cutoffs = new double[n_defined];
	upper_cutoffs = new double[n_defined];
	n_to_force_match = 0;
	n_tabulated = 0;
}

void free_interaction_data(CG_MODEL_DATA* cg)
{
    printf("Freeing interaction classes.\n"); fflush(stdout);
    std::list<InteractionClassComputer*>::iterator icomp_iterator;
	for(icomp_iterator=cg->icomp_list.begin(); icomp_iterator != cg->icomp_list.end(); icomp_iterator++) {
		delete (*icomp_iterator)->fm_s_comp;
	}
	
	// free three body interaction data
	if(cg->three_body_nonbonded_computer.fm_s_comp != NULL) delete cg->three_body_nonbonded_computer.fm_s_comp;
}

//--------------------------------------------------------------------
// Functions for setting up the potential model that will be used
// in the CG model from a range.in file using lots of hard-to-maintain
// tricks like 'if the range starts from -1, don't include this
// interaction in the model'
//--------------------------------------------------------------------

// Tabulated potential reading error reporting functions

void report_tabulated_interaction_format_error(const int line)
{
    printf("Wrong format in table.in:line %d!\n", line);
    exit(EXIT_FAILURE);
}

void report_tabulated_interaction_data_consistency_error(const int line)
{
    printf("Numbers of tabulated interactions from lower_cutoffs.in/pair_bond_interaction_lower_cutoffs.in and table.in are not consistent:line %d!\n", line);
    exit(EXIT_FAILURE);
}

void read_all_interaction_ranges(CG_MODEL_DATA* const cg)
{
    // Determine the number of interactions that are actually present in the model for each class of interactions, 
    // allocate a hash array and an index array, then set up the hash array.
    // The index array must be filled in from the range specifications in rmin.in and rmin_b.in.
	std::list<InteractionClassSpec*>::iterator iclass_iterator;
	for(iclass_iterator=cg->iclass_list.begin(); iclass_iterator != cg->iclass_list.end(); iclass_iterator++) {
		(*iclass_iterator)->setup_for_defined_interactions(&cg->topo_data);
	}

    // Now deal with three body nonbonded interactions if needed; these do not fit the normal scheme.	
	// This is equivalent to determine_defined_intrxns functions inside of setup_for_defined_interactions.
    if (cg->three_body_nonbonded_interactions.class_subtype > 0) {
        cg->three_body_nonbonded_interactions.defined_to_possible_intrxn_index_map = std::vector<unsigned>(cg->three_body_nonbonded_interactions.get_n_defined(), 0);
        
		// Set up the hash table for three body nonbonded interactions.
        int counter = 0;
        for (int i = 0; i < cg->n_cg_types; i++) {
            for (int j = 0; j < cg->tb_n[i]; j++) {
                cg->three_body_nonbonded_interactions.defined_to_possible_intrxn_index_map[counter] = calc_three_body_interaction_hash(i + 1, cg->tb_list[i][2 * j], cg->tb_list[i][2 * j + 1], cg->n_cg_types);
                counter++;
            }
        }
        // Free the dummy topology used to define three body potentials.
        delete [] cg->tb_n;
        for (int i = 0; i < cg->n_cg_types; i++) delete [] cg->tb_list[i];
        delete [] cg->tb_list;
	}
	// This is equivelant to the rest of setup_for_defined_interactions.
    if (cg->three_body_nonbonded_interactions.class_subtype > 0) {
        // Allocate space for the three body nonbonded hash tables analogously to the bonded interactions.
        cg->three_body_nonbonded_interactions.defined_to_matched_intrxn_index_map = std::vector<unsigned>(cg->three_body_nonbonded_interactions.get_n_defined(), 0);   
		cg->three_body_nonbonded_interactions.defined_to_tabulated_intrxn_index_map = std::vector<unsigned>(cg->three_body_nonbonded_interactions.get_n_defined(), 0);   
        cg->three_body_nonbonded_interactions.lower_cutoffs = new double[cg->three_body_nonbonded_interactions.get_n_defined()];
        cg->three_body_nonbonded_interactions.upper_cutoffs = new double[cg->three_body_nonbonded_interactions.get_n_defined()];

        // The three body interaction basis functions depend only 
        // on a single angle by default.
        for (int i = 0; i < cg->three_body_nonbonded_interactions.get_n_defined(); i++) {
            cg->three_body_nonbonded_interactions.defined_to_matched_intrxn_index_map[i] = i + 1;
			cg->three_body_nonbonded_interactions.defined_to_tabulated_intrxn_index_map[i] = 0;
            cg->three_body_nonbonded_interactions.lower_cutoffs[i] = 0.0;
            cg->three_body_nonbonded_interactions.upper_cutoffs[i] = 180.0;
        }

	} else {
        cg->three_body_nonbonded_interactions.defined_to_matched_intrxn_index_map = std::vector<unsigned>(1, 0);
        cg->three_body_nonbonded_interactions.defined_to_tabulated_intrxn_index_map = std::vector<unsigned>(1, 0);  
        cg->three_body_nonbonded_interactions.upper_cutoffs = new double[1];
        cg->three_body_nonbonded_interactions.lower_cutoffs = new double[1];
        cg->three_body_nonbonded_interactions.interaction_column_indices = std::vector<unsigned>(1, 0);
    	
    }

    // Read normal range specifications.
    // Open the range files.
    FILE* nonbonded_range_in = open_file("rmin.in", "r");
    FILE* bonded_range_in = open_file("rmin_b.in", "r");
    // Read the ranges.
	for(iclass_iterator=cg->iclass_list.begin(); iclass_iterator != cg->iclass_list.end(); iclass_iterator++) {
        if ((*iclass_iterator)->n_defined == 0) continue;
        if ((*iclass_iterator)->class_type == kPairNonbonded) {
            (*iclass_iterator)->read_interaction_class_ranges(nonbonded_range_in);
        } else {
            (*iclass_iterator)->read_interaction_class_ranges(bonded_range_in);
        }
    }
    // Close the range files.
    fclose(nonbonded_range_in);
    fclose(bonded_range_in);

	// Check that specified nonbonded interactions do not extend past the nonbonded cutoff
	check_nonbonded_interaction_range_cutoffs(&cg->pair_nonbonded_interactions, cg->pair_nonbonded_cutoff);
	
    // Allocate space for the column index of each block of basis functions associated with each class of interactions active
    // in the model and meant for force matching, then fill them in class by class.
	for(iclass_iterator=cg->iclass_list.begin(); iclass_iterator != cg->iclass_list.end(); iclass_iterator++) {
		(*iclass_iterator)->setup_indices_in_fm_matrix();
	}
	
	// Now, handle similar actions for three body interactions.
    if (cg->three_body_nonbonded_interactions.class_subtype > 0) {
		cg->three_body_nonbonded_interactions.interaction_column_indices = std::vector<unsigned>(cg->three_body_nonbonded_interactions.get_n_defined(), 0);
		cg->three_body_nonbonded_interactions.interaction_column_indices[0] = 0;

        if (cg->three_body_nonbonded_interactions.class_subtype == 3) {
            // For this style, the whole interaction contributes only one single basis function.
            for (int i = 1; i < cg->three_body_nonbonded_interactions.get_n_defined() + 1; i++) cg->three_body_nonbonded_interactions.interaction_column_indices[i] = i;
        } else {  
            // For other styles, the potential contributes more basis functions.
            if (cg->three_body_nonbonded_interactions.get_basis_type() == kBSpline) { // Set up a B-spline basis for this interaction.
                for (int i = 1; i < cg->three_body_nonbonded_interactions.get_n_defined() + 1; i++) cg->three_body_nonbonded_interactions.interaction_column_indices[i] = i * (cg->three_body_nonbonded_interactions.get_bspline_k() - 2 + floor(180.0 / cg->three_body_nonbonded_interactions.get_fm_binwidth() + 0.5) + 1);
            } else if (cg->three_body_nonbonded_interactions.get_basis_type() == kLinearSpline) { // Set up a linear spline basis for this interaction.
                for (int i = 1; i < cg->three_body_nonbonded_interactions.get_n_defined() + 1; i++) cg->three_body_nonbonded_interactions.interaction_column_indices[i] = i * (floor(180.0 / cg->three_body_nonbonded_interactions.get_fm_binwidth() + 0.5) + 1);
			}
		}
	}
	
}

void read_tabulated_interaction_file(CG_MODEL_DATA* const cg, int n_cg_types)
{
    int line = 0;
    FILE* external_spline_table = open_file("table.in", "r");   

	std::list<InteractionClassSpec*>::iterator iclass_iterator;
	for(iclass_iterator=cg->iclass_list.begin(); iclass_iterator != cg->iclass_list.end(); iclass_iterator++) {
		line = (*iclass_iterator)->read_table(external_spline_table, line, cg->n_cg_types);
	}

    fclose(external_spline_table);
}

int InteractionClassSpec::read_table(FILE* external_spline_table, int line, int n_ypes) 
{
    char buff[100];
    char parameter_name[50];
    int hash_val, index_among_defined;
    int n_external_splines_to_read = 0;
    
    // Read the file header.
    fgets(buff, 100, external_spline_table);
    line++;
    sscanf(buff, "%s%d%lf", parameter_name, &n_external_splines_to_read, &external_table_spline_binwidth);
    if (strcmp(parameter_name, get_table_name().c_str()) != 0) report_tabulated_interaction_format_error(line);
    if (n_external_splines_to_read != n_tabulated) report_tabulated_interaction_data_consistency_error(line);
    if (n_external_splines_to_read <= 0) return line;
    
    // Read each of the tabulated interactions.
    external_table_spline_coefficients = new double*[n_external_splines_to_read];
    for (int i = 0; i < n_external_splines_to_read; i++) {
        // Read the types of the interaction.
        fgets(buff, 100, external_spline_table); line++;
        std::istringstream buffss(buff);
        std::vector<int> types(get_n_body());
        for (unsigned j = 0; j < types.size(); j++) buffss >> types[j];
        // Find it in the defined interactions.
        hash_val = calc_interaction_hash(types, n_cg_types);
        index_among_defined = get_index_from_hash(hash_val);
        // Read the values.
        line = read_bspline_table(external_spline_table, line, index_among_defined);
    }
    return line;
}

int InteractionClassSpec::read_bspline_table(FILE* external_spline_table, int line, int index_among_defined)
{
    char buff[100];
    int index_among_tabulated, n_external_spline_control_points;

    fgets(buff, 100, external_spline_table);
    line++;
    sscanf(buff, "%lf%lf", &lower_cutoffs[index_among_defined], &upper_cutoffs[index_among_defined]);
    n_external_spline_control_points = floor((upper_cutoffs[index_among_defined] - lower_cutoffs[index_among_defined]) / external_table_spline_binwidth + 0.5) + 1;
    index_among_tabulated = defined_to_tabulated_intrxn_index_map[index_among_defined] - 1;
    external_table_spline_coefficients[index_among_tabulated] = new double[n_external_spline_control_points];
    for (int j = 0; j < n_external_spline_control_points; j++) {
        fgets(buff, 100, external_spline_table);
        line++;
        sscanf(buff, "%lf", &external_table_spline_coefficients[index_among_tabulated][j]);
     }
     return line;
}

void InteractionClassComputer::calc_grid_of_force_vals(const std::vector<double> &spline_coeffs, const int index_among_defined, const double binwidth, std::vector<double> &axis_vals, std::vector<double> &force_vals) 
{
    // Size the output vectors of positions and forces conservatively.
    unsigned num_entries = int( (ispec->upper_cutoffs[index_among_defined] - ispec->lower_cutoffs[index_among_defined]) / binwidth  + 1.0);
    axis_vals = std::vector<double>(num_entries);
    force_vals = std::vector<double>(num_entries);    
    // Calculate forces by iterating over the grid points from low to high.
    double max = ispec->upper_cutoffs[index_among_defined];
    unsigned counter = 0;
    for (double axis = ((int)(ispec->lower_cutoffs[index_among_defined] / binwidth) + 1) * binwidth; axis < max; axis += binwidth) {
        force_vals[counter] = fm_s_comp->evaluate_spline(index_among_defined, interaction_class_column_index, spline_coeffs, axis);
        axis_vals[counter] = axis;
        counter++;
    }
    // Set the correct size for the output vectors of positions and forces.
    axis_vals.resize(counter);
    force_vals.resize(counter);
}

void InteractionClassComputer::calc_grid_of_force_and_deriv_vals(const std::vector<double> &spline_coeffs, const int index_among_defined, const double binwidth, std::vector<double> &axis_vals, std::vector<double> &force_vals, std::vector<double> &deriv_vals)
{
    BSplineAndDerivComputer* s_comp_ptr = static_cast<BSplineAndDerivComputer*>(fm_s_comp);
    unsigned num_entries = int( (ispec->upper_cutoffs[index_among_defined] - ispec->lower_cutoffs[index_among_defined]) / binwidth  + 1.0);
    axis_vals = std::vector<double>(num_entries);
    force_vals = std::vector<double>(num_entries);
	deriv_vals = std::vector<double>(num_entries);
	
    // Calculate forces by iterating over the grid points from low to high.
    double max = ispec->upper_cutoffs[index_among_defined];
    unsigned counter = 0;
    for (double axis = ((int)(ispec->lower_cutoffs[index_among_defined] / binwidth) + 1) * binwidth; axis < max; axis += binwidth) {
    	axis_vals[counter] = axis;
        force_vals[counter] = s_comp_ptr->evaluate_spline(index_among_defined, interaction_class_column_index, spline_coeffs, axis);
        deriv_vals[counter] = s_comp_ptr->evaluate_spline_deriv(index_among_defined, interaction_class_column_index, spline_coeffs, axis);   
    	counter++;
    }
}