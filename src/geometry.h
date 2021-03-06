//
//  geometry.h
//  
//
//  Copyright (c) 2016 The Voth Group at The University of Chicago. All rights reserved.
//

#ifndef _geometry_h
#define _geometry_h

#include <array>
#include "trajectory_input.h"

// Calculate translation-invariant geometrical parameters 
// as functions of tuples of particle indices, subject to a
// cutoff. Returns 'false' if not in cutoff, 'true' otherwise.
// These functions calculate the parameter and its derivatives
// with respect to the first n-1 particles, since the final 
// derivative with respect to the last particle is simply
// the negative sum of the others.
bool conditionally_calc_squared_distance_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives);
bool conditionally_calc_distance_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &paritlce_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives);
bool conditionally_calc_angle_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives);
bool conditionally_calc_angle_and_intermediates(const int* particle_ids, std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, std::array<double, DIMENSION>* &dist_derivs_01, std::array<double, DIMENSION>* &dist_derivs_02, std::array<double, DIMENSION>* &derivatives, double &param_val, double &rr_01, double &rr2_02);
bool conditionally_calc_sw_angle_and_intermediates(const int* particle_ids, std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff, const double gamma, std::array<double, DIMENSION>* &dist_derivs_01, std::array<double, DIMENSION>* &dist_derivs_02, std::array<double, DIMENSION>* &derivatives, double &param_val, double &rr1, double &rr2, double &angle_prefactor, double &dr1_prefactor, double &dr2_prefactor);
bool conditionally_calc_dihedral_and_derivatives(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, const double cutoff2, double &param_val, std::array<double, DIMENSION>* &derivatives);

// As above, but without derivatives and unconditionally, for 
// rangefinding and density.
void calc_squared_distance(const int* particle_ids, const std::array<double, DIMENSION>*const &particle_positions, const real *simulation_box_half_lengths, double &param_val);
void calc_distance(const int* particle_ids, std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, double &param_val);
void calc_angle(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, double &param_val);
void calc_dihedral(const int* particle_ids, const std::array<double, DIMENSION>* const &particle_positions, const real *simulation_box_half_lengths, double &param_val);

// Wrapping function (apply periodic boundary conditions)
void get_minimum_image(const int l, std::array<double, DIMENSION>* const &x, const real *simulation_box_half_lengths);

#endif