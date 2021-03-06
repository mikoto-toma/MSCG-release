//
//  interaction_model.cpp
//  
//
//  Copyright (c) 2016 The Voth Group at The University of Chicago. All rights reserved.
//

#ifndef _interaction_model_h
#define _interaction_model_h

#include <array>
#include <fstream>
#include <list>
#include <string>
#include <vector>

#include "interaction_hashing.h"
#include "splines.h"
#include "topology.h"
#include "misc.h"
#include "control_input.h"

#ifndef DIMENSION
#define DIMENSION 3
#endif

struct MATRIX_DATA;
class PairCellList;
class ThreeBCellList;
struct InteractionClassComputer;
struct ThreeBodyNonbondedClassComputer;
struct DensityClassSpec;

// Function called externally
void free_interaction_data(CG_MODEL_DATA* cg);
// Variable checking routine
void check_input_values(CG_MODEL_DATA* cg);

//-------------------------------------------------------------
// Enumerated type definitions
//-------------------------------------------------------------

enum InteractionClassType {kPairNonbonded = 2, kPairBonded = -2, kAngularBonded = -3, kDihedralBonded = -4, kThreeBodyNonbonded = 3, kDensity = 4};
// function pointer "type" used for polymorphism of matrix element calculation (for pair nonbonded types)
typedef void (*calc_pair_matrix_elements)(InteractionClassComputer* const, std::array<double, DIMENSION>* const &, const real*, MATRIX_DATA* const);
typedef void (*calc_interaction_matrix_elements)(InteractionClassComputer* const info, MATRIX_DATA* const mat, const int n_body, int* particle_ids, std::array<double, DIMENSION>* derivatives, const double param_value, const int virial_flag, const double param_deriv, const double distance);

//-------------------------------------------------------------
// Interaction-model-related type definitions
//-------------------------------------------------------------

// This stores parameters that define an interaction class.

struct InteractionClassSpec {
	protected:
	BasisType basis_type;
    int bspline_k;
    double fm_binwidth;

	public:
    InteractionClassType class_type;
    int class_subtype;
    int basis_funcs_per_evaluation;
    
    // output only parameters
    int output_spline_coeffs_flag;
    double output_binwidth;
	int output_parameter_distribution;
	FILE** output_range_file_handles;

	// n_defined is the number of unique type combinations for n_cg_sites and the interaction type.
	// defined_to_possible is used for bonded-type interactions and converts the type combination hash
	// // to an index that runs along the interaction combinations that actually exist in the model
	// // (this is all interactions that could "possibly" be force matched in the system).
	// defined_to_matched_intrxn_index_map indicates (for non-bonded-type interactions) which interactions
	// // will actually be determined (through force matching).
	// // n_to_force_match is the total number of interactions to be fit
	// // n_force is the total number of antisymmetric (i.e. FM) interactions to be determined.
	// defined_to_tabulated_intrxn_inex_map indicates which interactions are tabulated
	// // (force interactions).
	// // n_from_table is the total number of external tables (i.e. force) to be read.
	// // n_force is the total number of antisymmetric tables (i.e. forces).
	int n_cg_types;    
    int n_defined;
    std::vector<unsigned> defined_to_possible_intrxn_index_map;
    std::vector<unsigned> defined_to_matched_intrxn_index_map;
    std::vector<unsigned> defined_to_tabulated_intrxn_index_map;
    std::vector<unsigned> defined_to_periodic_intrxn_index_map;
    std::vector<unsigned> interaction_column_indices;
    int n_to_force_match;
    int n_force;
    int n_from_table;
    int n_tabulated;
    int format; //Used to determine output name ordering.
    
    double *lower_cutoffs;
    double *upper_cutoffs;
    double cutoff;
    
    double external_table_spline_binwidth;
    double **external_table_spline_coefficients;
	
	public:
	// These virtual functions need to be implemented for every new interaction type.
	virtual void determine_defined_intrxns(TopologyData*) = 0;
	virtual int get_n_body (void) const = 0;
    virtual std::string get_full_name(void) const = 0;
    virtual std::string get_short_name(void) const = 0;
    virtual std::string get_table_name(void) const = 0;
    virtual char get_char_id(void) const = 0;

