//
//  geometry.cpp
//  
//
//  Copyright (c) 2016 The Voth Group at The University of Chicago. All rights reserved.
//

#include <cmath>
#include "geometry.h"

#ifndef MAXFLOAT
#define MAXFLOAT 1.0E10
#endif

// Function prototypes for internal functions.
void subtract_min_image_vectors(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, std::array<double, DIMENSION> &displacement);
void subtract_min_image_particles(const std::array<double, DIMENSION> &particle_position1, const std::array<double, DIMENSION> &particle_position2, const real *simulation_box_half_lengths, std::array<double, DIMENSION> &displacement);
void cross_product(const std::array<double, DIMENSION> &a, const std::array<double, DIMENSION> &b, std::array<double, DIMENSION> &c);
double dot_product(const std::array<double, DIMENSION> &a, const std::array<double, DIMENSION> &b);
double dot_product(const double* a, const double* b);
inline void check_sine(double &s);
inline void check_cos(double &cos_theta);

//------------------------------------------------------------
// Small helper functions used internally.
//------------------------------------------------------------

void subtract_min_image_vectors(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, std::array<double, DIMENSION> &displacement)
{
    for (int i = 0; i < DIMENSION; i++) {
        displacement[i] = particle_positions[particle_ids[1]][i] - particle_positions[particle_ids[0]][i];
        if (displacement[i] > simulation_box_half_lengths[i]) displacement[i] -= 2.0 * simulation_box_half_lengths[i];
        else if (displacement[i] < -simulation_box_half_lengths[i]) displacement[i] += 2.0 * simulation_box_half_lengths[i];
    }
}

void subtract_min_image_particles(const std::array<double, DIMENSION> &particle_position1, const std::array<double, DIMENSION> &particle_position2, const real *simulation_box_half_lengths, std::array<double, DIMENSION> &displacement)
{
    for (int i = 0; i < DIMENSION; i++) {
        displacement[i] = particle_position2[i] - particle_position1[i];
        if (displacement[i] > simulation_box_half_lengths[i]) displacement[i] -= 2.0 * simulation_box_half_lengths[i];
        else if (displacement[i] < -simulation_box_half_lengths[i]) displacement[i] += 2.0 * simulation_box_half_lengths[i];
    }
}

void get_minimum_image(const int l, std::array<double, DIMENSION>* const &x, const real *simulation_box_half_lengths)
{
    for (int i = 0; i < DIMENSION; i++) {
        if (x[l][i] < 0) x[l][i] += 2.0 * simulation_box_half_lengths[i];
        else if (x[l][i] >= 2.0 * simulation_box_half_lengths[i]) x[l][i] -= 2.0 * simulation_box_half_lengths[i];
    }
}

// NOTE: Cross product is only defined for 2^n - 1 dimensions (and only 3 dimensions in the code at the moment).
void cross_product(const std::array<double, DIMENSION> &a, const std::array<double, DIMENSION> &b, std::array<double, DIMENSION> &c)
{
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
}

double dot_product(const std::array<double, DIMENSION> &a, const std::array<double, DIMENSION> &b)
{
    double t = 0.0;
    for (int i = 0; i < DIMENSION; i++) t += a[i] * b[i];
    return t;
}

double dot_product(const double* a, const double* b)
{
    double t = 0.0;
    for (int i = 0; i < DIMENSION; i++) t += a[i] * b[i];
    return t;
}

//------------------------------------------------------------
// Calculate translation-invariant geometrical functions of 
// n particle positions and n-1 of their derivatives.
//------------------------------------------------------------

// Calculate a squared distance and one derivative.

bool conditionally_calc_squared_distance_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives)
{
    double rr2 = 0.0;
    std::array<double, DIMENSION> displacement;
    subtract_min_image_vectors(particle_ids, particle_positions, simulation_box_half_lengths, displacement);
    for (int i = 0; i < DIMENSION; i++) {
        rr2 += displacement[i] * displacement[i];
    }
    if (rr2 > cutoff2) {
        return false;
    } else {
        for (int i = 0; i < DIMENSION; i++) {
            derivatives[0][i] = 2.0 * displacement[i];
        }
        param_val = rr2;
        return true;
    }
}

// Calculate a distance and one derivative.

bool conditionally_calc_distance_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives)
{
    bool within_cutoff = conditionally_calc_squared_distance_and_derivatives(particle_ids, particle_positions, simulation_box_half_lengths, cutoff2, param_val, derivatives);

    if (within_cutoff) {
        param_val = sqrt(param_val);
        for (int i = 0; i < DIMENSION; i++) {
            derivatives[0][i] = 0.5 * derivatives[0][i] / param_val;
        }
        return true;
    } else {
        return false;
    }
}