	// Optionally overriden functions
	virtual void read_rmin_class(std::string* &elements, const int position, const int index_among_defined, char* mode);
	virtual std::string get_interaction_name(char **type_names, const int intrxn_index_among_defined, const std::string &delimiter) const;
    virtual std::vector<int> get_interaction_types(const int intrxn_index_among_defined) const;
    virtual void setup_indices_in_fm_matrix(void);
	
	// Helper and implementation functions.
	std::string get_basename(char **type_names, const int intrxn_index_among_defined, const std::string& delimiter);
	void adjust_cutoffs_for_type(int i);
	void adjust_cutoffs_for_basis(int i);
    void setup_for_defined_interactions(TopologyData* topo_data); 
	void dummy_setup_for_defined_interactions(TopologyData* topo_data);
	void read_interaction_class_ranges(std::ifstream &range_in); 
	void smart_read_interaction_class_ranges(std::ifstream &range_in, char** name);
    int read_table(std::ifstream &external_spline_table, int line, int offset);
	int read_bspline_table(std::ifstream &external_spline_table, int line, int offset);
	void copy_table(const int base_defined, const int target_defined, const int num_lines);
	void free_force_tabulated_interaction_data(void);
	
	inline int get_index_from_hash(const int hash_val) const {if (defined_to_possible_intrxn_index_map.size() == 0) return hash_val; else return SearchIntTable(defined_to_possible_intrxn_index_map, hash_val);}
    inline int get_hash_from_index(const int index) const {if (defined_to_possible_intrxn_index_map.size() > 0) return defined_to_possible_intrxn_index_map[index]; else return index;}

	// Functions meant to be eliminated.	
	// set_n_defined is only used in topology for three body interactions it should be eliminated eventually
	inline void set_n_defined(int n) {
		n_defined = n;
	};
	
	inline void set_basis_type(BasisType type) {
		basis_type = type;
	};
		
	// Accessor (getter) functions.
	inline int get_num_basis_func(void) const {
		return interaction_column_indices[n_to_force_match];
    };

	inline BasisType get_basis_type(void) {
		return basis_type;
	};
	inline int get_n_defined(void) {
		return n_defined;
	};
	inline int get_bspline_k(void) {
		return bspline_k;
	};
	inline double get_fm_binwidth(void) {
		return fm_binwidth;
	};

	InteractionClassSpec() {
		n_tabulated = n_to_force_match = n_from_table = 0;
		n_defined = 0;
		class_subtype = 0;
	};
	
	~InteractionClassSpec() {
		delete [] lower_cutoffs;
		delete [] upper_cutoffs;
		
	    if (n_tabulated > 0) {
    	    for (int i = 0; i < n_tabulated; i++) {
        	    delete [] external_table_spline_coefficients[i];
        	}
        	delete [] external_table_spline_coefficients;
    	}
	}
};

// Info needed for FM calculation of each interaction class, very closely
// related to the below struct. (Will be rebuilt from the below struct later.)

struct InteractionClassComputer {
	
    // Raw interaction class specifications
    InteractionClassSpec *ispec;
    double cutoff2;                             // Squared cutoff; used only for nonbonded interactions
	
    // Matrix-locations for storing results of computation
    int trajectory_block_frame_index;           // Index of the current frame in the current block of frames
    int current_frame_starting_row;             // Starting row number for the block of the FM matrix determined by the current frame
    int interaction_class_column_index;         // The starting column of the FM matrix block corresponding to the current class of interactions
    int basis_function_column_index;            // Starting column index for the matrix block corresponding to the current active interaction in the current class

    // Interacting particle indices: 
    // pair interactions: k-l; 
    // three-body interactions: k-j-l; 
    // four-body interactions: k-i-j-l.
    int k;
    int l;
    int i;
    int j;
   
    // Temps for determining which interaction the particles interact with.
    int index_among_defined_intrxns;
    int index_among_matched_interactions;
    int index_among_tabulated_interactions;
    
    // Calculation intermediates for the interaction
    double intrxn_param;                       // The interaction parameter for any single-parameter interaction (ie distance, angle, dihedral angle)
    double intrxn_param_less_lower_cutoff;     // Pair distance from the pair_nonbonded_interaction_lower_cutoffs_XOR_lower_cutoffs
    double stillinger_weber_angle_parameter;   // Current interaction's SW angle param (three-body interactions only).
    
    // Function called to calculate matrix elements corresponding to an interaction in the current class of interactions.
    void (*calculate_fm_matrix_elements)(InteractionClassComputer* const self, std::array<double, DIMENSION>* const &x, const real *simulation_box_half_lengths, MATRIX_DATA* const mat);
    // Function called to evaluate the values of functions in an interaction's basis set
    void (*set_up_fm_bases)(void);
	// Function called to accumulate the interactions into the matrix for the interaction.
	calc_interaction_matrix_elements process_interaction_matrix_elements;
	
	virtual void class_set_up_computer(void) = 0;  
	//virtual void class_set_up_range(void) = 0;
    // Function to calculate index of an actual interaction among all possible interactions for the current class
	virtual int calculate_hash_number(int* const cg_site_types, const int n_cg_types) = 0;
	
	virtual void calculate_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths) = 0;
	
	void set_up_computer(InteractionClassSpec* const ispec_pt, int *curr_iclass_col_index);	

	void calc_external_spline_interaction(void);
	void calc_grid_of_table_force_vals(const int index_among_defined_intrxns, const double binwidth, std::vector<double> &axis_vals, std::vector<double> &force_vals);
	void calc_grid_of_force_vals(const std::vector<double> &spline_coeffs, const int index_among_defined_intrxns, const double binwidth, std::vector<double> &axis_vals, std::vector<double> &force_vals);
	void calc_grid_of_force_and_deriv_vals(const std::vector<double> &spline_coeffs, const int index_among_defined_intrxns, const double binwidth, std::vector<double> &axis_vals, std::vector<double> &force_vals, std::vector<double> &deriv_vals);
	
	void walk_neighbor_list(MATRIX_DATA* const mat, calc_pair_matrix_elements calc_matrix_elements, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);
	void walk_3B_neighbor_list(MATRIX_DATA* const mat, const int n_cg_types, const TopologyData& topo_data, const ThreeBCellList& three_body_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);

	void set_indices(void) {
		index_among_matched_interactions   = ispec->defined_to_matched_intrxn_index_map[index_among_defined_intrxns];
		index_among_tabulated_interactions = ispec->defined_to_tabulated_intrxn_index_map[index_among_defined_intrxns];
	};
	
    // Spline computation objects for force matched and
    // tabulated interactions.
    SplineComputer* fm_s_comp;
    SplineComputer* table_s_comp;

    // Preallocating this temporary is worth ~20% of runtime in serial_fm.
    std::vector<double> fm_basis_fn_vals;
    std::vector<double> table_basis_fn_vals;

	InteractionClassComputer() {
		fm_s_comp = NULL;
		table_s_comp = NULL;
	}
	
	protected:
	void calculate_bspline_matrix_elements(void);
	void calculate_linear_spline_matrix_elements(void);
};

struct PairNonbondedClassSpec: InteractionClassSpec {
	inline PairNonbondedClassSpec(ControlInputs* control_input) {
		class_type = kPairNonbonded;
		class_subtype = 1;
		cutoff = control_input->pair_nonbonded_cutoff;
		basis_type = (BasisType) control_input->basis_set_type;
		output_spline_coeffs_flag = control_input->output_spline_coeffs_flag;
		fm_binwidth = control_input->pair_nonbonded_fm_binwidth;
		bspline_k = control_input->nonbonded_bspline_k;
		output_binwidth = control_input->pair_nonbonded_output_binwidth;
		output_parameter_distribution = control_input->output_pair_nonbonded_parameter_distribution;
	}
		
	void determine_defined_intrxns(TopologyData *topo_data) {
        n_defined = calc_n_distinct_pairs(topo_data->n_cg_types);
        format = 0;
	}
	
	inline int get_n_body () const { return 2;}
    inline std::string get_full_name(void) const {return "pair nonbonded";}
    inline std::string get_short_name(void) const {return "";}
    inline std::string get_table_name(void) const {return "short_range";}
    inline char get_char_id(void) const {return 'n';}
};

struct PairBondedClassSpec: InteractionClassSpec {
	inline PairBondedClassSpec(ControlInputs* control_input) {
		class_type = kPairBonded;
		class_subtype = 1;
		cutoff = VERYLARGE;
		basis_type = (BasisType) control_input->basis_set_type;
		output_spline_coeffs_flag = control_input->output_spline_coeffs_flag;
		fm_binwidth = control_input->pair_bond_fm_binwidth;
		bspline_k = control_input->pair_bond_bspline_k;
		output_binwidth = control_input->pair_bond_output_binwidth;
		output_parameter_distribution = control_input->output_pair_bond_parameter_distribution;
	}
	