// Calculate the angle between three particles and its derivatives.

bool conditionally_calc_angle_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives)
{   
    std::array<double, DIMENSION>* dist_derivs_20 = new std::array<double, DIMENSION>[1];
    std::array<double, DIMENSION>* dist_derivs_21 = new std::array<double, DIMENSION>[1];
    int particle_ids_20[2] = {particle_ids[2], particle_ids[0]};
    int particle_ids_21[2] = {particle_ids[2], particle_ids[1]};
    double rr2_20, rr2_21;
    bool within_cutoff_20 = conditionally_calc_squared_distance_and_derivatives(particle_ids_20, particle_positions, simulation_box_half_lengths, cutoff2, rr2_20, dist_derivs_20);
    bool within_cutoff_21 = conditionally_calc_squared_distance_and_derivatives(particle_ids_21, particle_positions, simulation_box_half_lengths, cutoff2, rr2_21, dist_derivs_21);
    
    if (!within_cutoff_20 || !within_cutoff_21) {
    	delete [] dist_derivs_20;
    	delete [] dist_derivs_21;
        return false;
    } else {
        // Calculate the cosine
        double rr_20 = sqrt(rr2_20);
        double rr_21 = sqrt(rr2_21);
		double cos_theta = dot_product(dist_derivs_20[0], dist_derivs_21[0]) / (4.0 * rr_20 * rr_21);
		check_cos(cos_theta);
        
        // Calculate the angle.
        double theta = acos(cos_theta);
        param_val = theta * DEGREES_PER_RADIAN;

        // Calculate the derivatives.
        double sin_theta = sin(theta);
        double rr_01_1 = 1.0 / (rr_20 * rr_21 * sin_theta);
        double rr_00c = cos_theta / (rr_20 * rr_20 * sin_theta);
        double rr_11c = cos_theta / (rr_21 * rr_21 * sin_theta);

        for (unsigned i = 0; i < DIMENSION; i++) {
        	// derivatives for the end particles
        	derivatives[0][i] = 0.5 * DEGREES_PER_RADIAN * (dist_derivs_21[0][i] * rr_01_1 - rr_00c * dist_derivs_20[0][i]);
            derivatives[1][i] = 0.5 * DEGREES_PER_RADIAN * (dist_derivs_20[0][i] * rr_01_1 - rr_11c * dist_derivs_21[0][i]);
        }
        delete [] dist_derivs_20;
    	delete [] dist_derivs_21;
        return true;
    }
}

// Calculate a the cosine of an angle along with its derivatives.

bool conditionally_calc_angle_and_intermediates(const int* particle_ids, std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, std::array<double, DIMENSION>* &dist_derivs_20, std::array<double, DIMENSION>* &dist_derivs_21, std::array<double, DIMENSION>* &derivatives, double &param_val, double &rr_20, double &rr_21)
{
    int particle_ids_20[2] = {particle_ids[2], particle_ids[0]};
    int particle_ids_21[2] = {particle_ids[2], particle_ids[1]};
    double rr2_20, rr2_21;
    bool within_cutoff_20 = conditionally_calc_squared_distance_and_derivatives(particle_ids_20, particle_positions, simulation_box_half_lengths, cutoff2, rr2_20, dist_derivs_20);
    bool within_cutoff_21 = conditionally_calc_squared_distance_and_derivatives(particle_ids_21, particle_positions, simulation_box_half_lengths, cutoff2, rr2_21, dist_derivs_21);
    
    if (!within_cutoff_20 || !within_cutoff_21) {
        return false;
    } else {
        // Calculate the cosine
        rr_20 = sqrt(rr2_20);
        rr_21 = sqrt(rr2_21);
		double cos_theta = dot_product(dist_derivs_20[0], dist_derivs_21[0]) / (4.0 * rr_20 * rr_21);
        check_cos(cos_theta);
        
        // Calculate the angle.
        double theta = acos(cos_theta);
        param_val = theta * DEGREES_PER_RADIAN;

        // Calculate the derivatives.
        double sin_theta = sin(theta);
        double rr_01_1 = 1.0 / (rr_20 * rr_21 * sin_theta);
        double rr_00c = cos_theta / (rr_20 * rr_20 * sin_theta);
        double rr_11c = cos_theta / (rr_21 * rr_21 * sin_theta);

        for (unsigned i = 0; i < DIMENSION; i++) {
            derivatives[0][i] = - 0.5 * (dist_derivs_21[0][i] * rr_01_1 + rr_00c * dist_derivs_20[0][i]);
            derivatives[1][i] = - 0.5 * (dist_derivs_20[0][i] * rr_01_1 + rr_11c * dist_derivs_21[0][i]);
        }
    }    
    return true;
}