	inline ~PairBondedClassSpec() {}
	
	void determine_defined_intrxns(TopologyData *topo_data) {
		int n_possible_interactions = calc_n_distinct_pairs(topo_data->n_cg_types);
        n_defined = calc_n_active_interactions(topo_data->bond_type_activation_flags, n_possible_interactions);
        defined_to_possible_intrxn_index_map = std::vector<unsigned>(n_defined, 0);
        set_up_interaction_type_hash_array(topo_data->bond_type_activation_flags, n_possible_interactions, defined_to_possible_intrxn_index_map);
		format = 0;
	}
	
	int get_n_body () const { return 2;}
    inline std::string get_full_name(void) const {return "pair bonded";}
    inline std::string get_short_name(void) const {return "bon";}
    inline std::string get_table_name(void) const {return "bond";}
    inline char get_char_id(void) const {return 'b';}
};

struct AngularClassSpec: InteractionClassSpec {
	inline AngularClassSpec(ControlInputs* control_input) {
		class_type = kAngularBonded;
    	basis_type = (BasisType) control_input->basis_set_type;
    	output_spline_coeffs_flag = control_input->output_spline_coeffs_flag;
    	class_subtype = control_input->angle_interaction_style;
    	fm_binwidth = control_input->angle_fm_binwidth;
    	bspline_k = control_input->angle_bspline_k;
    	output_binwidth = control_input->angle_output_binwidth;
		output_parameter_distribution = control_input->output_angle_parameter_distribution;
		cutoff = VERYLARGE;
    }
	
	inline ~AngularClassSpec() {}
	
	void determine_defined_intrxns(TopologyData *topo_data) {
		int n_possible_interactions = calc_n_distinct_triples(topo_data->n_cg_types);
        n_defined = calc_n_active_interactions(topo_data->angle_type_activation_flags, n_possible_interactions);
        defined_to_possible_intrxn_index_map = std::vector<unsigned>(n_defined, 0);
        set_up_interaction_type_hash_array(topo_data->angle_type_activation_flags, n_possible_interactions, defined_to_possible_intrxn_index_map);
		format = topo_data->angle_format;
	}
	
	int get_n_body () const { return 3;}
    inline std::string get_full_name(void) const {return "angular bonded";}
    inline std::string get_short_name(void) const {return "ang";}
    inline std::string get_table_name(void) const {return "angle";}
    inline char get_char_id(void) const {return 'a';}
};

struct DihedralClassSpec: InteractionClassSpec {
	inline DihedralClassSpec(ControlInputs* control_input) {
		class_type = kDihedralBonded;
    	basis_type = (BasisType) control_input->basis_set_type;
    	output_spline_coeffs_flag = control_input->output_spline_coeffs_flag;
    	class_subtype = control_input->dihedral_interaction_style;
    	fm_binwidth = control_input->dihedral_fm_binwidth;
		bspline_k = control_input->dihedral_bspline_k;
		output_binwidth = control_input->dihedral_output_binwidth;
		output_parameter_distribution = control_input->output_dihedral_parameter_distribution;
		cutoff = VERYLARGE;
	}
	
	inline ~DihedralClassSpec() {}
	
	void determine_defined_intrxns(TopologyData *topo_data) {
		int n_possible_interactions = calc_n_distinct_quadruples(topo_data->n_cg_types);
        n_defined = calc_n_active_interactions(topo_data->dihedral_type_activation_flags, n_possible_interactions);
        defined_to_possible_intrxn_index_map = std::vector<unsigned>(n_defined, 0);
        set_up_interaction_type_hash_array(topo_data->dihedral_type_activation_flags, n_possible_interactions, defined_to_possible_intrxn_index_map);
		format = topo_data->dihedral_format;
	}
	
	int get_n_body () const { return 4;}
    inline std::string get_full_name(void) const {return "dihedral bonded";}
    inline std::string get_short_name(void) const {return "dih";}
    inline std::string get_table_name(void) const {return "dihedral";}
    inline char get_char_id(void) const {return 'd';}
};

struct ThreeBodyNonbondedClassSpec: InteractionClassSpec {

	double three_body_gamma;
    double* three_body_nonbonded_cutoffs;
    double* stillinger_weber_angle_parameters_by_type;
    double stillinger_weber_angle_parameter;
 	
    // Three body topology temporaries.
    int* tb_n;
    int** tb_list;

	inline ThreeBodyNonbondedClassSpec(ControlInputs* control_input) {
		class_type = kThreeBodyNonbonded;
		basis_type = (BasisType) control_input->basis_set_type;
		cutoff = VERYLARGE;
		output_spline_coeffs_flag = control_input->output_spline_coeffs_flag;
		class_subtype = control_input->three_body_flag;
		fm_binwidth = control_input->three_body_fm_binwidth;
		bspline_k = control_input->three_body_bspline_k;
		output_binwidth = control_input->three_body_nonbonded_output_binwidth;
		three_body_gamma = control_input->gamma;
		n_defined = 0;
		output_parameter_distribution = 0;
	}
	
	inline ~ThreeBodyNonbondedClassSpec() {
	    if (class_subtype > 0) {
			delete [] three_body_nonbonded_cutoffs;
			delete [] stillinger_weber_angle_parameters_by_type;
		}
	}
	
	void determine_defined_intrxns(TopologyData *topo_data) {
		defined_to_possible_intrxn_index_map = std::vector<unsigned>(get_n_defined(), 0);
      	
      	if (class_subtype == 0) {
      		n_to_force_match = 0;
			return;
		}
       	
		// Set up the hash table for three body nonbonded interactions.
       	int counter = 0;
       	for (int ii = 0; ii < n_cg_types; ii++) {
           	for (int jj = 0; jj < tb_n[ii]; jj++) {
               	defined_to_possible_intrxn_index_map[counter] = calc_three_body_interaction_hash(ii + 1, tb_list[ii][2 * jj], tb_list[ii][2 * jj + 1], n_cg_types);
               	counter++;
           	}
       	}
       	// Free the dummy topology used to define three body potentials.
       	delete [] tb_n;
       	for (int ii = 0; ii < n_cg_types; ii++) delete [] tb_list[ii];
       	delete [] tb_list;
       	format = 0;
	}
	
	int get_n_body () const { return 3;}
    inline std::string get_full_name(void) const {return "three body nonbonded";}
    inline std::string get_short_name(void) const {return "";}
    inline std::string get_table_name(void) const {return "three_body";}
    inline char get_char_id(void) const {return '3';}
    void setup_indices_in_fm_matrix(void);
    
};

struct DensityClassSpec: InteractionClassSpec {

	// Density interactions are defined between density groups, 
	// with the CG sites that belong to a given density group having 
	// interactions determined by the  density of another (possibly the same) density group.
	// The first density_group specifies where the density is calculated (at which CG sites), 
	// while the second density_group specifies which what the density is calculated of.
	// So, the density of sites belonging to the second density_group are calculated at every site belonging to the first density_group.
	
	int n_cg_sites;			// Needed for initializing density_computer's density array
	
	int n_density_groups;	// The number of density groups defined in top.in
	char** density_group_names; //The names of each density_group (used for output).
	bool* density_groups;	// Which CG site types belong to each density group.
	double* density_sigma;	// The interaction parameter needed to specify the weight function for density.
							// This parameter can be different for each pair of density groups interacting.
							// For continuously varying (Gaussian) weight functions, it is the standard deviation.
							// For switching (tanh) weight functions, it is the "width" determined by the denomenator (x-a)/b.
	double* density_switch; // This is only used for switching(tanh) weight functions.
							// For switching (tanh) weight functions, it is the center (point of inflection) for the switching curve.
							// It is specified as a percentage of the cutoff.
	
	int density_weights_flag; // 0 for number density, 1 to read other weights for each CG type in each density group (e.g. mass, charge)
	double* density_weights;  // Specifies the weight of each CG type to each density group
	
	unsigned long* site_to_density_group_intrxn_index_map; 
							// This indicates which density_group interactions are active between a pair of site types.
							// This map is not symmetric since it is different to calculate a density at sites of a given type (first index)
							// and to calculate a density of a given type (or set) (second index).
							// This is a flat array if [1st_type, 2nd_type] = [1st_type * n_cg_types + 2nd_type];
							// The value at an index of this array indicates which density_group - density_group interactions are active
							// based on a set of bitwise flags (i.e. The value is a binary number representing a series of bolean flags for each type).
	