// Calculate a terms for Stillinger-Weber interactions.

bool conditionally_calc_sw_angle_and_intermediates(const int* particle_ids, std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff, const double gamma, std::array<double, DIMENSION>* &dist_derivs_01, std::array<double, DIMENSION>* &dist_derivs_02, std::array<double, DIMENSION>* &derivatives, double &param_val, double &rr1, double &rr2, double &angle_prefactor, double &dr1_prefactor, double &dr2_prefactor)
{	
	bool within_cutoff = conditionally_calc_angle_and_intermediates(particle_ids, particle_positions, simulation_box_half_lengths, cutoff*cutoff, dist_derivs_01, dist_derivs_02, derivatives, param_val, rr1, rr2);
	if(within_cutoff == false) {
		return false;
	} else {
		double r1_less_cutoff = rr1 - cutoff;
		double r2_less_cutoff = rr2 - cutoff;

		double sw_exp1 = exp(gamma / r1_less_cutoff);
		double sw_exp2 = exp(gamma / r2_less_cutoff);

		double sw_exp_dr1 = gamma / (r1_less_cutoff * r1_less_cutoff) * sw_exp1;
		double sw_exp_dr2 = gamma / (r2_less_cutoff * r2_less_cutoff) * sw_exp2;

		angle_prefactor = sw_exp1 * sw_exp2 * DEGREES_PER_RADIAN;
		dr1_prefactor = sw_exp2 * sw_exp_dr1;
		dr2_prefactor = sw_exp1 * sw_exp_dr2;
	}
	return true;
}

// Calculate a dihedral angle and its derivatives.
// Thanks to Andrew Jewett (jewett.aij  g m ail) for inspiration from LAMMPS dihedral_table.cpp

bool conditionally_calc_dihedral_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives)
{
    // Find the relevant displacements for defining the angle.
    std::array<double, DIMENSION> disp03, disp23, disp12;
    int particle_ids_03[2] = {particle_ids[3], particle_ids[0]};
    int particle_ids_23[2] = {particle_ids[3], particle_ids[2]};
    int particle_ids_12[2] = {particle_ids[2], particle_ids[1]};
    subtract_min_image_vectors(particle_ids_03, particle_positions, simulation_box_half_lengths, disp03);
    subtract_min_image_vectors(particle_ids_23, particle_positions, simulation_box_half_lengths, disp23);
    subtract_min_image_vectors(particle_ids_12, particle_positions, simulation_box_half_lengths, disp12);

    // Calculate the dihedral, which requires many intermediates.
    // It is the dot product of the cross product. 
    // Note: To calculate the cosine, the vectors in the final dot product need to be effectively normalized.
    double rrbc = 1.0 / sqrt(dot_product(disp23, disp23));	// central bond
    std::array<double, DIMENSION> pb, pc, cross_bc;
    cross_product(disp03, disp23, pb); // This defines the normal vector to the first 3 sites
    cross_product(disp12, disp23, pc); // This defines the normal vector to the last 3 sites
    cross_product(pb, pc, cross_bc);
    
    double pb2 = dot_product(pb, pb);
    double rpb1 = 1.0 / sqrt(pb2);
    double pc2 = dot_product(pc, pc);
    double rpc1 = 1.0 / sqrt(pc2);
    
    double pbpc = dot_product(pb, pc);
    double cos_theta = pbpc * rpb1 * rpc1;
    check_cos(cos_theta);
    double theta = acos(cos_theta) * DEGREES_PER_RADIAN;
    
	// This variable is only used to determine the sign of the angle
	double sign = - dot_product( pb, disp12) * rpb1 * rrbc; // This is the s calculation that LAMMPS used.
	if (sign < 0.0) { 
		param_val = - theta;
	} else {
		param_val = theta;
	}
	    
    // Calculate the derivatives
    double dot03_23 = dot_product(disp03, disp23);
    double dot12_23 = dot_product(disp12, disp23);
    double r23_2    = dot_product(disp23, disp23);
    double fcoef = dot03_23 / r23_2;
	double hcoef = 1.0 + dot12_23 / r23_2; 
	double dtf,dth;
	for (unsigned i = 0; i < DIMENSION; i++) {		
		dtf = pb[i] / (rrbc * pb2);				                
		dth = - pc[i] / (rrbc * pc2);
		
		derivatives[0][i] = -dtf; // first normal times projection of bond onto it
		derivatives[1][i] = -dth; //second normal times projection of bond onto it
		derivatives[2][i] = dtf * fcoef + dth * hcoef;
	}
    return true;
}

//------------------------------------------------------------
// Without derivatives.
//------------------------------------------------------------

// Calculate a squared distance.