	inline DensityClassSpec(ControlInputs* control_input) {
		// PairNonbondedInteractionClass constructor
		cutoff = control_input->density_cutoff_distance;
		if ( (control_input->density_cutoff_distance - VERYSMALL > control_input->pair_nonbonded_cutoff) &&
			 (control_input->density_flag != 0) ) {
			printf("Density cutoff distance (%lf) must be less than pair nonbonded cutoff (%lf)!\n", control_input->density_cutoff_distance, control_input->pair_nonbonded_cutoff);
			printf("Setting density cutoff equal to pair_nonbonded_cutoff.\n");
			cutoff = control_input->pair_nonbonded_cutoff;
		}
		
		basis_type = (BasisType) control_input->basis_set_type;
		output_spline_coeffs_flag = control_input->output_spline_coeffs_flag;
		
		// Density specific variables
		class_type = kDensity;
		class_subtype = control_input->density_flag;
		density_weights_flag = control_input->density_weights_flag;
		fm_binwidth = control_input->density_fm_binwidth;
		bspline_k = control_input->density_bspline_k;
		output_binwidth = control_input->density_output_binwidth;
		output_parameter_distribution = control_input->output_density_parameter_distribution;
		
		// Initialize some pointers to NULL and numbers to 0.
		n_density_groups = 0;
		density_sigma = NULL;
		density_switch = NULL;
	}

	inline std::string get_full_name(void) const {return "density";}
    inline std::string get_short_name(void) const {return "den";}
	inline std::string get_table_name(void) const {return "density";}
    inline char get_char_id(void) const {return 'p';}
	int get_n_body (void) const {return 2;}
	void read_rmin_class(std::string* &elements, const int position, const int index_among_defined, char* mode);
	std::vector<int> get_interaction_types(const int index_among_defined_intrxns) const;
	std::string get_interaction_name(char **type_names, const int intrxn_index_among_defined, const std::string &delimiter) const;
	
	void determine_defined_intrxns(TopologyData *topo_data) {
		n_cg_sites = int(topo_data->n_cg_sites);
		n_density_groups = topo_data->n_density_groups;
		density_group_names = topo_data->density_group_names;
		density_groups = topo_data->density_groups;
		density_weights = topo_data->density_weights;
		n_defined = n_density_groups * n_density_groups;
		format = 0;
	};
	
	~DensityClassSpec() {
		if((class_subtype > 0) && (get_n_defined() > 0)) {
			delete [] density_groups;
			delete [] density_weights;
			delete [] site_to_density_group_intrxn_index_map;			
			for(int i = 0; i < n_density_groups; i++) delete [] density_group_names[i];
			delete [] density_group_names;
			delete [] density_sigma;
			delete [] density_switch;
		} else {
			if (density_sigma != NULL) delete [] density_sigma;
			if (density_switch != NULL) delete [] density_switch;
		}
	};
};

struct PairNonbondedClassComputer : InteractionClassComputer {
	void class_set_up_computer(void);
	//void class_set_up_range(void);
	void calculate_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);

    int calculate_hash_number(int* const cg_site_types, const int n_cg_types) {
	    return calc_two_body_interaction_hash(cg_site_types[k], cg_site_types[l], n_cg_types);
	}
};

struct PairBondedClassComputer : InteractionClassComputer {
	void class_set_up_computer(void);
	//void class_set_up_range(void);
	void calculate_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths); 

    int calculate_hash_number(int* const cg_site_types, const int n_cg_types) {
	    return calc_two_body_interaction_hash(cg_site_types[k], cg_site_types[l], n_cg_types);
	}
};

struct AngularClassComputer : InteractionClassComputer {
	void class_set_up_computer(void);
	//void class_set_up_range(void);
	void calculate_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);

    int calculate_hash_number(int* const cg_site_types, const int n_cg_types) {
	    return calc_three_body_interaction_hash(cg_site_types[j], cg_site_types[k], cg_site_types[l], n_cg_types);
	}
};

struct DihedralClassComputer : InteractionClassComputer {
	void class_set_up_computer(void);
	//void class_set_up_range(void);
	void calculate_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);

    int calculate_hash_number(int* const cg_site_types, const int n_cg_types) {
		return calc_four_body_interaction_hash(cg_site_types[i], cg_site_types[j], cg_site_types[k], cg_site_types[l], n_cg_types);
	}
};

struct ThreeBodyNonbondedClassComputer : InteractionClassComputer {
	double coef1[100];
 	
	void special_set_up_computer(InteractionClassSpec* const ispec_pt, int *curr_iclass_col_index);
	void class_set_up_computer(void) {} ;
	//void class_set_up_range(void);
	void calculate_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths) {};
	void calculate_3B_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const ThreeBCellList& three_body_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);
	
    void calculate_bspline_elements_and_deriv_elements(double* coef1);
	void calculate_bspline_deriv_elements(double* coef1);
    int calculate_hash_number(int* const cg_site_types, const int n_cg_types) {
	    return calc_three_body_interaction_hash(cg_site_types[j], cg_site_types[k], cg_site_types[l], n_cg_types);
	}
};

struct DensityClassComputer : InteractionClassComputer {
	// Precomputed intermediates for the calculation of density.
	double* denomenator;
	double* u_cutoff;
	double* f_cutoff;
	double* c0;
	double* c2;
	double* c4;
	double* c6;
	
	// Stores the density weight for the current interaction.
	double curr_weight;
	
	// A "flattened" 2D-array that stores the density of each density group at each CG site. 
	// Only the needed elements are calculated
	double* density_values;		// It is "flattened" via the "hash" [matched_density_index * n_cg_sites + cg_site_index].
								// First, this will hold the accumulating weight function contributions that determine 
								// the density of each density group at every relavent CG site.
								// Then, this holds the spline derivative evaluated at the density value 
								// (since the density value itself is no longer needed).

	// Specific Implementaitons of InteractionClassComputer functions
	void class_set_up_computer(void);
	//void class_set_up_range(void);
	void calculate_interactions(MATRIX_DATA* const mat, int traj_block_frame_index, int curr_frame_starting_row, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);
	void walk_density_neighbor_list(MATRIX_DATA* const mat, calc_pair_matrix_elements calc_matrix_elements, const int n_cg_types, const TopologyData& topo_data, const PairCellList& pair_cell_list, std::array<double, DIMENSION>* const &x, const real* simulation_box_half_lengths);
	
	// Additional Computer functions specific to Density.
	void reset_density_array(void);
	
	// Additional function pointer to calculate the density_values array before computing the interaction
	void (*calculate_density_values)(InteractionClassComputer* const self, std::array<double, DIMENSION>* const &x, const real *simulation_box_half_lengths, MATRIX_DATA* const mat);
   	void (*process_density)(InteractionClassComputer* const self, std::array<double, DIMENSION>* const &x, const real *simulation_box_half_lengths, MATRIX_DATA* const mat);
	double (*calculate_density_derivative)(DensityClassComputer* const icomp, DensityClassSpec* const ispec, const double distance);
	
	// Need to implement these functions
	int calculate_hash_number(int* const cg_site_types, const int n_cg_types) {return -1;}
		
	inline ~DensityClassComputer() {
		if(ispec->get_n_defined() > 0) {
			delete [] denomenator;
			delete [] u_cutoff;
			delete [] f_cutoff;
			delete [] density_values;

	    	if(ispec->class_subtype == 4) {
	    		delete [] c0;
	    		delete [] c2;
	  	  		delete [] c4;
	    		delete [] c6;
	    	}
		}
	};
};

// Major struct responsible for keeping track of all cg model parameters, interaction definitions,
// and basis set specifications.

struct CG_MODEL_DATA {
   
    // Cutoff specifications.
    double pair_nonbonded_cutoff;           // Nonbonded pair interaction cutoff
    double pair_nonbonded_cutoff2;          // Squared cutoff distance for pair nonbonded interactions
    double three_body_nonbonded_cutoff2;    // Squared cutoff distance for three body nonbonded interactions

    // Topology specifications.
    TopologyData topo_data;
    
    // CG site number and type specifications.
    int n_cg_types;
    int n_cg_sites;
    char **name;