void calc_squared_distance(const int* particle_ids, const std::array<double, DIMENSION>*const &particle_positions, const real *simulation_box_half_lengths, double &param_val)
{
	std::array<double, DIMENSION> displacement;
	double rr2 = 0.0;
    for (int i = 0; i < DIMENSION; i++) {
        subtract_min_image_vectors(particle_ids, particle_positions, simulation_box_half_lengths, displacement);  
        rr2 += displacement[i] * displacement[i];
    }
    param_val = rr2;
}

// Calculate a distance.

void calc_distance(const int* particle_ids, std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, double &param_val)
{
    calc_squared_distance(particle_ids, particle_positions, simulation_box_half_lengths, param_val);
    param_val = sqrt(param_val);
}

// Calculate the angle between three particles.

void calc_angle(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, double &param_val)
{   
    std::array<double, DIMENSION>* dist_derivs_20 = new std::array<double, DIMENSION>[1];
    std::array<double, DIMENSION>* dist_derivs_21 = new std::array<double, DIMENSION>[1];
    int particle_ids_20[2] = {particle_ids[2], particle_ids[0]};
    int particle_ids_21[2] = {particle_ids[2], particle_ids[1]};
    double rr2_20, rr2_21;
    conditionally_calc_squared_distance_and_derivatives(particle_ids_20, particle_positions, simulation_box_half_lengths, MAXFLOAT, rr2_20, dist_derivs_20);
    conditionally_calc_squared_distance_and_derivatives(particle_ids_21, particle_positions, simulation_box_half_lengths, MAXFLOAT, rr2_21, dist_derivs_21);
    
    // Calculate the cosine
    double rr_20 = sqrt(rr2_20);
    double rr_21 = sqrt(rr2_21);
    double cos_theta = dot_product(dist_derivs_20[0], dist_derivs_21[0]) / (4.0 * rr_20 * rr_21);
    check_cos(cos_theta);
    
    // Calculate the angle.
    double theta = acos(cos_theta);
    param_val = theta * DEGREES_PER_RADIAN;

    delete [] dist_derivs_20;
    delete [] dist_derivs_21;
}

// Calculate a dihedral angle.
// Thanks to Andrew Jewett (jewett.aij  g m ail) for inspiration from LAMMPS dihedral_table.cpp

void calc_dihedral(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, double &param_val)
{
    // Find the relevant displacements for defining the angle.
    std::array<double, DIMENSION> disp03, disp23, disp12;
    int particle_ids_03[2] = {particle_ids[3], particle_ids[0]};
    int particle_ids_23[2] = {particle_ids[3], particle_ids[2]};
    int particle_ids_12[2] = {particle_ids[2], particle_ids[1]};
    subtract_min_image_vectors(particle_ids_03, particle_positions, simulation_box_half_lengths, disp03);
    subtract_min_image_vectors(particle_ids_23, particle_positions, simulation_box_half_lengths, disp23);
    subtract_min_image_vectors(particle_ids_12, particle_positions, simulation_box_half_lengths, disp12);

    // Calculate the dihedral, which requires many intermediates.
    // It is the dot product of the cross product. 
    // Note: To calculate the cosine, the vectors in the final dot product need to be effectively normalized.
    double rrbc = 1.0 / sqrt(dot_product(disp23, disp23));	// central bond
    std::array<double, DIMENSION> pb, pc, cross_bc;
    cross_product(disp03, disp23, pb); // This defines the normal vector to the first 3 sites
    cross_product(disp12, disp23, pc); // This defines the normal vector to the last 3 sites
    cross_product(pb, pc, cross_bc);
    
    double pb2 = dot_product(pb, pb);
    double rpb1 = 1.0 / sqrt(pb2);
    double pc2 = dot_product(pc, pc);
    double rpc1 = 1.0 / sqrt(pc2);
    
    double pbpc = dot_product(pb, pc);
    double cos_theta = pbpc * rpb1 * rpc1;
    check_cos(cos_theta);
    double theta = acos(cos_theta) * DEGREES_PER_RADIAN;
    
	// This variable is only used to determine the sign of the angle
	double sign = - dot_product( pb, disp12) * rpb1 * rrbc; // This is the s calculation that LAMMPS used.
	if (sign < 0.0) { 
		param_val = - theta;
	} else {
		param_val = theta;
	}    	
}

inline void check_sine(double &s)
{
    if (s < 0.0 && s > -VERYSMALL_F) s = -VERYSMALL_F;
    if (s > 0.0 && s < VERYSMALL_F) s = VERYSMALL_F;
}

inline void check_cos(double &cos_theta)
{
        double max = 1.0 - VERYSMALL_F;
        double min = -1.0 + VERYSMALL_F;
        if (cos_theta > max) cos_theta = max;
        else if (cos_theta < min) cos_theta = min;
}