    // Interaction class specification structs.
    PairNonbondedClassSpec pair_nonbonded_interactions;
    PairBondedClassSpec pair_bonded_interactions;
    AngularClassSpec angular_interactions;
    DihedralClassSpec dihedral_interactions;
    ThreeBodyNonbondedClassSpec three_body_nonbonded_interactions;
	DensityClassSpec density_interactions;
	
    // Interaction class computation structs.
    PairNonbondedClassComputer pair_nonbonded_computer;
    PairBondedClassComputer pair_bonded_computer;
    AngularClassComputer angular_computer;
    DihedralClassComputer dihedral_computer;
    ThreeBodyNonbondedClassComputer three_body_nonbonded_computer;
	DensityClassComputer density_computer;
	
	// List for interactions and computer used by iterators.
	std::list<InteractionClassSpec*> iclass_list;
	std::list<InteractionClassComputer*> icomp_list;
	
    // Non-matrix-associated output flags.
    int output_spline_coeffs_flag;          // 1 to output spline coefficients as well as force tables; 0 otherwise

	inline CG_MODEL_DATA(ControlInputs* control_input) :
		pair_nonbonded_cutoff(control_input->pair_nonbonded_cutoff),
		topo_data(control_input->max_pair_bonds_per_site, control_input->max_angles_per_site, control_input->max_dihedrals_per_site),
		pair_nonbonded_interactions(control_input), pair_bonded_interactions(control_input),
		angular_interactions(control_input), dihedral_interactions(control_input),
		three_body_nonbonded_interactions(control_input),
		density_interactions(control_input),
		output_spline_coeffs_flag(control_input->output_spline_coeffs_flag)
{
    	topo_data.excluded_style = control_input->excluded_style;
		topo_data.density_excluded_style = control_input->density_excluded_style;
		pair_nonbonded_cutoff2 = pair_nonbonded_cutoff * pair_nonbonded_cutoff;
		
		iclass_list.push_back(&pair_nonbonded_interactions);
		iclass_list.push_back(&pair_bonded_interactions);
		iclass_list.push_back(&angular_interactions);
		iclass_list.push_back(&dihedral_interactions);
		iclass_list.push_back(&density_interactions);
		
		icomp_list.push_back(&pair_nonbonded_computer);
		icomp_list.push_back(&pair_bonded_computer);
		icomp_list.push_back(&angular_computer);
		icomp_list.push_back(&dihedral_computer);
		icomp_list.push_back(&density_computer);
		
		check_input_values(this);
	}
		
	~CG_MODEL_DATA() {    
    	// Free tabulated potential data

	    printf("Freeing interaction classes.\n"); fflush(stdout);
    	std::list<InteractionClassComputer*>::iterator icomp_iterator;
    	for(icomp_iterator=icomp_list.begin(); icomp_iterator != icomp_list.end(); icomp_iterator++) {
        	if( (*icomp_iterator)->fm_s_comp != NULL ) delete (*icomp_iterator)->fm_s_comp;
    	}
    	if (three_body_nonbonded_computer.fm_s_comp != NULL) delete three_body_nonbonded_computer.fm_s_comp;    	

    	printf("Freeing tabulated reference potential information.\n");
	    for(icomp_iterator=icomp_list.begin(); icomp_iterator != icomp_list.end(); icomp_iterator++) {
        	if( (*icomp_iterator)->fm_s_comp != NULL ) {
        		delete (*icomp_iterator)->table_s_comp;
    		}
    	}	
		if(three_body_nonbonded_computer.table_s_comp != NULL) delete three_body_nonbonded_computer.table_s_comp;
		
		topo_data.free_topology_data();
	};
	
};

//--------------------------------------------------------------------
// Functions for setting up the potential model that will be used
// in the CG model from a range.in file.
//--------------------------------------------------------------------

// Read interaction ranges and assign the interactions to be force matched, tabulated, or null.
void read_all_interaction_ranges(CG_MODEL_DATA* const cg);

// Special set-up density interactions.
void density_additional_setup_for_defined_interactions(InteractionClassSpec* ispec, TopologyData* topo_data);

// Read tabulated interaction data from file
void read_tabulated_interaction_file(CG_MODEL_DATA* const cg, int n_cg_types);

// Reset upper and lower cutoffs for BI calculation
void reset_interaction_cutoff_arrays(CG_MODEL_DATA* const cg);

// Select the correct type name array for the interaction.
char** select_name(InteractionClassSpec* const ispec, char ** const cg_name);

#endif